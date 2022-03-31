#include <Arduino.h>
#include "FreqPlanNA.h"


uint64_t channelFrequency(uint8_t channelIndex){
    if(channelIndex >= NUM_CHANNELS){
        return 0;
    }
    return CHANNEL_LOWER_BOUND + CHANNEL_INTERVALS * channelIndex;
}