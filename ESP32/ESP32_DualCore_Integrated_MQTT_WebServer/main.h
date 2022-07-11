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
#include "driver/adc.h"
#include "driver/timer.h"
#include "driver/rtc_io.h"

#define WIFI_AP_CONNECTED 1
#define WIFI_APSTA_CONNECTED 2

#define CELLULAR_CONNECTION_SUCCESS 1
#define CELLULAR_CONNECTION_FAILURE 2

#define UPLINK_MODE_WIFI 1
#define UPLINK_MODE_CELLULAR 2

String HOSTNAME = "ESP32_LORA";
bool SLEEP_ENABLED = false;

/* The default SSID and password for the access point */
const char *ap_ssid = "ESP32AP";
const char *ap_password = "12344321";

bool use_google_iot = true;

typedef struct wifi_credentials
{
    char ssid[20];
    char identity[30];
    char password[50];
} WiFiCredentials;

typedef struct mqtt_credentials
{
    char server_ip[20];
    char port[6];
    char username[30];
    char password[50];
} MQTTCredentials;

char network_id[8];

typedef struct google_iot_credentials
{
    char project_id[64];
    char location[32];
    char registry_id[32];
    char device_id[32];
    char private_key_str[128];
    char root_cert[1024];
} GoogleIOTCredentials;

/* ----- BEGIN PACKET INFORMATION AND DEFINITIONS ----- */
//              Message parts and sizes diagram
// |  src  |  message len  |  gateway addr  | other info |  data  |
// |  2B   |      1B       |       2B       |     2B     |  0-64B |
typedef struct packet
{
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
#define MAX_PAYLOAD_SIZE 5 + 512
byte input_byte_array[3 * MAX_PAYLOAD_SIZE]; // Extra large just in case

unsigned int input_byte_array_index = 0;
bool byte_array_complete = false;

/* ----- END SERIAL SETTINGS ----- */

/* ----- BEGIN TIME VARIABLES ----- */
/* Timer related defines */
#define TIMER_DIVIDER (16)                           //  Hardware timer clock divider
#define TIMER_SCALE (TIMER_BASE_CLK / TIMER_DIVIDER) // convert counter value to seconds

#define SLEEP_TIMER_SECONDS 7               // Amount of time to wait until trying to go into hibernation mode
#define RTC_GPIO_WAKEUP_PIN GPIO_NUM_4      // D4
#define ARDUINO_SIGNAL_READY_PIN GPIO_NUM_5 // D5

time_t now;
char strftime_buf[64];
struct tm timeinfo;

/* ----- END TIME VARIABLES ----- */

/* ----- BEGIN CELLULAR CONNECTION VARIABLES ----- */
#define TINY_GSM_MODEM_SIM7600
#include <TinyGsmClient.h>
#include <SoftwareSerial.h>

// Define the pins for the Arduino serial port
#define RXD2 18
#define TXD2 19
#define ARDUINO_BAUD_RATE 19200

// Define the pins for the Cellular serial port
#define RXCellular 16
#define TXCellular 17

SoftwareSerial ss;
#define SerialCellular Serial2
#define SERIAL_SIZE_RX 1024

#define SerialArduino ss

TinyGsm cellular_modem(SerialCellular);
TinyGsmClient cellular_client(cellular_modem);
PubSubClient mqttClientCellular(cellular_client);

#define TINY_GSM_USE_GPRS true
#define GSM_AUTOBAUD_MIN 9600
#define GSM_AUTOBAUD_MAX 115200
#define GSM_BAUD_RATE 57600

const char apn[] = "iot.truphone.com";
const char gprsUser[] = "";
const char gprsPass[] = "";

/* ----- END CELLULAR CONNECTION VARIABLES ----- */

WiFiClient espClient;
PubSubClient mqttClientWiFi(espClient);

WebServer server(80);
