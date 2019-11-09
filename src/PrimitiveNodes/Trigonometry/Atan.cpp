//
// Created by Christopher Yarp on 10/13/19.
//

#include "Atan.h"

#include "General/ErrorHelpers.h"

Atan::Atan() : PrimitiveNode() {

}

Atan::Atan(std::shared_ptr<SubSystem> parent) : PrimitiveNode(parent) {

}

std::shared_ptr<Atan>
Atan::createFromGraphML(int id, std::string name, std::map<std::string, std::string> dataKeyValueMap,
                      std::shared_ptr<SubSystem> parent, GraphMLDialect dialect) {
    //==== Create Node and set common properties ====
    std::shared_ptr<Atan> newNode = NodeFactory::createNode<Atan>(parent);
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
        throw std::runtime_error(ErrorHelpers::genErrorStr("Unsupported Dialect when parsing XML - Atan", newNode));
    }

    return newNode;
}

std::set<GraphMLParameter> Atan::graphMLParameters() {
    std::set<GraphMLParameter> parameters;

    //There are no parameters

    return  parameters;
}

xercesc::DOMElement *Atan::emitGraphML(xercesc::DOMDocument *doc, xercesc::DOMElement *graphNode,
                                     bool include_block_node_type) {
    //Create Node
    xercesc::DOMElement* thisNode = emitGraphMLBasics(doc, graphNode);

    //Add Parameters / Attributes to Node
    if(include_block_node_type) {
        GraphMLHelper::addDataNode(doc, thisNode, "block_node_type", "Standard");
    }

    GraphMLHelper::addDataNode(doc, thisNode, "block_function", "Atan");

    return thisNode;
}

std::string Atan::typeNameStr(){
    return "Atan";
}

std::string Atan::labelStr() {
    std::string label = Node::labelStr();

    label += "\nFunction: " + typeNameStr();

    return label;
}

void Atan::validate() {
    Node::validate(); //Perform the node level validation

    //Should have 1 input ports and 1 output port
    if(inputPorts.size() != 1){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - Atan - Should Have Exactly 1 Input Port", getSharedPointer()));
    }

    if(outputPorts.size() != 1){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - Atan - Should Have Exactly 1 Output Port", getSharedPointer()));
    }

    if(inputPorts[0]->getDataType().isComplex() || outputPorts[0]->getDataType().isComplex()){
        //TODO: Implement Complex Atan
        throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - Atan - Complex Atan not yet implemented", getSharedPointer()));
    }
}

std::set<std::string> Atan::getExternalIncludes() {
    std::set<std::string> extIncludesSet = Node::getExternalIncludes();
    extIncludesSet.insert("#include <math.h>");

    return extIncludesSet;
}

CExpr Atan::emitCExpr(std::vector<std::string> &cStatementQueue, SchedParams::SchedType schedType, int outputPortNum, bool imag) {
    //TODO: Implement complex Atan

    //Get the Expression for the input (should only be 1)
    std::shared_ptr<OutputPort> srcOutputPort = getInputPort(0)->getSrcOutputPort();
    int srcOutputPortNum = srcOutputPort->getPortNum();
    std::shared_ptr<Node> srcNode = srcOutputPort->getParent();
    std::string inputExpr = srcNode->emitC(cStatementQueue, schedType, srcOutputPortNum, imag);

    DataType dstType = getOutputPort(0)->getDataType();

    //TODO: Implement Vector Support
    if (getInputPort(0)->getDataType().getWidth() > 1) {
        throw std::runtime_error(ErrorHelpers::genErrorStr("C Emit Error - Atan Support for Vector Types has Not Yet Been Implemented", getSharedPointer()));
    }

    //TODO: Implement Fixed Point Support
    if ((!getInputPort(0)->getDataType().isCPUType()) || (!getOutputPort(0)->getDataType().isCPUType())) {
        throw std::runtime_error(ErrorHelpers::genErrorStr(
                "C Emit Error - Atan for Fixed Point Types has Not Yet Been Implemented", getSharedPointer()));
    }

    DataType inputType = getInputPort(0)->getDataType();
    //We are using C11, can use atanf
    DataType rtnType;
    std::string fctnCall;
    if(inputType.getTotalBits() <= 32){
        rtnType = DataType(true, true, false, 32, 0, 1); //The atanf function returns a float
        fctnCall = "atanf(" + inputExpr + ")";
    }else{
        rtnType = DataType(true, true, false, 64, 0, 1); //The atan function returns a double
        fctnCall = "atan(" + inputExpr + ")";
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

Atan::Atan(std::shared_ptr<SubSystem> parent, Atan* orig) : PrimitiveNode(parent, orig){

}

std::shared_ptr<Node> Atan::shallowClone(std::shared_ptr<SubSystem> parent) {
    return NodeFactory::shallowCloneNode<Atan>(parent, this);
}
