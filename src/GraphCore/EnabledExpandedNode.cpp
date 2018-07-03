//
// Created by Christopher Yarp on 6/26/18.
//

#include "EnabledExpandedNode.h"

EnabledExpandedNode::EnabledExpandedNode() {

}

EnabledExpandedNode::EnabledExpandedNode(std::shared_ptr<SubSystem> parent) : EnabledSubSystem(parent), ExpandedNode(parent) {

}

EnabledExpandedNode::EnabledExpandedNode(std::shared_ptr<SubSystem> parent, std::shared_ptr<Node> orig) : EnabledSubSystem(parent), ExpandedNode(parent, orig) {

}
