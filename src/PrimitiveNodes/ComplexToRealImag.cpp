//
// Created by Christopher Yarp on 9/10/18.
//

#include "ComplexToRealImag.h"
#include "GraphCore/NodeFactory.h"
#include "General/ErrorHelpers.h"

ComplexToRealImag::ComplexToRealImag() {

}

ComplexToRealImag::ComplexToRealImag(std::shared_ptr<SubSystem> parent) : PrimitiveNode(parent) {

}

std::shared_ptr<ComplexToRealImag>
ComplexToRealImag::createFromGraphML(int id, std::string name, std::map<std::string, std::string> dataKeyValueMap,
                                     std::shared_ptr<SubSystem> parent, GraphMLDialect dialect) {
    std::shared_ptr<ComplexToRealImag> newNode = NodeFactory::createNode<ComplexToRealImag>(parent);
    newNode->setId(id);
    newNode->setName(name);

    //There are no properties to import

    return newNode;
}

std::set<GraphMLParameter> ComplexToRealImag::graphMLParameters() {
    //No Parameters
    return Node::graphMLParameters();
}

xercesc::DOMElement *ComplexToRealImag::emitGraphML(xercesc::DOMDocument *doc, xercesc::DOMElement *graphNode,
                                                    bool include_block_node_type) {
    xercesc::DOMElement* thisNode = emitGraphMLBasics(doc, graphNode);
    if(include_block_node_type) {
        GraphMLHelper::addDataNode(doc, thisNode, "block_node_type", "Standard");
    }

    GraphMLHelper::addDataNode(doc, thisNode, "block_function", "ComplexToRealImag");

    return thisNode;
}

std::string ComplexToRealImag::typeNameStr(){
    return "ComplexToRealImag";
}

std::string ComplexToRealImag::labelStr() {
    std::string label = Node::labelStr();

    label += "\nFunction: " + typeNameStr();

    return label;
}

void ComplexToRealImag::validate() {
    Node::validate();

    if(inputPorts.size() != 1){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - ComplexToRealImag - Should Have Exactly 1 Input Port", getSharedPointer()));
    }

    if(outputPorts.size() != 2){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - ComplexToRealImag - Should Have Exactly 2 Output Ports", getSharedPointer()));
    }

    //Check that input is complex, and the outputs are real
    if(!(getInputPort(0)->getDataType().isComplex())){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - ComplexToRealImag - Input Port Must Be Complex", getSharedPointer()));
    }

    if(getOutputPort(0)->getDataType().isComplex()){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - ComplexToRealImag - Output Port 0 Must Be Real", getSharedPointer()));
    }

    if(getOutputPort(1)->getDataType().isComplex()){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - ComplexToRealImag - Output Port 1 Must Be Real", getSharedPointer()));
    }

}

CExpr ComplexToRealImag::emitCExpr(std::vector<std::string> &cStatementQueue, SchedParams::SchedType schedType, int outputPortNum, bool imag) {
    //We can ignore imag since we validated that the outputs are both real

    std::shared_ptr<OutputPort> srcOutputPort = getInputPort(0)->getSrcOutputPort();
    int srcOutputPortNum = srcOutputPort->getPortNum();
    std::shared_ptr<Node> srcNode = srcOutputPort->getParent();

    std::string emitStr;

    if(outputPortNum == 0) {
        //Output Port 0 Is Real
        emitStr = srcNode->emitC(cStatementQueue, schedType, srcOutputPortNum, false);

    }else{ //if(outputPortNum == 1){ --Already validated the number of output ports is 2
        //Output Port 1 Is Imag
        emitStr = srcNode->emitC(cStatementQueue, schedType, srcOutputPortNum, true);
    }

    return CExpr(emitStr, false);
}

ComplexToRealImag::ComplexToRealImag(std::shared_ptr<SubSystem> parent, ComplexToRealImag* orig) : PrimitiveNode(
        parent, orig) {
    //Nothing new to copy, call superclass constructor
}

std::shared_ptr<Node> ComplexToRealImag::shallowClone(std::shared_ptr<SubSystem> parent) {
    return NodeFactory::shallowCloneNode<ComplexToRealImag>(parent, this);
}
