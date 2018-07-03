//
// Created by Christopher Yarp on 6/26/18.
//

#include "Arc.h"
#include "Port.h"

Arc::Arc() {
    srcPort = std::shared_ptr<Port>(nullptr);
    dstPort = std::shared_ptr<Port>(nullptr);
}

Arc::Arc(std::shared_ptr<Port> srcPort, std::shared_ptr<Port> dstPort, DataType dataType, double sampleTime) : srcPort(srcPort), dstPort(dstPort), dataType(dataType), sampleTime(sampleTime) {

}

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

void Arc::setSrcPortUpdateNewUpdatePrev(std::shared_ptr<Port> srcPort) {
    //Remove arc from old port if not null
    if(Arc::srcPort != nullptr)
    {
        Arc::srcPort->removeArc(shared_from_this());
    }

    //Set the src port to the one specified
    Arc::srcPort = srcPort;

    //Add this arc to the new src port
    srcPort->addArc(shared_from_this());
}

void Arc::setDstPortUpdateNewUpdatePrev(std::shared_ptr<Port> dstPort) {
    //Remove arc from old port if not null
    if(Arc::dstPort != nullptr)
    {
        Arc::dstPort->removeArc(shared_from_this());
    }

    //Set the dst port to the one specified
    Arc::dstPort = dstPort;

    //Add this arc to the new dst port
    dstPort->addArc(shared_from_this());
}


