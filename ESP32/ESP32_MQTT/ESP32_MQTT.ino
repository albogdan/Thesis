#include <WiFi.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include "esp_sntp.h"
#include "driver/timer.h"
#include "driver/rtc_io.h"
#include <AutoConnect.h>
#include <WebServer.h>
#include "mqtt_params.h"
#include <HTTPClient.h>
#include <ESPmDNS.h>

#include <FS.h>
#include <SPIFFS.h>
/* NOTE: CRASHES WHEN IT CAN'T CONNECT TO AN MQTT SERVER BECAUSE OF NULL POINTER
 *  SHOULD MAKE A WAY SO THAT IT SWITCHES OVER TO SD CARD IF THAT HAPPENS
 */

 
/* Timer related defines */
#define TIMER_DIVIDER         (16)  //  Hardware timer clock divider
#define TIMER_SCALE           (TIMER_BASE_CLK / TIMER_DIVIDER)  // convert counter value to seconds

#define SLEEP_TIMER_SECONDS 60 // Amount of time to wait until trying to go into hibernation mode
#define RTC_GPIO_WAKEUP_PIN GPIO_NUM_4
#define ARDUINO_SIGNAL_READY_PIN GPIO_NUM_23


// Define sensor types
#define SENSOR_TEMPERATURE 1
#define SENSOR_HUMIDITY 2
#define SENSOR_RAINFALL 3

/* ----- BEGIN WIFI SETTINGS ----- */
String HOSTNAME = "ESP32_LORA";
char mqttServer[40] = "";
char mqttPort[6] = "";
WiFiClient espClient;
WebServer web_server;
AutoConnect captivePortal(web_server);

const char* AUX_SETTING_URI = "/mqtt_setting";
const char* AUX_SAVE_URI    = "/mqtt_save";
// Declare AutoConnectElements for the page /mqtt_setting
ACStyle(style, "label+input,label+select{position:sticky;left:120px;width:230px!important;box-sizing:border-box;}");
ACText(header, "<h2>MQTT Broker Settings</h2>", "text-align:center;color:#2f4f4f;padding:10px;");
ACText(caption, "Setup the Address and Port of the MQTT Server", "font-family:serif;color:#4682b4;");
ACInput(mqttserver, "", "Address", "^(([a-zA-Z0-9]|[a-zA-Z0-9][a-zA-Z0-9\\-]*[a-zA-Z0-9])\\.)*([A-Za-z0-9]|[A-Za-z0-9][A-Za-z0-9\\-]*[A-Za-z0-9])$", "Server Address");
ACInput(mqttport, "", "Port", "^[0-9]{4-6}$", "Server Port");
ACElement(newline, "<hr>");
ACSubmit(save, "Save", AUX_SAVE_URI);
AutoConnectAux mqtt_settings(AUX_SETTING_URI, "MQTT Settings", true, {style, header, caption, mqttserver, mqttport, newline, save});

// Declare AutoConnectElements for the page /mqtt_save
ACText(caption2, "<h4>Parameters saved as:</h4>", "text-align:center;color:#2f4f4f;padding:10px;");
ACText(parameters);
AutoConnectAux mqtt_save(AUX_SAVE_URI, "MQTT Setting", false, {caption2,parameters});

/* ----- END WIFI SETTINGS ----- */


/* ----- BEGIN MQTT SETTINGS ----- */
long lastReconnectAttempt = 0;
PubSubClient mqttClient(espClient);

/* ----- END MQTT SETTINGS ----- */

/* ----- BEGIN PACKET INFORMATION AND DEFINITIONS ----- */
//              Message parts and sizes diagram
// |  src  |  type  |  seq num  | agg. bit + length |  data  |
// |  2B   |  1B    |    1B     |          1B       |  0-64B |
typedef struct packet {
  byte src[2];
  byte agg_src[2];
  byte type;
  byte seq_num;
  bool aggregated_message;
  unsigned int message_length;
  int *node_data_fields;
  int *node_data;
} Packet;

/* Questions:
 *  Do we want to send the type too?
 *  Do we want to send the seq num too?
 */
/* ----- END PACKET INFORMATION AND DEFINITIONS ----- */


/* ----- BEGIN SERIAL SETTINGS ----- */
// The maximum size of a message - this is calculated from the 
// message metadata plus data max size. See diagram above.
#define MAX_PAYLOAD_SIZE 5+64 
byte input_byte_array[3*MAX_PAYLOAD_SIZE]; // Extra large just in case

unsigned int input_byte_array_index = 0;
bool byte_array_complete = false;

// Define the pins for the Arduino serial port
#define RXD2 16
#define TXD2 17

/* ----- END SERIAL SETTINGS ----- */

int loop_count = 0;


/* ----- BEGIN TIME VARIABLES ----- */
time_t now;
char strftime_buf[64];
struct tm timeinfo;
/* ----- END TIME VARIABLES ----- */

/* ----- BEGIN FUNCTION DEFINITIONS ----- */
void print_packet(Packet payload);

/* ----- END FUNCTION DEFINITIONS ----- */

void setup()
{
    // Define the USB serial port and start it
    Serial.begin(115200);
    delay(10);
    Serial.println("[SETUP] Serial0 setup complete");

    // Define the Arduino serial port and start it
    Serial2.begin(115200, SERIAL_8N1, RXD2, TXD2);
    delay(10);
    Serial.println("[SETUP] Serial2 setup complete");

    // Load the MQTT config from SPIFFS
    load_mqtt_config();

    // Add the MQTT pages to the Captive Portal
    captivePortal.join({mqtt_settings, mqtt_save});

    captivePortal.on(AUX_SETTING_URI, loadParams);
    captivePortal.on(AUX_SAVE_URI, saveParams);


    // Restore configured MQTT broker settings
    PageArgument  args;
    AutoConnectAux& mqtt_setting = *captivePortal.aux(AUX_SETTING_URI);
    loadParams(mqtt_setting, args);

    // Now we connect to a WiFi network
    Serial.println("[SETUP] Setting up the Captive WiFi Portal");
    if (captivePortal.begin()) {
      Serial.println("");
      Serial.println("[INFO] WiFi connected");
      Serial.print("[INFO] IP address: ");
      Serial.println(WiFi.localIP());  
    }
    

    Serial.println("[SETUP] Setting up MQTT server...");
    setupMqtt(mqttServer, mqttPort);

    Serial.println("[SETUP] Connecting to MQTT server...");
    connectToMqtt();

    //Print the wakeup reason for ESP32
    print_wakeup_reason();


    // Set timezone to Eastern Standard Time
    setenv("TZ", "EST5EDT4,M3.2.0/02:00:00,M11.1.0/02:00:00", 1);
    tzset();

    // Setup periodic synch with the NTP server
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "pool.ntp.org");
    sntp_init();

    // If the year is not correct, sync it up
    if (timeinfo.tm_year < (2019 - 1900)) {
        int retry = 0;
        const int retry_count = 10;
        while (sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET && ++retry < retry_count) {
            Serial.print("[SETUP] Waiting for system time to be set...(");
            Serial.print(retry);
            Serial.print("/");
            Serial.print(retry_count);
            Serial.println(")");
            vTaskDelay(2000 / portTICK_PERIOD_MS);
        }
    }

    Serial.print("[INFO] The current time is: ");
    update_current_local_time();
    Serial.println(strftime_buf);

    Serial.println("[INFO] Initializing sleep timer");
    initialize_sleep_timer(SLEEP_TIMER_SECONDS);

    pinMode(ARDUINO_SIGNAL_READY_PIN, OUTPUT);
    digitalWrite(ARDUINO_SIGNAL_READY_PIN, HIGH);
    Serial.println("[SETUP] Boot sequence complete.");
}

// Retreive the value of each element entered by '/mqtt_setting'.
String saveParams(AutoConnectAux& aux, PageArgument& args) {
  mqttserver.value.trim();
  mqttport.value.trim();

  Serial.println("saving config");
  if (SPIFFS.begin()) {
    Serial.println("mounted file system");
    if (SPIFFS.exists("/config.json")) {
      //file exists, reading and loading
      Serial.println("reading config file");
      File configFile = SPIFFS.open("/config.json", "w");
      if (configFile) {
        Serial.println("opened config file");
        DynamicJsonDocument json(1024);
        json["mqtt_server"] = mqttserver.value.c_str();
        json["mqtt_port"] = mqttport.value.c_str();
        
        serializeJson(json, Serial);
        serializeJson(json, configFile);
        configFile.close();
      }
    }
  } else {
    Serial.println("failed to mount FS");
  }  
  // Echo back saved parameters to AutoConnectAux page.
  String echo = "Server: " + mqttserver.value + "<br>";
  echo += "Port: " + mqttport.value + "<br>";
  parameters.value = echo;
  return String("");
}

// Load parameters from persistent storage
String loadParams(AutoConnectAux& aux, PageArgument& args) {
  load_mqtt_config();
  mqttserver.value = mqttServer;
  mqttport.value = mqttPort;
  return String("");
}

void load_mqtt_config() {
  if (SPIFFS.begin()) {
    Serial.println("mounted file system");
    if (SPIFFS.exists("/config.json")) {
      //file exists, reading and loading
      Serial.println("reading config file");
      File configFile = SPIFFS.open("/config.json", "r");
      if (configFile) {
        Serial.println("opened config file");
        size_t size = configFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);

        configFile.readBytes(buf.get(), size);
        DynamicJsonDocument json(1024);
        auto deserializeError = deserializeJson(json, buf.get());
        serializeJson(json, Serial);
        if ( ! deserializeError ) {
          Serial.println("\nparsed json");
          strcpy(mqttServer, json["mqtt_server"]);
          strcpy(mqttPort, json["mqtt_port"]);
        } else {
          Serial.println("failed to load json config");
        }
        configFile.close();
      }
    }
  } else {
    Serial.println("failed to mount FS");
  }  
}


void update_current_local_time(){
    time(&now);
    localtime_r(&now, &timeinfo);
    strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
}

void loop() {

  // Check if we received a complete string through Serial2
  if (byte_array_complete) {
    parse_input_string(input_byte_array);

    // Clear the byte array: reset the index pointer
    input_byte_array_index = 0;
    byte_array_complete = false;
  }

  // Check if there's anything in the Serial2 buffer
  while (Serial2.available()) {
    // Get the new byte:
    byte inByte = (byte)Serial2.read();

    // Add it to the inputString:
    input_byte_array[input_byte_array_index] = inByte;
    input_byte_array_index += 1;

    // If the incoming character is a newline, set a flag so the main loop can
    // do something about it:
    if (inByte == '\n') {
      byte_array_complete = true;
    }
  }
  captivePortal.handleClient();
  mqttClient.loop();
  
}

void parse_input_string(byte *input_byte_array) {
    Packet packet;
    packet.src[0] = input_byte_array[0];
    packet.src[1] = input_byte_array[1];
    packet.type = input_byte_array[2];
    packet.seq_num = input_byte_array[3];
    packet.aggregated_message = (input_byte_array[4] & 0x80)>> 7;
    
    // Parse non-aggregated messages here
    if(!packet.aggregated_message) {
        // Get the data
        Serial.println("Non-aggregated packet acquired");
        packet.message_length = input_byte_array[4] & 0x7F;
        packet.node_data_fields = (int*)malloc(packet.message_length*sizeof(int));
        packet.node_data = (int*)malloc(packet.message_length*sizeof(int));
        
        for(int i=0; i<packet.message_length/2; i+=1) {
          /* Uncomment if want to convert to chars
          if (input_byte_array[5+2*i] == SENSOR_TEMPERATURE) {
            packet.node_data_fields[i] = 'T';
          } else if (input_byte_array[5+2*i] == SENSOR_HUMIDITY) {
            packet.node_data_fields[i] = 'H';
          } else if(input_byte_array[5+2*i] == SENSOR_RAINFALL) {
            packet.node_data_fields[i] = 'R';
          } */
          packet.node_data_fields[i] = (int)input_byte_array[5+2*i];
          packet.node_data[i] = (int)input_byte_array[5+2*i+1];
        }
        
        print_packet(packet);
        publish_packet_to_mqtt(packet);
        free(packet.node_data_fields);
        free(packet.node_data);
        
    // Parse aggregated messages here
    } else {
        Serial.println("Aggregated packet acquired");
        packet.agg_src[0] = input_byte_array[5];
        packet.agg_src[1] = input_byte_array[6];
        packet.message_length = input_byte_array[7] & 0x7F;

        packet.node_data_fields = (int*)malloc(packet.message_length*sizeof(int));
        packet.node_data = (int*)malloc(packet.message_length*sizeof(int));
        
        for(int i=0; i<packet.message_length/2; i+=1) {
            packet.node_data_fields[i] = (int)input_byte_array[8+2*i];
            packet.node_data[i] = (int)input_byte_array[8+2*i+1];
        }
        
        print_packet(packet);
        publish_packet_to_mqtt(packet);
        free(packet.node_data_fields);
        free(packet.node_data);
    }
}

void print_packet(Packet payload){
    Serial.print("SRC: 0x");
    Serial.print(payload.src[0], HEX);
    Serial.print(" + 0x");
    Serial.print(payload.src[1], HEX);
    Serial.print(" | TYPE: 0x");
    Serial.print(payload.type, HEX);
    Serial.print(" | SEQ_NUM: 0x");
    Serial.print(payload.seq_num, HEX);
    Serial.print(" | AGG_BOOL: ");
    Serial.print(payload.aggregated_message, DEC);
    Serial.print(" | LEN: ");
    Serial.print(payload.message_length);
    Serial.print(" | DATA: { | ");
    for(int j=0; j<payload.message_length/2; j++) {
        Serial.print(payload.node_data_fields[j]);
        Serial.print(":");
        Serial.print(payload.node_data[j]);
        Serial.print(" | ");
    }
    Serial.println("}");  
}

void publish_packet_to_mqtt(Packet payload) {
    String message;
    // 2 static fields + two arrays of size message_length/2
    // Need 4 space for the time field
    const int capacity = JSON_OBJECT_SIZE(6) + 2*JSON_ARRAY_SIZE(payload.message_length/2);
    DynamicJsonDocument doc(capacity);
        
    update_current_local_time();
    doc["time"] = now;
    
    if (payload.aggregated_message) {
        doc["src_1"] = payload.agg_src[0];
        doc["src_2"] = payload.agg_src[1];
    } else {
        doc["src_1"] = payload.src[0];
        doc["src_2"] = payload.src[1];
    }
    JsonArray node_fields = doc.createNestedArray("node_fields");
    JsonArray node_data = doc.createNestedArray("node_data");
    for(int i=0; i< payload.message_length/2; i++){
        node_fields.add(payload.node_data_fields[i]);
        node_data.add(payload.node_data[i]);
    }
    serializeJson(doc, message);
    String topic = "data/" + HOSTNAME;
    char topic_char_array[topic.length() + 1];
    char message_char_array[message.length() + 1];

    topic.toCharArray(topic_char_array, topic.length() + 1);
    message.toCharArray(message_char_array, message.length() + 1);
    Serial.print("Publishing to MQTT: ");
    Serial.println(message);
    Serial.println();
    
    if (mqttClient.connected()) {
      mqttClient.publish(topic_char_array, message_char_array);
    }  else {
      reconnect_mqtt_connection();
    }

    yield();
}


void setupMqtt(char *mqttServer, char *mqttPort)
{
    mqttClient.setServer(mqttServer, atoi(mqttPort));
    mqttClient.setCallback(callbackMqtt);
    mqttClient.setBufferSize(1024);   // MAKE SURE TO CHECK THAT THIS IS BIG ENOUGH
}

boolean connectToMqtt()
{
    char hostname_char[HOSTNAME.length() + 1];
    HOSTNAME.toCharArray(hostname_char, HOSTNAME.length() + 1);
    while (!mqttClient.connected())
    {
        Serial.print("[SETUP] Connecting to MQTT at ");
        Serial.print(mqttServer);
        Serial.print(":");
        Serial.println(mqttPort);
        if (mqttClient.connect(hostname_char))
        { //, mqttUser, mqttPassword )) {
            Serial.print("[INFO] MAC: ");
            Serial.println(WiFi.macAddress());
            Serial.println("[SETUP] Connected to MQTT!\n");
            mqttSubscribeToTopics(&mqttClient);
            return true;
        }
        else
        {
            Serial.print("[SETUP] Connection failed with state ");
            Serial.println(mqttClient.state());
            delay(200);

            // Temp fix to allow for you to change the MQTT information if its not connected
            while(1) {
              captivePortal.handleClient();
            }
            return false;
        }
        
    }
}


void reconnect_mqtt_connection()
{
    if (WiFi.status() == WL_CONNECTED)
    {
        long now = millis();
        if (now - lastReconnectAttempt > 10000)
        {
            lastReconnectAttempt = now;
            // Attempt to reconnect
            if (connectToMqtt())
            {
                lastReconnectAttempt = 0;
            }
        }
    }
    else
    {
        mqttClient.loop();
    }
}

void mqttSubscribeToTopics(PubSubClient *mqttClient)
{
    mqttClient->subscribe("ping");

    // All subscriptions should be of the form {HOSTNAME/endpoint}
    mqttClient->subscribe("ESP32_LORA/test");    // Subscribe function only takes char*, not String
}

void callbackMqtt(char *topic, byte *payload, unsigned int length)
{

    Serial.print("Message arrived in topic: ");
    Serial.println(topic);
    String topic_string = String(topic);

    Serial.print("Message:");
    String message = "";
    for (int i = 0; i < length; i++)
    {
        message += (char)payload[i];
    }
    Serial.println(message);

    if (topic_string.equals("ping"))
    {
        Serial.println("Calling ping response");
        return_ping();
    }
    else if (topic_string.equals("test"))
    {
        Serial.println("Message received in test topic");
    }

    Serial.println();
    Serial.println("-----------------------");
}

// Returns a ping with information about the system
void return_ping()
{
    String message;
    const size_t capacity = JSON_OBJECT_SIZE(8);
    DynamicJsonDocument doc(capacity);

    doc["ip_address"] = WiFi.localIP().toString();
    doc["mac_address"] = WiFi.macAddress();
    doc["hostname"] = HOSTNAME;
    doc["current_ssid"] = WiFi.SSID();

    serializeJson(doc, message);

    String topic = "ping/response/" + HOSTNAME;

    char topic_output[topic.length() + 1];
    char message_output[message.length() + 1];

    topic.toCharArray(topic_output, topic.length() + 1);
    message.toCharArray(message_output, message.length() + 1);

    Serial.print("Replying with: ");
    Serial.println(message);
    if (mqttClient.connected()) {
      mqttClient.publish(topic_output, message_output);
    }  else {
      reconnect_mqtt_connection();
    }
    yield();
}

void publish_arduino_data(String arduino_data) {
    String message;
    const size_t capacity = JSON_OBJECT_SIZE(4);
    DynamicJsonDocument doc(capacity);

    doc["data_1"] = arduino_data.substring(0, arduino_data.indexOf(','));
    doc["data_2"] = arduino_data.substring(arduino_data.indexOf(',')+1, arduino_data.indexOf('\r'));

    serializeJson(doc, message);

    String topic = "data/" + HOSTNAME;

    char topic_char_array[topic.length() + 1];
    char message_char_array[message.length() + 1];

    topic.toCharArray(topic_char_array, topic.length() + 1);
    message.toCharArray(message_char_array, message.length() + 1);

    Serial.print("Publishing to MQTT: ");
    Serial.println(message);
    if (mqttClient.connected()) {
      mqttClient.publish(topic_char_array, message_char_array);
    }  else {
      reconnect_mqtt_connection();
    }
    yield();
}

/* --------------------- BEGIN SLEEP RELATED FUNCTIONS --------------------- */

void hibernate_mode() {
    // Tell Arduino we're turning off
    digitalWrite(ARDUINO_SIGNAL_READY_PIN, LOW);

    // Disable so even deeper sleep
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_SLOW_MEM, ESP_PD_OPTION_OFF);
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_FAST_MEM, ESP_PD_OPTION_OFF);

    // Need to keep this on so that we can wake up using a pin interrupt
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON);

    // Configure GPIO4 = RTC_GPIO_10 as ext0 wake up source for HIGH logic level
    // *IMPORTANT*: These are referred to by their actual GPIO pins, not the RTC_GPIO pin number
    rtc_gpio_pulldown_en(RTC_GPIO_WAKEUP_PIN);
    rtc_gpio_pullup_dis(RTC_GPIO_WAKEUP_PIN);
    esp_sleep_enable_ext0_wakeup(RTC_GPIO_WAKEUP_PIN,1);

    //Go to sleep now
    esp_deep_sleep_start();
}

//Function that prints the reason by which ESP32 has been awaken from sleep
void print_wakeup_reason(){
    esp_sleep_wakeup_cause_t wakeup_reason;
    wakeup_reason = esp_sleep_get_wakeup_cause();
    switch(wakeup_reason)
    {
        case ESP_SLEEP_WAKEUP_EXT0 : Serial.println("[WAKE] Wakeup caused by external signal using RTC_IO"); break;
        case ESP_SLEEP_WAKEUP_EXT1 : Serial.println("[WAKE] Wakeup caused by external signal using RTC_CNTL"); break;
        case ESP_SLEEP_WAKEUP_TIMER : Serial.println("[WAKE] Wakeup caused by timer"); break;
        case ESP_SLEEP_WAKEUP_TOUCHPAD : Serial.println("[WAKE] Wakeup caused by touchpad"); break;
        case ESP_SLEEP_WAKEUP_ULP : Serial.println("[WAKE] Wakeup caused by ULP program"); break;
        default : Serial.printf("[WAKE] Wakeup was not caused by deep sleep: %d\n",wakeup_reason); break;
    }
}

static intr_handle_t s_timer_handle;
static void initialize_sleep_timer(unsigned int timer_interval_sec){
    timer_idx_t timer = TIMER_0;
    timer_group_t group = TIMER_GROUP_0;
    bool auto_reload = true;
    /* Select and initialize basic parameters of the timer */
    timer_config_t config = {
        .alarm_en = TIMER_ALARM_EN,
        .counter_en = TIMER_PAUSE,
        .intr_type = TIMER_INTR_LEVEL,
        .counter_dir = TIMER_COUNT_UP,
        .auto_reload = auto_reload,
        .divider = TIMER_DIVIDER,
    }; // default clock source is APB
    timer_init(group, timer, &config);

    /* Timer's counter will initially start from value below.
       Also, if auto_reload is set, this value will be automatically reload on alarm */
    timer_set_counter_value(group, timer, 0);

    /* Configure the alarm value and the interrupt on alarm. */
    timer_set_alarm_value(group, timer, timer_interval_sec * TIMER_SCALE);
    timer_enable_intr(group, timer);

    timer_isr_register(group, timer, &sleep_timer_isr_callback, NULL, 0, &s_timer_handle);

    timer_start(group, timer);
}


static void sleep_timer_isr_callback(void* args) {
    Serial.println("[INTERRUPT] A sleep timer interrupt has occurred.");
    
    // Reset the timers
    TIMERG0.int_clr_timers.t0 = 1;
    TIMERG0.hw_timer[0].config.alarm_en = 1;

    if(!digitalRead(RTC_GPIO_WAKEUP_PIN)) {
        // Go to sleep
        Serial.println("[INTERRUPT] Going to sleep.");
        hibernate_mode();
    }
}

/* --------------------- END SLEEP RELATED FUNCTIONS --------------------- */
