//
// Created by Christopher Yarp on 6/26/18.
//

#include "EnabledSubSystem.h"

EnabledSubSystem::EnabledSubSystem() {

}

EnabledSubSystem::EnabledSubSystem(std::shared_ptr<SubSystem> parent) : SubSystem(parent) {

}

void EnabledSubSystem::init() {
    Node::init();

    enablePort = Port(shared_from_this(), Port::PortType::ENABLE, 0);
}
