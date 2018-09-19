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

MediumLevelNode::MediumLevelNode(std::shared_ptr<SubSystem> parent, MediumLevelNode* orig) : Node(parent, orig) {
    //Nothing new to copy here, just call superclass constructor
}
