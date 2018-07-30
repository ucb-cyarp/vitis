//
// Created by Christopher Yarp on 6/26/18.
//

#include "EnableOutput.h"
#include "GraphMLTools/GraphMLHelper.h"
#include "EnabledSubSystem.h"

EnableOutput::EnableOutput() {

}

EnableOutput::EnableOutput(std::shared_ptr<SubSystem> parent) : EnableNode(parent) {

}


xercesc::DOMElement *EnableOutput::emitGraphML(xercesc::DOMDocument *doc, xercesc::DOMElement *graphNode, bool include_block_node_type) {
    xercesc::DOMElement* thisNode = emitGraphMLBasics(doc, graphNode);
    if(include_block_node_type) {
        GraphMLHelper::addDataNode(doc, thisNode, "block_node_type", "Special Output Port");
    }

    return thisNode;
}

std::string EnableOutput::labelStr() {
    std::string label = Node::labelStr();

    label += "\nType: Enable Output";

    return label;
}

void EnableOutput::validate() {
    EnableNode::validate();

    //Parent checked above to be an Enabled SubSystem
    std::shared_ptr<EnabledSubSystem> parentEnabled = std::dynamic_pointer_cast<EnabledSubSystem>(parent);

    std::vector<std::shared_ptr<EnableOutput>> parentOutputNodes = parentEnabled->getEnableOutputs();

    unsigned long numEnableOutputs = parentOutputNodes.size();
    bool found = false;
    for(unsigned long i = 0; i<numEnableOutputs; i++){
        if(parentOutputNodes[i] == getSharedPointer()){
            found = true;
            break;
        }
    }

    if(!found){
        throw std::runtime_error("EnableInput not found in parent EnabledInput list");
    }
}
