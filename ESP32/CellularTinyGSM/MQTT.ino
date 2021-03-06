///#include <ESP32Ping.h>


long lastReconnectAttempt = 0;


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


void loop_and_check_mqtt_connection(char *username, char *password)
{
    if (!mqttClient.connected())
    {
        long now = millis();
        if (now - lastReconnectAttempt > 10000)
        {
            lastReconnectAttempt = now;
            // Attempt to reconnect
            if (connectToMqtt(username, password))
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
        mqttClient.publish("testest", "Test Success");
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
//
//    doc["ip_address"] = WiFi.localIP().toString();
//    doc["mac_address"] = WiFi.macAddress();
    doc["hostname"] = HOSTNAME;
//    doc["current_ssid"] = WiFi.SSID();

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
