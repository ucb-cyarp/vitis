//
// Created by Christopher Yarp on 7/17/18.
//

#include "SelectPort.h"

#include "DataType.h"
#include "Arc.h"
#include "Node.h"

SelectPort::SelectPort() {

}

SelectPort::SelectPort(Node *parent, int portNum) : InputPort(parent, portNum) {

}

void SelectPort::validate() {
    InputPort::validate(); //Validate input has 1 and only 1 connected arc

    //Check the datatype of the input
    DataType inputType = (*(arcs.begin())).lock()->getDataType();

    if(inputType.isComplex() || inputType.isFloatingPt() || inputType.getWidth() != 1){
        throw std::runtime_error("Validation Failed - Select Port Not Integer Type");
    }
}

std::shared_ptr<SelectPort> SelectPort::getSharedPointerSelectPort() {
    if(parent != nullptr) {
        return std::shared_ptr<SelectPort>(parent->getSharedPointer(), this);
    }
    else {
        //Hopefully should not happen but just in case
        //return std::shared_ptr<Port>(nullptr);
        throw std::runtime_error("Pointer requested from port that has no parent");
    }
}
