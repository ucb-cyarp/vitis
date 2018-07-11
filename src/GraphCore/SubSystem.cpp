//
// Created by Christopher Yarp on 6/25/18.
//

#include "SubSystem.h"

SubSystem::SubSystem() {

}

void SubSystem::addChild(std::shared_ptr<Node> child) {
    children.insert(child);
}

void SubSystem::removeChild(std::shared_ptr<Node> child) {
    children.insert(child);
}

SubSystem::SubSystem(std::shared_ptr<SubSystem> parent) : Node(parent) {

}

