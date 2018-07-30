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
