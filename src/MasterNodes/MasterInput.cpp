//
// Created by Christopher Yarp on 6/26/18.
//

#include "MasterInput.h"
#include "General/GeneralHelper.h"
#include "GraphCore/NodeFactory.h"

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
    //Get a Dummy Variable Object So We can use its formatter
    Variable var;
    var.setName(getCInputName(outputPortNum));
    var.setDataType(getOutputPort(outputPortNum)->getDataType());

    std::shared_ptr<OutputPort> outputPort = getOutputPort(outputPortNum);

    std::string expr;
    if(blockSize > 1){
        int width = outputPort->getDataType().getWidth();
        if(width > 1){
            expr = "(" + var.getCVarName(imag) + "+" + GeneralHelper::to_string(width) + "*" + indVarName + ")";
        }else{
            expr = "(" +  var.getCVarName(imag) + "[" + indVarName + "])";
        }
    }else{
        expr = var.getCVarName(imag);
    }

    return CExpr(expr, true);
}

std::shared_ptr<Node> MasterInput::shallowClone(std::shared_ptr<SubSystem> parent) {
    return NodeFactory::shallowCloneNode<MasterInput>(parent, this);
}
