
void setupMqtt(const char *mqttServer, const int mqttPort)
{
    mqttClient.setServer(mqttServer, mqttPort);
    mqttClient.setCallback(callbackMqtt);
    mqttClient.setBufferSize(1024);   // MAKE SURE TO CHECK THAT THIS IS BIG ENOUGH
}

boolean connectToMqtt()
{
    char hostname_char[HOSTNAME.length() + 1];
    HOSTNAME.toCharArray(hostname_char, HOSTNAME.length() + 1);
    while (!mqttClient.connected())
    {
        Serial.println("[SETUP] Connecting to MQTT...");
        if (mqttClient.connect(hostname_char, "roger", "password"))
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
            Serial.print(mqttClient.state());
            delay(200);
            return false;
        }
    }
}


void check_mqtt_connection()
{
    if (!mqttClient.connected())
    {
        long now = millis();
        if (now - lastReconnectAttempt > 10000)
        {
            lastReconnectAttempt = now;
            // Attempt to reconnect
            if (connectToMqtt())
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
