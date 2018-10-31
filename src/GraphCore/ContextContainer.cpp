//
// Created by Christopher Yarp on 10/22/18.
//

#include "ContextContainer.h"

Context ContextContainer::getContainerContext() const{
    return containerContext;
}

void ContextContainer::setContainerContext(const Context &context) {
    ContextContainer::containerContext = context;
}

std::vector<std::shared_ptr<Node>> ContextContainer::findStateNodesInContext() {
    std::vector<std::shared_ptr<Node>> nodesWithContext;

    for(auto child = children.begin(); child != children.end(); child++){
        if((*child)->hasState()){
            nodesWithContext.push_back(*child);
        }
    }

    return nodesWithContext;
}

ContextContainer::ContextContainer(std::shared_ptr<SubSystem> parent) : SubSystem(parent) {

}

ContextContainer::ContextContainer() {

}

ContextContainer::ContextContainer(std::shared_ptr<SubSystem> parent, ContextContainer *orig) : SubSystem(parent, orig), containerContext(orig->containerContext) {

}

std::shared_ptr<Node> ContextContainer::shallowClone(std::shared_ptr<SubSystem> parent) {
    return NodeFactory::shallowCloneNode<ContextContainer>(parent, this);
}