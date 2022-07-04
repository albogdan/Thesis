#include "main.h"
#include "esp32-mqtt.h"

bool mqtt_connection_made = false;
unsigned int wifi_connection_status = false;
unsigned int cellular_connection_status = false;

unsigned int uplink_connection_mode;

PubSubClient mqttClient;
unsigned long lastMillis = 0;

TaskHandle_t CottonCandyGateway;

GoogleIOTCredentials credentials;

void setup(void)
{
  // Start the USB serial port
  Serial.begin(115200);
  delay(10);
  Serial.println("[SETUP] Serial0 setup complete");

  // Define the Arduino serial port and start it
  SerialArduino.begin(ARDUINO_BAUD_RATE, SWSERIAL_8N1, RXD2, TXD2); // Software Serial
  delay(10);
  Serial.println("[SETUP] SerialArduino setup complete");
  //
  //  xTaskCreatePinnedToCore(
  //                  handleCottonCandyGateway,   /* Task function. */
  //                  "CottonCandyGateway",     /* name of task. */
  //                  10000,       /* Stack size of task */
  //                  NULL,        /* parameter of the task */
  //                  1,           /* priority of the task */
  //                  &CottonCandyGateway,      /* Task handle to keep track of created task */
  //                  0);          /* pin task to core 0 */
  //
  Serial.print("[INFO] setup() running on core ");
  Serial.println(xPortGetCoreID());

  // Start the SPI Flash Files System
  Serial.println("[INFO] Starting SPI Flash File System");
  SPIFFS.begin();

  // Set the NetID if it doesn't exist
  if (!SPIFFS.exists("/net_id.json"))
  {
    char q[] = "12332122";
    setNetId(q);
  }

  getNetId(network_id);
  Serial.print("[INFO] NetworkID: ");
  Serial.println(network_id);

  // If there is no default uplink_mode.json, create it and
  // set default mode as WiFi
  if (!SPIFFS.exists("/uplink_mode.json"))
  {
    uplink_connection_mode = UPLINK_MODE_WIFI;
    saveUplinkMode(UPLINK_MODE_WIFI);
  }
  else
  {
    getUplinkMode(&uplink_connection_mode);
  }

  if (uplink_connection_mode == UPLINK_MODE_WIFI)
  {
    // Connect to the WiFi network
    Serial.println("[INFO] Connecting to WiFi");
    wifi_connection_status = connectToWiFi();

    // Define the MQTT client
    mqttClient = mqttClientWiFi;
  }
  else if (uplink_connection_mode == UPLINK_MODE_CELLULAR)
  {
    // Start the WiFi AP
    WiFi.hostname(HOSTNAME);
    WiFi.mode(WIFI_MODE_AP);
    connectToAPWiFi();
    wifi_connection_status = WIFI_AP_CONNECTED;

    // Define the MQTT client
    mqttClient = mqttClientCellular;
    delay(6000);
    // Connect to Cellular
    // SerialCellular.begin(GSM_BAUD_RATE, SWSERIAL_8N1, RXCellular, TXCellular); // Software Serial, known baud rate
    SerialCellular.begin(115200);
    // TinyGsmAutoBaud(SerialCellular, GSM_AUTOBAUD_MIN, GSM_AUTOBAUD_MAX); // Use when baud rate unknown, Hardware Serial

    delay(500);
    Serial.println("[SETUP] Cellular Serial Setup Complete");
    cellular_connection_status = connectToCellular();
  }

  // Set up and start the HTTP server for configurations
  Serial.println("[INFO] Starting WebServer");
  httpServerSetupAndStart();

  // Set timezone to Eastern Standard Time
  setenv("TZ", "EST5EDT4,M3.2.0/02:00:00,M11.1.0/02:00:00", 1);
  tzset();

  Serial.println("Trying to setup Google IOT");
  getGoogleIoTCredentials(&credentials);
  setupCloudIoT(credentials.project_id, credentials.location, credentials.registry_id, credentials.device_id, credentials.private_key_str, credentials.root_cert);

  if (wifi_connection_status == WIFI_APSTA_CONNECTED || cellular_connection_status == CELLULAR_CONNECTION_SUCCESS)
  {
    Serial.println("Wifi connection success");
    mqtt_connection_made = setupAndConnectToMqtt();
    if (!mqtt_connection_made)
      Serial.println("[ERROR] MQTT connection unsuccessful");

    if (wifi_connection_status == WIFI_APSTA_CONNECTED)
    {
      Serial.println("Trying to setup Google IOT");
      getGoogleIoTCredentials(&credentials);
      setupCloudIoT(credentials.project_id, credentials.location, credentials.registry_id, credentials.device_id, credentials.private_key_str, credentials.root_cert);
      mqtt_connection_made = true;
    }
    else
    {
      Serial.println("Uuable to setup Google CLoud IOT");
    }

    configure_time();
  }
  else
  {
    Serial.println("Wifi connection failed");
  }
  btStop();

  if (false)
  {
    // Initialize the sleep timer CHANGE THIS TO ALSO HAPPEN WHEN CELLULAR IS CONNECTED
    Serial.println("[INFO] Initializing sleep timer");
    initialize_sleep_timer(SLEEP_TIMER_SECONDS);
  }

  // Signal to the Arduino that the device has booted
  Serial.println("[SETUP] Boot sequence complete.");
  pinMode(ARDUINO_SIGNAL_READY_PIN, OUTPUT);
  digitalWrite(ARDUINO_SIGNAL_READY_PIN, HIGH);
}

bool first_loop = true;

void loop(void)
{
  if (first_loop)
  {
    Serial.print("[INFO] loop() running on core ");
    Serial.println(xPortGetCoreID());
    first_loop = false;
  }
  if (true)
  {
    // Serial.println("MQTT connection made");
    //  mqttClient.loop();
    mqtt->loop();

    delay(10); // <- fixes some issues with WiFi stability

    if (!google_mqttClient->connected())
    {
      connect();
    }

    if (millis() - lastMillis > 6000)
    {
      lastMillis = millis();
      // publishTelemetry(mqttClient, "/sensors", getDefaultSensor());
      Serial.println("Trying to publish message");
      if (publishTopic1(getDefaultTestData()))
      {
        Serial.println("Successfully published messsage!");
      }
      // else
      // {
      //   Serial.println("Failed to publish messsage!");
      // }
      // publishTelemetry();
    }

    //    loop_and_check_mqtt_connection();
  }

  // Check if we received a complete string through SerialArduino
  if (byte_array_complete)
  {
    // Publish the string to MQTT
    Serial.println("[INFO] Publishing");
    parse_input_string(input_byte_array);

    // Clear the byte array: reset the index pointer
    input_byte_array_index = 0;
    byte_array_complete = false;
  }
  /*
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
      break;
    }
  }
  */

  while (SerialArduino.available())
  {
    if (input_byte_array_index < 2)
    { // Receive the src
      input_byte_array[input_byte_array_index] = (byte)SerialArduino.read();
      input_byte_array_index += 1;
      Serial.println("[INFO] src complete");
    }
    else if (input_byte_array_index == 2)
    { // Receive the length
      input_byte_array[input_byte_array_index] = (byte)SerialArduino.read();
      input_byte_array_index += 1;
      Serial.println("[INFO] len complete");
    }
    else if (input_byte_array_index > 2 && input_byte_array_index < input_byte_array[2] + 2)
    { // Receive the data
      input_byte_array[input_byte_array_index] = (byte)SerialArduino.read();
      input_byte_array_index += 1;
      Serial.println("[INFO] data complete");
    }
    else if (input_byte_array_index == input_byte_array[2] + 2)
    { // Receive the data
      input_byte_array[input_byte_array_index] = (byte)SerialArduino.read();
      input_byte_array_index += 1;
      byte_array_complete = true;
      Serial.println("[INFO] Byte array complete");
      break;
    }
    else
    {
      Serial.print("[ERROR] Invalid array index:");
      Serial.println(input_byte_array_index);
    }
  }

  // Allow the CPU to handle HTTP requests
  server.handleClient();
  delay(2); // allow the cpu to switch to other tasks
  //  update_current_local_time();
  //  Serial.println(strftime_buf);
}

// Data length < 100
// Address cannot be 0x00 or 0xFF
// Remove println in gateway code

void configure_time()
{
  if (cellular_connection_status == CELLULAR_CONNECTION_SUCCESS)
  {
    Serial.println("[SETUP] Waiting for system time to be set from cellular...");
    cellular_modem.NTPServerSync();
    int year3 = 0;
    int month3 = 0;
    int day3 = 0;
    int hour3 = 0;
    int min3 = 0;
    int sec3 = 0;
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
    time_now.tv_sec = now; // Assign time_here to this object.
    time_now.tv_usec = 0;  // As time_t can hold only seconds.

    settimeofday(&time_now, NULL);
  }
  else if (wifi_connection_status == WIFI_APSTA_CONNECTED)
  {
    // Setup periodic synch with the NTP server
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, (char *)ntp_primary);
    sntp_init();

    // If the year is not correct, sync it up
    if (timeinfo.tm_year < (2019 - 1900))
    {
      int retry = 0;
      const int retry_count = 10;
      while (sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET && ++retry < retry_count)
      {
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
  time(&now);                                                    // Update the time
  localtime_r(&now, &timeinfo);                                  // Convert to the timeinfo variable
  strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo); // Print the timeinfo variable
}
