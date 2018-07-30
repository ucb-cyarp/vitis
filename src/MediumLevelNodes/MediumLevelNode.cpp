//
// Created by Christopher Yarp on 7/17/18.
//

#include "MediumLevelNode.h"

MediumLevelNode::MediumLevelNode() {

}

MediumLevelNode::MediumLevelNode(std::shared_ptr<SubSystem> parent) : Node(parent) {

}

bool MediumLevelNode::canExpand() {
    return true;
}
