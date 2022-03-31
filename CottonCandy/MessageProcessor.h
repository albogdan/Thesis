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


#ifndef HEADER_MESSAGE_PROCESSOR
#define HEADER_MESSAGE_PROCESSOR

#include "DeviceDriver.h"

#define MESSAGE_JOIN              1
#define MESSAGE_JOIN_ACK          2
#define MESSAGE_JOIN_CFM          3

/* We deprecated those two message types */
//#define MESSAGE_CHECK_ALIVE       4
//#define MESSAGE_REPLY_ALIVE       5

#define MESSAGE_GATEWAY_REQ       6
#define MESSAGE_NODE_REPLY        7
#define MESSAGE_AGGREGATED_REPLY  8

#define MSG_LEN_GENERIC           3

//The following message lengths exclude the length of generic header
#define MSG_LEN_JOIN_ACK          7
#define MSG_LEN_HEADER_GATEWAY_REQ  2
#define MSG_LEN_HEADER_NODE_REPLY 2

#define MASK_GATEWAY_REQ_NEW_NEXT_TIME      0x80
#define MASK_GATEWAY_REQ_NEW_MAX_BACKOFF    0x40
#define MASK_GATEWAY_REQ_QUERY_TYPE         0x3F

#define FIELD_LEN_GATEWAY_REQ_NEXT_TIME 4
#define FIELD_LEN_GATEWAY_REQ_MAX_BACKOFF 1

#define MASK_NODE_REPLY_AGGREGATED 0x80
#define MASK_NODE_REPLY_FETCH_MORE 0x40

#define MAX_LEN_DATA_NODE_REPLY 64

#define UNSIGNED_LONG_SIZE sizeof(unsigned long)

#define TRUNCATED_CMAC_SIZE 4

#define TRX_BUFFER_SIZE 100

/* We will use polymorphism here */
class GenericMessage
{

public:
    byte type;
    byte srcAddr[2];
    
    /**
     * For every message receveid, there will be an RSSI value associated
     */
    int rssi;

    /**
     * Message length
     */ 
    uint8_t len;

    GenericMessage(byte type, byte* srcAddr);
    virtual void toBytes(byte* const msg);

    virtual ~GenericMessage();
};

/*--------------------Join Beacon-------------------*/
class Join: public GenericMessage
{
public:
    Join(byte* srcAddr);
};

/*--------------------JoinACK Message-------------------*/
class JoinAck: public GenericMessage
{
public:
    /* Message fields */
    uint8_t hopsToGateway;
    int rssiFeedback;
    uint8_t numChildren;
    unsigned long nextReqTime;

    JoinAck(byte* srcAddr, uint8_t hopsToGateway, uint8_t numChildren, int rssiFeedback, unsigned long nextReqTime);
    
    virtual void toBytes(byte* const msg);
};

/*--------------------JoinCFM Message-------------------*/
class JoinCFM: public GenericMessage
{
public:
    JoinCFM(byte* srcAddr);
};

/*--------------------GatewayRequest Message-------------------*/
class GatewayRequest: public GenericMessage
{
public:

    /**
     * Bit number from right(lowest) to left(highest) 
     * 
     * Bit 7: new GatewayReqTime
     * Bit 6: new BackoffTime
     * Bit 5-0: reserved for network management
     * 
     */ 
    byte option;

    unsigned long nextReqTime;
    byte childBackoffTime;
    byte ulChannel;

    GatewayRequest(byte* srcAddr, byte queryType, byte ulChannel, unsigned long nextReqTime = 0, byte childBackoffTime = 0);
    bool newMaxBackoff();
    bool newNextReqTime();

    virtual void toBytes(byte* const msg);
};

/*--------------------NodeReply Message-------------------*/
class NodeReply: public GenericMessage
{
public:    
    byte option;
    byte dataLength;
    byte* data; // maximum length 64 bytes

    NodeReply(byte* srcAddr, byte option,
                byte dataLength, byte* data);
    NodeReply(const NodeReply &reply);
    ~NodeReply();

    bool aggregated();
    bool fetchMore();

    virtual void toBytes(byte* const msg);
};

/*
 * Reads from device buffer, constructs a message and returns a pointer to it.
 * The timeout value will be used for terminating the receiving in the following
 * 2 scenarios:
 * 
 *      1. No valid message has been received
 *      2. Valid messages are received but are truncated due to collisions or
 *         other wireless signal corruption. In this case, the program can
 *         block to read the remaining data. For messages with variable length 
 *         (i.e. NodeReply). They can be received but the "length" field can be 
 *         corrupted. Thus, the program might block to read an arbitarily long 
 *         byte array, which causes the program to hang.
 * 
 * Note that the timeout value does not limit the program run-time. The actual run
 * time might exceed 1 second.
 * 
 * !! Caller needs to free the memory after using the returned pointer
 */
GenericMessage* receiveMessage(DeviceDriver* driver, unsigned long timeout);
int sendMessage(DeviceDriver* driver, byte* destAddr, GenericMessage* msg);

/*
 * Read certain bytes from device buffer (helper function)
 */
static uint8_t readMsgFromBuff(DeviceDriver* driver, byte* buff, uint8_t msgLen, unsigned long timeout);

#endif
