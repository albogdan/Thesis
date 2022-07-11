#include <ESP32Ping.h>
#include "esp32-mqtt.h"

long lastReconnectAttempt = 0;
IPAddress server_ip_address; // The remote ip to ping

bool setupAndConnectToMqtt()
{

  Serial.println("[INFO] Getting MQTT server credentials");
  MQTTCredentials credentials;
  if (getMQTTCredentials(&credentials))
  {
    Serial.println("[SETUP] Checking if MQTT server exists");

    server_ip_address.fromString(credentials.server_ip);

    bool ping_success = true;
    /*if (uplink_connection_mode == UPLINK_MODE_WIFI) {
      ping_success = Ping.ping(server_ip_address, 2);
    }
    else
    {
      ping_success = true; // TODO: IMPLEMENT PING FOR THE CELLULAR SHIELD

    }*/
    if (ping_success)
    {
      setupMqtt(credentials.server_ip, credentials.port);

      return connectToMqtt(credentials.username, credentials.password);
    }
    else
    {
      Serial.println("[ERROR] MQTT server ping failed");
      return false;
    }
  }

  return false;
}

void setupMqtt(char *mqttServer, char *mqttPort)
{
  Serial.println("[SETUP] Setting up MQTT server...");
  mqttClient.setServer(mqttServer, atoi(mqttPort));
  mqttClient.setCallback(callbackMqtt);
  mqttClient.setBufferSize(1024); // MAKE SURE TO CHECK THAT THIS IS BIG ENOUGH
  mqttClient.setSocketTimeout(5);
}

boolean connectToMqtt(char *username, char *password)
{
  char hostname_char[HOSTNAME.length() + 1];
  HOSTNAME.toCharArray(hostname_char, HOSTNAME.length() + 1);

  Serial.println("[SETUP] Connecting to MQTT...");
  if (mqttClient.connected())
  {
    Serial.println("[INFO] MQTT Client is already connected!");
    return true;
  }
  else if (mqttClient.connect(hostname_char, username, password))
  { //, mqttUser, mqttPassword )) {
    Serial.print("[INFO] MAC: ");
    Serial.println(WiFi.macAddress());
    Serial.println("[SETUP] Connected to MQTT!mqttSubscribeToTopics\n");
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

  if (use_google_iot)
  {
  }
  else
  {
    if (!mqttClient.connected())
    {
      long now_mqtt = millis();
      if (now_mqtt - lastReconnectAttempt > 10000L)
      {
        lastReconnectAttempt = now_mqtt;
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
}

bool setMQTTCredentials(MQTTCredentials *credentials)
{
  // Creating new file
  File configFile = SPIFFS.open("/mqtt_config.json", "w");

  if (configFile)
  {
    Serial.println("[INFO] Opened MQTT config file");
    DynamicJsonDocument json(1024);
    json["server_ip"] = credentials->server_ip;
    json["port"] = credentials->port;
    json["username"] = credentials->username;
    json["password"] = credentials->password;

    // serializeJson(json, Serial);
    serializeJson(json, configFile);
    configFile.close();
    return true;
  }
  Serial.println("Returning false");
  return false;
}

bool getMQTTCredentials(MQTTCredentials *credentials)
{
  if (SPIFFS.exists("/mqtt_config.json"))
  {
    // File exists, reading and loading
    File configFile = SPIFFS.open("/mqtt_config.json", "r");

    if (configFile)
    {
      Serial.println("[INFO] Opened config file");
      size_t size = configFile.size();
      // Allocate a buffer to store contents of the file.
      std::unique_ptr<char[]> buf(new char[size]);

      configFile.readBytes(buf.get(), size);
      DynamicJsonDocument json(1024);
      auto deserializeError = deserializeJson(json, buf.get());
      // serializeJson(json, Serial);
      if (!deserializeError)
      {
        strcpy(credentials->server_ip, json["server_ip"]);
        strcpy(credentials->port, json["port"]);
        strcpy(credentials->username, json["username"]);
        strcpy(credentials->password, json["password"]);

        configFile.close();
        Serial.print("[INFO] Server IP: ");
        Serial.println(credentials->server_ip);
        Serial.print("[INFO] Port: ");
        Serial.println(credentials->port);
        Serial.print("[INFO] Username: ");
        Serial.println(credentials->username);
        Serial.print("[INFO] Password: ");
        Serial.println(credentials->password);
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

void mqttSubscribeToTopics(PubSubClient *mqttClient)
{

  mqttClient->subscribe("ping");
  // All subscriptions should be of the form {HOSTNAME/endpoint}
  mqttClient->subscribe("ESP32_LORA/test"); // Subscribe function only takes char*, not String
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

String generate_ping_data()
{
  String message;
  const size_t capacity = JSON_OBJECT_SIZE(8);
  DynamicJsonDocument doc(capacity);

  doc["ip_address"] = WiFi.localIP().toString();
  doc["mac_address"] = WiFi.macAddress();
  doc["hostname"] = HOSTNAME;
  doc["current_ssid"] = WiFi.SSID();

  serializeJson(doc, message);
  return message;
}

void return_ping()
{

  String topic = "ping/response/" + HOSTNAME;

  publish_data(topic, generate_ping_data());
}

String generate_arduino_data(String arduino_data)
{
  String message;
  const size_t capacity = JSON_OBJECT_SIZE(4);
  DynamicJsonDocument doc(capacity);

  doc["data_1"] = arduino_data.substring(0, arduino_data.indexOf(','));
  doc["data_2"] = arduino_data.substring(arduino_data.indexOf(',') + 1, arduino_data.indexOf('\r'));

  serializeJson(doc, message);
  return message;
}

void publish_arduino_data(String arduino_data)
{
  publish_data(get_host_topic(), generate_arduino_data(arduino_data));
}

String get_host_topic()
{
  String topic = "data/" + HOSTNAME;
  return topic;
}

String generate_test_data()
{
  String payload = "Test message";
  String message;
  // 2 static fields + two arrays of size message_length/2
  // Need 4 space for the time field
  const int capacity = JSON_OBJECT_SIZE(6) + 2 * JSON_ARRAY_SIZE(payload.length() / 2);
  DynamicJsonDocument doc(capacity);

  update_current_local_time();
  String src = "0x99";
  doc["src"] = src;

  update_current_local_time();
  doc["time"] = now;
  doc["data"] = payload;

  serializeJson(doc, message);
  return message;
}

void publish_default_test_data()
{
  publish_data(get_host_topic(), generate_test_data());
}

void publish_data(String topic, String message)
{
  char topic_char_array[topic.length() + 1];
  topic.toCharArray(topic_char_array, topic.length() + 1);

  char message_char_array[message.length() + 1];
  message.toCharArray(message_char_array, message.length() + 1);

  Serial.print("Publishing to MQTT topic ' ");
  Serial.print(topic);
  Serial.print(" ':");
  Serial.println(message);
  Serial.println();

  mqttClient.publish(topic_char_array, message_char_array);

  yield();
}
