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

#ifndef HEADER_FORWARD_ENGINE
#define HEADER_FORWARD_ENGINE

#include "DeviceDriver.h"
#include "MessageProcessor.h"
#include "Utilities.h"
#include "FreqPlanNA.h"

/*-------------States of a Node------------*/
enum State
{
    DISCONNECTED,
    CONNECTED,
    READY1,
    READY2,
    TALK_TO_CHILDREN,
    LISTEN_TO_PARENT,
    OBSERVE,
    HIBERNATE1,
    HIBERNATE2,
    HIBERNATE3,
    INVALID
};


/* Setting the first bit of the address to 1 indicates a gateways */
#define GATEWAY_ADDRESS_MASK 0x80

/* The default timeout value for receiving a message is 500 milliseconds */
#define RECEIVE_TIMEOUT 500

/* The RSSI threshold for choosing a parent node */
#define MIN_LINK_QUALITY -110

/* The maximum number of children a node can have */
#define MAX_NUM_CHILDREN 2

/* The minimum backoff time when the node reply back (ms) */
#define MIN_BACKOFF_TIME 100

/** The maximum backoff time when tranmitting a JoinACK message (ms)
 * 
 * Unlike the backoff time for sending NodeReply and forwarding GatewayReq, this is a static value
 * since the chance of collisions depends on the number of nearby connected nodes, which can not be
 * easily inferred. 
*/
#define MAX_JOIN_ACK_BACKOFF_TIME (unsigned long)1000

/**
 * @brief RTC read can result in error up to 1 second due to the lack of milisecond resolution
 * For example, the actual time is 50.9 second, but RTC will read only 50 second omitting the values
 * after the decimal.
 */
#define MAX_RTC_READ_ERROR_SECOND 1

/** The maximum backoff time (in seconds) for one child node to send NodeReply or forward GatewayReq 
* 
Parent node (including gateway) uses this value and multiply it with the number of child nodes
* to inform the child nodes of the maximum backoff time they have to wait.
*/
#define MAX_BACKOFF_TIME_FOR_ONE_CHILD 3

/** Default time for waiting for the next GatewayRequest is a day (24 hours = 86,400 seconds).
 * If the user do not specify the GatewayReq time during setup,  the node will wait forever for
 * the next GatewayRequest. If connection is broken before the next GatewayRequest comes in,
 * the node will not self-heal until a full day is passed
*/
#define DEFAULT_NEXT_GATEWAY_REQ_TIME 86400

#define INITIAL_MAX_JOIN_ATTEMPTS 10
#define SELF_HEAL_MAX_JOIN_ATTEMPTS 3

/**
 * Maximum backoff time (in ms) after a failed join attempt
 */ 
#define DEFAULT_MAX_JOIN_BACKOFF (unsigned long)30000

/** Gateway request may arrive later than expected due to transmission and processing delays.
 * The node marks a missing gateway request if none is received for 
 *         (NEXT_GATEWAY_REQ_TIME_TOLERANCE_FACTOR * Advertised next request time interval)
*/
#define NEXT_GATEWAY_REQ_TIME_TOLERANCE_FACTOR 1.2


#define DEFAULT_INTERFERING_MARGIN_THRESHOLD 30

#define AGGREGATION_BUFFER_SIZE 256

/**
 * Preparation time before a DCP official starts
 */ 
#define EARLY_WAKE_UP_TIME (time_t)5

#define MILLISECOND_MULTIPLIER (unsigned long)1E3

typedef enum
{
    NO_SLEEP,

    /**
     * This mode turns off the MCU but always leaves the transceiver on in RX mode.
     * The MCU is woken up as soon as the transceiver receives a packet
     */
    SLEEP_TRANSCEIVER_INTERRUPT,

    /**
     * This mode turns off both the MCU and transceiver until the RTC alarm is
     * triggered. The RTC alarm wakes up the MCU and put the entire system into
     * the "SLEEP_TRANSCEIVER_INTERRUPT" mode until the receiving time period
     * expires
     */
    SLEEP_RTC_INTERRUPT
} SleepMode;

struct ParentInfo
{
    byte parentAddr[2];
    uint8_t hopsToGateway;
    uint8_t numChildren;
    uint8_t channel;
    int linkQuality;
    time_t nextGatewayReqTime;
};

struct ChildNode
{
    byte nodeAddr[2];
    bool confirmed = false;

    ChildNode *next;

    time_t joinAckExpiryTime;

    NodeReply* reply = nullptr;
};

void wake();

class ForwardEngine
{

public:
    /**
     * Copy constructor
     */
    ForwardEngine(const ForwardEngine &node);

    /**
     * Destructor
     */
    ~ForwardEngine();

    /**
     * Constructor. Requires driver and assigned addr
     */
    ForwardEngine(byte *addr, DeviceDriver *driver);

    /**
     * Try to join an existing network by finding a parent. Return true if successfully joined an 
     * existing network
     */
    bool join();

    /**
     * Node exits an existing network
     */
    void disconnect();

    /**
     * This is the core loop where the node operates sending and receiving after it  joined the network.
     * Returns true if the node is started successfully.
     * 
     * Note: This method can be used without calling join() prior. In this case, the node assumes that
     * there is no existing network and it is the first node in the new network. 
     */
    bool run();

    //Setter for the node address
    void setAddr(byte *addr);

    /**
     * Getter for the node address
     */
    byte *getMyAddr();

    /**
     * Getter for the parent address
     */
    byte *getParentAddr();

    void setGatewayReqTime(unsigned long gatewayReqTime);

    unsigned long getGatewayReqTime();

    void onReceiveRequest(void (*callback)(byte **, byte *));
    void onReceiveResponse(void (*callback)(byte *, byte, byte *));
    void preDataCollectionCallback(void(*callback)());
    void postDataCollectionCallback(void(*callback)());


    void setSleepMode(uint8_t sleepMode, uint8_t rtcInterruptPin, uint8_t rtcVccPin);

private:

    bool runNode();
    bool runGateway();

    void handleJoin(Join* join);
    void handleJoinCFM(JoinCFM* cfm);
    void handleReq(GatewayRequest* req);
    void handleReply(NodeReply* reply);

    void receiveUntillInterrupt();
    void talkToChildren();
    bool hibernate(time_t hibernationEnd);

    uint8_t cleanChildrenList(time_t currentTime);

    /**
     * callback function pointer when Node receives Gateway Requests
     * arguments are to pass back msg and num of bytes
     */
    void (*onRecvRequest)(byte **, byte *);

    /**
     * callback function pointer when Gateway receives responses from Nodes
     * argument is msg, num of bytes and sender address
     */
    void (*onRecvResponse)(byte *, byte, byte *);

    /**
     * callback function pointer when Gateway begins data collection
     * argument is none
     */
    void (*onPreDataCollection)();

    /**
     * callback function pointer when Gateway ends data collection
     * argument is none
     */
    void (*onPostDataCollection)();

    /* Basic information*/
    byte myAddr[2];
    DeviceDriver *myDriver;
    ParentInfo myParent;
    uint8_t hopsToGateway;

    /**
     * Time interval for gateway to request data from nodes
     */
    time_t gatewayReqTime = DEFAULT_NEXT_GATEWAY_REQ_TIME;

    /**
     * Maximum time value in milliseconds for the random backoff time before 
     * transmitting gateway requests and node replies.
     * 
     * This value can be dynamically changed by the parent node based on its
     * number of child nodes. The parent node informs the child node the new
     * max backoff time in the Gateway Request message. 
     */
    uint16_t maxBackoffTime = MIN_BACKOFF_TIME;

    uint8_t sleepMode = SleepMode::NO_SLEEP;

    /* Timing parameters used in sleep cycles */
    time_t receivingPeriod = 300; //By default, the receiving period is 5 minutes
    time_t receivingPeriodTimeout;

    time_t requestInterval = 10;

    bool m_alarmSetForReceiving = false;
    bool m_useDLChannel = false;

    /* A counter for missing consecutive gateway requests */
    uint8_t consecutiveMissingReqs = 0;

    /* Number of Consecutive Transitions into HIBERNATION during DCP*/
    uint8_t hibernationCounter = 0;

    /* Multi-channel parameters*/
    uint8_t m_channel;
    //float m_interferingThreshold = DEFAULT_INTERFERING_MARGIN_THRESHOLD;

    /* Join paramters*/
    uint8_t m_joinAttempts = 0;
    uint8_t m_maxJoinAttempts = INITIAL_MAX_JOIN_ATTEMPTS;

    uint8_t m_txPwr = MIN_TX_PWR;

    /* Child management*/
    uint8_t numChildren;
    uint8_t numOutgoingJoinAcks;
    ChildNode *childrenList; /* A linked list*/

    bool rtcError = false;
};

#endif
