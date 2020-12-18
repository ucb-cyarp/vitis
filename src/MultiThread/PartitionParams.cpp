//
// Created by Christopher Yarp on 9/24/19.
//

#include "PartitionParams.h"
#include <stdexcept>

PartitionParams::PartitionType PartitionParams::parsePartitionTypeStr(std::string str) {
    if(str == "MANUAL" || str == "manual"){
        return PartitionType::MANUAL;
    }else{
        throw std::runtime_error("Unable to parse partitioner: " + str);
    }
}

std::string PartitionParams::partitionTypeToString(PartitionParams::PartitionType partitionType) {
    if(partitionType == PartitionType::MANUAL) {
        return "MANUAL";
    }else{
        throw std::runtime_error("Unknown partitioner");
    }
}

PartitionParams::FIFOIndexCachingBehavior PartitionParams::parseFIFOIndexCachingBehavior(std::string str) {
    if(str == "NONE" || str == "none") {
        return FIFOIndexCachingBehavior::NONE;
    }else if (str == "PRODUCER_CONSUMER_CACHE" || str == "producer_consumer_cache"){
        return FIFOIndexCachingBehavior::PRODUCER_CONSUMER_CACHE;
    }else if(str == "PRODUCER_CACHE" || str == "producer_cache"){
        return FIFOIndexCachingBehavior::PRODUCER_CACHE;
    }else if(str == "CONSUMER_CACHE" || str == "consumer_cache"){
        return FIFOIndexCachingBehavior::CONSUMER_CACHE;
    }else{
        throw std::runtime_error("Unknown FIFO Index Caching Behavior");
    }
}

std::string
PartitionParams::fifoIndexCachingBehaviorToString(PartitionParams::FIFOIndexCachingBehavior fifoIndexCachingBehavior) {
    if(fifoIndexCachingBehavior == FIFOIndexCachingBehavior::NONE){
        return "NONE";
    }else if(fifoIndexCachingBehavior == FIFOIndexCachingBehavior::PRODUCER_CONSUMER_CACHE){
        return "PRODUCER_CONSUMER_CACHE";
    }else if(fifoIndexCachingBehavior == FIFOIndexCachingBehavior::PRODUCER_CACHE){
        return "PRODUCER_CACHE";
    }else if(fifoIndexCachingBehavior == FIFOIndexCachingBehavior::CONSUMER_CACHE){
        return "CONSUMER_CACHE";
    }else{
        throw std::runtime_error("Unknown FIFO Index Caching Behavior");
    }
}
