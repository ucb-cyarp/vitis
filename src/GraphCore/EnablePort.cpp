//
// Created by Christopher Yarp on 7/17/18.
//

#include "EnablePort.h"

#include "DataType.h"
#include "Node.h"
#include "Arc.h"

EnablePort::EnablePort() {

}

EnablePort::EnablePort(Node *parent, int portNum) : InputPort(parent, portNum) {

}

void EnablePort::validate() {
    InputPort::validate(); //Validate input has 1 and only 1 connected arc

    //Check the datatype of the input
    DataType inputType = (*(arcs.begin())).lock()->getDataType();

    if(!inputType.isBool() || inputType.getWidth() != 1){
        throw std::runtime_error("Validation Failed - Enable Port Not Boolean");
    }
}

std::shared_ptr<EnablePort> EnablePort::getSharedPointerEnablePort() {
    if(parent != nullptr) {
        return std::shared_ptr<EnablePort>(parent->getSharedPointer(), this);
    }
    else {
        //Hopefully should not happen but just in case
        //return std::shared_ptr<Port>(nullptr);
        throw std::runtime_error("Pointer requested from port that has no parent");
    }
}