//
// Created by Christopher Yarp on 6/26/18.
//

#include "Arc.h"

std::shared_ptr<Port> Arc::getSrcPort() const {
    return srcPort;
}

void Arc::setSrcPort(const std::shared_ptr<Port> &srcPort) {
    Arc::srcPort = srcPort;
}

std::shared_ptr<Port> Arc::getDstPort() const {
    return dstPort;
}

void Arc::setDstPort(const std::shared_ptr<Port> &dstPort) {
    Arc::dstPort = dstPort;
}

DataType Arc::getDataType() const {
    return dataType;
}

void Arc::setDataType(const DataType &dataType) {
    Arc::dataType = dataType;
}

double Arc::getSampleTime() const {
    return sampleTime;
}

void Arc::setSampleTime(double sampleTime) {
    Arc::sampleTime = sampleTime;
}

int Arc::getDelay() const {
    return delay;
}

void Arc::setDelay(int delay) {
    Arc::delay = delay;
}

int Arc::getSlack() const {
    return slack;
}

void Arc::setSlack(int slack) {
    Arc::slack = slack;
}
