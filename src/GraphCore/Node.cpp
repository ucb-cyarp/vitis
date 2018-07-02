//
// Created by Christopher Yarp on 6/25/18.
//

#include <vector>
#include <memory>

#include "SubSystem.h"
#include "Port.h"
#include "Node.h"

Node::Node() : id(-1), partitionNum(0)
{
    parent = std::shared_ptr<SubSystem>(nullptr);

    //NOTE: CANNOT init ports here since
}

Node::Node(std::shared_ptr<SubSystem> parent) : id(-1), partitionNum(0), parent(parent) { }

void Node::init() {
    //Nothing required for this case since ports are the only thing that require this and generic nodes are initialized with no ports
}


