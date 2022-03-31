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

#ifndef HEADER_EBYTE_DEVICE_DRIVER
#define HEADER_EBYTE_DEVICE_DRIVER

#include "Arduino.h"
#include "DeviceDriver.h"
#include "SoftwareSerial.h"
#include "Utilities.h"
#include <avr/sleep.h>

#define BAUD_RATE 9600
#define EBYTE_ADDRESS_SIZE 2

#define BASE_FREQUENCY 850.125E6
#define CHANNEL_INTERVAL 1E6

class EbyteDeviceDriver : public DeviceDriver
{
public:
    EbyteDeviceDriver(uint8_t rx, uint8_t tx, uint8_t m0, uint8_t m1, uint8_t aux_pin, byte *addr, uint8_t channel);

    ~EbyteDeviceDriver();

    bool init();

    int send(byte *destAddr, byte *msg, uint8_t msgLen);

    byte recv();

    int available();

    int getLastMessageRssi();

    void enterStandbyMode();
    void enterRxMode();
    void enterWorMode();
    void enterSleepMode();

    uint8_t getInterruptPin();

    static uint8_t getInterruptPinBehavior();

    static void wakeISR();

    void powerDownMCU();

    uint8_t getDeviceType();

private:
    SoftwareSerial *module;
    uint8_t rx;
    uint8_t tx;
    uint8_t m0;
    uint8_t m1;
    uint8_t aux_pin;

    uint8_t myChannel;

    /*-----------Module Registers Configuration-----------*/
    void setAddress(byte *addr);
    void setChannel(uint8_t channel);
    void setNetId(uint8_t netId);
    void setOthers(byte config);
    void setEnableRSSI();

    void setFrequency(unsigned long frequency);
    void setMode(DeviceMode mode);

    /**
     * The function sets the air rate to 9.6kbps by default
     */
    void setAirRate();

    /*-----------Helper Function-----------*/
    void receiveConfigReply(int replyLen);
};

#endif
