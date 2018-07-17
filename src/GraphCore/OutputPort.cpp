//
// Created by Christopher Yarp on 7/17/18.
//

#include "OutputPort.h"
#include "DataType.h"
#include "Arc.h"
#include "Node.h"

OutputPort::OutputPort() {

}

OutputPort::OutputPort(Node *parent, int portNum) : Port(parent, portNum) {

}

void OutputPort::validate() {
    if(arcs.size() < 1){
        throw std::runtime_error("Validation Failed - Output Port Unconnected (Should be Connected to Unconnected Master)");
    }

    auto firstArc = arcs.begin();

    DataType outputType = (*firstArc).lock()->getDataType();

    for(auto arc = (firstArc++); arc != arcs.end(); arc++){
        if((*arc).lock()->getDataType() != outputType){
            throw std::runtime_error("Validation Failed - Output Port DataType Mismatch");
        }
    }
}

std::shared_ptr<OutputPort> OutputPort::getSharedPointerOutputPort() {
    if(parent != nullptr) {
        return std::shared_ptr<OutputPort>(parent->getSharedPointer(), this);
    }
    else {
        //Hopefully should not happen but just in case
        //return std::shared_ptr<Port>(nullptr);
        throw std::runtime_error("Pointer requested from port that has no parent");
    }
}