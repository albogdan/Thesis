
#define TINY_GSM_MODEM_SIM7600

#include <TinyGsmClient.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <SoftwareSerial.h>

SoftwareSerial mySerial;

#define TINY_GSM_DEBUG Serial
#define SerialAT Serial2

TinyGsm modem(SerialAT);
TinyGsmClient client(modem);
PubSubClient mqttClient(client);

#define TINY_GSM_USE_GPRS true
#define GSM_AUTOBAUD_MIN 9600
#define GSM_AUTOBAUD_MAX 115200
const int port = 80;
const char server[]   = "vsh.pp.ua";
const char resource[] = "/TinyGSM/logo.txt";
// Your GPRS credentials, if any

const char apn[]      = "iot.truphone.com";
const char gprsUser[] = "";
const char gprsPass[] = "";

// MQTT parameters
char *mqttServer = "";
char *mqttUsername = "";
char *mqttPassword = "";
char *mqttPort = "1883";
String HOSTNAME = "GSM_TEST_LORA";

// Define the pins for the Arduino serial port
#define RXD2 18
#define TXD2 19

void setup() {

  Serial.begin(115200);
  delay(10);
  Serial.println("[SETUP] Serial0 setup complete");

  //SerialAT.begin(57600,SERIAL_8N1,RXD2,TXD2); // Hardware Serial
  //SerialAT.begin(57600,SWSERIAL_8N1,RXD2,TXD2); // Software Serial
  TinyGsmAutoBaud(SerialAT, GSM_AUTOBAUD_MIN, GSM_AUTOBAUD_MAX);
  delay(3000);
  Serial.println("[SETUP] Serial1 Setup Complete");
  modem.init();

  String modemInfo = modem.getModemInfo();
  Serial.print("Modem Info: ");
  Serial.println(modemInfo);


  Serial.println("Connecting GPRS");
//  modem.gprsConnect(apn, gprsUser, gprsUser);
//  modem.restart();
  Serial.print("Waiting for network...");
  if (!modem.waitForNetwork()) {
    Serial.println(" fail");
    delay(10000);
    return;
  }
  Serial.println(" success");
  
  if (modem.isNetworkConnected()) { Serial.println("Network connected"); }

  Serial.print(F("Connecting to "));
  Serial.print(apn);
  if (!modem.gprsConnect(apn, gprsUser, gprsPass)) {
    Serial.println(" fail");
    delay(10000);
    return;
  }
  Serial.println(" success");

  if (modem.isGprsConnected()) { Serial.println("GPRS connected"); }
  modem.setBaud(57600);
  setupMqtt(mqttServer, mqttPort);
  
}

void loop() {
  if(!modem.isNetworkConnected()) {
    Serial.println("Network disconnected");
    if (!modem.waitForNetwork(180000L, true)) {
      Serial.println(" fail");
      delay(10000);
      return;
    }
    if (modem.isNetworkConnected()) {
      Serial.println("Network re-connected");
    }
    if (!modem.isGprsConnected()) {
      Serial.println("GPRS disconnected!");
      Serial.print(F("Connecting to "));
      Serial.print(apn);
      if (!modem.gprsConnect(apn, gprsUser, gprsPass)) {
        Serial.println(" fail");
        delay(10000);
        return;
      }
      if (modem.isGprsConnected()) { Serial.println("GPRS reconnected"); }
    }
  }

/*
  Serial.print("Connecting to ");
  Serial.println(server);
  if (!client.connect(server, port)) {
    Serial.println(" fail");
    delay(10000);
    return;
  }
  Serial.println(" success");

  // Make a HTTP GET request:
  Serial.println("Performing HTTP GET request...");
  client.print(String("GET ") + resource + " HTTP/1.1\r\n");
  client.print(String("Host: ") + server + "\r\n");
  client.print("Connection: close\r\n\r\n");
  client.println();

  uint32_t timeout = millis();
  while (client.connected() && millis() - timeout < 10000L) {
    // Print available data
    while (client.available()) {
      char c = client.read();
      Serial.print(c);
      timeout = millis();
    }
  }
  Serial.println();

  // Shutdown

  client.stop();
  Serial.println(F("Server disconnected"));
*/

  loop_and_check_mqtt_connection(mqttUsername, mqttPassword);



}
