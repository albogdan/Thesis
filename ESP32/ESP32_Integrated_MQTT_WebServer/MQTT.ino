///#include <ESP32Ping.h>


long lastReconnectAttempt = 0;


bool setupAndConnectToMqtt() {
  Serial.println("[INFO] Getting MQTT server credentials");
  MQTTCredentials credentials;
  if (getMQTTCredentials(&credentials)) {
    Serial.println("[SETUP] Checking if MQTT server exists");
    bool ping_success = true;//WiFi.ping(credentials.server_ip); //Ping 3 times
    if (ping_success) {
      Serial.println("[SETUP] Setting up MQTT server...");
      setupMqtt(credentials.server_ip, credentials.port);
    
      Serial.println("[SETUP] Connecting to MQTT server...");
      return connectToMqtt(credentials.username, credentials.password);
    } else {
      Serial.println("[ERROR] MQTT Server Ping Failed");
      return false;
    }
  }
  return false;
}


void setupMqtt(char *mqttServer, char *mqttPort)
{
    mqttClient.setServer(mqttServer, atoi(mqttPort));
    mqttClient.setCallback(callbackMqtt);
    mqttClient.setBufferSize(1024);   // MAKE SURE TO CHECK THAT THIS IS BIG ENOUGH
    mqttClient.setSocketTimeout(5);
}

boolean connectToMqtt(char *username, char *password)
{
    char hostname_char[HOSTNAME.length() + 1];
    HOSTNAME.toCharArray(hostname_char, HOSTNAME.length() + 1);

    Serial.println("[SETUP] Connecting to MQTT...");
    if (mqttClient.connect(hostname_char, username, password))
    { //, mqttUser, mqttPassword )) {
        Serial.print("[INFO] MAC: ");
        Serial.println(WiFi.macAddress());
        Serial.println("[SETUP] Connected to MQTT!\n");
        mqttSubscribeToTopics(&mqttClient);
        return true;
    }
    else
    {
        Serial.print("[SETUP] Connection failed with state ");
        Serial.println(mqttClient.state());
        delay(200);
        return false;
    }
}


void loop_and_check_mqtt_connection()
{
    if (!mqttClient.connected())
    {
        long now = millis();
        if (now - lastReconnectAttempt > 10000)
        {
            lastReconnectAttempt = now;
            // Attempt to reconnect
            mqtt_connection_made = setupAndConnectToMqtt();
            if (mqtt_connection_made)
            {
                lastReconnectAttempt = 0;
            }
        }
    }
    else
    {
        mqttClient.loop();
    }
}



bool setMQTTCredentials(MQTTCredentials *credentials) {
    // Creating new file
    File configFile = SPIFFS.open("/mqtt_config.json", "w");
    
    if (configFile) {
      Serial.println("[INFO] Opened MQTT config file");
      DynamicJsonDocument json(1024);
      json["server_ip"] = credentials->server_ip;
      json["port"] = credentials->port;
      json["username"] = credentials->username;
      json["password"] = credentials->password;
      
      //serializeJson(json, Serial);
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
    File configFile = SPIFFS.open("/mqtt_config.json", "r");
    
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
        strcpy(credentials->server_ip, json["server_ip"]);
        strcpy(credentials->port, json["port"]);
        strcpy(credentials->username, json["username"]);
        strcpy(credentials->password, json["password"]);
        
        configFile.close();
        Serial.print("[INFO] Server IP: ");
        Serial.println(credentials -> server_ip);
        Serial.print("[INFO] Port: ");
        Serial.println(credentials -> port);
        Serial.print("[INFO] Username: ");
        Serial.println(credentials -> username);
        Serial.print("[INFO] Password: ");
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

void mqttSubscribeToTopics(PubSubClient *mqttClient)
{
    mqttClient->subscribe("ping");

    // All subscriptions should be of the form {HOSTNAME/endpoint}
    mqttClient->subscribe("ESP32_LORA/test");    // Subscribe function only takes char*, not String
}

void callbackMqtt(char *topic, byte *payload, unsigned int length)
{

    Serial.print("Message arrived in topic: ");
    Serial.println(topic);
    String topic_string = String(topic);

    Serial.print("Message:");
    String message = "";
    for (int i = 0; i < length; i++)
    {
        message += (char)payload[i];
    }
    Serial.println(message);

    if (topic_string.equals("ping"))
    {
        Serial.println("Calling ping response");
        return_ping();
    }
    else if (topic_string.equals("test"))
    {
        Serial.println("Message received in test topic");
    }

    Serial.println();
    Serial.println("-----------------------");
}

// Returns a ping with information about the system
void return_ping()
{
    String message;
    const size_t capacity = JSON_OBJECT_SIZE(8);
    DynamicJsonDocument doc(capacity);

    doc["ip_address"] = WiFi.localIP().toString();
    doc["mac_address"] = WiFi.macAddress();
    doc["hostname"] = HOSTNAME;
    doc["current_ssid"] = WiFi.SSID();

    serializeJson(doc, message);

    String topic = "ping/response/" + HOSTNAME;

    char topic_output[topic.length() + 1];
    char message_output[message.length() + 1];

    topic.toCharArray(topic_output, topic.length() + 1);
    message.toCharArray(message_output, message.length() + 1);

    Serial.print("Replying with: ");
    Serial.println(message);
    mqttClient.publish(topic_output, message_output);

    yield();
}

void publish_arduino_data(String arduino_data) {
    String message;
    const size_t capacity = JSON_OBJECT_SIZE(4);
    DynamicJsonDocument doc(capacity);

    doc["data_1"] = arduino_data.substring(0, arduino_data.indexOf(','));
    doc["data_2"] = arduino_data.substring(arduino_data.indexOf(',')+1, arduino_data.indexOf('\r'));

    serializeJson(doc, message);

    String topic = "data/" + HOSTNAME;

    char topic_char_array[topic.length() + 1];
    char message_char_array[message.length() + 1];

    topic.toCharArray(topic_char_array, topic.length() + 1);
    message.toCharArray(message_char_array, message.length() + 1);

    Serial.print("Publishing to MQTT: ");
    Serial.println(message);
    mqttClient.publish(topic_char_array, message_char_array);

    yield();
}
