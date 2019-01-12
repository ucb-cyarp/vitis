//
// Created by Christopher Yarp on 6/27/18.
//

#include "MasterOutput.h"
#include "General/GeneralHelper.h"
#include "GraphCore/NodeFactory.h"

MasterOutput::MasterOutput() {

}

MasterOutput::MasterOutput(std::shared_ptr<SubSystem> parent, MasterNode* orig) : MasterNode(parent, orig){

}

std::string MasterOutput::getCOutputName(int portNum) {
    return getInputPort(portNum)->getName() + "_outPort" + GeneralHelper::to_string(portNum);
}

std::shared_ptr<Node> MasterOutput::shallowClone(std::shared_ptr<SubSystem> parent) {
    return NodeFactory::shallowCloneNode<MasterOutput>(parent, this);
}