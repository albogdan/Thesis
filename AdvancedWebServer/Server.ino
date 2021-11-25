
#include <FS.h>
#include <SPIFFS.h>


#define WIFI_CONNECT_TIMEOUT_SECONDS 5


// ---------------------------------- BEGIN WIFI HELPERS ----------------------------------

void connectToWiFi()
{
  WiFi.hostname(HOSTNAME);
  WiFi.mode(WIFI_MODE_APSTA);
  connectToSTAWiFi();
  connectToAPWiFi();
  
//  if (connectToSTAWiFi()) {
//    connectToAPWiFi();
//  } else {
//    WiFi.disconnect();
//    WiFi.mode(WIFI_MODE_AP);
//    connectToAPWiFi();
//  }
}

bool connectToSTAWiFi(){
  Serial.println("[INFO] Connecting to STA WiFi");
  WiFiCredentials credentials;
  if (getWiFiCredentials(&credentials)) {
    Serial.println("[INFO] Got WiFi credentials. Starting AP_STA mode");
    WiFi.begin(credentials.ssid, credentials.password);
    unsigned int timeout = 4 * WIFI_CONNECT_TIMEOUT_SECONDS;
    while (WiFi.status() != WL_CONNECTED && (timeout > 0))
    {
        delay(250);
        Serial.print(".");
        timeout -=1;
    }
    Serial.println('\n');
    Serial.print("[INFO] Connected to ");
    Serial.println(WiFi.SSID()); // Tell us what network we're connected to
    Serial.print("[INFO] STA IP address:\t");
    Serial.println(WiFi.localIP()); // Send the IP address of the ESP32
    if (WiFi.status() == WL_CONNECTED){
      return true;
    }
  }
  return false;
}
void connectToAPWiFi() {
  Serial.println("[INFO] Creating AP WiFi");
  WiFi.softAP(ap_ssid, ap_password);
  Serial.print("[INFO] AP IP address:\t");
  Serial.println(WiFi.softAPIP()); // Send the IP address of the ESP32 AP
}


bool getWiFiCredentials(WiFiCredentials *credentials) {
  if (SPIFFS.exists("/wifi_config.json")) {  
    //File exists, reading and loading
    Serial.println("reading config file");
    File configFile = SPIFFS.open("/wifi_config.json", "r");
    
    if (configFile) {
      Serial.println("[INFO] Opened config file");
      size_t size = configFile.size();
      // Allocate a buffer to store contents of the file.
      std::unique_ptr<char[]> buf(new char[size]);

      configFile.readBytes(buf.get(), size);
      DynamicJsonDocument json(1024);
      auto deserializeError = deserializeJson(json, buf.get());
      serializeJson(json, Serial);
      if ( ! deserializeError ) {
        Serial.println("\nparsed json");
        strcpy(credentials->ssid, json["ssid"]);
        strcpy(credentials->password, json["password"]);
        configFile.close();
        Serial.print("SSID: ");
        Serial.println(credentials -> ssid);
        Serial.print("Password: ");
        Serial.println(credentials -> password);
        return true;
      } else {
        Serial.println("[INFO] Failed to load JSON config");
        configFile.close();
        return false;
      }
    }
    Serial.println("[INFO] No config file found");
  }
  return false;
}

bool saveWiFiCredentials(WiFiCredentials *credentials) {
//  if (SPIFFS.exists("/wifi_config.json")) {
    //File exists, reading and loading
    Serial.println("reading config file");
    File configFile = SPIFFS.open("/wifi_config.json", "w");
    
    if (configFile) {
      Serial.println("[INFO] Opened WiFi config file");
      DynamicJsonDocument json(1024);
      json["ssid"] = credentials->ssid;
      json["password"] = credentials->password;
      
      serializeJson(json, Serial);
      serializeJson(json, configFile);
      configFile.close();
      connectToSTAWiFi();
      return true;
    }
//  }
  Serial.println("Returning false");
  return false; 
}

bool saveMQTTCredentials(MQTTCredentials *credentials) {
    //File exists, reading and loading
    Serial.println("reading config file");
    File configFile = SPIFFS.open("/mqtt_config.json", "w");
    
    if (configFile) {
      Serial.println("[INFO] Opened MQTT config file");
      DynamicJsonDocument json(1024);
      json["server_ip"] = credentials->server_ip;
      json["port"] = credentials->port;
      
      serializeJson(json, Serial);
      serializeJson(json, configFile);
      configFile.close();
      return true;
    }
  Serial.println("Returning false");
  return false; 
}

bool getMQTTCredentials(MQTTCredentials *credentials) {
  if (SPIFFS.exists("/mqtt_config.json")) {  
    //File exists, reading and loading
    Serial.println("reading config file");
    File configFile = SPIFFS.open("/mqtt_config.json", "r");
    
    if (configFile) {
      Serial.println("[INFO] Opened config file");
      size_t size = configFile.size();
      // Allocate a buffer to store contents of the file.
      std::unique_ptr<char[]> buf(new char[size]);

      configFile.readBytes(buf.get(), size);
      DynamicJsonDocument json(1024);
      auto deserializeError = deserializeJson(json, buf.get());
      serializeJson(json, Serial);
      if ( ! deserializeError ) {
        Serial.println("\nparsed json");
        strcpy(credentials->server_ip, json["server_ip"]);
        strcpy(credentials->port, json["port"]);
        configFile.close();
        Serial.print("Server IP: ");
        Serial.println(credentials -> server_ip);
        Serial.print("Port: ");
        Serial.println(credentials -> port);
        return true;
      } else {
        Serial.println("[INFO] Failed to load JSON config");
        configFile.close();
        return false;
      }
    }
    Serial.println("[INFO] No config file found");
  }
  return false;
}

// ---------------------------------- END WIFI HELPERS ----------------------------------


// ---------------------------------- BEGIN SERVER HANDLERS ----------------------------------

void httpServerSetupAndStart()
{
    Serial.println("[INFO] Setting up web server");
    // Setup endpoints
    server.on("/loadInfo", HTTP_GET, handleInfoPageLoad);
    server.on("/wifiSave", HTTP_GET, handleSetWiFiCredentials);
    server.on("/mqttSave", HTTP_GET, handleSetMQTTCredentials);
    server.on("/restart", HTTP_POST, handleESPRestart);
    server.on("/clearall", HTTP_POST, handleClearAllCredentials);

    server.onNotFound([]() {                                  // If the client requests any URI
        if (!handleFileRead(server.uri()))                    // send it if it exists
            server.send(404, "text/plain", "404: Not Found"); // otherwise, respond with a 404 (Not Found) error
    });
    if (MDNS.begin("esp32"))
    { // Start the mDNS responder for esp8266.local
        Serial.println("[INFO] mDNS responder started");
    }
    else
    {
        Serial.println("[ERROR] Error setting up MDNS responder!");
    }
    server.begin();
}

void handleSetWiFiCredentials() {
  WiFiCredentials credentials;
  server.arg("ssid").toCharArray(credentials.ssid, server.arg("ssid").length() + 1);
  server.arg("password").toCharArray(credentials.password, server.arg("password").length() + 1);
  Serial.print("SSID: ");
  Serial.println(credentials.ssid);
  Serial.print("Password: ");
  Serial.println(credentials.password);
  if (saveWiFiCredentials(&credentials))
    Serial.println("[INFO] Saved WiFi credentials.");
  else
    Serial.println("[INFO] Failed to save WiFi credentials.");
  server.sendHeader("Location","/");
  server.send(303);
}

void handleSetMQTTCredentials() {
  MQTTCredentials credentials;
  server.arg("server_ip").toCharArray(credentials.server_ip, server.arg("server_ip").length() + 1);
  server.arg("port").toCharArray(credentials.port, server.arg("port").length() + 1);
  Serial.print("Server IP: ");
  Serial.println(credentials.server_ip);
  Serial.print("Port: ");
  Serial.println(credentials.port);
  if (saveMQTTCredentials(&credentials))
    Serial.println("[INFO] Saved MQTT credentials.");
  else
    Serial.println("[INFO] Failed to save MQTT credentials.");
  server.sendHeader("Location","/");
  server.send(303);
}

void handleInfoPageLoad() {
  Serial.println("[INFO] Handling Info Page Load");
  String jsonMap;
  const size_t capacity = JSON_OBJECT_SIZE(8);
  DynamicJsonDocument doc(capacity);
  if (WiFi.status() == WL_CONNECTED)
    doc["sta_status"] = "Connected";
  else
    doc["sta_status"] = "Disconnected";    
  doc["sta_ip_address"] = WiFi.localIP();
  doc["ap_ip_address"] = WiFi.softAPIP();
  doc["mac_address"] = WiFi.macAddress();
  doc["rssi"] = WiFi.RSSI();
  serializeJson(doc, jsonMap);
  server.send(200, "text/json", jsonMap);
}

void handleESPRestart(){
  Serial.println("[INFO] Restarting ESP32.");
  ESP.restart();
}

void handleClearAllCredentials(){
  if (SPIFFS.exists("/wifi_config.json")) {
    SPIFFS.remove("/wifi_config.json");
  }
  if (SPIFFS.exists("/mqtt_config.json")) {
    SPIFFS.remove("/mqtt_config.json");
  }
  
  server.sendHeader("Location","/");
  server.send(303);
}


// ---------------------------------- END SERVER HANDLERS ----------------------------------

// ---------------------------------- BEGIN SERVER HELPERS ----------------------------------

// https://arduinojson.org/v6/assistant/
/*void getSerializedJson(String *map)
{
    const size_t capacity = JSON_OBJECT_SIZE(5);
    DynamicJsonDocument doc(capacity);
    doc["onSpeed"] = SPEED_ON;
    doc["offSpeed"] = SPEED_OFF;
    doc["light"] = photoresistorValue;
    doc["onThres"] = PHOTORESISTOR_THRESHOLD_ON;
    doc["offThres"] = PHOTORESISTOR_THRESHOLD_OFF;

    serializeJson(doc, *map);
}*/

String getContentType(String filename)
{
    if (filename.endsWith(".html"))
        return "text/html";
    else if (filename.endsWith(".css"))
        return "text/css";
    else if (filename.endsWith(".js"))
        return "application/javascript";
    else if (filename.endsWith(".ico"))
        return "image/x-icon";
    else if (filename.endsWith(".gz"))
        return "application/x-gzip";
    return "text/plain";
}

bool handleFileRead(String path)
{ // send the right file to the client (if it exists)
    Serial.println("handleFileRead: " + path);
    if (path.endsWith("/"))
        path += "index.html";                  // If a folder is requested, send the index file
    String contentType = getContentType(path); // Get the MIME type
    String pathWithGz = path + ".gz";
    if (SPIFFS.exists(pathWithGz) || SPIFFS.exists(path))
    {                                                       // If the file exists, either as a compressed archive, or normal
        if (SPIFFS.exists(pathWithGz))                      // If there's a compressed version available
            path += ".gz";                                  // Use the compressed version
        File file = SPIFFS.open(path, "r");                 // Open the file
        size_t sent = server.streamFile(file, contentType); // Send it to the client
        file.close();                                       // Close the file again
        Serial.println(String("\tSent file: ") + path);
        return true;
    }
    Serial.println(String("\tFile Not Found: ") + path);
    return false; // If the file doesn't exist, return false
}

void startSpiffs(){ 
  Serial.println("[INFO] Starting SPI Flash File System");
  SPIFFS.begin(); // Start the SPI Flash Files System
}
// ---------------------------------- END SERVER HELPERS ----------------------------------
