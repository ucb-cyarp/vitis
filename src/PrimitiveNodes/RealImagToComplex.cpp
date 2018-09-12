//
// Created by Christopher Yarp on 9/10/18.
//

#include "RealImagToComplex.h"

RealImagToComplex::RealImagToComplex() {

}

RealImagToComplex::RealImagToComplex(std::shared_ptr<SubSystem> parent) : PrimitiveNode(parent) {

}

std::shared_ptr<RealImagToComplex>
RealImagToComplex::createFromGraphML(int id, std::string name, std::map<std::string, std::string> dataKeyValueMap,
                                     std::shared_ptr<SubSystem> parent, GraphMLDialect dialect) {
    std::shared_ptr<RealImagToComplex> newNode = NodeFactory::createNode<RealImagToComplex>(parent);
    newNode->setId(id);
    newNode->setName(name);

    //There are no properties to import

    return newNode;
}

std::set<GraphMLParameter> RealImagToComplex::graphMLParameters() {
    //No Parameters
    return Node::graphMLParameters();
}

xercesc::DOMElement *RealImagToComplex::emitGraphML(xercesc::DOMDocument *doc, xercesc::DOMElement *graphNode,
                                                    bool include_block_node_type) {
    xercesc::DOMElement* thisNode = emitGraphMLBasics(doc, graphNode);
    if(include_block_node_type) {
        GraphMLHelper::addDataNode(doc, thisNode, "block_node_type", "Standard");
    }

    GraphMLHelper::addDataNode(doc, thisNode, "block_function", "RealImagToComplex");

    return thisNode;
}

std::string RealImagToComplex::labelStr() {
    std::string label = Node::labelStr();

    label += "\nFunction: RealImagToComplex";

    return label;
}

void RealImagToComplex::validate() {
    Node::validate();

    if(inputPorts.size() != 2){
        throw std::runtime_error("Validation Failed - RealImagToComplex - Should Have Exactly 2 Input Ports");
    }

    if(outputPorts.size() != 1){
        throw std::runtime_error("Validation Failed - RealImagToComplex - Should Have Exactly 1 Output Port");
    }

    //Check that input is complex, and the outputs are real
    if(!(getOutputPort(0)->getDataType().isComplex())){
        throw std::runtime_error("Validation Failed - RealImagToComplex - Output Port Must Be Complex");
    }

    if(!(getInputPort(0)->getDataType().isComplex())){
        throw std::runtime_error("Validation Failed - RealImagToComplex - Input Port 0 Must Be Real");
    }

    if(!(getInputPort(1)->getDataType().isComplex())){
        throw std::runtime_error("Validation Failed - RealImagToComplex - Input Port 1 Must Be Real");
    }
}

CExpr RealImagToComplex::emitCExpr(std::vector<std::string> &cStatementQueue, int outputPortNum, bool imag) {
    std::shared_ptr<OutputPort> srcOutputPort;
    int srcOutputPortNum;
    std::shared_ptr<Node> srcNode;

    if(imag){
        //Input Port 1 Is Imag
        srcOutputPort = getInputPort(1)->getSrcOutputPort();
        srcOutputPortNum = srcOutputPort->getPortNum();
        srcNode = srcOutputPort->getParent();
    }else{
        //Input Port 0 is Real
        srcOutputPort = getInputPort(0)->getSrcOutputPort();
        srcOutputPortNum = srcOutputPort->getPortNum();
        srcNode = srcOutputPort->getParent();
    }

    std::string emitStr = srcNode->emitC(cStatementQueue, srcOutputPortNum, false); //The input was validated to be real

    return CExpr(emitStr, false);
}
