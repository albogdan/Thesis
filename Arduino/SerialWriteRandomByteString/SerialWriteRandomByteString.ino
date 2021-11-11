#include "payloads.h"

long rand_num_agg_non_agg;  // Decides whether to send an aggregated or non-aggregated packet
long rand_num_payload;      // Decides which payload to send within the packet

#define ESP_32_WAKEUP_PIN 4

void setup() {
  Serial.begin(115200);
  // put your setup code here, to run once:
  randomSeed(analogRead(0));
  pinMode(ESP_32_WAKEUP_PIN, OUTPUT);
  digitalWrite(ESP_32_WAKEUP_PIN, LOW);
}

void loop() {
  // Initialize connection
  digitalWrite(ESP_32_WAKEUP_PIN, HIGH);

  // Wait for the ESP32 to initialize
  delay(6000);

  // Send 10 requests
  for (int i = 0; i<10; i++) {
    // Generate a random number from 0 to 1
    rand_num_agg_non_agg = random(2);
  
    if (rand_num_agg_non_agg) {
      send_non_agg_packet();
    } else {
      send_agg_packet();
    }
    delay(2000);
  }
  // Connection complete, let the ESP32 hibernate
  digitalWrite(ESP_32_WAKEUP_PIN, LOW);

  // Run this code only every 20 seconds
  delay(20000);
}

void send_agg_packet() {
//  Serial.println("Sending aggregated packet");
  // Generate a random number from 0 to EXAMPLE_PAYLOAD_AGG_COUNT
  rand_num_payload = random(EXAMPLE_PAYLOAD_AGG_COUNT);
  int data_length = (payloads_agg[rand_num_payload][4] & 0x7F);
  /*
  // Print the first 5 bytes, which will always be the same
  Serial.print(payloads_agg[rand_num_payload][0]);
  Serial.print(payloads_agg[rand_num_payload][1]);
  Serial.print(payloads_agg[rand_num_payload][2]);
  Serial.print(payloads_agg[rand_num_payload][3]);
  Serial.print(payloads_agg[rand_num_payload][4]);
  
  for (int i = 0; i< (payloads_agg[rand_num_payload][4] & 0x7F); i++){
    Serial.print(payloads_agg[rand_num_payload][5+i]);
  }
  */

  Serial.write(payloads_agg[rand_num_payload], 5+data_length);

  Serial.println();
}

void send_non_agg_packet() {
//  Serial.println("Sending non-aggregated /packet");
  // Generate a random number from 0 to EXAMPLE_PAYLOAD_NON_AGG_COUNT
  rand_num_payload = random(EXAMPLE_PAYLOAD_NON_AGG_COUNT);
//  Serial.print("Payload num: ");
//  Serial.println(rand_num_payload);
  // Print the first 5 bytes, which will always be the same
  int data_length = (payloads_non_agg[rand_num_payload][4] & 0x7F);
  /*
  Serial.print(payloads_non_agg[rand_num_payload][0]);
  Serial.print(payloads_non_agg[rand_num_payload][1]);
  Serial.print(payloads_non_agg[rand_num_payload][2]);
  Serial.print(payloads_non_agg[rand_num_payload][3]);
  Serial.print(payloads_non_agg[rand_num_payload][4]);

  for (int i = 0; i< (payloads_non_agg[rand_num_payload][4] & 0x7F); i++){
    Serial.print(payloads_non_agg[rand_num_payload][5+i]);
  }

  */
  
  Serial.write(payloads_non_agg[rand_num_payload], 5+data_length);
  Serial.println();
}
