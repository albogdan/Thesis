#include <WiFi.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>



String HOSTNAME = "ESP32_LORA";

const char* ssid     = "ssid_here";
const char* password = "password_here";
const char *mqttServer = "mqtt_server_here";
const int mqttPort = 1883;
long lastReconnectAttempt = 0;

WiFiClient espClient;
PubSubClient mqttClient(espClient);

void setup()
{
    Serial.begin(115200);
    delay(10);

    // We start by connecting to a WiFi network

    Serial.println();
    Serial.println();
    Serial.print("[INFO] Connecting to ");
    Serial.println(ssid);

    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println("");
    Serial.println("[INFO] WiFi connected");
    Serial.println("[INFO] IP address: ");
    Serial.println(WiFi.localIP());

    Serial.println("[INFO] Setting up MQTT server...");
    setupMqtt(mqttServer, mqttPort);

    Serial.println("[INFO] Connecting to MQTT server...");
    connectToMqtt();
    
}

void loop() {
  check_mqtt_connection();

}


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
        Serial.println("Connecting to MQTT...");
        if (mqttClient.connect(hostname_char))
        { //, mqttUser, mqttPassword )) {
            Serial.print("MAC: ");
            Serial.println(WiFi.macAddress());
            Serial.println("Connected to MQTT!\n");
            mqttSubscribeToTopics(&mqttClient);
            return true;
        }
        else
        {
            Serial.print("Connection failed with state ");
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
