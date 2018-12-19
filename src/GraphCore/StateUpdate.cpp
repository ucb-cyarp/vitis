//
// Created by Christopher Yarp on 10/25/18.
//

#include "StateUpdate.h"

std::shared_ptr<Node> StateUpdate::getPrimaryNode() const {
    return primaryNode;
}

void StateUpdate::setPrimaryNode(const std::shared_ptr<Node> &primaryNode) {
    StateUpdate::primaryNode = primaryNode;
}

StateUpdate::StateUpdate() : primaryNode(nullptr){

}

StateUpdate::StateUpdate(std::shared_ptr<SubSystem> parent) : Node(parent), primaryNode(nullptr) {

}

StateUpdate::StateUpdate(std::shared_ptr<SubSystem> parent, StateUpdate *orig) : Node(parent, orig) {

}

void StateUpdate::validate() {
    Node::validate();

    if(primaryNode == nullptr){
        throw std::runtime_error("A StateUpdate node has no primary node");
    }

    if(!primaryNode->hasState()){
        throw std::runtime_error("A StateUpdate node has a primary node which does not have state");
    }

    if(inputPorts.size() > 1){
        throw std::runtime_error("StateUpdate can have either 0 or 1 input ports");
    }

    if(inputPorts.size() != outputPorts.size()){
        throw std::runtime_error("StateUpdate must have an equal number of input and output ports");
    }
}

std::shared_ptr<Node> StateUpdate::shallowClone(std::shared_ptr<SubSystem> parent) {
    return NodeFactory::shallowCloneNode<StateUpdate>(parent, this);
}

void StateUpdate::emitCStateUpdate(std::vector<std::string> &cStatementQueue) {
    primaryNode->emitCStateUpdate(cStatementQueue);
}

std::set<GraphMLParameter> StateUpdate::graphMLParameters() {
    return Node::graphMLParameters();
}

xercesc::DOMElement *
StateUpdate::emitGraphML(xercesc::DOMDocument *doc, xercesc::DOMElement *graphNode, bool include_block_node_type) {
    throw std::runtime_error("XML Emit for StateUpdate not yet implemented");
}

bool StateUpdate::canExpand() {
    return false;
}

CExpr StateUpdate::emitCExpr(std::vector<std::string> &cStatementQueue, int outputPortNum, bool imag){
    //TODO: Implement Vector Support
    if(getInputPort(outputPortNum)->getDataType().getWidth()>1 || getInputPort(outputPortNum)->getDataType().getWidth()>1){
        throw std::runtime_error("C Emit Error - StateUpdate Support for Vector Types has Not Yet Been Implemented");
    }

    //Get the expressions for each input
    std::string inputExpr;

    std::shared_ptr<OutputPort> srcOutputPort = getInputPort(outputPortNum)->getSrcOutputPort();
    int srcOutputPortNum = srcOutputPort->getPortNum();
    std::shared_ptr<Node> srcNode = srcOutputPort->getParent();

    inputExpr = srcNode->emitC(cStatementQueue, srcOutputPortNum, imag);

    return CExpr(inputExpr, false);
}