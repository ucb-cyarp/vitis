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

    if(!inputType.isBool() || !inputType.isScalar()){
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

bool EnablePort::hasInternalFanout(bool imag) {
    //TODO: Possibly look at being less conservative
    //Internal fanout only really occurs if the enable context is entered multiple times.  This is a decision made by
    //the scheduler and not here.  In case the context is entered multiple times, (ie. the enable line needs to be
    //checked multiple times) the safest solution is to declare it as having internal fanout to force the assignment
    //of a temporary variable that can be referenced multiple times.
    //
    //Note, normal fanout to multiple enable output ports would casue a temporary variable to be used.  The internal
    //fanout is important when there is only 1 output port but the potential for the scheduler to move in and out of
    //the enable context multiple times.
    return true;
}
