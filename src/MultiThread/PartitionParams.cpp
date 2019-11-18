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
