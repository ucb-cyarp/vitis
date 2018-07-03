//
// Created by Christopher Yarp on 6/26/18.
//

#include "ExpandedNode.h"

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
