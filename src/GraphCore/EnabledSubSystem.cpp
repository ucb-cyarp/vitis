//
// Created by Christopher Yarp on 6/26/18.
//

#include "EnabledSubSystem.h"

EnabledSubSystem::EnabledSubSystem() {
    enablePort = std::unique_ptr<EnablePort>(new EnablePort(this, 0)); //Don't need to do this in init as a raw pointer is passed to the port
    //However, any call to get a shared_ptr of the node or port need to be conducted after a shared pointer has returned
}

EnabledSubSystem::EnabledSubSystem(std::shared_ptr<SubSystem> parent) : SubSystem(parent) {
    enablePort = std::unique_ptr<EnablePort>(new EnablePort(this, 0)); //Don't need to do this in init as a raw pointer is passed to the port
    //However, any call to get a shared_ptr of the node or port need to be conducted after a shared pointer has returned
}

std::string EnabledSubSystem::labelStr() {
    std::string label = Node::labelStr();

    label += "\nType: Enabled Subsystem";

    return label;
}

void EnabledSubSystem::validate() {
    Node::validate();

    enablePort->validate();
}
