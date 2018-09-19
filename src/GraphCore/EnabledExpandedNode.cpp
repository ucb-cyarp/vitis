//
// Created by Christopher Yarp on 6/26/18.
//

#include "EnabledExpandedNode.h"

#include "GraphMLTools/GraphMLHelper.h"
#include "NodeFactory.h"

EnabledExpandedNode::EnabledExpandedNode() {

}

EnabledExpandedNode::EnabledExpandedNode(std::shared_ptr<SubSystem> parent) : EnabledSubSystem(parent), ExpandedNode(parent) {

}

EnabledExpandedNode::EnabledExpandedNode(std::shared_ptr<SubSystem> parent, std::shared_ptr<Node> orig, void* nop) : EnabledSubSystem(parent), ExpandedNode(parent, orig, nop) {

}


std::string EnabledExpandedNode::labelStr() {
    std::string label = Node::labelStr();

    label += "\nType: Enabled Expanded";

    return label;
}

xercesc::DOMElement *EnabledExpandedNode::emitGraphML(xercesc::DOMDocument *doc, xercesc::DOMElement *graphNode,
                                                      bool include_block_node_type) {
    //Get the parameters from the orig node.  Have the original emit everything but its block type
    xercesc::DOMElement* thisNode = origNode->emitGraphML(doc, graphNode, false);

    if(include_block_node_type) {
        GraphMLHelper::addDataNode(doc, thisNode, "block_node_type", "Expanded");
    }

    emitGramphMLSubgraphAndChildren(doc, thisNode);

    return thisNode;
}

EnabledExpandedNode::EnabledExpandedNode(std::shared_ptr<SubSystem> parent, EnabledExpandedNode* orig) : EnabledSubSystem(parent, orig), ExpandedNode(parent, orig){
    //Nothing extra to copy, call superclass constructors
}

std::shared_ptr<Node> EnabledExpandedNode::shallowClone(std::shared_ptr<SubSystem> parent) {
    return NodeFactory::shallowCloneNode<EnabledExpandedNode>(parent, this);
}
