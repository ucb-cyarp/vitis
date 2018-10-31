//
// Created by Christopher Yarp on 10/22/18.
//

#include <General/GeneralHelper.h>
#include "ContextFamilyContainer.h"

std::vector<std::shared_ptr<ContextContainer>> ContextFamilyContainer::getSubContextContainers() const {
    return subContextContainers;
}

void ContextFamilyContainer::setSubContextContainers(const std::vector<std::shared_ptr<ContextContainer>> &subContextContainers) {
    ContextFamilyContainer::subContextContainers = subContextContainers;
}

ContextFamilyContainer::ContextFamilyContainer() {

}

ContextFamilyContainer::ContextFamilyContainer(std::shared_ptr<SubSystem> parent) : SubSystem(parent) {

}

ContextFamilyContainer::ContextFamilyContainer(std::shared_ptr<SubSystem> parent, ContextFamilyContainer *orig) {

}

std::shared_ptr<Node> ContextFamilyContainer::shallowClone(std::shared_ptr<SubSystem> parent) {
    return NodeFactory::shallowCloneNode<ContextFamilyContainer>(parent, this);
}

void ContextFamilyContainer::shallowCloneWithChildren(std::shared_ptr<SubSystem> parent,
                                                      std::vector<std::shared_ptr<Node>> &nodeCopies,
                                                      std::map<std::shared_ptr<Node>, std::shared_ptr<Node>> &origToCopyNode,
                                                      std::map<std::shared_ptr<Node>, std::shared_ptr<Node>> &copyToOrigNode) {
    SubSystem::shallowCloneWithChildren(parent, nodeCopies, origToCopyNode, copyToOrigNode);

    //Copy the containerOrder vector with the cloned versions
    std::shared_ptr<ContextFamilyContainer> nodeCopy = std::dynamic_pointer_cast<ContextFamilyContainer>(origToCopyNode[getSharedPointer()]);

    std::vector<std::shared_ptr<ContextContainer>> subContextContainersCopy;

    for(unsigned long i = 0; i<subContextContainers.size(); i++){
        subContextContainersCopy.push_back(std::dynamic_pointer_cast<ContextContainer>(origToCopyNode[subContextContainers[i]]));
    }

    nodeCopy->setSubContextContainers(subContextContainersCopy);
}

std::shared_ptr<ContextContainer> ContextFamilyContainer::getSubContextContainer(unsigned long subContext) {
    return subContextContainers[subContext];
}

void ContextFamilyContainer::rewireArcsToContextFamilyContainerAndRecurse() {
    //Itterate through the nodes in the Context Containers within this ContextFmaily container

    for(unsigned long i = 0; i<subContextContainers.size(); i++){
        std::set<std::shared_ptr<Node>> childNodes = subContextContainers[i]->getChildren();

        for(auto child = childNodes.begin(); child != childNodes.end(); child++) {
            if(GeneralHelper::isType<Node, ContextFamilyContainer>(*child) != nullptr){
                //check if the node is a ContextFamilyContainer, call recursively on it before proceeding.  This will pull arcs that should be to this
                std::shared_ptr<ContextFamilyContainer> asFamilyContainer = std::dynamic_pointer_cast<ContextFamilyContainer>(*child);
                asFamilyContainer->rewireArcsToContextFamilyContainerAndRecurse();
            }

            //Check input arcs (including special inputs and ordering constraints)
            std::vector<std::shared_ptr<InputPort>> childInputPorts = (*child)->getInputPortsIncludingSpecialAndOrderConstraint();

            for(unsigned long portInd = 0; portInd<childInputPorts.size(); portInd++){
                std::set<std::shared_ptr<Arc>> inputPortArcs = childInputPorts[portInd]->getArcs();

                for(auto arc = inputPortArcs.begin(); arc != inputPortArcs.end(); arc++){
                    //Check if the arc has a src that is outside of the context the node is in
                    if(!Context::isEqOrSubContext((*arc)->getSrcPort()->getParent()->getContext(), (*child)->getContext())){ //ContextFamilyContainers and ContextContainers are assigned contexts when created in the encapsulate function in design.cpp
                        //The src to this node is coming from outside the context the node is in -> move the arc up
                        std::shared_ptr<InputPort> containerInputPort = getInputPortCreateIfNot(0);
                        (*arc)->setDstPortUpdateNewUpdatePrev(containerInputPort); //Can move because inputPortArcs is a copy of the arc list and will therefore not be effected by the move
                    }
                }
            }

            //Check output arcs
            std::vector<std::shared_ptr<OutputPort>> childOutputPorts = (*child)->getOutputPortsIncludingOrderConstraint();

            for(unsigned long portInd = 0; portInd<childOutputPorts.size(); portInd++){
                std::set<std::shared_ptr<Arc>> outputPortArcs = childOutputPorts[portInd]->getArcs();

                for(auto arc = outputPortArcs.begin(); arc != outputPortArcs.end(); arc++){
                    //Check if the arc has a dest that is outside of the context of this node
                    if(!Context::isEqOrSubContext((*arc)->getDstPort()->getParent()->getContext(), (*child)->getContext())){
                        //The dst of this node is outside of the context the node is in -> move the arc up
                        std::shared_ptr<OutputPort> containerOutputPort = getOutputPortCreateIfNot(0);
                        (*arc)->setSrcPortUpdateNewUpdatePrev(containerOutputPort); //Can move because outputPortArcs is a copy of the arc list and will therefore not be effected by the move
                    }
                }
            }
        }
    }
}
