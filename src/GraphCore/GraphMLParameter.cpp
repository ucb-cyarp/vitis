//
// Created by Christopher Yarp on 6/26/18.
//

#include "GraphMLParameter.h"

GraphMLParameter::GraphMLParameter() : key(""), type(""), nodeParam(false) {

}

GraphMLParameter::GraphMLParameter(const std::string &key, const std::string &type, bool nodeParam) : key(key), type(type), nodeParam(nodeParam) {

}

std::string GraphMLParameter::getKey() const{
    return key;
}

void GraphMLParameter::setKey(const std::string &key) {
    GraphMLParameter::key = key;
}

bool GraphMLParameter::operator==(const GraphMLParameter &rhs) const {
    return key == rhs.key &&
           type == rhs.type &&
           nodeParam == rhs.nodeParam;
}

bool GraphMLParameter::operator!=(const GraphMLParameter &rhs) const {
    return !(rhs == *this);
}

bool GraphMLParameter::operator<(const GraphMLParameter &rhs) const {
    if (key < rhs.key)
        return true;
    if (rhs.key < key)
        return false;
    if (type < rhs.type)
        return true;
    if (rhs.type < type)
        return false;
    return nodeParam == false && rhs.nodeParam == true;
}

bool GraphMLParameter::operator>(const GraphMLParameter &rhs) const {
    return rhs < *this;
}

bool GraphMLParameter::operator<=(const GraphMLParameter &rhs) const {
    return !(rhs < *this);
}

bool GraphMLParameter::operator>=(const GraphMLParameter &rhs) const {
    return !(*this < rhs);
}

const std::string GraphMLParameter::getType() const {
    return type;
}

void GraphMLParameter::setType(const std::string &type) {
    GraphMLParameter::type = type;
}

bool GraphMLParameter::isNodeParam() const {
    return nodeParam;
}

void GraphMLParameter::setNodeParam(bool nodeParam) {
    GraphMLParameter::nodeParam = nodeParam;
}
