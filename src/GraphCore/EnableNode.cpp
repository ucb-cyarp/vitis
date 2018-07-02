//
// Created by Christopher Yarp on 6/26/18.
//

#include "EnableNode.h"
#include "Port.h"

EnableNode::EnableNode() {

}

EnableNode::EnableNode(std::shared_ptr<SubSystem> parent) : Node(parent) {

}

void EnableNode::init() {
    Node::init();

    enablePort = Port(shared_from_this(), Port::PortType::ENABLE, 0);
}
