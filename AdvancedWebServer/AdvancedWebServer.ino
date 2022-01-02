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

#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <PubSubClient.h>
#include "main.h"

bool mqtt_connection_made = false;
void setup(void) {
  Serial.begin(115200);

  startSpiffs();

  connectToWiFi(); 

  httpServerSetupAndStart();
  
  Serial.println("HTTP server started");

  mqtt_connection_made = setupAndConnectToMqtt();
  if (!mqtt_connection_made)
    Serial.println("[ERROR] MQTT connection unsuccessful");
}

void loop(void) {
  server.handleClient();
  delay(2);//allow the cpu to switch to other tasks
}

void drawGraph() {
  String out = "";
  char temp[100];
  out += "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\" width=\"400\" height=\"150\">\n";
  out += "<rect width=\"400\" height=\"150\" fill=\"rgb(250, 230, 210)\" stroke-width=\"1\" stroke=\"rgb(0, 0, 0)\" />\n";
  out += "<g stroke=\"black\">\n";
  int y = rand() % 130;
  for (int x = 10; x < 390; x += 10) {
    int y2 = rand() % 130;
    sprintf(temp, "<line x1=\"%d\" y1=\"%d\" x2=\"%d\" y2=\"%d\" stroke-width=\"1\" />\n", x, 140 - y, x + 10, 140 - y2);
    out += temp;
    y = y2;
  }
  out += "</g>\n</svg>\n";

  server.send(200, "image/svg+xml", out);
}
