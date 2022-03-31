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
 * This example is similar to the Node.ino example. However, this program
 * is written for a specific hardware platform to improve the power consumption.
 * 
 * Our test uses the following hardware setup. You can interchange some
 * pin assignments based your situation. Please be careful with the wirings.
 * Use the example at your own risk.
 * 
 * 1. Atmega328P-based board (our tests use the barebone circuit)
 * 
 * 2. DS3231 RTC 
 *    SQW connected to Digital Pin 2 (INT0).
 *    
 *    I2C on DS3231 are connected to common ATmega328 I2C pins.
 *    
 *    Note: If you want to acheive the best power consumption,
 *    use a battery to operate the RTC for time-keeping.
 *    
 *    VCC connected to Digital Pin 7. (You do not need to do this 
 *    if you do not concern about the power consumption)
 *    
 *    Remember to 
 *    1) desolder the charing resistor pack (IMPORTANT)
 *    2) desolder the pull-up resistor pack for SQW and I2C lines, and
 *       add your own pull-up resistors for I2C.
 *    
 * 3. Adafruit RFM9X LoRa breakout 
 *    IRQ pin connected to Digital Pin 3 (INT1).
 *    
 *    For CS and RST pins you can really use any available digital pins.
 *    
 *    SPI on the breakout are connected to common ATmega328 SPI pins.
 */

#include <LoRaMesh.h>
#include <AdafruitDeviceDriver.h>
#include <EEPROM.h>

#define CS_PIN 10
#define RST_PIN 9
#define INT_PIN 3

#define RTC_VCC 4
#define RTC_INT 2

LoRaMesh *manager;

DeviceDriver *myDriver;

union LongToBytes{
  long l;
  byte b[sizeof(long)];
};

/**
 * Callback function that is called when the node receives the request from the gateway
 * and needs to reply back. Users can read sensor value, do some processing and send data back
 * to the gateway.
 * 
 * "data" points to the payload portion of the reply packet that will be sent to the gateway
 * once the function returns. Users can write their own sensor value into the "data" byte array.
 * The length of the payload can be specified by writting it to "len"
 */
void onReceiveRequest(byte **data, byte *len)
{

  // In the example, we simply send a random integer value to the gateway
  Serial.println(F("onReciveRequest callback"));

  // Generate a random value from 0 to 1000
  // In practice, this should be a sensor reading
  long sensorValue = random(0,1000);

  Serial.print(F("Sending number = "));
  Serial.print(sensorValue);
  Serial.println(F(" to the gateway"));

  // Specify the length of the payload
  *len = sizeof(long);

  // Encode this long-type value into a 4-byte array
  // Note: The encoding here using C++ union is little-endian. Although the common practice in networking
  // is big-endian, to make things simple, we use the same union in both the sender(Node) and receiver(Gateway)
  // such that the number can be decoded correctly.
  union LongToBytes myConverter;
  myConverter.l = sensorValue;

  // Copy the encoded 4-byte array into the data (aka payload)
  memcpy(*data, myConverter.b, *len);
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
  //  ;
    // wait for serial port to connect. Needed for native USB port only
  //}
  delay(500);

  // 2-byte long address
  // For regular nodes: The first bit of the address is 0
  byte myAddr[2] = {0x00, 0xA0};

  for(uint8_t i = 0; i < 2; i++){
    myAddr[i] = EEPROM.read(i);
  }

  if (DeviceDriver::compareAddr(myAddr, BROADCAST_ADDR)){
    Serial.println(F("Warning: EEPROM is unset or you assigned a broadcast address. Will use a random node address now"));
    myAddr[0] = (byte)random(0x80);
    myAddr[1] = (byte)random(0xFF);
  }else if (myAddr[0] & 0x80){
        //If the pre-assigned address is for a gateway
        Serial.println(F("Warning: This is a gateway address. Will use a random first address byte now"));
        myAddr[0] = (byte)random(0x80);
  }

  Serial.print(F("Using address: 0x"));
  Serial.print(myAddr[0], HEX);
  Serial.print(myAddr[1], HEX);
  Serial.println();

  myDriver = new AdafruitDeviceDriver(myAddr,CS_PIN,RST_PIN,INT_PIN);

  // Use the default configuration
  myDriver->init();

  // Create a LoRaMesh object
  manager = new LoRaMesh(myAddr, myDriver);

  // Set up the callback funtion
  manager->onReceiveRequest(onReceiveRequest);

  // Set the sleep mode
  manager->setSleepMode(SleepMode::SLEEP_RTC_INTERRUPT, RTC_INT, RTC_VCC);
}

void loop()
{
  // Start the node
  manager->run();
}
