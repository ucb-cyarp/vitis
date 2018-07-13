//
// Created by Christopher Yarp on 6/26/18.
//

#include "EnableInput.h"
#include "GraphMLTools/GraphMLHelper.h"

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
