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

std::string MasterOutput::typeNameStr(){
    return "Master Output";
}

void MasterOutput::setPortBlockSizesBasedOnClockDomain(int baseBlockSize) {
    //The below logic should work even if blocking is not used (block size == 1)

    std::vector<std::shared_ptr<InputPort>> outputPortVec = getInputPorts();
    for(const std::shared_ptr<InputPort> &inputPort : outputPortVec){
        if(ioClockDomains.find(inputPort) != ioClockDomains.end()){
            std::pair<int, int> rateRelToBase = ioClockDomains[inputPort]->getRateRelativeToBase();
            blockSizes[inputPort] = baseBlockSize*rateRelToBase.first/rateRelToBase.second;
        }else{
            //This port is operating at the base rate
            //It's arc is already expanded by the block size
            blockSizes[inputPort] = 1;
        }
    }
}