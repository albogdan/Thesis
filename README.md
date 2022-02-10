## Thesis Repository 2021-2022
This is the repository for designing an end-to-end IoT system for large scale environmental monitoring. It integrates [CottonCandy](https://github.com/infernoDison/cottonCandy) on an Arduino with the ESP32 to create a Gateway. As well, it contains python scripts for sending data to Firebase Realtime Database, and reading it and adding it to Elasticsearch for display on


## File Hierarchy
### Arduino
Contains all the files for testing the Gateway and Nodes that run CottonCandy.

### ESP32
Contains various pieces and iterations of the ESP32 Gateway code. 
- `ESP32_MQTT` relates to integration of PubSubClient with the Mosquitto broker.
- `AdvancedWebServerESP32` deals with creating a web interface for management of various ESP32 Gateway parameters, such as WiFi and MQTT connections.
- `ESP32_Integrated_MQTT_WebServer` deals with integration of `AdvancedWebServerESP32` and `ESP32_MQTT`.

### Services
Contains services for data paths between `Mosquitto MQTT Broker`, `Firebase Realtime Database`, and `Elasticsearch`.
- `mqtt_firebase.py` deals with subscribing to topic from `Mosquitto` (the same one that the ESP32 Gateway publishes to) and uploading the received data to a `Firebase Realtime Database` document.
- `firebase_to_elasticsearch.py` deals with listening to events that a `Firebase Realtime Database` document publishes. It takes the event data and inserts it into an `Elasticsearch` instance, so Kibana can be used to visualize the data.


## ESP32/8266 Connect to UofT WiFi
* UofT WiFi is WPA2 Enterprise, so need a few extra commands. In the `setup()` method we replace  ```WiFi.begin(ssid, password);``` with

```C
// WPA2 enterprise magic starts here
WiFi.disconnect(true);
WiFi.mode(WIFI_STA);
esp_wifi_sta_wpa2_ent_set_identity((uint8_t *)EAP_ID, strlen(EAP_ID));
esp_wifi_sta_wpa2_ent_set_username((uint8_t *)EAP_USERNAME, strlen(EAP_USERNAME));
esp_wifi_sta_wpa2_ent_set_password((uint8_t *)EAP_PASSWORD, strlen(EAP_PASSWORD));
esp_wpa2_config_t config = WPA2_CONFIG_INIT_DEFAULT();
esp_wifi_sta_wpa2_ent_enable(&config);
// WPA2 enterprise magic ends here

WiFi.begin(ssid);
```
* As well, we add a couple of definitions and includes at the top of the file

```C
#include "esp_wpa2.h"
#define EAP_USERNAME "UTorID"
#define EAP_ID "UTorID"
#define EAP_PASSWORD "your_password"
```
