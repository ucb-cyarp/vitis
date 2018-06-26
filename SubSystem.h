//
// Created by Christopher Yarp on 6/25/18.
//

#ifndef VITIS_SUBSYSTEM_H
#define VITIS_SUBSYSTEM_H

#include "Node.h"
#include <vector>

class SubSystem : Node{
protected:
    std::vector<Node> children;


};


#endif //VITIS_SUBSYSTEM_H
