#pragma once

#include <ArduinoJson.h>
#include <FS.h>
#include <SPIFFS.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <PubSubClient.h>

#include "esp_sntp.h"
#include "driver/timer.h"
#include "driver/rtc_io.h"

String HOSTNAME = "ESP32_LORA";

/* The default SSID and password for the access point */
const char *ap_ssid = "ESP32AP";
const char *ap_password = "12344321";

typedef struct wifi_credentials{
    char ssid[20];
    char identity[30];
    char password[50];
} WiFiCredentials;

typedef struct mqtt_credentials{
    char server_ip[20];
    char port[6];
    char username[30];
    char password [50];
} MQTTCredentials;

/* ----- BEGIN PACKET INFORMATION AND DEFINITIONS ----- */
//              Message parts and sizes diagram
// |  src  |  message len  |  gateway addr  | other info |  data  |
// |  2B   |      1B       |       2B       |     2B     |  0-64B |
typedef struct packet {
  byte src[2];
  unsigned int message_length;
  float data_value;
} Packet;

/* Questions:
 *  Do we want to send the type too?
 *  Do we want to send the seq num too?
 */
/* ----- END PACKET INFORMATION AND DEFINITIONS ----- */
/* ----- BEGIN SERIAL SETTINGS ----- */
// The maximum size of a message - this is calculated from the 
// message metadata plus data max size. See diagram above.
#define MAX_PAYLOAD_SIZE 5+64 
byte input_byte_array[3*MAX_PAYLOAD_SIZE]; // Extra large just in case

unsigned int input_byte_array_index = 0;
bool byte_array_complete = false;

// Define the pins for the Arduino serial port
#define RXD2 16
#define TXD2 17

/* ----- END SERIAL SETTINGS ----- */

/* ----- BEGIN TIME VARIABLES ----- */
/* Timer related defines */
#define TIMER_DIVIDER         (16)  //  Hardware timer clock divider
#define TIMER_SCALE           (TIMER_BASE_CLK / TIMER_DIVIDER)  // convert counter value to seconds

#define SLEEP_TIMER_SECONDS 10 // Amount of time to wait until trying to go into hibernation mode
#define RTC_GPIO_WAKEUP_PIN GPIO_NUM_4        // D4
#define ARDUINO_SIGNAL_READY_PIN GPIO_NUM_5   // D5


time_t now;
char strftime_buf[64];
struct tm timeinfo;
/* ----- END TIME VARIABLES ----- */


WiFiClient espClient;
PubSubClient mqttClient(espClient);

WebServer server(80);
