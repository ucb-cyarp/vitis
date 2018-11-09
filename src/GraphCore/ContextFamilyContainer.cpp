//
// Created by Christopher Yarp on 10/22/18.
//

#include <General/GeneralHelper.h>
#include "ContextFamilyContainer.h"
#include "PrimitiveNodes/Mux.h"
#include "EnabledSubSystem.h"

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

std::shared_ptr<ContextRoot> ContextFamilyContainer::getContextRoot() const {
    return contextRoot;
}

void ContextFamilyContainer::setContextRoot(const std::shared_ptr<ContextRoot> &contextRoot) {
    ContextFamilyContainer::contextRoot = contextRoot;
}

void ContextFamilyContainer::rewireArcsToContextFamilyContainerAndRecurse(std::vector<std::shared_ptr<Arc>> &arcs_to_delete) {
    //Itterate through the nodes in the Context Containers within this ContextFmaily container

    for(unsigned long i = 0; i<subContextContainers.size(); i++){
        std::set<std::shared_ptr<Node>> childNodes = subContextContainers[i]->getChildren();

        for(auto child = childNodes.begin(); child != childNodes.end(); child++) {
            if(GeneralHelper::isType<Node, ContextFamilyContainer>(*child) != nullptr){
                //check if the node is a ContextFamilyContainer, call recursively on it before proceeding.  This will pull arcs that should be to this
                std::shared_ptr<ContextFamilyContainer> asFamilyContainer = std::dynamic_pointer_cast<ContextFamilyContainer>(*child);
                asFamilyContainer->rewireArcsToContextFamilyContainerAndRecurse(arcs_to_delete);
            }

            //Check input arcs (including special inputs and ordering constraints)
            std::vector<std::shared_ptr<InputPort>> childInputPorts = (*child)->getInputPortsIncludingSpecialAndOrderConstraint();

            for(unsigned long portInd = 0; portInd<childInputPorts.size(); portInd++){
                std::set<std::shared_ptr<Arc>> inputPortArcs = childInputPorts[portInd]->getArcs();

                for(auto arc = inputPortArcs.begin(); arc != inputPortArcs.end(); arc++){
                    //Check if the arc has a src that is outside of the context the node is in
                    std::shared_ptr<Node> srcNode = (*arc)->getSrcPort()->getParent();
                    if(!Context::isEqOrSubContext(srcNode->getContext(), (*child)->getContext())){ //ContextFamilyContainers and ContextContainers are assigned contexts when created in the encapsulate function in design.cpp
                        if(srcNode != std::dynamic_pointer_cast<Node>(contextRoot)) { //TOOD: fix diamond inheritance issue
                            //The src to this node is coming from outside the context the node is in -> move the arc up
                            std::shared_ptr<InputPort> containerInputPort = getInputPortCreateIfNot(0);
                            (*arc)->setDstPortUpdateNewUpdatePrev(containerInputPort); //Can move because inputPortArcs is a copy of the arc list and will therefore not be effected by the move
                        }else{
                            //We currently do not allow contexts to explicitally be dependant on the output of the node that is the root of the context.
                            //If the context is a node that has its own input and output ports (ex a mux and not an enabled subsustem), this would constitute a combinational loop as delays should stop the context from expanding
                            throw std::runtime_error("Currently, nodes in a context are not permitted to be explicitally dependent on the root of that context.");
                        }
                    }
                }
            }

            //Check output arcs
            std::vector<std::shared_ptr<OutputPort>> childOutputPorts = (*child)->getOutputPortsIncludingOrderConstraint();

            for(unsigned long portInd = 0; portInd<childOutputPorts.size(); portInd++){
                std::set<std::shared_ptr<Arc>> outputPortArcs = childOutputPorts[portInd]->getArcs();

                for(auto arc = outputPortArcs.begin(); arc != outputPortArcs.end(); arc++){
                    //Check if the arc has a dest that is outside of the context of this node
                    std::shared_ptr<Node> dstNode = (*arc)->getDstPort()->getParent();
                    if(!Context::isEqOrSubContext(dstNode->getContext(), (*child)->getContext())){
                        if(dstNode != std::dynamic_pointer_cast<Node>(contextRoot)) {
                            //The dst of this node is outside of the context the node is in -> move the arc up
                            std::shared_ptr<OutputPort> containerOutputPort = getOutputPortCreateIfNot(0);
                            (*arc)->setSrcPortUpdateNewUpdatePrev(containerOutputPort); //Can move because outputPortArcs is a copy of the arc list and will therefore not be effected by the move
                        }else{
                            //The destination node is the contextRoot.  This arc can be removed (for scheduling purposes) since the subContexts will be scheduled first
                            (*arc)->setSrcPortUpdateNewUpdatePrev(nullptr);
                            (*arc)->setDstPortUpdateNewUpdatePrev(nullptr);
                            arcs_to_delete.push_back(*arc);
                        }
                    }
                }
            }
        }
    }

    //rewire the contextRoot
    std::set<std::shared_ptr<Node>> nodesToInspect;

    if(contextRoot == nullptr){
        throw std::runtime_error("Tried to Rewire ContextRoot that is null");
    }else if(GeneralHelper::isType<ContextRoot, Mux>(contextRoot) != nullptr){
        //The context root is a mux, we just schedule this
        nodesToInspect.insert(std::dynamic_pointer_cast<Mux>(contextRoot)); //TODO: fix diamond inheritance issue
    }else if(GeneralHelper::isType<ContextRoot, EnabledSubSystem>(contextRoot) != nullptr){
        //Add the subsystem and EnableNodes
        std::shared_ptr<EnabledSubSystem> asEnabledSubsystem = std::dynamic_pointer_cast<EnabledSubSystem>(contextRoot); //TODO: fix diamond inheritance issue
        nodesToInspect.insert(asEnabledSubsystem);

        //Add EnableInputs and EnableOutputs
        std::vector<std::shared_ptr<EnableInput>> enableInputs = asEnabledSubsystem->getEnableInputs();
        nodesToInspect.insert(enableInputs.begin(), enableInputs.end());

        std::vector<std::shared_ptr<EnableOutput>> enableOutputs = asEnabledSubsystem->getEnableOutputs();
        nodesToInspect.insert(enableOutputs.begin(), enableOutputs.end());

        //Verify no other children (all should have been moved into the context by now)
        if(asEnabledSubsystem->getChildren().size() != enableInputs.size() + enableOutputs.size()){
            throw std::runtime_error("When rewiring, an enabled subsystem was encountered that had nodes not yet moved into a context");
        }
    }else{
        throw std::runtime_error("When rewiring, a context root was encountered which is not yet implemented");
    }


    for(auto child = nodesToInspect.begin(); child != nodesToInspect.end(); child++) {
        //Check input arcs (including special inputs and ordering constraints)
        std::vector<std::shared_ptr<InputPort>> childInputPorts = (*child)->getInputPortsIncludingSpecialAndOrderConstraint();

        for(unsigned long portInd = 0; portInd<childInputPorts.size(); portInd++){
            std::set<std::shared_ptr<Arc>> inputPortArcs = childInputPorts[portInd]->getArcs();

            for(auto arc = inputPortArcs.begin(); arc != inputPortArcs.end(); arc++){
                //At this point, any arc connected to this nodes from inside the ContextContainers should have been deleted or an error was thrown.
                //However, this node may still have incomign arcs and outgoing arcs from/to nodes not within its context.  These arcs should be moved

                if(nodesToInspect.find((*arc)->getSrcPort()->getParent()) == nodesToInspect.end()){ //If the src node is not one of the nodes to inspect (ie. this node is not a dependency that needs to be resolved)
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
                //Check if the arc has a dest that is outside of the nodes to inspect
                if(nodesToInspect.find((*arc)->getDstPort()->getParent()) == nodesToInspect.end()){
                    //The dst of this node is outside of the context the node is in -> move the arc up
                    std::shared_ptr<OutputPort> containerOutputPort = getOutputPortCreateIfNot(0);
                    (*arc)->setSrcPortUpdateNewUpdatePrev(containerOutputPort); //Can move because outputPortArcs is a copy of the arc list and will therefore not be effected by the move
                }
            }
        }
    }
}