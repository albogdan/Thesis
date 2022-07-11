/******************************************************************************
 * Copyright 2018 Google
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *****************************************************************************/
// This file contains static methods for API requests using Wifi / MQTT
#ifndef __ESP32_MQTT_H__
#define __ESP32_MQTT_H__

#include <Client.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <MQTT.h>
#include <Regexp.h>
#include "src/CloudIoTCoreMqtt.h"
#include "src/CloudIoTCore.h"

const int jwt_exp_secs = 3600;
const char *ntp_primary = "pool.ntp.org";
const char *ntp_secondary = "time.nist.gov";

bool is_live = false;

bool publishTopic1(String data);

// !!REPLACEME!!
// The MQTT callback function for commands and configuration updates
// Place your message handler code here.
void messageReceived(String &topic, String &payload)
{

  char topic_char_array[topic.length() + 1];
  topic.toCharArray(topic_char_array, topic.length() + 1);

  Serial.println("Responding to incoming: " + topic + " - " + payload);
  MatchState ms;
  ms.Target(topic_char_array);
  char result = ms.Match("/commands/(.+)");
  Serial.println("Found match result :  " + result);

  Serial.print("Captures: ");
  Serial.println(ms.level);

  // char buf[100]; // large enough to hold expected string
  // Serial.print("Matched on: ");

  for (int j = 0; j < ms.level; j++)
  {
    Serial.print("Capture number: ");
    Serial.println(j);
    Serial.print("Text: ");
    String topic_recieved = String(ms.GetCapture(topic_char_array, j));

    if (topic_recieved == HOSTNAME + "/test")
    {

      Serial.println("Recieved a " + HOSTNAME + "/test message");

      publishTopic1("{\"type\":\"response_to_test_command\", \"host_name\" : \"" + HOSTNAME + "\"}");
    }

    else if (topic_recieved == "ping")
    {
      Serial.println("Recieved a ping message");
    }

  } // end of for each capture

  // once youve sent a live message process get the command, formulate the answer and publish a message in response.
}
///////////////////////////////

// Initialize WiFi and MQTT for this board
WiFiClientSecure *netClient;
CloudIoTCoreDevice *device;
CloudIoTCoreMqtt *mqtt;
MQTTClient *google_mqttClient;
unsigned long iat = 0;
String jwt;

///////////////////////////////
// Helpers specific to this board
///////////////////////////////
String getDefaultSensor()
{
  return "Wifi: " + String(WiFi.RSSI()) + "db";
}

String getDefaultTestData()
{
  return "{\"from\":\"esp32\"}";
}

String getJwt()
{
  iat = time(nullptr);
  Serial.println("Refreshing JWT");
  jwt = device->createJWT(iat, jwt_exp_secs);
  return jwt;
}

void setupWifi(char *ssid, char *password)
{
  Serial.println("Starting wifi");

  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false); // May help with disconnect? Seems to have been removed from WiFi
  WiFi.begin(ssid, password);
  Serial.println("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(100);
  }

  configTime(0, 0, ntp_primary, ntp_secondary);
  Serial.println("Waiting on time sync...");
  while (time(nullptr) < 1510644967)
  {
    delay(10);
  }
}

void connectWifi()
{
  Serial.print("checking wifi...");
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(1000);
  }
}

///////////////////////////////
// Orchestrates various methods from preceeding code.
///////////////////////////////

bool publishTopic1(String data)
{
  return mqtt->publishTelemetryTopic1(data);
  // return mqtt->publishTelemetry(data);
}

bool publishTelemetry(String data)
{
  return mqtt->publishTelemetry(data);
}

bool publishTelemetry(const char *data, int length)
{
  return mqtt->publishTelemetry(data, length);
}

bool publishTelemetry(String subfolder, String data)
{
  return mqtt->publishTelemetry(subfolder, data);
}

bool publishTelemetry(String subfolder, const char *data, int length)
{
  return mqtt->publishTelemetry(subfolder, data, length);
}

void connect()
{
  connectWifi();
  mqtt->mqttConnect();
}

void setupCloudIoT(char *project_id, char *location, char *registry_id, char *device_id, char *private_key_str, char *root_cert)
{
  device = new CloudIoTCoreDevice(
      project_id, location, registry_id, device_id,
      private_key_str);

  // Wifi should be set up already

  char *ssid = "PiNet";
  char *password = "raspberry";
  setupWifi(ssid, password);

  netClient = new WiFiClientSecure();
  netClient->setCACert(root_cert);
  google_mqttClient = new MQTTClient(512);
  google_mqttClient->setOptions(180, true, 1000); // keepAlive, cleanSession, timeout
  mqtt = new CloudIoTCoreMqtt(google_mqttClient, netClient, device);
  mqtt->setUseLts(true);
  mqtt->startMQTT();
}

#endif // __ESP32_MQTT_H__