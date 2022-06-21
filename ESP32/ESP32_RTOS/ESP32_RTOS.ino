#include "main.h"

bool mqtt_connection_made = false;
unsigned int wifi_connection_status = false;
unsigned int cellular_connection_status = false;

unsigned int uplink_connection_mode;

PubSubClient mqttClient;

void setup(void)
{
  // Start the USB serial port
  Serial.begin(115200);
  delay(10);
  Serial.println("[SETUP] Serial0 setup complete");

  // Define the Arduino serial port and start it
  SerialArduino.begin(ARDUINO_BAUD_RATE, SWSERIAL_8N1, RXD2, TXD2); // Hardware Serial
  delay(10);
  Serial.println("[SETUP] SerialArduino setup complete");

  // Start the SPI Flash Files System
  Serial.println("[INFO] Starting SPI Flash File System");
  SPIFFS.begin();

  // If there is no default uplink_mode.json, create it and
  // set default mode as WiFi
  if (!SPIFFS.exists("/uplink_mode.json")) {
    uplink_connection_mode = UPLINK_MODE_WIFI;
    saveUplinkMode(UPLINK_MODE_WIFI);
  } else {
    getUplinkMode(&uplink_connection_mode);
  }

  
  if (uplink_connection_mode == UPLINK_MODE_WIFI) {
    // Connect to the WiFi network
    Serial.println("[INFO] Connecting to WiFi");
    wifi_connection_status = connectToWiFi();

    // Define the MQTT client
    mqttClient = mqttClientWiFi;
  }

  // Set up and start the HTTP server for configurations
  Serial.println("[INFO] Starting WebServer");
  httpServerSetupAndStart();

  // Set timezone to Eastern Standard Time
  setenv("TZ", "EST5EDT4,M3.2.0/02:00:00,M11.1.0/02:00:00", 1);
  tzset();

  if(wifi_connection_status == WIFI_APSTA_CONNECTED || cellular_connection_status == CELLULAR_CONNECTION_SUCCESS) {
    mqtt_connection_made = setupAndConnectToMqtt();
    if (!mqtt_connection_made)
      Serial.println("[ERROR] MQTT connection unsuccessful");

    configure_time();
  }

  if (SLEEP_ENABLED) {
    // Initialize the sleep timer CHANGE THIS TO ALSO HAPPEN WHEN CELLULAR IS CONNECTED
    Serial.println("[INFO] Initializing sleep timer");
    initialize_sleep_timer(SLEEP_TIMER_SECONDS);
  }

  // Signal to the Arduino that the device has booted
  Serial.println("[SETUP] Boot sequence complete.");
  pinMode(ARDUINO_SIGNAL_READY_PIN, OUTPUT);
  digitalWrite(ARDUINO_SIGNAL_READY_PIN, HIGH);

}

void loop(void)
{
  if(mqtt_connection_made) {
    mqttClient.loop();
//    loop_and_check_mqtt_connection();
  }
  
  // Check if we received a complete string through SerialArduino
  if (byte_array_complete)
  {
    // Publish the string to MQTT
    parse_input_string(input_byte_array);

    // Clear the byte array: reset the index pointer
    input_byte_array_index = 0;
    byte_array_complete = false;
  }

  // Check if there's anything in the SerialArduino buffer
  while (SerialArduino.available())
  {
    // Get the new byte
    byte inByte = (byte)SerialArduino.read();

    // Add it to the inputString:
    input_byte_array[input_byte_array_index] = inByte;
    input_byte_array_index += 1;

    // If the incoming character is a newline, set a flag so 
    // the main loop can do something about it:
    if (inByte == '\n')
    {
      byte_array_complete = true;
    }
  }

  // Allow the CPU to handle HTTP requests
  server.handleClient();
  delay(2); //allow the cpu to switch to other tasks
//  update_current_local_time();
//  Serial.println(strftime_buf);
}

void configure_time() {
  if (cellular_connection_status == CELLULAR_CONNECTION_SUCCESS) {
    Serial.println("[SETUP] Waiting for system time to be set from cellular...");
    cellular_modem.NTPServerSync();
    int   year3    = 0;
    int   month3   = 0;
    int   day3     = 0;
    int   hour3    = 0;
    int   min3     = 0;
    int   sec3     = 0;
    float timezone = 0;
    cellular_modem.getNetworkTime(&year3, &month3, &day3, &hour3, &min3, &sec3,
                             &timezone);
    timeinfo.tm_sec = sec3;
    timeinfo.tm_min = min3;
    timeinfo.tm_hour = hour3;
    timeinfo.tm_mday = day3;
    timeinfo.tm_mon = month3 - 1;
    timeinfo.tm_year = year3 - 1900;

    now = mktime(&timeinfo);
    timeval time_now;
    time_now.tv_sec = now; //Assign time_here to this object.
    time_now.tv_usec = 0; //As time_t can hold only seconds.

    settimeofday(&time_now, NULL);

  } else if(wifi_connection_status == WIFI_APSTA_CONNECTED) {
    // Setup periodic synch with the NTP server
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "pool.ntp.org");
    sntp_init();

    // If the year is not correct, sync it up
    if (timeinfo.tm_year < (2019 - 1900)) {
        int retry = 0;
        const int retry_count = 10;
        while (sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET && ++retry < retry_count) {
            Serial.print("[SETUP] Waiting for system time to be set through NTP...(");
            Serial.print(retry);
            Serial.print("/");
            Serial.print(retry_count);
            Serial.println(")");
            vTaskDelay(2000 / portTICK_PERIOD_MS);
        }
    }
  }
  Serial.print("[INFO] The current time is: ");
  update_current_local_time();
  Serial.println(strftime_buf);
}


void update_current_local_time()
{
  time(&now); // Update the time
  localtime_r(&now, &timeinfo); // Convert to the timeinfo variable
  strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo); // Print the timeinfo variable
}
