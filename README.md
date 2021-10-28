## Thesis repository 2021-2022


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