//
// Created by Christopher Yarp on 10/18/21.
//

#include "BlockingDomainBridge.h"
#include "General/ErrorHelpers.h"

BlockingDomainBridge::BlockingDomainBridge() : blockSize(0), subBlockSizeIn(0), baseSubBlockSizeIn(0),
                                               subBlockSizeOut(0), baseSubBlockSizeOut(0) {

}

BlockingDomainBridge::BlockingDomainBridge(std::shared_ptr<SubSystem> parent) : Node(parent), blockSize(0),
                                                                                subBlockSizeIn(0),
                                                                                baseSubBlockSizeIn(0),
                                                                                subBlockSizeOut(0),
                                                                                baseSubBlockSizeOut(0) {

}

BlockingDomainBridge::BlockingDomainBridge(std::shared_ptr<SubSystem> parent, BlockingDomainBridge *orig) :
                                                                                Node(parent, orig),
                                                                                blockSize(orig->blockSize),
                                                                                subBlockSizeIn(orig->subBlockSizeIn),
                                                                                baseSubBlockSizeIn(orig->baseSubBlockSizeIn),
                                                                                subBlockSizeOut(orig->subBlockSizeOut),
                                                                                baseSubBlockSizeOut(orig->baseSubBlockSizeOut) {

}


int BlockingDomainBridge::getBlockSize() const {
    return blockSize;
}

void BlockingDomainBridge::setBlockSize(int blockSize) {
    BlockingDomainBridge::blockSize = blockSize;
}

int BlockingDomainBridge::getSubBlockSizeIn() const {
    return subBlockSizeIn;
}

void BlockingDomainBridge::setSubBlockSizeIn(int subBlockSizeIn) {
    BlockingDomainBridge::subBlockSizeIn = subBlockSizeIn;
}

int BlockingDomainBridge::getBaseSubBlockSizeIn() const {
    return baseSubBlockSizeIn;
}

void BlockingDomainBridge::setBaseSubBlockSizeIn(int baseSubBlockSizeIn) {
    BlockingDomainBridge::baseSubBlockSizeIn = baseSubBlockSizeIn;
}

int BlockingDomainBridge::getSubBlockSizeOut() const {
    return subBlockSizeOut;
}

void BlockingDomainBridge::setSubBlockSizeOut(int subBlockSizeOut) {
    BlockingDomainBridge::subBlockSizeOut = subBlockSizeOut;
}

int BlockingDomainBridge::getBaseSubBlockSizeOut() const {
    return baseSubBlockSizeOut;
}

void BlockingDomainBridge::setBaseSubBlockSizeOut(int baseSubBlockSizeOut) {
    BlockingDomainBridge::baseSubBlockSizeOut = baseSubBlockSizeOut;
}

std::shared_ptr<Node> BlockingDomainBridge::shallowClone(std::shared_ptr<SubSystem> parent) {
    return NodeFactory::shallowCloneNode<BlockingDomainBridge>(parent, this);
}

std::string BlockingDomainBridge::typeNameStr() {
    return "BlockingDomainBridge";
}

bool BlockingDomainBridge::canExpand() {
    return false;
}

std::set<GraphMLParameter> BlockingDomainBridge::graphMLParameters() {
    std::set<GraphMLParameter> params = Node::graphMLParameters();

    params.insert(GraphMLParameter("BlockSize", "int", true));
    params.insert(GraphMLParameter("SubBlockSizeIn", "int", true));
    params.insert(GraphMLParameter("BaseSubBlockSizeIn", "int", true));
    params.insert(GraphMLParameter("SubBlockSizeOut", "int", true));
    params.insert(GraphMLParameter("BaseSubBlockSizeOut", "int", true));


    return params;
}

xercesc::DOMElement *BlockingDomainBridge::emitGraphML(xercesc::DOMDocument *doc, xercesc::DOMElement *graphNode,
                                                       bool include_block_node_type) {
    xercesc::DOMElement* thisNode = emitGraphMLBasics(doc, graphNode);
    if(include_block_node_type) {
        GraphMLHelper::addDataNode(doc, thisNode, "block_node_type", "BlockingDomainBridge");
    }

    GraphMLHelper::addDataNode(doc, thisNode, "block_function", typeNameStr());

    GraphMLHelper::addDataNode(doc, thisNode, "BlockSize", GeneralHelper::to_string(blockSize));
    GraphMLHelper::addDataNode(doc, thisNode, "SubBlockSizeIn", GeneralHelper::to_string(subBlockSizeIn));
    GraphMLHelper::addDataNode(doc, thisNode, "BaseSubBlockSizeIn", GeneralHelper::to_string(baseSubBlockSizeIn));
    GraphMLHelper::addDataNode(doc, thisNode, "SubBlockSizeOut", GeneralHelper::to_string(subBlockSizeOut));
    GraphMLHelper::addDataNode(doc, thisNode, "BaseSubBlockSizeOut", GeneralHelper::to_string(baseSubBlockSizeOut));

    return thisNode;
}

void BlockingDomainBridge::validate() {
    Node::validate();

    if(inputPorts.size() != 1 || outputPorts.size() != 1){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Expected BlockingDomainBridge to have exactly one input and output port.", getSharedPointer()));
    }

    //TODO: Implement
}

CExpr BlockingDomainBridge::emitCExpr(std::vector<std::string> &cStatementQueue, SchedParams::SchedType schedType,
                                      int outputPortNum, bool imag) {
    //TODO: Needs to be implemented in much the same way as FIFOs including the need for absorbing delays into initial conditions
    //      This is required because blocking domains are currently treated as contexts with all nodes under the blocking domain
    //      scheduled together.  If delays are not absorbed, it is possible for the different blocking domains to have a
    //      scheduling cycle

    throw std::runtime_error(ErrorHelpers::genErrorStr("Sub-Blocking transitions are not currently allowed within a single partition.", getSharedPointer()));

    return Node::emitCExpr(cStatementQueue, schedType, outputPortNum, imag);

//    //Get the Expression for the input (should only be 1)
//    std::shared_ptr<OutputPort> srcOutputPort = getInputPort(0)->getSrcOutputPort();
//    int srcOutputPortNum = srcOutputPort->getPortNum();
//    std::shared_ptr<Node> srcNode = srcOutputPort->getParent();
//    CExpr inputExpr = srcNode->emitC(cStatementQueue, schedType, srcOutputPortNum, imag);
//    inputExpr.setExpr("/*BLOCKING-DOMAIN-BRIDGE: " + getFullyQualifiedOrigName() + " " + (imag ? "Imag" : "Real") + "*/");
//    return inputExpr;
//
//    return Node::emitCExpr(cStatementQueue, schedType, outputPortNum, imag);
}
