/*    
    Copyright 2020, Network Research Lab at the University of Toronto.

    This file is part of CottonCandy.

    CottonCandy is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    CottonCandy is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with CottonCandy.  If not, see <https://www.gnu.org/licenses/>.
*/

/**
 * This example demonstrates how to create a gateway in a tree-based mesh network 
 * and receives a sensor values from nodes in the network.
 */ 

#include <LoRaMesh.h>
#include <AdafruitDeviceDriver.h>
#include <SoftwareSerial.h>

#define CS_PIN 10
#define RST_PIN 9
#define INT_PIN 3

#define RTC_VCC 4
#define RTC_INT 2

// The time (seconds) between gateway requesting data
// Currently set to every 10 mins
#define GATEWAY_REQ_TIME 180

#define ESP_32_WAKEUP_PIN PB6
#define ESP_32_READY_PIN PB7

#define NUM_TEST_MESSAGES 10

LoRaMesh *manager;

DeviceDriver *myDriver;

bool DEBUG_ENABLE = true;

SoftwareSerial ss(5,6); //RX, TX

// 2-byte long address 
// For Gateway only: The first bit of the address has to be 1
byte myAddr[2] = {0x80, 0xA0};


/**
 * Callback function that will be called when Gateway begins data collection
 */
void preDataCollection()
{
  //PORTB |= (1 << PORTB6); // Wake up the ESP32, set ESP_32_WAKEUP_PIN high
  PORTB &= ~(1 << PORTB6); // Allow the ESP32 to sleep, set ESP_32_WAKEUP_PIN low
}

/**
 * Callback function that will be called when Gateway ends data collection
 */
void postDataCollection()
{
  // Connection complete, let the ESP32 hibernate
  //PORTB &= ~(1 << PORTB6); // Allow the ESP32 to sleep, set ESP_32_WAKEUP_PIN low
  PORTB |= (1 << PORTB6); // Wake up the ESP32, set ESP_32_WAKEUP_PIN high
}


/**
 * Callback function that will be called when Gateway receives the reply from a node
 */

 
void recieveResponse(byte *data, byte len, byte *srcAddr)
{
  // Here we output the data to the ESP using digital pin 5,6
  // Initialize connection
  //PORTB |= (1 << PORTB6); // Wake up the ESP32, set ESP_32_WAKEUP_PIN high
  //PORTB &= ~(1 << PORTB6); // Allow the ESP32 to sleep, set ESP_32_WAKEUP_PIN low
  ss.begin(19200);   // Begin serial channel

  // Wait for the ESP32 to initialize
  while (!(PINB & (1<<PORTB7))) { // ESP_32_READY_PIN
    delay(100);
  }
  delay(700);
//  for (int i = 0; i < NUM_TEST_MESSAGES; i++) {
    //Serial.println("Sending src addr");
    ss.write(srcAddr,2);
    //Serial.println("Sending len");
    ss.write(len);
    //Serial.println("Sending data");
    ss.write(data, len);
    ss.println();
    delay(5);
//  }
  //while (PINB & (1<<PORTB7)) { // ESP_32_READY_PIN
    delay(100);
  //}
  // Connection complete, let the ESP32 hibernate
  //PORTB &= ~(1 << PORTB6); // Allow the ESP32 to sleep, set ESP_32_WAKEUP_PIN low
  //PORTB |= (1 << PORTB6); // Wake up the ESP32, set ESP_32_WAKEUP_PIN high
  ss.end();  
}

void setup()
{
  Serial.begin(19200);

  // Wait for serial port to connect.
  // Please comment out this while loop when using Adafruit feather or other ATmega32u4 boards 
  // if you do not want to connect Adafruit feather to a USB port for debugging. Otherwise, the
  // feather board does not start until a serial monitor is opened.
  //while (!Serial)
  //{
  //  ; // wait for serial port to connect. Needed for native USB port only
  //}

  // Uncomment the next line for using Adafruit LoRa Feather
    myDriver = new AdafruitDeviceDriver(myAddr,CS_PIN,RST_PIN,INT_PIN);

  // Initialize the driver
  myDriver->init();

  // Create a LoRaMesh object
  manager = new LoRaMesh(myAddr, myDriver);

  // For Gateway only: Set up the time interval between requests
  manager->setGatewayReqTime(GATEWAY_REQ_TIME);

  // Set up the callback fucntions
  manager->onReceiveResponse(recieveResponse);
  manager->preDataCollectionCallback(preDataCollection);
  manager->postDataCollectionCallback(postDataCollection);

  // Set the sleep mode
  manager->setSleepMode(SleepMode::SLEEP_RTC_INTERRUPT, RTC_INT, RTC_VCC);

  // Initialize the ESP32 signal pins
  DDRB |= (1 << DDB6); // Set PB6 as output
  //PORTB &= ~(1 << PORTB6); // Allow the ESP32 to sleep, set ESP_32_WAKEUP_PIN low
  PORTB |= (1 << PORTB6); // Wake up the ESP32, set ESP_32_WAKEUP_PIN high
  delay(1000);
}

void loop()
{
  Serial.println("Loop starts");

  // Start the gateway
  manager->run();
}
