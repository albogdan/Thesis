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

#include "Utilities.h"

int8_t translateInterruptPin(uint8_t digitalPin){

  #if defined (__AVR_ATmega328P__)
  return digitalPinToInterrupt(digitalPin);
  #endif
  
  #if defined (__AVR_ATmega32U4__)
  /* Source: https://www.arduino.cc/reference/en/language/functions/external-interrupts/attachinterrupt/ */
  switch(digitalPin){
    case 0:
      return 2;

    case 1:
      return 3;

    case 2:
      return 1;

    case 3:
      return 0;

    case 7:
      return 4;

    default:
      return NOT_AN_INTERRUPT;
  }
  #endif

  return NOT_AN_INTERRUPT;
}


void deepSleep(uint8_t irqPin, void (*wake)(), uint8_t mode){
  
  int8_t interruptNumber = translateInterruptPin(irqPin);

  if(interruptNumber == NOT_AN_INTERRUPT){
      Serial.println(F("Refuse to enter sleep: The IRQ pin is not connected to an valid interrupt pin"));
      return;
  }

  /** This feature does not seem to work on Adafruit 32u4 Feather board where the transceiver IRQ is internally
  * connected to pin 7 (INT4).
  * 
  * If you are using other 32u4 MCUs without on-board transcievers, you can comment out the following lines 
  * 
  * Reference:
  * 1. https://learn.adafruit.com/adafruit-feather-32u4-radio-with-lora-radio-module/pinouts
  * 2. Page 43 Section 7.3 "Power-down Mode" in the Atmega16u4/32u4 data sheet.
  */
  #if defined (__AVR_ATmega32U4__)
  if(interruptNumber == INTF4){
      Serial.println(F("Refuse to enter sleep: INT4 on 32u4 cannot wake up the MCU from power-down mode"));
      return;
  }
  #endif

  //Make sure the debugging messages are printed correctly before goes to sleep
  Serial.flush();

  byte adcState = ADCSRA;
  // disable ADC
  ADCSRA = 0;

  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  sleep_enable();

  // Do not interrupt before we go to sleep, or the
  // ISR will detach interrupts and we won't wake.
  noInterrupts();

  // If an interrupt handler (ISR) has not been attached, attach it
  if(wake){
    attachInterrupt(interruptNumber, wake, mode);
  }
  
  EIFR = bit(interruptNumber); // clear flag for transceiver-based interrupt

  // Software brown-out only works in ATmega328p
  #if defined(__AVR_ATmega328P__) || defined(__AVR_ATmega168P__)
  // turn off brown-out enable in software
  // BODS must be set to one and BODSE must be set to zero within four clock cycles
  MCUCR = bit(BODS) | bit(BODSE);
  // The BODS bit is automatically cleared after three clock cycles
  MCUCR = bit(BODS);
  #endif

  // We are guaranteed that the sleep_cpu call will be done
  // as the processor executes the next instruction after
  // interrupts are turned on.
  interrupts(); // one cycle
  sleep_cpu();  // one cycle
  //The MCU is turned off after this point

  /**
   * Now the MCU has woken up, wait a while for the system to fully start up
   * Note: this is based on experience, without delays, some bytes will be
   * lost when we read from software serial
   */
                    
  sleepForMillis(50);
  
  //Restore the ADC
  ADCSRA = adcState;
}

void turnOnRTC(uint8_t vcc)
{
    digitalWrite(vcc, HIGH);
    //pinMode(vcc, OUTPUT);
    delay(300);
    //RTC.begin();
}

void turnOffRTC(uint8_t vcc)
{
    //pinMode(vcc, INPUT);
    digitalWrite(vcc, LOW);

    // Turn off the I2C interface for RTC
    //TWCR &= ~(bit(TWEN) | bit(TWIE) | bit(TWEA));

    // turn off I2C pull-ups
    //digitalWrite(A4, LOW);
    //digitalWrite(A5, LOW);
}

void setAlarm(time_t t){
    RTC.alarm(ALARM_1);
    tmElements_t tm;
    breakTime(t, tm);
    RTC.setAlarm(ALM1_MATCH_DATE, tm.Second, tm.Minute, tm.Hour, tm.Day);
    delay(10);
}

time_t compileTime()
{
    const time_t FUDGE(10);    //fudge factor to allow for upload time, etc. (seconds, YMMV)
    const char *compDate = __DATE__, *compTime = __TIME__, *months = "JanFebMarAprMayJunJulAugSepOctNovDec";
    char compMon[4], *m;

    strncpy(compMon, compDate, 3);
    compMon[3] = '\0';
    m = strstr(months, compMon);

    tmElements_t tm;
    tm.Month = ((m - months) / 3 + 1);
    tm.Day = atoi(compDate + 4);
    tm.Year = atoi(compDate + 7) - 1970;
    tm.Hour = atoi(compTime);
    tm.Minute = atoi(compTime + 3);
    tm.Second = atoi(compTime + 6);

    time_t t = makeTime(tm);
    return t + FUDGE;        //add fudge factor to allow for compile time
}