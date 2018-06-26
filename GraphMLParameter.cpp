//
// Created by Christopher Yarp on 6/26/18.
//

#include "GraphMLParameter.h"

std::string GraphMLParameter::getKey() const{
    return key;
}

void GraphMLParameter::setKey(const std::string &key) {
    GraphMLParameter::key = key;
}
