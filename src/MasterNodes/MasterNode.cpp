//
// Created by Christopher Yarp on 7/11/18.
//

#include "MasterNode.h"
#include "GraphMLTools/GraphMLHelper.h"

MasterNode::MasterNode() {

}

xercesc::DOMElement *
MasterNode::emitGraphML(xercesc::DOMDocument *doc, xercesc::DOMElement *graphNode, bool include_block_node_type) {
    xercesc::DOMElement* thisNode = emitGraphMLBasics(doc, graphNode);
    if(include_block_node_type) {
        GraphMLHelper::addDataNode(doc, thisNode, "block_node_type", "Master");
    }

    return thisNode;
}

std::string MasterNode::labelStr() {
    std::string label = Node::labelStr();

    label += "\nType: Master";

    return label;
}

bool MasterNode::canExpand() {
    return false;
}