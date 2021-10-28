#include <WiFi.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>
#define uS_TO_S_FACTOR 1000000  //Conversion factor for micro seconds to seconds
#define TIME_TO_SLEEP  10        //Time ESP32 will go to sleep (in seconds)

// Define the pins for the Arduino serial port
#define RXD2 16
#define TXD2 17

String HOSTNAME = "ESP32_LORA";

// WiFi Settings
const char* ssid     = "ssid_here";
const char* password = "password_here";
const char *mqttServer = "mqtt_server_here";

// MQTT Settings
const int mqttPort = 1883;
long lastReconnectAttempt = 0;

// Serial Variables
String inputString = "";         // a String to hold incoming data
bool stringComplete = false;  // whether the string is complete

WiFiClient espClient;
PubSubClient mqttClient(espClient);

int loop_count = 0;
void setup()
{
    // Define the USB serial port and start it
    Serial.begin(115200);
    delay(10);
    Serial.println("[INFO] Serial0 setup complete");

    // Define the Arduino serial port and start it
    Serial2.begin(115200, SERIAL_8N1, RXD2, TXD2);
    delay(10);
    Serial.println("[INFO] Serial2 setup complete");

    // We start by connecting to a WiFi network
    Serial.println();
    Serial.println();
    Serial.print("[INFO] Connecting to ");
    Serial.println(ssid);

    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println("");
    Serial.println("[INFO] WiFi connected");
    Serial.println("[INFO] IP address: ");
    Serial.println(WiFi.localIP());

    Serial.println("[INFO] Setting up MQTT server...");
    setupMqtt(mqttServer, mqttPort);

    Serial.println("[INFO] Connecting to MQTT server...");
    connectToMqtt();

    // reserve 200 bytes for the inputString:
    inputString.reserve(200);

    //Print the wakeup reason for ESP32
    print_wakeup_reason();

}

void loop() {
  loop_count++;
  if (loop_count >= 5000) {
      Serial.println("[INFO] Entering hibernate mode");
      loop_count = 0;
      hibernate_mode();
  }

  // Check to make sure the MQTT connection is live
  check_mqtt_connection();

  // Check if we received a complete string through Serial2
  if (stringComplete) {
    // Publish the string to MQTT
    publish_arduino_data(inputString);

    // Clear the string:
    inputString = "";
    stringComplete = false;
  }

  // Check if there's anything in the Serial2 buffer
  while (Serial2.available()) {
    // Get the new byte:
    char inChar = (char)Serial2.read();

    // Add it to the inputString:
    inputString += inChar;

    // If the incoming character is a newline, set a flag so the main loop can
    // do something about it:
    if (inChar == '\n') {
      stringComplete = true;
    }
  }
}


void hibernate_mode() {
    // Disable so even deeper sleep
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_SLOW_MEM, ESP_PD_OPTION_OFF);
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_FAST_MEM, ESP_PD_OPTION_OFF);
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_OFF);

    //Configure GPIO33 as ext0 wake up source for HIGH logic level
//    esp_sleep_enable_ext0_wakeup(GPIO_NUM_33,1);

    // Configure RTC timer to wakeup after TIME_TO_SLEEP seconds
    esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);

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

void setupMqtt(const char *mqttServer, const int mqttPort)
{
    mqttClient.setServer(mqttServer, mqttPort);
    mqttClient.setCallback(callbackMqtt);
    mqttClient.setBufferSize(1024);   // MAKE SURE TO CHECK THAT THIS IS BIG ENOUGH
}

boolean connectToMqtt()
{
    char hostname_char[HOSTNAME.length() + 1];
    HOSTNAME.toCharArray(hostname_char, HOSTNAME.length() + 1);
    while (!mqttClient.connected())
    {
        Serial.println("Connecting to MQTT...");
        if (mqttClient.connect(hostname_char))
        { //, mqttUser, mqttPassword )) {
            Serial.print("MAC: ");
            Serial.println(WiFi.macAddress());
            Serial.println("Connected to MQTT!\n");
            mqttSubscribeToTopics(&mqttClient);
            return true;
        }
        else
        {
            Serial.print("Connection failed with state ");
            Serial.print(mqttClient.state());
            delay(200);
            return false;
        }
    }
}


void check_mqtt_connection()
{
    if (!mqttClient.connected())
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
    mqttClient.publish(topic_output, message_output);

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
    mqttClient.publish(topic_char_array, message_char_array);

    yield();
}
