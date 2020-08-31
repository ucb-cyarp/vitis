//
// Created by Christopher Yarp on 8/31/20.
//

#include "PartitionNode.h"

PartitionNode::PartitionNode() : Node() {

}

PartitionNode::PartitionNode(std::shared_ptr<SubSystem> parent) : Node(parent) {

}

PartitionNode::PartitionNode(std::shared_ptr<SubSystem> parent, PartitionNode *orig) : Node(parent, orig) {

}

std::string PartitionNode::typeNameStr() {
    return "PartitionNode";
}

std::string PartitionNode::labelStr() {
    std::string label = Node::labelStr();

    label += "\nFunction: " + typeNameStr() + "\nPartition:" + GeneralHelper::to_string(partitionNum);

    return label;
}

std::shared_ptr<Node> PartitionNode::shallowClone(std::shared_ptr<SubSystem> parent) {
    return NodeFactory::shallowCloneNode<PartitionNode>(parent, this);
}

xercesc::DOMElement *
PartitionNode::emitGraphML(xercesc::DOMDocument *doc, xercesc::DOMElement *graphNode, bool include_block_node_type) {
    xercesc::DOMElement* thisNode = emitGraphMLBasics(doc, graphNode);
    if(include_block_node_type) {
        GraphMLHelper::addDataNode(doc, thisNode, "block_node_type", "Standard");
    }

    GraphMLHelper::addDataNode(doc, thisNode, "block_function", "PartitionNode");

    return thisNode;
}

bool PartitionNode::canExpand() {
    return false;
}
