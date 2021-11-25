#include <ArduinoJson.h>
const char *ap_ssid = "ESP32AP";
const char *ap_password = "12344321";
typedef struct wifi_credentials{
    char ssid[20];
    char password[50];
} WiFiCredentials;

typedef struct mqtt_credentials{
    char server_ip[20];
    char port[6];
} MQTTCredentials;
String HOSTNAME = "ESP_32";

WiFiClient espClient;
PubSubClient mqttClient(espClient);

WebServer server(80);
