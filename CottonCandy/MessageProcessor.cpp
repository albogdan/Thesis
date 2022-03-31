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

#include "MessageProcessor.h"
#include "Utilities.h"

/* Security CMAC*/
#include "AES_CMAC.h"
AESTiny128 aes128;
AES_CMAC cmac(aes128);

const uint8_t key[16] = {0x01, 0x00, 0x00, 0x02, 0x00, 0x06, 0x05, 0x06, 0x03, 0x01,
                         0x01, 0x00, 0x00, 0x02, 0x00, 0x06};

/**
 * A preallocated buffer reserved for tx/rx events, i.e. receiving a packet
 * It is safe to share it among tx/rx events since they are not concurrent
 * 
 * The size is set to a large value which no packet would be larger than this
 */
static byte trxBuff[TRX_BUFFER_SIZE];

// The caller makes sure that the buff has a size > UNSIGNED_LONG_SIZE
static unsigned long bytesToLong(byte *buff)
{

    unsigned long l = 0;

    uint8_t shifter = (UNSIGNED_LONG_SIZE - 1) * 8;

    for (uint8_t i = 0; i < UNSIGNED_LONG_SIZE; i++)
    {
        l += (unsigned long)buff[i] << shifter;
        shifter -= 8;
    }


    return l;
}

static void longToBytes(byte* const buff, unsigned long l)
{
    uint8_t shifter = (UNSIGNED_LONG_SIZE - 1) * 8;

    for (uint8_t i = 0; i < UNSIGNED_LONG_SIZE; i++)
    {
        buff[i] = (byte)(l >> shifter) & 0xFF;
        shifter -= 8;
    }
}

GenericMessage::GenericMessage(byte type, byte *srcAddr)
{
    this->type = type;
    memcpy(this->srcAddr, srcAddr, 2);

    len = MSG_LEN_GENERIC;
}

void GenericMessage::toBytes(byte* const msg)
{
    msg[0] = this->type;
    memcpy(msg + 1, this->srcAddr, 2);
}

GenericMessage::~GenericMessage()
{
}

/*--------------------Join Beacon-------------------*/
Join::Join(byte *srcAddr) : GenericMessage(MESSAGE_JOIN, srcAddr)
{
}

/*--------------------JoinACK Message-------------------*/
JoinAck::JoinAck(byte *srcAddr, uint8_t hopsToGateway, uint8_t numChildren, int rssiFeedback, unsigned long nextReqTime) : GenericMessage(MESSAGE_JOIN_ACK, srcAddr)
{
    this->hopsToGateway = hopsToGateway;
    this->rssiFeedback = rssiFeedback;
    this->numChildren = numChildren;
    this->nextReqTime = nextReqTime;

    len = MSG_LEN_GENERIC + MSG_LEN_JOIN_ACK;
}

void JoinAck::toBytes(byte* const msg)
{
    GenericMessage::toBytes(msg);

    uint8_t index = MSG_LEN_GENERIC;
    msg[index] = hopsToGateway;
    index++;

    msg[index] = numChildren;
    index++;

    // Convert the negative rssi to a (positive) byte
    msg[index] = (uint8_t)(abs(rssiFeedback));
    index++;

    byte b[UNSIGNED_LONG_SIZE];
    longToBytes(b, nextReqTime);
    memcpy(msg + index, b, UNSIGNED_LONG_SIZE);
}

/*--------------------JoinCFM Message-------------------*/
JoinCFM::JoinCFM(byte *srcAddr) : GenericMessage(MESSAGE_JOIN_CFM, srcAddr)
{
}

/*--------------------GatewayRequest Message-------------------*/
GatewayRequest::GatewayRequest(byte *srcAddr, byte queryType, byte ulChannel, unsigned long nextReqTime, byte childBackoffTime) : GenericMessage(MESSAGE_GATEWAY_REQ, srcAddr)
{
    this->option = queryType & 0x3F;
    this->ulChannel = ulChannel;

    len = MSG_LEN_GENERIC + MSG_LEN_HEADER_GATEWAY_REQ;

    if (nextReqTime != 0)
    {
        this->option |= MASK_GATEWAY_REQ_NEW_NEXT_TIME;
        this->nextReqTime = nextReqTime;
        len += FIELD_LEN_GATEWAY_REQ_NEXT_TIME;
    }

    if (childBackoffTime != 0)
    {
        this->option |= MASK_GATEWAY_REQ_NEW_MAX_BACKOFF;
        this->childBackoffTime = childBackoffTime;
        len += FIELD_LEN_GATEWAY_REQ_MAX_BACKOFF;
    }
}

void GatewayRequest::toBytes(byte* const msg)
{
    GenericMessage::toBytes(msg);

    uint8_t index = MSG_LEN_GENERIC;

    msg[index] = option;
    index++;

    msg[index] = ulChannel;
    index++;

    //Serial.println(option, HEX);

    if (option & MASK_GATEWAY_REQ_NEW_NEXT_TIME)
    {
        byte b[UNSIGNED_LONG_SIZE];
        
        longToBytes(b, nextReqTime);

        memcpy(msg + index, b, UNSIGNED_LONG_SIZE);
        index += FIELD_LEN_GATEWAY_REQ_NEXT_TIME;
    }

    if (option & MASK_GATEWAY_REQ_NEW_MAX_BACKOFF)
    {
        msg[index] = childBackoffTime;
    }
}

bool GatewayRequest::newMaxBackoff()
{
    return (option & MASK_GATEWAY_REQ_NEW_MAX_BACKOFF);
}

bool GatewayRequest::newNextReqTime()
{
    return (option & MASK_GATEWAY_REQ_NEW_NEXT_TIME);
}

/*--------------------NodeReply Message-------------------*/
NodeReply::NodeReply(byte *srcAddr, byte option,
                     byte dataLength, byte *data) : GenericMessage(MESSAGE_NODE_REPLY, srcAddr)
{
    this->option = option;
    this->dataLength = dataLength;
    this->data = new byte[dataLength];
    memcpy(this->data, data, dataLength);

    len = MSG_LEN_GENERIC + MSG_LEN_HEADER_NODE_REPLY + dataLength;
}

NodeReply::~NodeReply()
{
    if(this->data != nullptr){
        delete[] this->data;
    }
}

/* Copy constructor */
NodeReply::NodeReply(const NodeReply &reply) : GenericMessage(reply)
{
    this->option = reply.option;
    this->dataLength = reply.dataLength;
    this->data = new byte[dataLength];
    memcpy(this->data, reply.data, dataLength);

    len = MSG_LEN_GENERIC + MSG_LEN_HEADER_NODE_REPLY + dataLength;
}

bool NodeReply::aggregated(){
    return option & MASK_NODE_REPLY_AGGREGATED;
}

bool NodeReply::fetchMore(){
    return option & MASK_NODE_REPLY_FETCH_MORE;
}

void NodeReply::toBytes(byte* const msg)
{
    GenericMessage::toBytes(msg);

    uint8_t index = MSG_LEN_GENERIC;
    msg[index] = option;
    index++;

    msg[index] = dataLength;
    index++;

    memcpy(msg + index, data, dataLength);
}

GenericMessage *receiveMessage(DeviceDriver *driver, unsigned long timeout)
{
    unsigned long startTime = getTimeMillis();
    GenericMessage *msg = nullptr;

    byte msgType = 0xFF;
    byte destAddr[2];
    byte srcAddr[2];
    byte* buffPtr = trxBuff;

    while ((unsigned long)(getTimeMillis() - startTime) < timeout)
    {
        if(!readMsgFromBuff(driver, trxBuff, 2 + MSG_LEN_GENERIC, timeout)){
            continue;
        }

        msgType = trxBuff[2];      
        memcpy(destAddr, trxBuff, 2);
        memcpy(srcAddr, trxBuff + 3, 2);
        
        break;
    }

    if(msgType == 0xFF){
        return nullptr;
    }

    buffPtr += (2 + MSG_LEN_GENERIC);
 
    // get the complete message from device buffer
    switch (msgType)
    {
    case MESSAGE_JOIN:
    {
        msg = new Join(srcAddr);
        break;
    }

    case MESSAGE_JOIN_ACK:
    {
        // we have already read the msg type
        if(!readMsgFromBuff(driver, buffPtr, MSG_LEN_JOIN_ACK, timeout)){
            break;
        }

        uint8_t hopsToGateway = buffPtr[0];
        uint8_t numChildren = buffPtr[1];
        int rssiFeedback = -((int)buffPtr[2]);
        unsigned long nextReqTime = bytesToLong(buffPtr + 3);

        msg = new JoinAck(srcAddr, hopsToGateway, numChildren, rssiFeedback, nextReqTime);
        break;
    }

    case MESSAGE_JOIN_CFM:
    {
        msg = new JoinCFM(srcAddr);
        break;
    }
    case MESSAGE_GATEWAY_REQ:
    {
        // we have already read the msg type & src addr
        if(!readMsgFromBuff(driver, buffPtr, MSG_LEN_HEADER_GATEWAY_REQ, timeout)){
            break;
        }

        byte option = buffPtr[0];
        byte ulChannel = buffPtr[1];

        unsigned long nextReqTime = 0;
        byte childBackoffTime = 0;

        buffPtr += MSG_LEN_HEADER_GATEWAY_REQ;

        if (option & MASK_GATEWAY_REQ_NEW_NEXT_TIME)
        {
            if(readMsgFromBuff(driver, buffPtr, UNSIGNED_LONG_SIZE, timeout)){
                nextReqTime = bytesToLong(buffPtr);
                buffPtr += UNSIGNED_LONG_SIZE;
            }
        }

        if (option & MASK_GATEWAY_REQ_NEW_MAX_BACKOFF)
        {
            if(readMsgFromBuff(driver, buffPtr, 1, timeout)){
                childBackoffTime = buffPtr[0];
            }
            
        }
        //Serial.println(nextReqTime);
        //Serial.println(childBackoffTime);

        msg = new GatewayRequest(srcAddr, option, ulChannel, nextReqTime, childBackoffTime);

        break;
    }

    case MESSAGE_NODE_REPLY:
    {
        // we have already read the msg type
        // need to know the data length before getting the data

        // get Header first
        if(!readMsgFromBuff(driver, buffPtr, MSG_LEN_HEADER_NODE_REPLY, timeout)){
            break;
        }

        byte option = buffPtr[0];
        byte dataLength = buffPtr[1];

        if(dataLength > MAX_LEN_DATA_NODE_REPLY){
            break;
        }

        buffPtr += MSG_LEN_HEADER_NODE_REPLY;
        if(readMsgFromBuff(driver, buffPtr, dataLength, timeout)){
            msg = new NodeReply(srcAddr, option, dataLength, buffPtr);
        }

        break;
    }

    default:
        msg = nullptr;
        break;
    }

    if (msg != nullptr)
    {
        bool secure = true;
        byte receivedMac[TRUNCATED_CMAC_SIZE];

        if(!readMsgFromBuff(driver, receivedMac, TRUNCATED_CMAC_SIZE, timeout)){
            secure = false;
        }
        else{
            //unsigned long start = getTimeMillis();
            byte mac[16];
            /*
            for(int i = 0; i < msg->len + 2; i++){
                Serial.print(trxBuff[i],HEX);
                Serial.print("-");
            }
            Serial.println();
            */

            cmac.generateMAC(mac, key, trxBuff, msg->len + 2);
            for(uint8_t i = 0; i < TRUNCATED_CMAC_SIZE; i++){
                //Serial.print(receivedMac[i],HEX);
                //Serial.print("/");
                //Serial.print(mac[i],HEX);
                //Serial.print("-");
                if(mac[i] != receivedMac[i]){
                    secure = false;
                    break;
                }
            }
            //Serial.println(F("Time for CMAC verification: "));
            //Serial.println(getTimeMillis() - start);
        }

        if(!secure){
            Serial.println(F("Warning: Packet MAC corrupted. Discard."));
            delete msg;
            msg = nullptr;
            return nullptr;
        }

        msg->rssi = driver->getLastMessageRssi();
    }

    return msg;
}

int sendMessage(DeviceDriver* driver, byte* destAddr, GenericMessage* msg){
    if(driver == nullptr){
        return -1;
    }

    //Insert the destination address at the very beginning
    memcpy(trxBuff, destAddr, 2);

    uint8_t finalPacketLen = 2;
    
    msg->toBytes(trxBuff + finalPacketLen);
    finalPacketLen += msg->len;

    byte mac[16];
    cmac.generateMAC(mac, key, trxBuff, finalPacketLen);
    
    //Use the first 4 bytes of the MAC
    memcpy(trxBuff + finalPacketLen, mac, TRUNCATED_CMAC_SIZE);
    finalPacketLen += TRUNCATED_CMAC_SIZE;

    /*
    for(uint8_t i = 0; i< finalPacketLen; i++){
        Serial.print(trxBuff[i], HEX);
        Serial.print('_');
    }
    */

    int result = 0;
    result= driver->send(destAddr, trxBuff, finalPacketLen);

    return result;
}

/*-------------------- Helpers -------------------*/
//Make sure the buff has enough space for the msgLen
uint8_t readMsgFromBuff(DeviceDriver *driver, byte* buff, uint8_t msgLen, unsigned long timeout)
{
    int i = 0;

    unsigned long startTime = getTimeMillis();

    while (i < msgLen && (unsigned long)(getTimeMillis() - startTime) < timeout)
    {
        if (driver->available())
        {
            byte c = driver->recv();
            buff[i] = c;
            //Serial.print(buff[i], HEX);
            //Serial.print(" _ ");
            i++;
        }
        // if(c == 0xFF){
        //     continue;
        // }
    }

    if(i < msgLen){
        Serial.println(F("Warning: Incomplete Message"));
        return 0;
    }

    return i;
}