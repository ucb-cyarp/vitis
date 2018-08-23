//
// Created by Christopher Yarp on 8/22/18.
//

#include "HighLevelNode.h"

HighLevelNode::HighLevelNode() {

}

HighLevelNode::HighLevelNode(std::shared_ptr<SubSystem> parent) : Node(parent) {

}

bool HighLevelNode::canExpand() {
    return true;
}
