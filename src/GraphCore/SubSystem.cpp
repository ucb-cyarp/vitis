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

//Collect the parameters from the child nodes
std::set<GraphMLParameter> SubSystem::graphMLParameters() {
    //This node has no parameters itself, however, its children may
    std::set<GraphMLParameter> parameters;

    for(std::set<std::shared_ptr<Node>>::iterator child = children.begin(); child != children.end(); child++){
        //insert the parameters from the child into the set
        std::set<GraphMLParameter> childParameters = (*child)->graphMLParameters();
        parameters.insert(childParameters.begin(), childParameters.end());
    }

    return parameters;
}

