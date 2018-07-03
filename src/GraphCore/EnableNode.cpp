//
// Created by Christopher Yarp on 6/26/18.
//

#include "EnableNode.h"
#include "Port.h"

EnableNode::EnableNode() {

}

EnableNode::EnableNode(std::shared_ptr<SubSystem> parent) : Node(parent) {
    enablePort = Port(this, Port::PortType::ENABLE, 0); //Don't need to do this in init as a raw pointer is passed to the port
    //However, any call to get a shared_ptr of the node or port need to be conducted after a shared pointer has returned
}
