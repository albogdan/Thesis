// ---------------------------------- BEGIN SERVER HANDLERS ----------------------------------

void httpServerSetupAndStart()
{
    Serial.println("[INFO] Setting up web server");
    // Setup endpoints
    server.on("/loadInfo", HTTP_GET, handleInfoPageLoad);
    server.on("/wifiSaveSTA", HTTP_GET, handleSetSTAWiFiCredentials);
    server.on("/wifiSaveAP", HTTP_GET, handleSetAPWiFiCredentials);
    server.on("/mqttSave", HTTP_GET, handleSetMQTTCredentials);
    server.on("/gatewayModeSave", HTTP_GET, handleSetGatewayMode);
    server.on("/restart", HTTP_POST, handleESPRestart);
    server.on("/factoryreset", HTTP_POST, handleFactoryReset);

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
    Serial.println("[INFO] HTTP Server started");
}

void handleSetSTAWiFiCredentials() {
  WiFiCredentials credentials;
  server.arg("ssid").toCharArray(credentials.ssid, server.arg("ssid").length() + 1);
  server.arg("password").toCharArray(credentials.password, server.arg("password").length() + 1);
  Serial.print("SERVER ARG IDENTITY: ");
  Serial.println(server.arg("identity"));
  if (server.arg("identity") != NULL) {
    Serial.println("SETTING IDENTITY");
    server.arg("identity").toCharArray(credentials.identity, server.arg("identity").length() + 1);
  } else {
    credentials.identity[0] = 0;
  }
  Serial.print("Identity len: ");
  Serial.println(strlen(credentials.identity));
  Serial.print("SSID: ");
  Serial.println(credentials.ssid);
  Serial.print("Password: ");
  Serial.println(credentials.password);
  if (setSTAWiFiCredentials(&credentials))
    Serial.println("[INFO] Saved STA WiFi credentials.");
  else
    Serial.println("[INFO] Failed to save STA WiFi credentials.");
  server.sendHeader("Location","/");
  server.send(303);
}
void handleSetAPWiFiCredentials() {
  WiFiCredentials credentials;
  
  server.arg("ssid").toCharArray(credentials.ssid, server.arg("ssid").length() + 1);
  server.arg("password").toCharArray(credentials.password, server.arg("password").length() + 1);

  Serial.print("SSID: ");
  Serial.println(credentials.ssid);
  Serial.print("Password: ");
  Serial.println(credentials.password);
  if (setAPWiFiCredentials(&credentials))
    Serial.println("[INFO] Saved AP WiFi credentials.");
  else
    Serial.println("[INFO] Failed to save AP WiFi credentials.");
  server.sendHeader("Location","/");
  server.send(303);
}
void handleSetMQTTCredentials() {
  MQTTCredentials credentials;
  server.arg("server_ip").toCharArray(credentials.server_ip, server.arg("server_ip").length() + 1);
  server.arg("port").toCharArray(credentials.port, server.arg("port").length() + 1);
  server.arg("username").toCharArray(credentials.username, server.arg("username").length() + 1);
  server.arg("password").toCharArray(credentials.password, server.arg("password").length() + 1);
  
  Serial.print("Server IP: ");
  Serial.println(credentials.server_ip);
  Serial.print("Port: ");
  Serial.println(credentials.port);
  if (setMQTTCredentials(&credentials))
    Serial.println("[INFO] Saved MQTT credentials.");
  else
    Serial.println("[INFO] Failed to save MQTT credentials.");
  server.sendHeader("Location","/");
  server.send(303);
}

void handleSetGatewayMode() {
  Serial.print("Gateway mode:");
  Serial.println(server.arg("gateway_mode"));
  if(server.arg("gateway_mode") == "wifi") {
    Serial.println("WiFi Gateway Mode");
    saveGatewayMode(GATEWAY_MODE_WIFI);
  } else if(server.arg("gateway_mode") == "cellular") {
    Serial.println("Cellular Gateway Mode");
    saveGatewayMode(GATEWAY_MODE_CELLULAR);
  }
  server.sendHeader("Location","/");
  server.send(303);
}

bool saveGatewayMode(unsigned int gateway_mode) {
    // Creating new file
    File configFile = SPIFFS.open("/gateway_mode.json", "w");

    if (configFile) {
      Serial.println("[INFO] Opened Gateway Mode config file");
      DynamicJsonDocument json(1024);
      json["gateway_mode"] = gateway_mode;

      //serializeJson(json, Serial);
      serializeJson(json, configFile);
      configFile.close();
      return true;
    }
  Serial.println("Returning false");
  return false;
}

bool getGatewayMode(unsigned int* gateway_mode) {
  if (SPIFFS.exists("/gateway_mode.json")) {
    //File exists, reading and loading
    File configFile = SPIFFS.open("/gateway_mode.json", "r");

    if (configFile) {
      Serial.println("[INFO] Opened Gateway Mode config file");
      size_t size = configFile.size();
      // Allocate a buffer to store contents of the file.
      std::unique_ptr<char[]> buf(new char[size]);

      Serial.println("[INFO] Reading config file bytes");
      configFile.readBytes(buf.get(), size);
      Serial.println("[INFO] Creating dynamic JSON document");
      DynamicJsonDocument json(1024);
      Serial.println("[INFO] Deserializing gateway mode config JSON");
      auto deserializeError = deserializeJson(json, buf.get());
      //serializeJson(json, Serial);
      if ( ! deserializeError ) {
        *gateway_mode = json["gateway_mode"];
        configFile.close();

        Serial.print("[INFO] Gateway Mode: ");
        Serial.println(*gateway_mode);
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

void handleInfoPageLoad() {
  Serial.println("[INFO] Handling Info Page Load");
  String jsonMap;
  const size_t capacity = JSON_OBJECT_SIZE(22);
  DynamicJsonDocument doc(capacity);

  // Gateway Mode Information
  unsigned int gateway_mode;
  getGatewayMode(&gateway_mode);
  if (gateway_mode == GATEWAY_MODE_WIFI) {
    doc["gateway_mode"] = "WiFi";
  } else if (gateway_mode == GATEWAY_MODE_CELLULAR) {
    doc["gateway_mode"] = "Cellular";
  }

  // STA Information
  if (WiFi.status() == WL_CONNECTED)
    doc["sta_status"] = "Connected";
  else
    doc["sta_status"] = "Disconnected";
  WiFiCredentials sta_credentials;
  if (getSTAWiFiCredentials(&sta_credentials)) {
    if (strlen(sta_credentials.identity) == 0) {
      doc["sta_encrypt_mode"] = "WPA2 Personal";
    } else {
      doc["sta_encrypt_mode"] = "WPA2 Enterprise";
    }
  } else {
    doc["sta_encrypt_mode"] = "N/A";
  }
  doc["sta_ip_address"] = WiFi.localIP();

  // AP Information
  doc["ap_ip_address"] = WiFi.softAPIP();
  WiFiCredentials ap_credentials;
  if (getAPWiFiCredentials(&ap_credentials)) {
    doc["ap_ssid"] = ap_credentials.ssid;
    doc["ap_password"] = ap_credentials.password;
  } else {
    doc["ap_ssid"] = ap_ssid;
    doc["ap_password"] = ap_password;
  }

  // MQTT Information
  if (mqtt_connection_made) {
    doc["mqtt_status"] = "Connected";
  } else {
    doc["mqtt_status"] = "Disconnected";
  }
  MQTTCredentials mqtt_credentials;
  if (getMQTTCredentials(&mqtt_credentials)) {
    doc["mqtt_ip"] = mqtt_credentials.server_ip;
    doc["mqtt_port"] = mqtt_credentials.port;
  } else {
    doc["mqtt_ip"] = "0.0.0.0";
    doc["mqtt_port"] = "N/A";
  }
  
  // Other Information
  doc["mac_address"] = WiFi.macAddress();
  doc["rssi"] = WiFi.RSSI();
  
  serializeJson(doc, jsonMap);
  server.send(200, "text/json", jsonMap);
}

void handleESPRestart(){
  Serial.println("[INFO] Restarting ESP32.");
  server.sendHeader("Location","/");
  server.send(303);
  ESP.restart();
}

void handleFactoryReset(){
  if (SPIFFS.exists("/sta_wifi_config.json")) {
    SPIFFS.remove("/sta_wifi_config.json");
  }
  if (SPIFFS.exists("/ap_wifi_config.json")) {
    SPIFFS.remove("/ap_wifi_config.json");
  }
  if (SPIFFS.exists("/mqtt_config.json")) {
    SPIFFS.remove("/mqtt_config.json");
  }
  if (SPIFFS.exists("/gateway_mode.json")) {
    SPIFFS.remove("/gateway_mode.json");
  }
  
  server.sendHeader("Location","/");
  server.send(303);
  ESP.restart();
}


// ---------------------------------- END SERVER HANDLERS ----------------------------------

// ---------------------------------- BEGIN SERVER HELPERS ----------------------------------

// https://arduinojson.org/v6/assistant/

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

}
// ---------------------------------- END SERVER HELPERS ----------------------------------
