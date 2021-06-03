//
// Created by Christopher Yarp on 4/29/20.
//

#include "DownsampleInput.h"
#include "MultiRateHelpers.h"
#include "General/ErrorHelpers.h"

DownsampleInput::DownsampleInput() {

}

DownsampleInput::DownsampleInput(std::shared_ptr<SubSystem> parent) : Downsample(parent) {

}


DownsampleInput::DownsampleInput(std::shared_ptr<SubSystem> parent, DownsampleInput *orig) : Downsample(parent, orig) {

}

std::set<GraphMLParameter> DownsampleInput::graphMLParameters() {
    return Downsample::graphMLParameters();
}

std::string DownsampleInput::typeNameStr() {
    return "DownsampleInput";
}

std::shared_ptr<DownsampleInput>
DownsampleInput::createFromGraphML(int id, std::string name, std::map<std::string, std::string> dataKeyValueMap,
                                   std::shared_ptr<SubSystem> parent, GraphMLDialect dialect) {
    std::shared_ptr<DownsampleInput> newNode = NodeFactory::createNode<DownsampleInput>(parent);

    newNode->populateDownsampleParametersFromGraphML(id, name, dataKeyValueMap, dialect);

    return newNode;
}

xercesc::DOMElement *
DownsampleInput::emitGraphML(xercesc::DOMDocument *doc, xercesc::DOMElement *graphNode, bool include_block_node_type) {
    xercesc::DOMElement* thisNode = emitGraphMLBasics(doc, graphNode);
    if(include_block_node_type) {
        GraphMLHelper::addDataNode(doc, thisNode, "block_node_type", "RateChange"); //This is a special type of block called a RateChange
    }
    GraphMLHelper::addDataNode(doc, thisNode, "block_function", "DownsampleInput");
    emitGraphMLProperties(doc, thisNode);

    return thisNode;
}

void DownsampleInput::validate() {
    Downsample::validate();

    std::shared_ptr<DownsampleInput> thisAsDownsampleInput = std::static_pointer_cast<DownsampleInput>(getSharedPointer());
    MultiRateHelpers::validateRateChangeInput_SetMasterRates(thisAsDownsampleInput, false);
}

std::shared_ptr<Node> DownsampleInput::shallowClone(std::shared_ptr<SubSystem> parent) {
    return NodeFactory::shallowCloneNode<DownsampleInput>(parent, this);
}

CExpr DownsampleInput::emitCExpr(std::vector<std::string> &cStatementQueue, SchedParams::SchedType schedType,
                                 int outputPortNum, bool imag) {
    //Get the Expression for the input (should only be 1)
    std::shared_ptr<OutputPort> srcOutputPort = getInputPort(0)->getSrcOutputPort();
    int srcOutputPortNum = srcOutputPort->getPortNum();
    std::shared_ptr<Node> srcNode = srcOutputPort->getParent();
    DataType inputDT = getInputPort(0)->getDataType();
    CExpr inputExpr = srcNode->emitC(cStatementQueue, schedType, srcOutputPortNum, imag);

    DataType outputDT = getOutputPort(0)->getDataType();

    //Just return the input expression

    //Create a temporary variable to avoid issue if this node is directly attached to state
    //at the input.  The state update is placed after this node but the variable from the delay is simply
    //passed through.  This could cause the state to be update before the result is used.
    //TODO: Remove Temporary when StateUpdate insertion logic improved to track passthroughs
    //Accomplished by returning a SCALAR_EXPR instead of a SCALAR_VAR

    if(inputDT.isScalar()) {
        return CExpr(inputExpr.getExpr(), CExpr::ExprType::SCALAR_EXPR); //This will create a new variable.  If input is a variable, this will create a copy
    }else{
        std::string outName = name + "_n" + GeneralHelper::to_string(id) + "_OutMat";
        Variable outputVar = Variable(outName, outputDT);
        cStatementQueue.push_back(outputVar.getCVarDecl(imag, true, true, true, false) + ";");

        std::tuple<std::vector<std::string>, std::vector<std::string>, std::vector<std::string>> forLoopStrs =
                EmitterHelpers::generateVectorMatrixForLoops(inputDT.getDimensions());

        std::vector<std::string> forLoopOpen = std::get<0>(forLoopStrs);
        std::vector<std::string> forLoopIndexVars = std::get<1>(forLoopStrs);
        std::vector<std::string> forLoopClose = std::get<2>(forLoopStrs);

        cStatementQueue.insert(cStatementQueue.end(), forLoopOpen.begin(), forLoopOpen.end());

        std::string assignExpr = outputVar.getCVarName(imag) + EmitterHelpers::generateIndexOperation(forLoopIndexVars) + " = " + inputExpr.getExprIndexed(forLoopIndexVars, true) + ";";
        cStatementQueue.push_back(assignExpr);

        cStatementQueue.insert(cStatementQueue.end(), forLoopClose.begin(), forLoopClose.end());

        return CExpr(outputVar.getCVarName(imag), CExpr::ExprType::ARRAY);
    }
}

bool DownsampleInput::isSpecialized() {
    return true;
}

bool DownsampleInput::isInput() {
    return true;
}
