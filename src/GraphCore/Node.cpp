//
// Created by Christopher Yarp on 6/25/18.
//

#include <vector>
#include <memory>

#include "Node.h"
#include "GraphCore/SubSystem.h"
#include "Port.h"

Node::Node() {
    id = -1;
    parent = std::shared_ptr<SubSystem>(nullptr);
    partitionNum = 0;
}
