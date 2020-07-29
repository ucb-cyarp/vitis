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
        //Because of C multidimensional array semantics, and because the added dimension for blocks >1 is prepended to
        //the dimensions, indexing the first dimension will return the correct value.  If the data type is a scalar, it
        //returns a scalar value for the given block.  If the data type is a vector or matrix, this will still return a
        //a pointer but a pointer to the correct block.
        //This same style is used in the ThreadCrossingFIFO
        expr = "(" + var.getCVarName(imag) + "[" + indVarName + "])";
    }else{
        expr = var.getCVarName(imag);
    }

    return CExpr(expr, true);
}

std::shared_ptr<Node> MasterInput::shallowClone(std::shared_ptr<SubSystem> parent) {
    return NodeFactory::shallowCloneNode<MasterInput>(parent, this);
}

std::string MasterInput::typeNameStr(){
    return "Master Input";
}