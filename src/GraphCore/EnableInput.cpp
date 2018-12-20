//
// Created by Christopher Yarp on 6/26/18.
//

#include "EnableInput.h"
#include "GraphMLTools/GraphMLHelper.h"
#include "EnabledSubSystem.h"
#include "NodeFactory.h"

EnableInput::EnableInput() {

}

EnableInput::EnableInput(std::shared_ptr<SubSystem> parent) : EnableNode(parent) {

}

xercesc::DOMElement *EnableInput::emitGraphML(xercesc::DOMDocument *doc, xercesc::DOMElement *graphNode, bool include_block_node_type) {
    xercesc::DOMElement* thisNode = emitGraphMLBasics(doc, graphNode);
    if(include_block_node_type) {
        GraphMLHelper::addDataNode(doc, thisNode, "block_node_type", "Special Input Port");
    }

    return thisNode;
}

std::string EnableInput::labelStr() {
    std::string label = Node::labelStr();

    label += "\nType: Enable Input";

    return label;
}

void EnableInput::validate() {
    EnableNode::validate();

    //Parent checked above to be an Enabled SubSystem
    std::shared_ptr<EnabledSubSystem> parentEnabled = std::dynamic_pointer_cast<EnabledSubSystem>(parent);

    std::vector<std::shared_ptr<EnableInput>> parentInputNodes = parentEnabled->getEnableInputs();

    unsigned long numEnableInputs = parentInputNodes.size();
    bool found = false;
    for(unsigned long i = 0; i<numEnableInputs; i++){
        if(parentInputNodes[i] == getSharedPointer()){
            found = true;
            break;
        }
    }

    if(!found){
        throw std::runtime_error("EnableInput not found in parent EnabledInput list");
    }

    if(inputPorts.size() != 1){
        throw std::runtime_error("EnableInput should have exactly 1 input port");
    }

    if(outputPorts.size() != 1){
        throw std::runtime_error("EnableInput should have exactly 1 output port");
    }
}


EnableInput::EnableInput(std::shared_ptr<SubSystem> parent, EnableInput* orig) : EnableNode(parent, orig){
    //Nothing extra to copy
}

std::shared_ptr<Node> EnableInput::shallowClone(std::shared_ptr<SubSystem> parent) {
    return NodeFactory::shallowCloneNode<EnableInput>(parent, this);
}

CExpr EnableInput::emitCExpr(std::vector<std::string> &cStatementQueue, SchedParams::SchedType schedType, int outputPortNum, bool imag) {
    //TODO: Implement Vector Support
    if(getInputPort(outputPortNum)->getDataType().getWidth()>1 || getInputPort(outputPortNum)->getDataType().getWidth()>1){
        throw std::runtime_error("C Emit Error - EnableInput Support for Vector Types has Not Yet Been Implemented");
    }

    //Get the expressions for each input
    std::string inputExpr;

    std::shared_ptr<OutputPort> srcOutputPort = getInputPort(outputPortNum)->getSrcOutputPort();
    int srcOutputPortNum = srcOutputPort->getPortNum();
    std::shared_ptr<Node> srcNode = srcOutputPort->getParent();

    inputExpr = srcNode->emitC(cStatementQueue, schedType, srcOutputPortNum, imag);

    return CExpr(inputExpr, false);

}
