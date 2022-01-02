#include "main.h"

bool mqtt_connection_made = false;
void setup(void)
{
  // Start the USB serial port
  Serial.begin(115200);
  delay(10);
  Serial.println("[SETUP] Serial0 setup complete");

  // Define the Arduino serial port and start it
  Serial2.begin(115200, SERIAL_8N1, RXD2, TXD2);
  delay(10);
  Serial.println("[SETUP] Serial2 setup complete");

  // Start the SPI Flash Files System
  Serial.println("[INFO] Starting SPI Flash File System");
  SPIFFS.begin();

  // Connect to the WiFi network
  Serial.println("[INFO] Connecting to WiFi");
  connectToWiFi();

  // Set up and start the HTTP server for configurations
  Serial.println("[INFO] Starting WebServer");
  httpServerSetupAndStart();

  mqtt_connection_made = setupAndConnectToMqtt();
  if (!mqtt_connection_made)
    Serial.println("[ERROR] MQTT connection unsuccessful");

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

  // Initialize the sleep timer
  Serial.println("[INFO] Initializing sleep timer");
  initialize_sleep_timer(SLEEP_TIMER_SECONDS);

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
  
  // Check if we received a complete string through Serial2
  if (byte_array_complete)
  {
    // Publish the string to MQTT
    parse_input_string(input_byte_array);

    // Clear the byte array: reset the index pointer
    input_byte_array_index = 0;
    byte_array_complete = false;
  }

  // Check if there's anything in the Serial2 buffer
  while (Serial2.available())
  {
    // Get the new byte
    byte inByte = (byte)Serial2.read();

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
}

void update_current_local_time()
{
  time(&now);
  localtime_r(&now, &timeinfo);
  strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
}
