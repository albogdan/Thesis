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

#ifndef HEADER_UTILITIES
#define HEADER_UTILITIES

#include "Arduino.h"
#include <DS3232RTC.h>
#include "avr/sleep.h"

// utility functions for logging
extern bool DEBUG_ENABLE;
#define Serial if(DEBUG_ENABLE)Serial

#define sleepForMillis delay
#define getTimeMillis millis

int8_t translateInterruptPin(uint8_t digitalPin);

void deepSleep(uint8_t INT = 2, void (*wake)() = nullptr, uint8_t mode = FALLING);

/*------------------ RTC Utilities ------------------*/
void turnOnRTC(uint8_t vcc);
void turnOffRTC(uint8_t vcc);
void setAlarm(time_t t);

time_t compileTime();

#endif