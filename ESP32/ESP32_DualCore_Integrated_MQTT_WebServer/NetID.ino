bool setNetId(char *network_id) {
  unsigned long id;
  Serial.println("Entered set function");
  if (network_id == NULL) {
    return false;
  }
    
  // Creating new file
  Serial.println("Creating new SPIFFS file");
  File configFile = SPIFFS.open("/net_id.json", "w");
  
  if (configFile) {
    Serial.println("[INFO] Opened NetworkID config file for writing");
    DynamicJsonDocument json(1024);
    
    json["network_id"] =  network_id;
  
    //serializeJson(json, Serial);
    serializeJson(json, configFile);
    configFile.close();
    return true;
  }
  return false; 
}

bool getNetId(char *network_id) {
  if (network_id == NULL) {
    return false;
  }
  if (SPIFFS.exists("/net_id.json")) {  
    //File exists, reading and loading
    File configFile = SPIFFS.open("/net_id.json", "r");
    
    if (configFile) {
      Serial.println("[INFO] Opened NetworkID config file for reading");
      size_t size = configFile.size();
      // Allocate a buffer to store contents of the file.
      std::unique_ptr<char[]> buf(new char[size]);

      configFile.readBytes(buf.get(), size);
      DynamicJsonDocument json(1024);
      auto deserializeError = deserializeJson(json, buf.get());
      //serializeJson(json, Serial);
      if ( ! deserializeError ) {
        strcpy(network_id, json["network_id"]);

        configFile.close();
        Serial.print("[INFO] Network ID: ");
        Serial.println(*network_id);
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
