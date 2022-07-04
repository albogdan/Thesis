#include "main.h"

bool setGoogleIoTCredentials(GoogleIOTCredentials *credentials)
{
  unsigned long id;
  Serial.println("Entered set function");
  if (credentials == NULL)
  {
    return false;
  }

  // Creating new file
  Serial.println("Creating new SPIFFS file");
  File configFile = SPIFFS.open("/google_iot_config.json", "w");

  if (configFile)
  {
    Serial.println("[INFO] Opened GoogleIoT config file for writing");
    DynamicJsonDocument json(2048);

    json["project_id"] = credentials->project_id;
    json["location"] = credentials->location;
    json["registry_id"] = credentials->registry_id;
    json["device_id"] = credentials->device_id;
    json["private_key_str"] = credentials->private_key_str;
    json["root_cert"] = credentials->root_cert;

    // serializeJson(json, Serial);
    serializeJson(json, configFile);
    configFile.close();
    return true;
  }
  return false;
}

bool getGoogleIoTCredentials(GoogleIOTCredentials *credentials)
{
  if (SPIFFS.exists("/google_iot_config.json"))
  {
    // File exists, reading and loading
    File configFile = SPIFFS.open("/google_iot_config.json", "r");

    if (configFile)
    {
      Serial.println("[INFO] Opened GoogleIoT config file for reading");
      size_t size = configFile.size();
      // Allocate a buffer to store contents of the file.
      std::unique_ptr<char[]> buf(new char[size]);

      configFile.readBytes(buf.get(), size);
      DynamicJsonDocument json(1024);
      auto deserializeError = deserializeJson(json, buf.get());
      // serializeJson(json, Serial);
      if (!deserializeError)
      {
        strcpy(credentials->project_id, json["project_id"]);
        strcpy(credentials->location, json["location"]);
        strcpy(credentials->registry_id, json["registry_id"]);
        strcpy(credentials->device_id, json["device_id"]);
        strcpy(credentials->private_key_str, json["private_key_str"]);
        strcpy(credentials->root_cert, json["root_cert"]);

        configFile.close();
        Serial.print("[INFO] Project ID: ");
        Serial.println(credentials->project_id);
        Serial.print("[INFO] Location: ");
        Serial.println(credentials->location);
        Serial.print("[INFO] Registry ID: ");
        Serial.println(credentials->registry_id);
        Serial.print("[INFO] Device ID: ");
        Serial.println(credentials->device_id);
        Serial.print("[INFO] Private Key Str: ");
        Serial.println(credentials->private_key_str);
        Serial.print("[INFO] Root Certificate: ");
        Serial.println(credentials->root_cert);
        return true;
      }
      else
      {
        Serial.println("[INFO] Failed to load JSON config");
        configFile.close();
        return false;
      }
    }
    Serial.println("[INFO] No config file found");
  }
  return false;
}
