//
// Created by Christopher Yarp on 9/17/19.
//

#include "ThreadCrossingFIFOParameters.h"
#include "ErrorHelpers.h"

ThreadCrossingFIFOParameters::ThreadCrossingFIFOType
ThreadCrossingFIFOParameters::parseThreadCrossingFIFOType(std::string str) {
    if(str == "LOCKLESS_X86" || str == "lockeless_x86"){
        return ThreadCrossingFIFOType::LOCKLESS_X86;
    }else{
        throw std::runtime_error(ErrorHelpers::genErrorStr("Unable to parse ThreadCrossingFIFOType " + str));
    }

}

std::string
ThreadCrossingFIFOParameters::threadCrossingFIFOTypeToString(ThreadCrossingFIFOParameters::ThreadCrossingFIFOType threadCrossingFifoType) {
    switch(threadCrossingFifoType) {
        case ThreadCrossingFIFOType::LOCKLESS_X86:
            return "LOCKLESS_X86";
        default:
            throw std::runtime_error(ErrorHelpers::genErrorStr("Unknown ThreadCrossingFIFOType"));
    }
}
