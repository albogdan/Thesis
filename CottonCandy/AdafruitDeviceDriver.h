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

#ifndef HEADER_ADAFRUIT_DEVICE_DRIVER
#define HEADER_ADAFRUIT_DEVICE_DRIVER

#include "Arduino.h"
#include "DeviceDriver.h"
#include <LoRa.h>
#include "Utilities.h"

/* for feather32u4 */
#define RFM95_CS 8
#define RFM95_RST 4
#define RFM95_INT 7
#define RF95_FREQ 915E6

#define MSG_QUEUE_CAPACITY 255

#define DEFAULT_SPREADING_FACTOR 7
#define DEFAULT_CHANNEL_BW 125E3
#define DEFAULT_CODING_RATE_DENOMINATOR 5

class AdafruitDeviceDriver : public DeviceDriver
{
public:
  /**
   * By default, the software assumes that you are using Adafruit 32u4 LoRa Feather and you do not need to provide the
   * pin mappings explicitly.
   * 
   * However, in case you are using the Adafruit Lora RFM9x Breakout board with a seperate microcontroller, you need to 
   * provide the pin mappings.
   */
  AdafruitDeviceDriver(byte *addr, uint8_t csPin = RFM95_CS, uint8_t rstPin = RFM95_RST, uint8_t intPin = RFM95_INT);

  ~AdafruitDeviceDriver();

  /**
   * Use the default configuration to initilize the LoRa parameters
   * 
   * Frequency = 915 MHz; Spreading factor = 7; Channel bandwith = 125kHz; Coding Rate = 4/5
   * 
   */
  bool init();

  bool init(long frequency, uint8_t sf, long bw, uint8_t cr);

  int send(byte *destAddr, byte *msg, uint8_t msgLen);

  byte recv();

  int available();

  int getLastMessageRssi();

  uint8_t getDeviceType();

  /** 
  * If you are using the AdafruitDeviceDriver: 
  *
  * The powerDownMCU() in the AdafruitDeviceDriver is written for Adafruit RFM9X
  * LoRa breakout.
  *
  * Currently we do not support Adafruit 32u4 LoRa Feather for transceiver-based
  * interrupt since its DIO0 pin is inaccessible (i.e. The DIO0 pin is connected
  * internally to pin 7 which does not support hardware-interrupt). At this
  * moment, calling powerDownMCU() on Adafruit 32u4 LoRa Feather will simply
  * return and will not put the MCU to sleep.
  *
  * However, if you are able to hard-wire the DIO0 pin on transciever IC of the
  * Adafruit 32u4 board to any of INT3:0, then you might be able to do the
  * powerDownMCU() correctly.
  */
  void powerDownMCU();

  byte random();

  //Collect statistics
  uint16_t getTotalInterferingMargin();
  void resetStatistics();

  /*-----------Module Registers Configuration-----------*/
  void setAddress(byte *addr);
  void setFrequency(unsigned long frequency);
  void setSpreadingFactor(uint8_t sf);
  void setChannelBandwidth(long bw);
  void setCodingRateDenominator(uint8_t cr);

  void setMode(DeviceMode mode);
  void setTxPwr(uint8_t pwr);

private:
  unsigned long m_freq;
  uint8_t m_sf;
  long m_bw;
  uint8_t m_cr;

  uint8_t m_txPwr = 17;

  uint8_t irqPin;
};

#endif
