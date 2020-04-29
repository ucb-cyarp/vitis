//
// Created by Christopher Yarp on 4/20/20.
//

#include "RateChange.h"
#include "General/ErrorHelpers.h"

RateChange::RateChange() : clockDomain(nullptr) {

}

RateChange::RateChange(std::shared_ptr<SubSystem> parent) : Node(parent), clockDomain(nullptr) {

}

RateChange::RateChange(std::shared_ptr<SubSystem> parent, RateChange *orig) : Node(parent, orig) {

}

void RateChange::removeKnownReferences() {
    Node::removeKnownReferences();

    //Also, remove from ClockDomain if reference set
    if(clockDomain){
        std::shared_ptr<RateChange> this_cast = std::static_pointer_cast<RateChange>(getSharedPointer());
        clockDomain->removeRateChange(this_cast);
    }
}

void RateChange::validate(){
    Node::validate();

    if(inputPorts.size() != 1){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - RateChange - Should Have Exactly 1 Input Port", getSharedPointer()));
    }

    if(outputPorts.size() != 1){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - RateChange - Should Have 1 or More Output Ports", getSharedPointer()));
    }

    //Check that this node has exactly one input and 1 or more outputs
}

bool RateChange::canExpand() {
    return false;
}
