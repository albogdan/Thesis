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

#include "ForwardEngine.h"
#include "MemoryFree.h"

static ChildNode *findChild(byte *addr, ChildNode *start);

uint8_t myRTCInterruptPin;
uint8_t myRTCVccPin;

/* Current State */
/**
 * There is no need to disable interrupt when reading 8-bit volatile variables 
 * Referenec: https://www.arduino.cc/reference/en/language/variables/variable-scope-qualifiers/volatile/
*/
volatile uint8_t state = INVALID;

/* Buffer Size */
volatile uint8_t bufferSize = 0;

volatile bool alarmSetForReceiving = false;

void wake()
{
    sleep_disable();

    switch (state)
    {
    case READY1:
    {
        state = READY2;
        alarmSetForReceiving = false;
        break;
    }
    case READY2:
    {
        state = OBSERVE;
        alarmSetForReceiving = false;
        break;
    }
    case HIBERNATE1:
    {
        state = DISCONNECTED;
        break;
    }
    case HIBERNATE2:
    {
        state = TALK_TO_CHILDREN;
        break;
    }
    case HIBERNATE3:
    case CONNECTED:
    {
        state = READY1;
        break;
    }
    case LISTEN_TO_PARENT:
    {
        state = HIBERNATE3;
        break;
    }
    default:
        break;
    }
}

ForwardEngine::ForwardEngine(byte *addr, DeviceDriver *driver)
{
    this->setAddr(addr);

    myDriver = driver;

    // A node is its own parent initially
    memcpy(myParent.parentAddr, myAddr, 2);
    myParent.hopsToGateway = 255;

    numChildren = 0;
    numOutgoingJoinAcks = 0;
    childrenList = nullptr;

    /**
     * Here we will set the random seed to a random value associated with the transceiver wideband RSSI
     * If the transceiver does not have a random function, analogRead(A3) will be used (A3 must not be
     * connected to anything)
     */
    randomSeed(analogRead(A3));

    sleepMode = SleepMode::NO_SLEEP;
}

ForwardEngine::~ForwardEngine()
{
    ChildNode *iter = childrenList;
    while (iter != nullptr)
    {
        ChildNode *temp = iter;

        if(iter->reply != nullptr){
            delete iter->reply;
        }
        iter = temp->next;
        delete temp;
    }
}

void ForwardEngine::setAddr(byte *addr)
{
    memcpy(myAddr, addr, 2);
}

byte *ForwardEngine::getMyAddr()
{
    return this->myAddr;
}

byte *ForwardEngine::getParentAddr()
{
    return this->myParent.parentAddr;
}

void ForwardEngine::setGatewayReqTime(unsigned long gatewayReqTime)
{
    this->gatewayReqTime = gatewayReqTime;
}

unsigned long ForwardEngine::getGatewayReqTime()
{
    return this->gatewayReqTime;
}

void ForwardEngine::onReceiveRequest(void (*callback)(byte **, byte *))
{
    this->onRecvRequest = callback;
}
void ForwardEngine::onReceiveResponse(void (*callback)(byte *, byte, byte *))
{
    this->onRecvResponse = callback;
}
void ForwardEngine::preDataCollectionCallback(void (*callback)())
{
    this->onPreDataCollection = callback;
}
void ForwardEngine::postDataCollectionCallback(void (*callback)())
{
    this->onPostDataCollection = callback;
}

/**
 * The join function is responsible for sending out a beacon to discover neighboring
 * nodes. After sending out the beacon, the node will receive messages for a given
 * period of time. Since the node might receive multiple replies of its beacon, as well
 * as the beacons from other nearby nodes, it waits for a period of time to collect info
 * from the nearby neighbors, and pick the best parent using the replies received.
 *
 * Returns: True if the node has joined a parent
 */
bool ForwardEngine::join()
{
    if (state != DISCONNECTED)
    {
        // The node has already joined a network
        return true;
    }

    Serial.println(F("Ready to join"));

    GenericMessage *msg = nullptr;

    ParentInfo bestCandidate;
    bestCandidate.hopsToGateway = 255;

    // Set the Tx power
    myDriver->setMode(STANDBY);
    myDriver->setTxPwr(m_txPwr);
    myDriver->setFrequency(channelFrequency(DOWNLINK_CHANNEL));

    Join beacon(myAddr);

    // Send out the beacon once to discover nearby nodes
    sendMessage(myDriver, BROADCAST_ADDR, &beacon);

    unsigned long previousTime = getTimeMillis();

    /**
     * In this loop, for a period of MAX_JOIN_ACK_BACKOFF_TIME + some air time,
     * the node will wait for joinAck messages
     *
     * It is possible that the node did not receive any joinAck messages at all.
     * In this case, the loop will timeout eventually.
     */

    // Add 300ms for turning on RTC at the candidate and 200ms for air time
    unsigned long timeout = MAX_JOIN_ACK_BACKOFF_TIME + 300 + 200;

    sleepForMillis(timeout);

    //Compensate for the 1.5 seconds in the delay, and 300ms for turning on my own RTC
    turnOnRTC(myRTCVccPin);
    time_t now = RTC.get() - 2;
    turnOffRTC(myRTCVccPin);

    while (myDriver->available())
    {
        // Now try to receive the message
        msg = receiveMessage(myDriver, RECEIVE_TIMEOUT);

        // If no joinAck message has been received
        if (msg == nullptr)
        {
            //Serial.println("no message");
            continue;
        }
        else if (msg->type != MESSAGE_JOIN_ACK)
        {
            //Serial.println("wrong message");
            delete msg;
            continue;
        }

        JoinAck *ack = (JoinAck *)msg;
        byte *nodeAddr = ack->srcAddr;

        ParentInfo candidate;
        memcpy(candidate.parentAddr, nodeAddr, 2);
        candidate.hopsToGateway = ack->hopsToGateway;
        candidate.numChildren = ack->numChildren;
        candidate.nextGatewayReqTime = ack->nextReqTime + now;

        /**
         * Usually the lower one will be the rssiFeedback since the new node is using smaller tx power
         * for broadcasting beacons
         */
        candidate.linkQuality = min(ack->rssi, ack->rssiFeedback);

        Serial.print(F("Parent Candidate: src=0x"));
        Serial.print(nodeAddr[0], HEX);
        Serial.print(nodeAddr[1], HEX);
        Serial.print(F(", Hops="));
        Serial.print(candidate.hopsToGateway);
        Serial.print(F(", # children="));
        Serial.print(candidate.numChildren);
        Serial.print(F(", Out RSSI="));
        Serial.print(ack->rssiFeedback);
        Serial.print(F(", In RSSI="));
        Serial.print(ack->rssi);
        Serial.print(F(", Link quality="));
        Serial.print(candidate.linkQuality);
        Serial.print(F(", Data collection at "));
        Serial.println(candidate.nextGatewayReqTime);

        if (candidate.linkQuality > MIN_LINK_QUALITY)
        {
            // Less hop is always prefered
            if (candidate.hopsToGateway == bestCandidate.hopsToGateway)
            {
                if (candidate.numChildren == bestCandidate.numChildren)
                {
                    // Final tiebreaker using the link quality
                    bestCandidate = (candidate.linkQuality > bestCandidate.linkQuality) ? candidate : bestCandidate;
                }
                else
                {
                    // First tiebreaker using the number of children
                    bestCandidate = (candidate.numChildren < bestCandidate.numChildren) ? candidate : bestCandidate;
                }
            }
            else
            {
                bestCandidate = (candidate.hopsToGateway < bestCandidate.hopsToGateway) ? candidate : bestCandidate;
            }
        }
        else if (m_txPwr == MAX_TX_PWR && bestCandidate.hopsToGateway == 255)
        {
            // Take whatever possible if we have reached the max Tx power and there is still no parent
            bestCandidate = candidate;
        }

        delete ack;
    }

    if (bestCandidate.hopsToGateway != 255)
    {
        // New parent has found
        Serial.print(F("Parent: 0x"));
        Serial.print(bestCandidate.parentAddr[0], HEX);
        Serial.println(bestCandidate.parentAddr[1], HEX);

        myParent = bestCandidate;

        hopsToGateway = myParent.hopsToGateway + 1;

        Serial.println(F("Send JoinCFM to parent"));
        // Send a confirmation to the parent node
        
        JoinCFM cfm(myAddr);
        sendMessage(myDriver,myParent.parentAddr, &cfm);

        //Reset the child list if there is any
        ChildNode *iter = childrenList;
        while (iter != nullptr)
        {
            ChildNode *temp = iter;

            if(iter->reply != nullptr){
                delete iter->reply;
            }
            iter = temp->next;
            delete temp;
        }

        childrenList = nullptr;
        numChildren = 0;
        numOutgoingJoinAcks = 0;

        return true;
    }
    else
    {
        return false;
    }
}

void ForwardEngine::handleJoin(Join *join)
{
    if (state != CONNECTED && state != READY1)
    {
        Serial.println(F("Wrong state for join messages"));
        // Only process join messages during the connected phase and the ready phase
        return;
    }
    // If a join message comes from the parent node, it suggests that the parent node has
    // disconnected from the gateway, do not reply back with a JoinACK
    if (DeviceDriver::compareAddr(join->srcAddr, myParent.parentAddr))
    {
        Serial.println(F("Parent node has disconnected from the gateway"));
        return;
    }

    turnOnRTC(myRTCVccPin);
    time_t now = RTC.get();

    // Remove those expired outgoing joinAcks
    uint8_t numExpires = cleanChildrenList(now);
    numOutgoingJoinAcks -= numExpires;

    ChildNode* c = findChild(join->srcAddr, childrenList);
    if( c != nullptr){
        if(c->confirmed){
            c->confirmed = false;
            numChildren --;
        }
    }else{
        // Only send out joinAcks when there is sufficient capacity
        if (numOutgoingJoinAcks + numChildren >= MAX_NUM_CHILDREN)
        {
            Serial.println(F("No more capacity for accepting new children"));
            return;
        }
        c = new ChildNode();
        memcpy(c->nodeAddr, join->srcAddr, 2);
        c->confirmed = false;

        c->next = childrenList;
        // Insert the node at the beginning
        childrenList = c;
    }

    unsigned long backoff = random(0, MAX_JOIN_ACK_BACKOFF_TIME);

    // Use full power when replies back
    myDriver->setTxPwr(MAX_TX_PWR);
    myDriver->setFrequency(channelFrequency(DOWNLINK_CHANNEL));

    sleepForMillis(backoff);
    
    //turnOnRTC(myRTCVccPin);
    now = RTC.get();
    turnOffRTC(myRTCVccPin);
    
    time_t timeTillNextReq = myParent.nextGatewayReqTime - now + ((unsigned long)maxBackoffTime)/MILLISECOND_MULTIPLIER;
    JoinAck ack(myAddr, hopsToGateway, numChildren, join->rssi, timeTillNextReq);
    sendMessage(myDriver, join->srcAddr, &ack);

    // revert the power
    myDriver->setTxPwr(m_txPwr);

    // If the node does not send back a CFM 4 seconds after its approximate discovery timeout, it is removed
    c->joinAckExpiryTime = now + MAX_JOIN_ACK_BACKOFF_TIME / MILLISECOND_MULTIPLIER + 1;

    numOutgoingJoinAcks++;

    Serial.print(F("Send joinAck to a potential child: src=0x"));
    Serial.print(join->srcAddr[0], HEX);
    Serial.println(join->srcAddr[1], HEX);
}

void ForwardEngine::handleJoinCFM(JoinCFM *cfm)
{
    ChildNode *child = findChild(cfm->srcAddr, childrenList);

    if (child == nullptr)
    {
        /**
         * This node is not registered. It should not happen since every
         * potential child is registered when a joinack is sent. However, if the
         * potential child has delayed sending the CFM (for unknown reasons),
         * then its record might be expired and removed.
         */
        ChildNode *node = new ChildNode();
        memcpy(node->nodeAddr, cfm->srcAddr, 2);
        node->confirmed = true;

        node->next = childrenList;
        childrenList = node;
        numChildren++;
    }
    else
    {
        // We have resigtered this node.
        if (!child->confirmed)
        {
            child->confirmed = true;
            numChildren++;
            numOutgoingJoinAcks--;
        }
        else
        {
            Serial.println(F("An existing child seems to re-join"));
        }
    }

    Serial.print(F("A new child has connected: 0x"));
    Serial.print(cfm->srcAddr[0], HEX);
    Serial.println(cfm->srcAddr[1], HEX);

    turnOnRTC(myRTCVccPin);
    time_t currentTime = RTC.get();
    turnOffRTC(myRTCVccPin);

    /**
     * If for a node or gateway,
     * 1. the max capacity is reached, and
     * 2. the gateway request will take more than 10 seconds to arrive (send)
     * Go to sleep
     */
    if (state == CONNECTED && numChildren >= MAX_NUM_CHILDREN && myParent.nextGatewayReqTime - currentTime > 2 * EARLY_WAKE_UP_TIME)
    {
        state = HIBERNATE3;
    }
}

void ForwardEngine::handleReq(GatewayRequest *req)
{
    // GatewayReq is broadcasted, we should only accept REQ from the parent
    if (!DeviceDriver::compareAddr(req->srcAddr, myParent.parentAddr) && state != OBSERVE)
    {
        Serial.println(F("Req is not received from parent. Ignore."));
        return;
    }

    // first gateway req in the cycle
    // Usually the device should be in the READY state when receiving a request.
    // However, in case the request arrives quickily (<5 seconds) after the node joins the network
    if (state == READY1 || state == READY2 || state == CONNECTED || state == OBSERVE)
    {

        turnOnRTC(myRTCVccPin);
        time_t receivingPeriodStart = RTC.get();
        turnOffRTC(myRTCVccPin);

        if (req->newNextReqTime())
        {
            // Get the expected time for the next gateway request
            gatewayReqTime = req->nextReqTime;
            Serial.print(F("Next DCP will be in "));
            Serial.println(gatewayReqTime);
        }

        // Estimate the time when the next request will arrive
        myParent.nextGatewayReqTime = receivingPeriodStart + gatewayReqTime;

        // Wake up in the next cycle to join the network
        if (state == OBSERVE)
        {
            state = HIBERNATE1;
            return;
        }

        /*
         * For less frequent data gathering every 10 minutes or more (e.g.
         * every hours), only receive for 5 minutes. For more frequent data
         * gathering every 20 minutes or less (e.g. every 5 minutes), receive
         * for 50% of the request interval 
         * 
         * TODO: Make this time more dynamic based on previous iterations
         */
        if (gatewayReqTime < 600)
        {
            // Calculate the end of the receiving period
            receivingPeriod = gatewayReqTime / 2;
        }
        else
        {
            receivingPeriod = 300;
        }

        receivingPeriodTimeout = receivingPeriodStart + receivingPeriod;

        //Serial.print(F("Receiving period: "));
        //Serial.print(receivingPeriodStart);
        //Serial.print(F(" to "));
        //Serial.println(receivingPeriodTimeout);    

        myParent.channel = req->ulChannel;
        Serial.print(F("Switch to parent channel: "));
        Serial.print(myParent.channel, DEC);
        Serial.println();

        uint64_t freq = channelFrequency(myParent.channel);

        myDriver->setMode(STANDBY);
        myDriver->setFrequency(freq);

        if (req->newMaxBackoff())
        {
            maxBackoffTime = (uint16_t)req->childBackoffTime * 1E3 + MIN_BACKOFF_TIME;
            Serial.print(F("Max backoff: "));
            Serial.println(maxBackoffTime);
        }

        // backoff to avoid collision
        uint16_t backoff = random(MIN_BACKOFF_TIME, maxBackoffTime);
        Serial.print(F("First backoff: "));
        Serial.println(backoff);

        sleepForMillis(backoff);

        // Use callback to get node data
        byte* data = new byte[MAX_LEN_DATA_NODE_REPLY];
        uint8_t dataLen = 0;

        /**
         * For our test bed, we also add 2-byte parent address
         * TODO: Remove it for real-life application
         */
        memcpy(data, myParent.parentAddr, 2);
        byte* payload = data + 2;

        if (onRecvRequest)
        {
            // Here it might take time to fetch the sensor data that the random backoff delay is not
            // accounted for
            onRecvRequest(&payload, &dataLen);
        }

        /**
         * For our test bed, we also add 2-byte parent address
         * TODO: Remove it for real-life application
         */
        dataLen += 2;

        if(dataLen > 0 && dataLen <= MAX_LEN_DATA_NODE_REPLY){
            byte option = 0b0010000;
            // Send reply to the parent
            NodeReply nReply(myAddr, option, dataLen, data);
            sendMessage(myDriver, myParent.parentAddr, &nReply);

            Serial.println(F("Done uploading local data"));
        }else{
            Serial.println(F("Sensor data must be between 0 to 64 bytes"));
        }

        delete[] data;

        // Prepare for the request
        // No need to keep the RX on while waiting
        myDriver->setMode(STANDBY);

        if(state != READY2){
            // Siblings will finish transmitting after the maxBackoff, so it is better to
            // wait until all of them finished transmitting before forwarding the messages
            uint16_t remainingTime = maxBackoffTime - backoff;
            backoff = random(remainingTime, remainingTime + maxBackoffTime);

            Serial.print(F("Second backoff: "));
            Serial.println(backoff);

            sleepForMillis(backoff);
            m_useDLChannel = true;
        }else{
            /**
             * If we are at READY2, the request is already delayed, and the descendants might
             * have already switched to their READY2 state for receiving the request. Therefore,
             * the request should be sent using the node's own channel right away
             */
            m_useDLChannel = false;
        }

        state = TALK_TO_CHILDREN;
    }
    else if (state == LISTEN_TO_PARENT)
    {
        // This should never happen
        if (bufferSize == 0)
        {
            state = TALK_TO_CHILDREN;
            return;
        }

        myDriver->setMode(STANDBY);
        myDriver->setTxPwr(m_txPwr);

        uint16_t backoff = random(MIN_BACKOFF_TIME, maxBackoffTime);
        Serial.print(F("Backoff: "));
        Serial.println(backoff);
        sleepForMillis(backoff);

        uint8_t i = 0;
        byte payload[MAX_LEN_DATA_NODE_REPLY];

        byte option = 0b10100000;
        NodeReply* singleReply;

        ChildNode *child = childrenList;
        // This is similar to a FIFO queue. We always delete at the head.
        while (child != nullptr)
        {
            Serial.print(F("Child: "));
            Serial.print(child->nodeAddr[0], HEX);
            Serial.print(child->nodeAddr[1], HEX);
            Serial.println();

            NodeReply *reply = child->reply;

            if (reply == nullptr)
            {
                Serial.println(F("No reply received"));
                child = child->next;
                continue;
            }
            Serial.print(F("Reply data len: "));
            Serial.println(reply->dataLength);

            // Although this checking is for the case that the packet is not aggregated and
            // requires a 3-byte mini-header, it also works for the aggregated case.
            if (reply->aggregated())
            {
                if(i + reply->dataLength > MAX_LEN_DATA_NODE_REPLY){
                    break;
                }
                // If the packet is already aggregated, we can simply copy the data potion
                memcpy(payload + i, reply->data, reply->dataLength);
                i += reply->dataLength;
            }
            else
            {
                if (i + 3 + reply->dataLength > MAX_LEN_DATA_NODE_REPLY)
                {
                    /**
                     * TODO: A problem would occur if the reply data length is > 61 bytes.
                     * Such packet should be sent without the aggregation header.
                     */
                    if( i == 0 && reply->dataLength > MAX_LEN_DATA_NODE_REPLY - 3){
                        option ^= MASK_NODE_REPLY_AGGREGATED;
                        singleReply = new NodeReply(*reply);
                        
                        delete reply;
                        child->reply = nullptr;
                        bufferSize -= reply->dataLength;
                    }
                    break;           
                }
                //Create mini-packets
                memcpy(payload + i, reply->srcAddr, 2);
                payload[i + 2] = reply->dataLength;
                memcpy(payload + i + 3, reply->data, reply->dataLength);

                i += (3 + reply->dataLength);
            }
            bufferSize -= reply->dataLength;

            // Remove the reply from the link list;
            delete reply;
            child->reply = nullptr;

            child = child->next;
        }

        if (bufferSize == 0)
        {
            state = TALK_TO_CHILDREN;
        }
        else
        {
            state = LISTEN_TO_PARENT;
            option |= 0x40; // Tell the parent that there are more
        }

        if(option & MASK_NODE_REPLY_AGGREGATED){
            NodeReply aggregatedReply = NodeReply(myAddr, option, i, payload);
            sendMessage(myDriver, myParent.parentAddr, &aggregatedReply);
        }else{
            //Simply send the original packet to the parent node
            sendMessage(myDriver, myParent.parentAddr, singleReply);
            delete singleReply;
        }

        Serial.println(F("Done uploading non-local data"));
    }
}

void ForwardEngine::handleReply(NodeReply *reply)
{
    if (state != TALK_TO_CHILDREN)
    {
        return;
    }

    if (bufferSize + reply->dataLength + 3 > AGGREGATION_BUFFER_SIZE)
    {
        Serial.println(F("NodeReply: Buffer is full. Packet dropped."));
        return;
    }

    //Serial.print(F("Data received from node "));
    //Serial.print(reply->srcAddr[0], HEX);
    //Serial.print(reply->srcAddr[1], HEX);
    //Serial.println();

    ChildNode* child = findChild(reply->srcAddr, childrenList);

    if(child == nullptr){
        /**
         *  TODO: A good question is whether we accept packet from a non-child who clearly knows me 
         */
        Serial.print(reply->srcAddr[0]);
        Serial.print(reply->srcAddr[1]);
        Serial.println(F(" is now added to the children list"));

        //A child that we did not put in the list
        ChildNode* newChild = new ChildNode();
        memcpy(newChild->nodeAddr, reply->srcAddr, 2);
        newChild->confirmed = true;
        newChild->reply = new NodeReply(*reply);
        
        //Insert at the begining.
        newChild->next = childrenList;
        childrenList = newChild;
        numChildren ++;
    }else{
        child->reply = new NodeReply(*reply);
    }

    bufferSize += reply->dataLength;

    return;
}

bool ForwardEngine::runGateway()
{
    state = CONNECTED;

    // Set the alarm for the next data collection cycle
    turnOnRTC(myRTCVccPin);
    time_t now = RTC.get();
    //Serial.println(now);
    //Serial.println(gatewayReqTime);
    myParent.nextGatewayReqTime = now + gatewayReqTime;
    setAlarm(myParent.nextGatewayReqTime);
    turnOffRTC(myRTCVccPin);

    Serial.print(F("First DCP after "));
    Serial.println(gatewayReqTime);

    // Gateway has the cost of 0
    hopsToGateway = 0;
    
    m_channel = (uint8_t)random(0, NUM_UL_CHANNELS);

    while (true)
    {
        if(DEBUG_ENABLE){
            Serial.print(F("Free memory: "));
            Serial.println(freeMemory());
        }
        
        turnOnRTC(myRTCVccPin);
        now = RTC.get();
        turnOffRTC(myRTCVccPin);

        // A quick check to see if the receiving period has ended
        if (state == TALK_TO_CHILDREN || state == LISTEN_TO_PARENT)
        {
            if (now >= receivingPeriodTimeout)
            {
                state = HIBERNATE3;
            }
        }

        switch (state)
        {
        case CONNECTED:
        {
            // Callback for initializing Gateway
            if(onPreDataCollection) {
                onPreDataCollection();
            }

            receiveUntillInterrupt();
            break;
        }
        case READY1:
        {
            Serial.println(F("Data collection starts"));

            // Update the time for the next iteration
            myParent.nextGatewayReqTime = now + gatewayReqTime;

            if (gatewayReqTime < 600)
            {
                // Calculate the end of the receiving period
                receivingPeriod = gatewayReqTime / 2;
            }
            else
            {
                receivingPeriod = 300;
            }

            receivingPeriodTimeout = now + receivingPeriod;
            m_useDLChannel = true;
            // For gateway, it does not need to wait for the request, it should issue request immediately
            state = TALK_TO_CHILDREN;
            continue;
        }
        case TALK_TO_CHILDREN:
        {
            Serial.println(F("Talking to children"));
            talkToChildren();
            break;
        }

        case HIBERNATE2:
        {
            hibernationCounter++;
            if (hibernationCounter == 5)
            {
                state = HIBERNATE3;
                continue;
            }

            Serial.print(F("Hibernate untill the next request after "));
            Serial.println(requestInterval);

            if(!hibernate(now + requestInterval)){
                delay(requestInterval * MILLISECOND_MULTIPLIER);
                state = TALK_TO_CHILDREN;
            }

            break;
        }
        case HIBERNATE3:
        {
            //TODO: Turn off the Gateway
            if (onPostDataCollection){
                onPostDataCollection();
            }
            hibernationCounter = 0;

            //Clean up the data if there are any
            ChildNode *child = childrenList;
            while (child != nullptr)
            {
                if (child->reply != nullptr)
                {
                    delete child->reply;
                    child->reply = nullptr;
                }
                child = child->next;
            }
            bufferSize = 0;

            time_t timeout = myParent.nextGatewayReqTime - EARLY_WAKE_UP_TIME;

            Serial.print(F("Hibernate untill the next DCP after "));
            Serial.println(timeout - now);

            if(!hibernate(timeout)){
                Serial.println(F("ERROR: The gateway has to reset the collection cycle due to RTC errors"));
                state = READY1;
                break;
            }

            turnOnRTC(myRTCVccPin);
            setAlarm(myParent.nextGatewayReqTime);
            turnOffRTC(myRTCVccPin);

            /** Unlike a node, the gateway should not go to the READY state immediately
            * because the READY state initiates the DCP. It should stay for a few seconds in case
            * some new nodes wants to join in
            */
            state = CONNECTED;
            break;
        }
        case LISTEN_TO_PARENT:
        {
            //reset the counter
            hibernationCounter = 0;
            bool fetchMore = false;
            // For gateway, it processes data here
            ChildNode *child = childrenList;
            while (child != nullptr)
            {
                NodeReply* reply = child->reply;
                if (reply != nullptr)
                {
                    Serial.print(F("Processing packet from "));
                    Serial.print(reply->srcAddr[0], HEX);
                    Serial.print(reply->srcAddr[1], HEX);
                    Serial.print(": ");

                    //Currently it simply print the data (maybe aggregated)
                    for(uint8_t i = 0; i < reply->dataLength; i ++){
                        Serial.print(reply->data[i], HEX);
                        Serial.print('-');
                    }
                    Serial.println();

                    if(!fetchMore && reply->fetchMore()){
                        fetchMore = true;
                    }

                    if(onRecvResponse){
                        if(reply->aggregated()){
                            //Break down the aggregated packet into smaller packets

                            uint8_t bytesRead = 0;
                            
                            while(bytesRead + 3 <= reply->dataLength){

                                //Parse the mini header
                                byte* srcAddrPtr = reply->data + bytesRead;
                                bytesRead += 2;

                                uint8_t datalen = reply->data[bytesRead];
                                bytesRead += 1;

                                if(bytesRead + datalen > reply->dataLength){
                                    Serial.println("Warning: Mismatched data length");
                                    break;
                                }

                                byte* dataPtr = reply->data + bytesRead;

                                onRecvResponse(dataPtr, datalen, srcAddrPtr);

                                bytesRead += datalen;
                            }
                        }else{
                            onRecvResponse(reply->data, reply->dataLength, reply->srcAddr);
                        }
                    }


                    delete reply;
                    child->reply = nullptr;
                }
                child = child->next;
            }
            bufferSize = 0;

            if(fetchMore){
                state = TALK_TO_CHILDREN;
            }else{
                state = HIBERNATE2;
            }
            break;
        }
        default:
        {
            break;
        }
        }

        if (myDriver->available() == 0)
        {
            // No need to receive anything if there is nothing to read
            continue;
        }

        GenericMessage *msg = receiveMessage(myDriver, RECEIVE_TIMEOUT);

        // Receive failed (e.g. unrecognizable packet format)
        if (msg == nullptr)
        {
            continue;
        }

        // Based on the received message, do the corresponding actions
        switch (msg->type)
        {
        case MESSAGE_JOIN:
        {
            handleJoin((Join *)msg);
            break;
        }
        case MESSAGE_JOIN_CFM:
        {
            handleJoinCFM((JoinCFM *)msg);
            break;
        }
        case MESSAGE_NODE_REPLY:
        {
            handleReply((NodeReply *)msg);
            break;
        }
        default:
        {
            break;
        }
        }

        delete msg;
    }
}

bool ForwardEngine::runNode()
{
    GenericMessage *msg = nullptr;

    // Uninitilized gateway cost
    hopsToGateway = 255;

    state = OBSERVE;
    m_maxJoinAttempts = SELF_HEAL_MAX_JOIN_ATTEMPTS;

    // The core network operations are carried out here
    while (true)
    {
        turnOnRTC(myRTCVccPin);
        time_t now = RTC.get();
        turnOffRTC(myRTCVccPin);

        // A quick check to see if the receiving period has ended
        if (state == TALK_TO_CHILDREN ||  state == LISTEN_TO_PARENT)
        {
            if (now >= receivingPeriodTimeout)
            {
                state = HIBERNATE3;
            }
        }

        if(DEBUG_ENABLE){
            Serial.print(F("Free memory: "));
            Serial.println(freeMemory());
        }

        switch (state)
        {
        case DISCONNECTED:
        {
            m_joinAttempts++;
            Serial.print(F("Current TX power at "));
            Serial.println(m_txPwr);
            if (join())
            {
                Serial.println(F("Joining successful"));
                state = CONNECTED;

                // Reset join attempts
                m_joinAttempts = 0;

                m_channel = (uint8_t)random(0, NUM_UL_CHANNELS);

                turnOnRTC(myRTCVccPin);

                time_t readyTime = myParent.nextGatewayReqTime - EARLY_WAKE_UP_TIME;
                // Set the alarm if there is plenty of time before the next data collection phase
                if (readyTime > RTC.get())
                {
                    Serial.print(F("Node will be ready at "));
                    Serial.print(readyTime);
                    Serial.println();
                    setAlarm(readyTime);
                }
                else
                {
                    // If we do not have too much time left before the data collection,
                    // Go to ready state
                    state = READY1;
                }
                turnOffRTC(myRTCVccPin);
            }
            else
            {
                if (m_joinAttempts < m_maxJoinAttempts)
                {
                    myDriver->setMode(SLEEP);
                    if (m_txPwr < MAX_TX_PWR)
                    {
                        // Increment the tx power
                        m_txPwr++;
                    }

                    /**
                     * Use truncated exponential random backoffs
                     */
                    unsigned long maxDelay = ((unsigned long)1 << m_joinAttempts) * MILLISECOND_MULTIPLIER + 1E3;

                    if (maxDelay > DEFAULT_MAX_JOIN_BACKOFF)
                    {
                        maxDelay = DEFAULT_MAX_JOIN_BACKOFF;
                    }

                    unsigned long delayTime = random(1E3, maxDelay);

                    Serial.print(F("Joining unsuccessful. Retry joining in "));
                    Serial.print(delayTime / MILLISECOND_MULTIPLIER);
                    Serial.println(F(" seconds"));

                    sleepForMillis(delayTime);
                    continue;
                }
                else
                {
                    // if a node enters observe state, it has to catch the
                    // few seconds before a data collection cycle begins.
                    // Therefore, it has less chances.
                    m_maxJoinAttempts = SELF_HEAL_MAX_JOIN_ATTEMPTS;
                    m_joinAttempts = 0;

                    state = OBSERVE;
                    Serial.println(F("Start the OBSERVE mode"));
                    continue;
                }
            }
            break;
        }
        case CONNECTED:
        case OBSERVE:{
            receiveUntillInterrupt();
            break;
        }
        case READY1:
        case READY2:
        {
            if (!alarmSetForReceiving){
                    turnOnRTC(myRTCVccPin);
                    //TODO: Justify this time 15 seconds
                    time_t timeout = RTC.get();
                    if(state == READY1){
                        timeout += 15;
                    }else{
                        //In the READY2 state, we will listen for the old receiving period
                        timeout += receivingPeriod;
                    }
                    setAlarm(timeout);
                    alarmSetForReceiving = true;
                    turnOffRTC(myRTCVccPin);
                }
            receiveUntillInterrupt();
            break;
        }
        case LISTEN_TO_PARENT:
        {
            Serial.println(F("Listen to parent"));

            // reset the counter
            hibernationCounter = 0;

            if (receivingPeriodTimeout <= now + 3)
            {
                state = HIBERNATE3;
                continue;
            }
            else if (!alarmSetForReceiving)
            {
                /** Need to have a timeout since if the parent is dead, the node
                 * will stuck in this state permanently
                 */
                turnOnRTC(myRTCVccPin);
                setAlarm(receivingPeriodTimeout);
                alarmSetForReceiving = true;
                turnOffRTC(myRTCVccPin);

                /**
                 * If the parent successfully fetches all the data within
                 * the receivingPeriodTimeout, the alarm above will be replaced
                 * by a new alarm in TALK_TO_CHILDREN
                 */
            }
            receiveUntillInterrupt();
            break;
        }

        case TALK_TO_CHILDREN:
        {
            Serial.println(F("Talking to children"));
            talkToChildren();
            continue;
        }

        case HIBERNATE2:
        {
            hibernationCounter++;
            if (hibernationCounter >= 5)
            {
                state = HIBERNATE3;
                continue;
            }
            if(!hibernate(now + requestInterval)){
                delay(requestInterval * MILLISECOND_MULTIPLIER);
                state = TALK_TO_CHILDREN;
            }
            break;
        }

        case HIBERNATE1:
        {
            time_t timeout = myParent.nextGatewayReqTime - EARLY_WAKE_UP_TIME + (time_t)MAX_RTC_READ_ERROR_SECOND;
            Serial.print(F("Join at the next DCP after "));
            Serial.println(timeout - now);

            if(!hibernate(timeout)){
                //We can simply use the gatewayReq time here since there is no delay from OBSERVE to HIBERNATE1
                delay((gatewayReqTime - EARLY_WAKE_UP_TIME) * MILLISECOND_MULTIPLIER);
                state = DISCONNECTED;
            }
            continue;
        }
        case HIBERNATE3:
        {

            // Clean up the buffer (the parent might fail to fetch them)
            ChildNode *child = childrenList;
            while (child != nullptr)
            {
                if (child->reply != nullptr)
                {
                    delete child->reply;
                    child->reply = nullptr;
                }
                child = child->next;
            }
            bufferSize = 0;

            // Reset the parameters
            alarmSetForReceiving = false;
            hibernationCounter = 0;

            time_t timeout = myParent.nextGatewayReqTime - EARLY_WAKE_UP_TIME;
            Serial.print(F("Hibernate untill the next DCP after "));
            Serial.println(timeout - now);

            if(!hibernate(timeout)){
                rtcError = true;
                //If the RTC fails, the node loses synchronization with the gateway 
                // and should enter self-healing
                state = OBSERVE;
            }
            break;
        }
        }

        if (myDriver->available() == 0)
        {
            // No need to receive anything if there is nothing to read
            continue;
        }

        msg = receiveMessage(myDriver, RECEIVE_TIMEOUT);

        // Receive failed (e.g. unrecognizable packet format)
        if (msg == nullptr)
        {
            continue;
        }

        // Based on the received message, do the corresponding actions
        switch (msg->type)
        {
        case MESSAGE_JOIN:
        {
            handleJoin((Join *)msg);
            break;
        }
        case MESSAGE_JOIN_CFM:
        {
            handleJoinCFM((JoinCFM *)msg);
            break;
        }
        case MESSAGE_GATEWAY_REQ:
        {
            handleReq((GatewayRequest *)msg);
            break;
        }
        case MESSAGE_NODE_REPLY:
        {
            handleReply((NodeReply *)msg);
            break;
        }
        default:
        {
            break;
        }
        }

        delete msg;
    }
}

bool ForwardEngine::run()
{
    if (myAddr[0] & GATEWAY_ADDRESS_MASK)
    {
        return runGateway();
    }
    else
    {
        return runNode();
    }
}

void ForwardEngine::setSleepMode(uint8_t sleepMode, uint8_t rtcInterruptPin, uint8_t rtcVccPin)
{

    if (sleepMode == SleepMode::SLEEP_RTC_INTERRUPT)
    {
        myRTCVccPin = rtcVccPin;
        pinMode(myRTCVccPin, OUTPUT);

        // Turn on the RTC
        digitalWrite(myRTCVccPin, HIGH);
        sleepForMillis(1000);

        time_t now = RTC.get();
        // If the user intends to use RTC-based interrupt/sleep mode
        // Make sure that a RTC is properly connected to the microcontroller
        if (now == 0)
        {
            Serial.println(F("Error: Unable to set RTC-based interrupt. I2C error with the RTC."));
            return;
        }

        if (translateInterruptPin(rtcInterruptPin) == NOT_AN_INTERRUPT)
        {
            Serial.println(F("Error: RTC interrupt (SQW) has to be connected to a valid interrupt pin"));
            return;
        }

        myRTCInterruptPin = rtcInterruptPin;

        RTC.set(compileTime());
        // Initialize the RTC module with Alarm1
        RTC.alarm(ALARM_1);
        RTC.squareWave(SQWAVE_NONE);

        now = RTC.get();
        //Set a 24-hour alarm to erase any old alarms on the RTC
        setAlarm(now + DEFAULT_NEXT_GATEWAY_REQ_TIME);

        RTC.alarmInterrupt(ALARM_1, true);
        RTC.alarmInterrupt(ALARM_2, false);
        pinMode(myRTCInterruptPin, INPUT_PULLUP);
        attachInterrupt(translateInterruptPin(myRTCInterruptPin), wake, FALLING);

        turnOffRTC(myRTCVccPin);
    }

    this->sleepMode = sleepMode;
    Serial.print(F("Sleep Mode set to: "));
    Serial.println(sleepMode);
}

ChildNode *findChild(byte *addr, ChildNode *start)
{
    ChildNode *iter = start;

    while (iter != nullptr)
    {
        if (DeviceDriver::compareAddr(iter->nodeAddr, addr))
        {
            break;
        }
        iter = iter->next;
    }

    return iter;
}

uint8_t ForwardEngine::cleanChildrenList(time_t currentTime)
{
    uint8_t childrenRemoved = 0;
    // Do a quick scan of the children list and clean up expired joinAcks
    if (childrenList == nullptr)
    {
        return 0;
    }

    ChildNode *iter = childrenList;
    ChildNode *prev = childrenList;

    Serial.print(F("Current time: "));
    Serial.println(currentTime);

    while (iter != nullptr)
    {
        Serial.print(F("Node "));
        Serial.print(iter->nodeAddr[0], HEX);
        Serial.print(iter->nodeAddr[1], HEX);
        Serial.print(F(", Confirmed: "));
        Serial.print(iter->confirmed);
        Serial.print(F(", Expiry time: "));
        Serial.println(iter->joinAckExpiryTime);
        // Remove the child that has an expired joinAck
        if (!iter->confirmed && iter->joinAckExpiryTime < currentTime)
        {

            Serial.println(F("Removing expired joinAck"));

            if(iter->reply != nullptr){
                delete iter->reply;
                iter->reply = nullptr;
            }

            if (iter == childrenList)
            {
                childrenList = iter->next;
                delete iter;
                iter = childrenList;
                prev = childrenList;
            }
            else
            {
                ChildNode *temp = iter->next;
                delete iter;
                prev->next = temp;
                iter = temp;
            }

            childrenRemoved++;
        }
        else
        {
            prev = iter;
            iter = iter->next;
        }
    }

    return childrenRemoved;
}

void ForwardEngine::talkToChildren()
{
    /**
     * Listen for only 1 second if there are 0 child (in case there is an unknown child). 
     * Otherwise use the adaptive backoff time
     */
    uint8_t maxChildBackoffTime = (numChildren == 0 ) ? 1 : numChildren * MAX_BACKOFF_TIME_FOR_ONE_CHILD ;

    myDriver->setMode(STANDBY);
    
    uint8_t channelToUse = m_channel;
    if (m_useDLChannel)
    {
        channelToUse = DOWNLINK_CHANNEL;
        m_useDLChannel = false;
    }
    Serial.print(F("Using channel: "));
    Serial.print(channelToUse, DEC);
    Serial.println();

    myDriver->setFrequency(channelFrequency(channelToUse));
    myDriver->setTxPwr(MAX_TX_PWR);

    turnOnRTC(myRTCVccPin);
    time_t now = RTC.get();

    byte queryType = 0b10000;
    // We simply broadcast the gatewayReq
    GatewayRequest gwReq(myAddr, queryType, m_channel, myParent.nextGatewayReqTime - now + ((unsigned long)maxBackoffTime)/MILLISECOND_MULTIPLIER, maxChildBackoffTime);

    sendMessage(myDriver, BROADCAST_ADDR, &gwReq);

    Serial.println(F("Request sent"));

    /** Set up the aggregation timeout (we can guarantee that
     * all children should have replied back). Add a 2-second margin
     * since 
     * 1. the child could send it at the last second with an air time of hundreds of miliseconds
     * 2. the resolution of the RTC is in seconds, therefore, we can miss ~0.99 seconds when reading it
     * 3. the child needs 300ms for turning on the RTC
     */ 
    now = RTC.get();
    time_t timeout = now + (time_t)(maxChildBackoffTime + 2);

    myDriver->setFrequency(channelFrequency(m_channel));
    myDriver->setMode(RX);

    if(timeout < compileTime() || RTC.oscStopped(true)){
        turnOffRTC(myRTCVccPin);
        //If RTC error occurs, we manually wait for the time period and parse the messages
        //after the timeout. The state is also switched manually.
        rtcError = true;

        delay((unsigned long)(maxChildBackoffTime + 2) * MILLISECOND_MULTIPLIER);
    }else{

        setAlarm(timeout);

        turnOffRTC(myRTCVccPin);
    
        while (true)
        {
           myDriver->powerDownMCU();

           turnOnRTC(myRTCVccPin);
           if(RTC.alarm(ALARM_1)){
               break;
           }
           turnOffRTC(myRTCVccPin);
        }
        turnOffRTC(myRTCVccPin);
    }
    Serial.println(F("End talking to children"));
    while(myDriver->available() > 0){
            Serial.println(F("Some data received"));
            GenericMessage* msg = receiveMessage(myDriver, RECEIVE_TIMEOUT); 

            if(msg == nullptr){
                continue;
            }

            if(msg->type == MESSAGE_NODE_REPLY){
                handleReply((NodeReply*)msg);
            }
            
            delete msg;
    }

    state = (bufferSize > 0) ? LISTEN_TO_PARENT : HIBERNATE2;
    return;
}

bool ForwardEngine::hibernate(time_t hibernationEnd)
{
    // Turn off the transceiver
    myDriver->setMode(SLEEP);

    turnOnRTC(myRTCVccPin);
    //Safe guard: in case the RTC.get() returns 0 due to errors, the alarm will be set to a point in the past and the MCU never wakes up
    if(hibernationEnd < compileTime() || hibernationEnd <= RTC.get() || RTC.oscStopped(true)){
      Serial.println(F("Error: RTC invalid alarm"));
      turnOffRTC(myRTCVccPin);
      rtcError = true;
      return false;
    }

    setAlarm(hibernationEnd);
    turnOffRTC(myRTCVccPin);

    deepSleep();

    turnOnRTC(myRTCVccPin);
    RTC.alarm(ALARM_1);
    turnOffRTC(myRTCVccPin);
    return true;
}

void ForwardEngine::receiveUntillInterrupt()
{
    uint8_t channelToUse = DOWNLINK_CHANNEL;
    switch(state){
        case CONNECTED:
        case READY1:
        case OBSERVE:
        {
            channelToUse = DOWNLINK_CHANNEL;
            break;
        }
        case LISTEN_TO_PARENT:
        case READY2:
        {
            channelToUse = myParent.channel;
            break;
        }
        default:
        {
            break;
        }
    }
    myDriver->setFrequency(channelFrequency(channelToUse));
    myDriver->setMode(RX);

    // Do not want available() to change during our checking
    noInterrupts();

    if (myDriver->available() == 0)
    {
        //Serial.println(F("Put MCU to sleep"));

        // Put the MCU to sleep and set the interrupt handler
        myDriver->powerDownMCU();

        turnOnRTC(myRTCVccPin);
        Serial.print(F("MCU wakes up due to "));
        if (RTC.alarm(ALARM_1))
        {
            Serial.println(F("alarm"));
        }
        else
        {
            Serial.println(F("packet"));
        }
        turnOffRTC(myRTCVccPin);
    }
    else
    {
        interrupts();
    }
}