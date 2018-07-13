//
// Created by Christopher Yarp on 6/26/18.
//

#include "EnableOutput.h"
#include "GraphMLTools/GraphMLHelper.h"

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