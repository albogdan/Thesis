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

#include "AdafruitDeviceDriver.h"

//Those are modified in the interrupt call
volatile byte msgQueue[MSG_QUEUE_CAPACITY];
volatile uint8_t queueTail; //next index to write
volatile uint8_t queueSize;

uint8_t queueHead; //next index to read;

byte *adafruitAddr;

uint16_t totalInterferingMargin;

AdafruitDeviceDriver::AdafruitDeviceDriver(byte *addr,
                                           uint8_t csPin, uint8_t rstPin, uint8_t intPin) : DeviceDriver()
{

    LoRa.setPins(csPin, rstPin, intPin);

    setAddress(addr);

    this->irqPin = intPin;

    queueHead = 0;
    queueTail = 0;
    queueSize = 0;
}

AdafruitDeviceDriver::~AdafruitDeviceDriver()
{
}

void onReceive(int packetSize)
{
    if (queueSize + packetSize > MSG_QUEUE_CAPACITY)
    {
        Serial.println(F("Queue is full"));
        return;
    }
    byte add0 = LoRa.read();
    byte add1 = LoRa.read();
    //Serial.print(add0, HEX);
    //Serial.print(add1, HEX);

    bool correctRecipient = true;
    if ((adafruitAddr[0] != add0 ||
         adafruitAddr[1] != add1) &&
        !(add0 == 0xFF && add1 == 0xFF))
    {

        // Compute the margin of the packet (i.e. How many dB higher than the minimum sensitivity)
        totalInterferingMargin += (LoRa.packetRssi() + 123);
        correctRecipient = false;
    }else{
        //Write the destination address into the buffer as well
        msgQueue[queueTail] = add0;
        queueTail = (queueTail + 1) % MSG_QUEUE_CAPACITY;
        msgQueue[queueTail] = add1;
        queueTail = (queueTail + 1) % MSG_QUEUE_CAPACITY;
        queueSize += 2;
    }
    // Serial.print("onReceive to queue, size: ");
    // Serial.println(packetSize);
    // Serial.print("Before, queue size: ");
    // Serial.println(queueSize);

    //Regardless of the correct recipient or not, we should read the buffer
    while (LoRa.available())
    {
        byte result = LoRa.read();
        // Serial.print("0x");
        // Serial.print(result,HEX);
        // Serial.print(" ");

        //Save the packet to buffer if the recipient is correct
        if(correctRecipient){
            queueSize++;
            msgQueue[queueTail] = result;
            queueTail = (queueTail + 1) % MSG_QUEUE_CAPACITY;
        }
    }
    //     Serial.println("");
    //     Serial.print("After, queue end: ");
    //     Serial.println(queueSize);
}

bool AdafruitDeviceDriver::init()
{
    return this->init(RF95_FREQ, DEFAULT_SPREADING_FACTOR, DEFAULT_CHANNEL_BW, DEFAULT_CODING_RATE_DENOMINATOR);
}

bool AdafruitDeviceDriver::init(long frequency, uint8_t sf, long bw, uint8_t cr)
{
    // Assign class member variables
    m_freq = frequency;
    m_sf = sf;
    m_bw = bw;
    m_cr = cr;

    if (!LoRa.begin(m_freq))
    {
        Serial.println(F("Starting LoRa failed!"));
        return false;
    }

    // Assign the parameters to the actual LoRa module which writes to the hardware register
    LoRa.setSpreadingFactor(sf);
    LoRa.setSignalBandwidth(m_bw);
    LoRa.setCodingRate4(m_cr);
    LoRa.enableCrc();

    LoRa.onReceive(onReceive);
    setMode(STANDBY);
    Serial.println(F("LoRa Module initialized"));

    return true;
}

int AdafruitDeviceDriver::send(byte *destAddr, byte *msg, uint8_t msgLen)
{
    LoRa.beginPacket();
    //LoRa.write(destAddr, 2);
    LoRa.write(msg, msgLen);
    int result = LoRa.endPacket(false) == 1 ? 1 : -1;
    //After transmission, the transceiver is still in TX state
    //Therefore, we should change it to the RX state
    LoRa.receive();
    m_mode = RX;
    return result;
}

byte AdafruitDeviceDriver::recv()
{
    if (available())
    {
        byte result = msgQueue[queueHead];
        //Serial.print("Received: 0x");
        //Serial.print(result,HEX);
        //Serial.println("");
        queueHead = (queueHead + 1) % MSG_QUEUE_CAPACITY;
        queueSize--;
        return result;
    }
    else
    {
        return -1;
    }
}
int AdafruitDeviceDriver::available()
{
    return queueSize;
}
int AdafruitDeviceDriver::getLastMessageRssi()
{

    int result = LoRa.packetRssi();
    return result;
}

/*-----------LoRa Configuration-----------*/
void AdafruitDeviceDriver::setAddress(byte *addr)
{

    if (sizeof(addr) < 2)
    {
        Serial.println(F("Error: Node address must be 2-byte long"));
    }
    else
    {
        m_addr[0] = addr[0];
        m_addr[1] = addr[1];
        adafruitAddr = m_addr;
    }
}

void AdafruitDeviceDriver::setFrequency(unsigned long frequency)
{
    if (m_freq == frequency)
    {
        return;
    }
    m_freq = frequency;

    DeviceMode prevMode = m_mode;
    setMode(STANDBY);
    LoRa.setFrequency(frequency);

    //Switch to the original mode once finished
    setMode(prevMode);
}

void AdafruitDeviceDriver::setSpreadingFactor(uint8_t sf)
{
    if (m_sf != sf)
    {
        m_sf = sf;
        LoRa.setSpreadingFactor(sf);
    }
}

void AdafruitDeviceDriver::setChannelBandwidth(long bw)
{
    if (m_bw != bw)
    {
        m_bw = bw;
        LoRa.setSignalBandwidth(bw);
    }
}

void AdafruitDeviceDriver::setCodingRateDenominator(uint8_t cr)
{
    if (m_cr != cr)
    {
        m_cr = cr;
        LoRa.setCodingRate4(cr);
    }
}

uint8_t AdafruitDeviceDriver::getDeviceType()
{
    return DeviceType::ADAFRUIT_LORA;
}

void AdafruitDeviceDriver::powerDownMCU()
{
    deepSleep(irqPin);
}

uint16_t AdafruitDeviceDriver::getTotalInterferingMargin()
{
    return totalInterferingMargin;
}

void AdafruitDeviceDriver::resetStatistics()
{
    totalInterferingMargin = 0;
}

byte AdafruitDeviceDriver::random()
{
    return LoRa.random();
}

void AdafruitDeviceDriver::setMode(DeviceMode mode)
{

    if (mode == m_mode)
    {
        return;
    }

    switch (mode)
    {
    case SLEEP:
        LoRa.sleep();
        m_mode = mode;
        break;

    case RX:
        LoRa.receive();
        m_mode = mode;
        break;

    case STANDBY:
        LoRa.idle();
        m_mode = mode;
        break;

    default:
        break;
    }

    return;
}

void AdafruitDeviceDriver::setTxPwr(uint8_t pwr)
{
    if (m_txPwr == pwr)
    {
        return;
    }
    m_txPwr = pwr;
    DeviceMode prevMode = m_mode;
    setMode(STANDBY);

    LoRa.setTxPower(m_txPwr, PA_OUTPUT_PA_BOOST_PIN);    

    //Switch to the original mode once finished
    setMode(prevMode);
}