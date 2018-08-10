//
// Created by Christopher Yarp on 7/17/18.
//

#include "InputPort.h"
#include "Node.h"

InputPort::InputPort() {

}

InputPort::InputPort(Node *parent, int portNum) : Port(parent, portNum) {

}

void InputPort::validate() {
    if(arcs.size() < 1){
        throw std::runtime_error("Validation Failed - Input Port Unconnected");
    }else if(arcs.size() > 1){
        throw std::runtime_error("Validation Failed - Input Port Has Multiple Drivers");
    }

    //Otherwise, port validated
}

std::shared_ptr<InputPort> InputPort::getSharedPointerInputPort() {
    if(parent != nullptr) {
        return std::shared_ptr<InputPort>(parent->getSharedPointer(), this);
    }
    else {
        //Hopefully should not happen but just in case
        //return std::shared_ptr<Port>(nullptr);
        throw std::runtime_error("Pointer requested from port that has no parent");
    }
}

std::shared_ptr<OutputPort> InputPort::getSrcOutputPort() {
    return arcs.begin()->lock()->getSrcPort();
}
