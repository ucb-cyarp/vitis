//
// Created by Christopher Yarp on 2019-02-28.
//

#include "DSPHelpers.h"

unsigned long DSPHelpers::bin2Gray(unsigned long bin) {
    unsigned long gray = 0;

    //63rd bit
    gray = gray | (((bin >> 63)&1)<<63);

    for(long i = 62; i>=0; i--){
        gray = gray | ((((bin >> i)&1)^((bin >> (i+1))&1))<<i);
    }

    return gray;
}
