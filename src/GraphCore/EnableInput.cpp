//
// Created by Christopher Yarp on 6/26/18.
//

#include "EnableInput.h"
#include "GraphMLTools/GraphMLHelper.h"
#include "EnabledSubSystem.h"

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
}


EnableInput::EnableInput(std::shared_ptr<SubSystem> parent, std::shared_ptr<EnableInput> orig) : EnableNode(parent, orig){
    //Nothing extra to copy
}