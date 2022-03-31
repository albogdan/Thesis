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

#define CS_PIN 10
#define RST_PIN 9
#define INT_PIN 3

#define RTC_VCC 4
#define RTC_INT 2

// The time (seconds) between gateway requesting data
// Currently set to every 60 seconds
#define GATEWAY_REQ_TIME 60

LoRaMesh *manager;

DeviceDriver *myDriver;

// 2-byte long address 
// For Gateway only: The first bit of the address has to be 1
byte myAddr[2] = {0x80, 0xA0};

/**
 * In this example, we will use union to encode and decode the long-type integer we are sending
 * in the network
 */
union LongToBytes{
  long l;
  byte b[sizeof(long)];
};

/**
 * Callback function that will be called when Gateway receives the reply from a node
 */
void onReciveResponse(byte *data, byte len, byte *srcAddr)
{

  // In the example, we will first print out the reply message in HEX from the node
  Serial.print(F("Gateway received a node reply from Node 0x"));
  Serial.print(srcAddr[0], HEX);
  Serial.print(srcAddr[1], HEX);

  Serial.print(F(". Data(HEX): "));

  for (int i = 0; i < len; i++)
  {
    Serial.print((data[i]), HEX);
  }

  // Since nodes are sending 4-byte long numbers (See example/Node/Node.ino), we can
  // convert the byte array back to a long-type integer
  if(len == 4){

    // Convert the byte array back to a long-type integer
    union LongToBytes myConverter;
    memcpy(myConverter.b, data, len);
    long value = myConverter.l;

    Serial.print(" Received integer = ");
    Serial.print(value);
  }

  Serial.print('\n');

}

void setup()
{
  Serial.begin(57600);

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

  // Set up the callback funtion
  manager->onReceiveResponse(onReciveResponse);

  // Set the sleep mode
  manager->setSleepMode(SleepMode::SLEEP_RTC_INTERRUPT, RTC_INT, RTC_VCC);

  delay(1000);
}

void loop()
{
  Serial.println(F("Loop starts"));

  // Start the gateway
  manager->run();
}
