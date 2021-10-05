//
// Created by Christopher Yarp on 6/26/18.
//

#include "MasterInput.h"
#include "General/GeneralHelper.h"
#include "GraphCore/NodeFactory.h"
#include "General/ErrorHelpers.h"

MasterInput::MasterInput() {

}

MasterInput::MasterInput(std::shared_ptr<SubSystem> parent, MasterNode* orig) : MasterNode(parent, orig){

}

std::string MasterInput::getCInputName(int portNum) {
    return getOutputPort(portNum)->getName() + "_inPort" + GeneralHelper::to_string(portNum);
}

//std::string
//MasterInput::emitC(std::vector<std::string> &cStatementQueue, int outputPortNum, bool imag, bool checkFanout,
//                   bool forceFanout) {
//    return emitCExpr(cStatementQueue, outputPortNum, imag).getExpr();
//}

CExpr MasterInput::emitCExpr(std::vector<std::string> &cStatementQueue, SchedParams::SchedType schedType, int outputPortNum, bool imag) {
    throw std::runtime_error(ErrorHelpers::genErrorStr("C Emit for Master Input Which is Used for Single Threaded Generator is Needs Revision", getSharedPointer()));
}

std::shared_ptr<Node> MasterInput::shallowClone(std::shared_ptr<SubSystem> parent) {
    return NodeFactory::shallowCloneNode<MasterInput>(parent, this);
}

std::string MasterInput::typeNameStr(){
    return "Master Input";
}

void MasterInput::setPortBlockSizesBasedOnClockDomain(int baseBlockSize) {
    //The below logic should work even if blocking is not used (block size == 1)

    std::vector<std::shared_ptr<OutputPort>> outputPortVec = getOutputPorts();
    for(const std::shared_ptr<OutputPort> &outputPort : outputPortVec){
        if(ioClockDomains.find(outputPort) != ioClockDomains.end()){
            std::pair<int, int> rateRelToBase = ioClockDomains[outputPort]->getRateRelativeToBase();
            blockSizes[outputPort] = baseBlockSize*rateRelToBase.first/rateRelToBase.second;
        }else{
            //This port is operating at the base rate
            //It's arc is already expanded by the block size
            blockSizes[outputPort] = 1;
        }
    }
}