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


#ifndef HEADER_DEVICE_DRIVER
#define HEADER_DEVICE_DRIVER

#include "Arduino.h"

#define MIN_TX_PWR 9 // We can go down to 2 but it is very short-range
#define MAX_TX_PWR 17 //We can go up to 23 but it will require more power

static byte BROADCAST_ADDR[2] = {0xFF, 0xFF};

typedef enum{
    UNKNOWN,
    EBYTE_E22,
    ADAFRUIT_LORA
}DeviceType;

typedef enum{
    SLEEP,
    STANDBY,
    TX,
    RX
}DeviceMode;

class DeviceDriver{

public:

    static bool compareAddr(byte* a, byte* b);
    
    //Default Constructor
    DeviceDriver();

    //Destructor
    virtual ~DeviceDriver();

    /**
     * Initialize the driver by configuring it to the correct mode, frequency etc.
     */
    virtual bool init();

    /**
     * Send a stream of bytes. Takes a byte array and its length.
     * Returns number of bytes successfully sent. Returns -1 if sending failed (timeout).
     */
    virtual int send(byte* destAddr, byte* msg, uint8_t msgLen) = 0;

    /**
     * Returns a byte received. Returns -1 if none available
     */
    virtual byte recv() = 0;

    virtual int getLastMessageRssi() = 0;

    /**
     * Returns number of bytes that are available.
     */
    virtual int available() = 0;

    virtual uint8_t getDeviceType() = 0;

    virtual void setTxPwr(uint8_t pwr);

    virtual void powerDownMCU();

    virtual void setFrequency(unsigned long frequency) = 0;

    virtual void setMode(DeviceMode mode) = 0;

    // Collect statistics (SNR & non-dest packets)
    virtual uint16_t getTotalInterferingMargin();
    virtual void resetStatistics();

    virtual byte random();

protected:
    DeviceMode m_mode = DeviceMode::SLEEP;

    byte m_addr[2];
};

#endif
