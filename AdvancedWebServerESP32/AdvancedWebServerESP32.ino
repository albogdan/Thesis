/*
TODO
1. Open the config file if it exists and read the SSID from it and try connecting in STA mode
1.a) If connection succeeds:
  2. Start the web server normally
1.b) If connection fails:
  2. Disconnect WiFi and start it in AP mode.
  3. Start the web server normally
  4. When get credentials for WiFi update them in the config
      4.a) TODO LATER: multiple credentials and try connecting to all of them
      4.b) TODO LATER: delete all credentials
  5. Restart the ESP, so now it will try connecting with the new credentials.



Alternatively, always start in AP and STA mode for both cases.

*/

#include "main.h"

bool mqtt_connection_made = false;
void setup(void) {
  Serial.begin(115200);

  Serial.println("[INFO] Starting SPI Flash File System");
  SPIFFS.begin(); // Start the SPI Flash Files System

  connectToWiFi(); 

  httpServerSetupAndStart();
  
  mqtt_connection_made = setupAndConnectToMqtt();
  if (!mqtt_connection_made)
    Serial.println("[ERROR] MQTT connection unsuccessful");
}

void loop(void) {
  server.handleClient();
  delay(2);//allow the cpu to switch to other tasks
}
