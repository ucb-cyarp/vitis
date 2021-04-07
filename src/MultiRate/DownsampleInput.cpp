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
    //TODO: Implement Vector Support
    if (!getInputPort(0)->getDataType().isScalar()) {
        throw std::runtime_error(ErrorHelpers::genErrorStr("C Emit Error - Ln Support for Vector Types has Not Yet Been Implemented", getSharedPointer()));
    }

    //Get the Expression for the input (should only be 1)
    std::shared_ptr<OutputPort> srcOutputPort = getInputPort(0)->getSrcOutputPort();
    int srcOutputPortNum = srcOutputPort->getPortNum();
    std::shared_ptr<Node> srcNode = srcOutputPort->getParent();
    DataType inputDT = getInputPort(outputPortNum)->getDataType();
    CExpr inputExpr = srcNode->emitC(cStatementQueue, schedType, srcOutputPortNum, imag);

    //Just return the input expression
    //TODO: Change check after vector support implemented
    if(inputExpr.isArrayOrBuffer()){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Enable Line to Downsample Input is Expected to be Driven by a Scalar Expression or Variable (from " + srcNode->getFullyQualifiedName() + ")", getSharedPointer()));
    }

    //Create a temporary variable to avoid issue if this node is directly attached to state
    //at the input.  The state update is placed after this node but the variable from the delay is simply
    //passed through.  This could cause the state to be update before the result is used.
    //TODO: Remove Temporary when StateUpdate insertion logic improved to track passthroughs
    //Accomplished by returning a SCALAR_EXPR instead of a SCALAR_VAR

    return CExpr(inputExpr.getExpr(), CExpr::ExprType::SCALAR_EXPR);
}

bool DownsampleInput::isSpecialized() {
    return true;
}
