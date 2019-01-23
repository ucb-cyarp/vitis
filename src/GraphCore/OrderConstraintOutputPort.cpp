//
// Created by Christopher Yarp on 10/27/18.
//

#include "OrderConstraintOutputPort.h"

#include "Node.h"

OrderConstraintOutputPort::OrderConstraintOutputPort() {

}

OrderConstraintOutputPort::OrderConstraintOutputPort(Node *parent) : OutputPort(parent, 0){

}

void OrderConstraintOutputPort::validate() {
    //Do nothing
    //This port is allowed to be disconnected
}

std::shared_ptr<OrderConstraintOutputPort> OrderConstraintOutputPort::getSharedPointerOrderConstraintPort() {
    if(parent != nullptr) {
        return std::shared_ptr<OrderConstraintOutputPort>(parent->getSharedPointer(), this);
    }
    else {
        //Hopefully should not happen but just in case
        //return std::shared_ptr<Port>(nullptr);
        throw std::runtime_error("Pointer requested from port that has no parent");
    }
}