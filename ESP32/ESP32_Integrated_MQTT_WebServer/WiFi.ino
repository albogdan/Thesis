#include "esp_wpa2.h" // For WPA2 Enterprise
#include "esp_wifi.h"
#define WIFI_CONNECT_TIMEOUT_SECONDS 8


// ---------------------------------- BEGIN WIFI CONNECTION FUNCTIONS ----------------------------------
int connectToWiFi() {
  WiFi.hostname(HOSTNAME);
  WiFi.mode(WIFI_MODE_APSTA);
//  connectToSTAWiFi();
//  connectToAPWiFi();
  
  if (connectToSTAWiFi()) {
    Serial.println("[INFO] STA connected successfully, starting AP");
    connectToAPWiFi();
    return WIFI_APSTA_CONNECTED;
  } else {
    Serial.println("[INFO] Failed to connect to STA, starting AP");
    WiFi.disconnect();
    WiFi.mode(WIFI_MODE_AP);
    connectToAPWiFi();
    return WIFI_AP_CONNECTED;
  }
}

bool connectToSTAWiFi(){
  Serial.println("[INFO] Connecting to STA WiFi");
  WiFiCredentials credentials;
  if (getSTAWiFiCredentials(&credentials)) {
    Serial.println("[INFO] Got WiFi credentials. Starting AP_STA mode");
    if (strlen(credentials.identity) == 0) { // Connect to WPA2 Personal WiFi network
      Serial.println("[INFO] Connecting to WPA2 Personal");
      WiFi.begin(credentials.ssid, credentials.password);
    } else { // Connect to WPA2 EnterpriseWiFi network
      Serial.println("[INFO] Connecting to WPA2 Enterprise");
      // WPA2 enterprise magic starts here
//      WiFi.disconnect(true);
//      WiFi.mode(WIFI_STA);
      esp_wifi_sta_wpa2_ent_set_identity((uint8_t *)credentials.identity, strlen(credentials.identity));
      esp_wifi_sta_wpa2_ent_set_username((uint8_t *)credentials.identity, strlen(credentials.identity));
      esp_wifi_sta_wpa2_ent_set_password((uint8_t *)credentials.password, strlen(credentials.password));
      esp_wpa2_config_t config = WPA2_CONFIG_INIT_DEFAULT();
      esp_wifi_sta_wpa2_ent_enable(&config);
      // WPA2 enterprise magic ends here
      WiFi.begin(credentials.ssid);
    }
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
  WiFiCredentials credentials;
  if (getAPWiFiCredentials(&credentials)) {
    WiFi.softAP(credentials.ssid, credentials.password);
  } else {
    WiFi.softAP(ap_ssid, ap_password);
  }
  Serial.print("[INFO] AP IP address:\t");
  Serial.println(WiFi.softAPIP()); // Send the IP address of the ESP32 AP
}
// ---------------------------------- END WIFI CONNECTION FUNCTIONS ----------------------------------

// ---------------------------------- BEGIN WIFI HELPERS ----------------------------------
bool getAPWiFiCredentials(WiFiCredentials *credentials) {
  if (SPIFFS.exists("/ap_wifi_config.json")) {  
    //File exists, reading and loading
    File configFile = SPIFFS.open("/ap_wifi_config.json", "r");
    
    if (configFile) {
      Serial.println("[INFO] Opened config file");
      size_t size = configFile.size();
      // Allocate a buffer to store contents of the file.
      std::unique_ptr<char[]> buf(new char[size]);

      configFile.readBytes(buf.get(), size);
      DynamicJsonDocument json(1024);
      auto deserializeError = deserializeJson(json, buf.get());
      //serializeJson(json, Serial);
      if ( ! deserializeError ) {
        strcpy(credentials->ssid, json["ssid"]);
        strcpy(credentials->password, json["password"]);
        configFile.close();
        Serial.print("[INFO] SSID: ");
        Serial.println(credentials -> ssid);
        //Serial.print("Password: ");
        //Serial.println(credentials -> password);
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
bool setAPWiFiCredentials(WiFiCredentials *credentials) {
    // Creating new file
    File configFile = SPIFFS.open("/ap_wifi_config.json", "w");
    
    if (configFile) {
      Serial.println("[INFO] Opened WiFi config file");
      DynamicJsonDocument json(1024);
      json["ssid"] = credentials->ssid;
      json["password"] = credentials->password;
      
      //serializeJson(json, Serial);
      serializeJson(json, configFile);
      configFile.close();
      ESP.restart();
      return true;
    }
  return false; 
}

bool getSTAWiFiCredentials(WiFiCredentials *credentials) {
  if (SPIFFS.exists("/sta_wifi_config.json")) {  
    //File exists, reading and loading
    File configFile = SPIFFS.open("/sta_wifi_config.json", "r");
    
    if (configFile) {
      Serial.println("[INFO] Opened config file");
      size_t size = configFile.size();
      // Allocate a buffer to store contents of the file.
      std::unique_ptr<char[]> buf(new char[size]);

      configFile.readBytes(buf.get(), size);
      DynamicJsonDocument json(1024);
      auto deserializeError = deserializeJson(json, buf.get());
      //serializeJson(json, Serial);
      if ( ! deserializeError ) {
        strcpy(credentials->ssid, json["ssid"]);
        strcpy(credentials->password, json["password"]);
        if (json.containsKey("identity")) {
          strcpy(credentials->identity, json["identity"]);
        } else {
          credentials->identity[0] = 0;
        }
        configFile.close();
        Serial.print("[INFO] SSID: ");
        Serial.println(credentials -> ssid);
        //Serial.print("Password: ");
        //Serial.println(credentials -> password);
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

bool setSTAWiFiCredentials(WiFiCredentials *credentials) {
    // Creating new file
    File configFile = SPIFFS.open("/sta_wifi_config.json", "w");
    
    if (configFile) {
      Serial.println("[INFO] Opened WiFi config file");
      DynamicJsonDocument json(1024);
      json["ssid"] = credentials->ssid;
      json["password"] = credentials->password;
      if (strlen(credentials->identity) != 0) {
        json["identity"] = credentials->identity;
      }
      
      //serializeJson(json, Serial);
      serializeJson(json, configFile);
      configFile.close();
      WiFi.disconnect();
      connectToWiFi();
      return true;
    }
  return false; 
}
// ---------------------------------- END WIFI HELPERS ----------------------------------
