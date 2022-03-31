/**
 * A bunch of channels (center frequencies) used by LoRaWAN for uplink in North America
 * Note that this channel list consists 8 channels from the uplink subband 2
 * Link: https://www.thethingsnetwork.org/docs/lorawan/frequency-plans/
 */ 

#ifndef HEADER_NA_UL_CHANNEL_LIST
#define HEADER_NA_UL_CHANNEL_LIST

#define CHANNEL_LOWER_BOUND 9024E5
#define CHANNEL_INTERVALS 2E5

#define NUM_CHANNELS 64
#define NUM_UL_CHANNELS 63

#define DOWNLINK_CHANNEL 63

#include <Arduino.h>

uint64_t channelFrequency(uint8_t channelIndex);

#endif