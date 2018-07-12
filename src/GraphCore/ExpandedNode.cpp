//
// Created by Christopher Yarp on 6/26/18.
//

#include "ExpandedNode.h"
#include "GraphMLTools/GraphMLHelper.h"

ExpandedNode::ExpandedNode() {
    origNode = std::shared_ptr<ExpandedNode>(nullptr);
}

ExpandedNode::ExpandedNode(std::shared_ptr<SubSystem> parent) : SubSystem(parent) {
    origNode = std::shared_ptr<ExpandedNode>(nullptr);
}

const std::shared_ptr<Node> ExpandedNode::getOrigNode() const {
    return origNode;
}

void ExpandedNode::setOrigNode(std::shared_ptr<Node> origNode) {
    ExpandedNode::origNode = origNode;
}

ExpandedNode::ExpandedNode(std::shared_ptr<SubSystem> parent, std::shared_ptr<Node> orig) : SubSystem(parent), origNode(orig) {

}


xercesc::DOMElement *
ExpandedNode::emitGraphML(xercesc::DOMDocument *doc, xercesc::DOMElement *graphNode, bool include_block_node_type) {

    //Get the parameters from the orig node.  Have the original emit everything but its block type
    xercesc::DOMElement* thisNode = origNode->emitGraphML(doc, graphNode, false);

    if(include_block_node_type) {
        GraphMLHelper::addDataNode(doc, thisNode, "block_node_type", "Expanded");
    }

    emitGramphMLSubgraphAndChildren(doc, thisNode);

    return thisNode;
}