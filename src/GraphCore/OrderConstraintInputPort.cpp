//
// Created by Christopher Yarp on 10/16/18.
//

#include "OrderConstraintInputPort.h"

#include "Node.h"

OrderConstraintInputPort::OrderConstraintInputPort() {

}

OrderConstraintInputPort::OrderConstraintInputPort(Node *parent) : InputPort(parent, -1){

}

void OrderConstraintInputPort::validate() {
    //Do nothing
    //This port allows multiple drivers
    //This port is allowed to be disconnected
}

std::shared_ptr<OrderConstraintInputPort> OrderConstraintInputPort::getSharedPointerOrderConstraintPort() {
    if(parent != nullptr) {
        return std::shared_ptr<OrderConstraintInputPort>(parent->getSharedPointer(), this);
    }
    else {
        //Hopefully should not happen but just in case
        //return std::shared_ptr<Port>(nullptr);
        throw std::runtime_error("Pointer requested from port that has no parent");
    }
}

bool OrderConstraintInputPort::hasInternalFanout(bool imag) {
    return false;
}
