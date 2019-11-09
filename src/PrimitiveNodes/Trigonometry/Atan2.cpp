//
// Created by Christopher Yarp on 10/13/19.
//

#include "Atan2.h"

#include "General/ErrorHelpers.h"

Atan2::Atan2() : PrimitiveNode() {

}

Atan2::Atan2(std::shared_ptr<SubSystem> parent) : PrimitiveNode(parent) {

}

std::shared_ptr<Atan2>
Atan2::createFromGraphML(int id, std::string name, std::map<std::string, std::string> dataKeyValueMap,
                      std::shared_ptr<SubSystem> parent, GraphMLDialect dialect) {
    //==== Create Node and set common properties ====
    std::shared_ptr<Atan2> newNode = NodeFactory::createNode<Atan2>(parent);
    newNode->setId(id);
    newNode->setName(name);

    //==== Import important properties ====
    std::string datatypeStr;

    if (dialect == GraphMLDialect::VITIS) {
        //Vitis Names -- There are no parameters to import

    } else if (dialect == GraphMLDialect::SIMULINK_EXPORT) {
        //Simulink Names -- There are no parameters to import
    } else
    {
        throw std::runtime_error(ErrorHelpers::genErrorStr("Unsupported Dialect when parsing XML - Atan2", newNode));
    }

    return newNode;
}

std::set<GraphMLParameter> Atan2::graphMLParameters() {
    std::set<GraphMLParameter> parameters;

    //There are no parameters

    return  parameters;
}

xercesc::DOMElement *Atan2::emitGraphML(xercesc::DOMDocument *doc, xercesc::DOMElement *graphNode,
                                     bool include_block_node_type) {
    //Create Node
    xercesc::DOMElement* thisNode = emitGraphMLBasics(doc, graphNode);

    //Add Parameters / Attributes to Node
    if(include_block_node_type) {
        GraphMLHelper::addDataNode(doc, thisNode, "block_node_type", "Standard");
    }

    GraphMLHelper::addDataNode(doc, thisNode, "block_function", "Atan2");

    return thisNode;
}

std::string Atan2::typeNameStr(){
    return "Atan2";
}

std::string Atan2::labelStr() {
    std::string label = Node::labelStr();

    label += "\nFunction: " + typeNameStr();

    return label;
}

void Atan2::validate() {
    Node::validate(); //Perform the node level validation

    //Should have 1 input ports and 1 output port
    if(inputPorts.size() != 2){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - Atan2 - Should Have Exactly 2 Input Port", getSharedPointer()));
    }

    if(outputPorts.size() != 1){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - Atan2 - Should Have Exactly 1 Output Port", getSharedPointer()));
    }

    if(inputPorts[0]->getDataType().isComplex() || outputPorts[0]->getDataType().isComplex()){
        //TODO: Implement Complex Atan2
        throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - Atan2 - Complex Atan2 not yet implemented", getSharedPointer()));
    }
}

std::set<std::string> Atan2::getExternalIncludes() {
    std::set<std::string> extIncludesSet = Node::getExternalIncludes();
    extIncludesSet.insert("#include <math.h>");

    return extIncludesSet;
}

CExpr Atan2::emitCExpr(std::vector<std::string> &cStatementQueue, SchedParams::SchedType schedType, int outputPortNum, bool imag) {
    //TODO: Implement complex Atan2

    //Get the Expression for the input (should only be 1)
    std::shared_ptr<OutputPort> srcOutputPortY = getInputPort(0)->getSrcOutputPort();
    int srcOutputPortYNum = srcOutputPortY->getPortNum();
    std::shared_ptr<Node> srcNodeY = srcOutputPortY->getParent();
    std::string inputExprY = srcNodeY->emitC(cStatementQueue, schedType, srcOutputPortYNum, imag);

    std::shared_ptr<OutputPort> srcOutputPortX = getInputPort(1)->getSrcOutputPort();
    int srcOutputPortXNum = srcOutputPortX->getPortNum();
    std::shared_ptr<Node> srcNodeX = srcOutputPortX->getParent();
    std::string inputExprX = srcNodeX->emitC(cStatementQueue, schedType, srcOutputPortXNum, imag);

    DataType dstType = getOutputPort(0)->getDataType();

    //TODO: Implement Vector Support
    if (getInputPort(0)->getDataType().getWidth() > 1 || getInputPort(1)->getDataType().getWidth() > 1) {
        throw std::runtime_error(ErrorHelpers::genErrorStr("C Emit Error - Atan2 Support for Vector Types has Not Yet Been Implemented", getSharedPointer()));
    }

    //TODO: Implement Fixed Point Support
    if ((!getInputPort(0)->getDataType().isCPUType()) || (!getOutputPort(0)->getDataType().isCPUType())) {
        throw std::runtime_error(ErrorHelpers::genErrorStr(
                "C Emit Error - Atan2 for Fixed Point Types has Not Yet Been Implemented", getSharedPointer()));
    }

    DataType inputType1 = getInputPort(0)->getDataType();
    DataType inputType2 = getInputPort(1)->getDataType();
    //We are using C11, can use atan2f
    DataType rtnType;
    std::string fctnCall;
    if(inputType1.getTotalBits() <= 32 && inputType2.getTotalBits() <= 32){
        rtnType = DataType(true, true, false, 32, 0, 1); //The atan2f function returns a float
        fctnCall = "atan2f(" + inputExprY + ", " + inputExprX + ")";
    }else{
        rtnType = DataType(true, true, false, 64, 0, 1); //The atan2 function returns a double
        fctnCall = "atan2(" + inputExprY + ", " + inputExprX + ")";
    }

    std::string finalExpr;

    if (dstType != rtnType) {
        //We actually need to do the cast
        finalExpr = "((" + dstType.toString(DataType::StringStyle::C, false) + ") " + fctnCall + ")";
    }else{
        finalExpr = fctnCall;
    }

    return CExpr(finalExpr, false);
}

Atan2::Atan2(std::shared_ptr<SubSystem> parent, Atan2* orig) : PrimitiveNode(parent, orig){

}

std::shared_ptr<Node> Atan2::shallowClone(std::shared_ptr<SubSystem> parent) {
    return NodeFactory::shallowCloneNode<Atan2>(parent, this);
}
