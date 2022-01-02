#include <WiFi.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include "esp_sntp.h"
#include "driver/timer.h"
#include "driver/rtc_io.h"
#include "main.h"


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
  // Check to make sure the MQTT connection is live
  check_mqtt_connection();


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

