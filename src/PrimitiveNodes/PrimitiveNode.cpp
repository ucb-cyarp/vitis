//
// Created by Christopher Yarp on 7/17/18.
//

#include "PrimitiveNode.h"

PrimitiveNode::PrimitiveNode() {

}

PrimitiveNode::PrimitiveNode(std::shared_ptr<SubSystem> parent) : Node(parent) {

}

bool PrimitiveNode::canExpand() {
    return false;
}

PrimitiveNode::PrimitiveNode(std::shared_ptr<SubSystem> parent, Node* orig) : Node(parent, orig) {
    //Nothing new to copy
}
