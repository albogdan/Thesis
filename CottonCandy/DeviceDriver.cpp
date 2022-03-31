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

#include "DeviceDriver.h"

DeviceDriver::DeviceDriver(){

}

DeviceDriver::~DeviceDriver(){

}

bool DeviceDriver::compareAddr(byte* a, byte* b){
    return (a[0] == b[0] && a[1] == b[1]);
}

bool DeviceDriver::init(){
    return true;
}

void DeviceDriver::powerDownMCU(){
    Serial.println(F("powerDownMCU not implemented in this dummy driver"));
}

void DeviceDriver::setTxPwr(uint8_t pwr){
    Serial.println(F("setTxPwr not implemented in this dummy driver"));
}

uint16_t DeviceDriver::getTotalInterferingMargin(){
    return 0;
}

void DeviceDriver::resetStatistics(){
    
}

//If the driver did not implement the random function, return the onboard random
byte DeviceDriver::random(){
    return analogRead(A0);
}