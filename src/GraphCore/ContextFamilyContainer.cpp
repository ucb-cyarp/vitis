//
// Created by Christopher Yarp on 10/22/18.
//

#include <General/GeneralHelper.h>
#include "ContextFamilyContainer.h"
#include "PrimitiveNodes/Mux.h"
#include "EnabledSubSystem.h"
#include "General/ErrorHelpers.h"

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

ContextFamilyContainer::ContextFamilyContainer(std::shared_ptr<SubSystem> parent, ContextFamilyContainer *orig) : SubSystem(parent, orig){
    //Since the rest of the node data are pointers to nodes, they need to be copied in the clone method where the cloned node pointers are known
}

std::shared_ptr<Node> ContextFamilyContainer::shallowClone(std::shared_ptr<SubSystem> parent) {
    return NodeFactory::shallowCloneNode<ContextFamilyContainer>(parent, this);
}

void ContextFamilyContainer::shallowCloneWithChildren(std::shared_ptr<SubSystem> parent,
                                                      std::vector<std::shared_ptr<Node>> &nodeCopies,
                                                      std::map<std::shared_ptr<Node>, std::shared_ptr<Node>> &origToCopyNode,
                                                      std::map<std::shared_ptr<Node>, std::shared_ptr<Node>> &copyToOrigNode) {

    //Because of sibbling context family containers, it is possible for the clone method to be called on a ContextFamilyContainer
    //more than once: once because it is a node in the graph and once because it is a sibbling.  We do not want the node cloned
    //more than once but we also don't want to introduce some state which identifies if it has been cloned before since we may
    //want to clone a design more than once.  We therefore check if the ContextFamilyContainer appears in the origToCopyNode map.
    //If it does, it means it has already been cloned and no action is taken here.  Otherwise, it is cloned.

    if(origToCopyNode.find(getSharedPointer()) == origToCopyNode.end()) {
        SubSystem::shallowCloneWithChildren(parent, nodeCopies, origToCopyNode, copyToOrigNode);

        //Copy the containerOrder vector with the cloned versions
        std::shared_ptr<ContextFamilyContainer> nodeCopy = std::dynamic_pointer_cast<ContextFamilyContainer>(
                origToCopyNode[getSharedPointer()]);

        std::vector<std::shared_ptr<ContextContainer>> subContextContainersCopy;

        for (unsigned long i = 0; i < subContextContainers.size(); i++) {
            subContextContainersCopy.push_back(
                    std::dynamic_pointer_cast<ContextContainer>(origToCopyNode[subContextContainers[i]]));
        }

        nodeCopy->setSubContextContainers(subContextContainersCopy);

        //Copy siblings
        //Note, there is a protection against double duplication of ContextFamilyContainers (and their children) provided above
        std::map<int, std::shared_ptr<ContextFamilyContainer>> sibblingMap;
        for (auto it = siblingContainers.begin(); it != siblingContainers.end(); it++) {
            int part = it->first;
            std::shared_ptr<ContextFamilyContainer> origContextFamilyContainer = it->second;
            std::shared_ptr<SubSystem> origContextFamilyContainerParent = origContextFamilyContainer->getParent();

            std::shared_ptr<Node> copyContextFamilyContainerParentNode = origToCopyNode[origContextFamilyContainerParent];
            if(copyContextFamilyContainerParentNode == nullptr){
                ErrorHelpers::genErrorStr("Error when cloning ContextFamilyContainer.  Sibling Parent Not Found: ", getSharedPointer());
            }
            std::shared_ptr<SubSystem> copyContextFamilyContainerParent = std::dynamic_pointer_cast<SubSystem>(copyContextFamilyContainerParentNode);

            origContextFamilyContainer->shallowCloneWithChildren(copyContextFamilyContainerParent, nodeCopies, origToCopyNode, copyToOrigNode);

            std::shared_ptr<Node> clonesContextFamilyContainer = origToCopyNode[origContextFamilyContainer];

            if (GeneralHelper::isType<Node, ContextFamilyContainer>(clonesContextFamilyContainer) == nullptr) {
                throw std::runtime_error(
                        ErrorHelpers::genErrorStr("Error when cloning ContextFamilyContainer.  Sibling Not Found: ", getSharedPointer()));
            }

            sibblingMap[part] = std::dynamic_pointer_cast<ContextFamilyContainer>(clonesContextFamilyContainer);
        }

        nodeCopy->setSiblingContainers(sibblingMap);
    }
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
                    //All nodes in a ContextFamilyContainer should have the same partition number
                    if((!Context::isEqOrSubContext(srcNode->getContext(), (*child)->getContext())) || (srcNode->getPartitionNum() != (*child)->getPartitionNum())){ //ContextFamilyContainers and ContextContainers are assigned contexts when created in the encapsulate function in design.cpp
                        if(srcNode != std::dynamic_pointer_cast<Node>(contextRoot)) { //TOOD: fix diamond inheritance issue
                            //The src to this node is coming from outside the context the node is in (or is in another partition) -> move the arc up
                            std::shared_ptr<InputPort> containerInputPort = getInputPortCreateIfNot(0);
                            (*arc)->setDstPortUpdateNewUpdatePrev(containerInputPort); //Can move because inputPortArcs is a copy of the arc list and will therefore not be effected by the move
                        }else{
                            //We currently do not allow contexts to explicitally be dependant on the output of the node that is the root of the context.
                            //If the context is a node that has its own input and output ports (ex a mux and not an enabled subsustem), this would constitute a combinational loop as delays should stop the context from expanding
                            throw std::runtime_error(ErrorHelpers::genErrorStr("Currently, nodes in a context are not permitted to be explicitally dependent on the root of that context.", getSharedPointer()));
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
                    if((!Context::isEqOrSubContext(dstNode->getContext(), (*child)->getContext())) || (dstNode->getPartitionNum() != (*child)->getPartitionNum())){
                        if(dstNode != std::dynamic_pointer_cast<Node>(contextRoot)) {
                            //The dst of this node is outside of the context the node is in (or is in a different partition) -> move the arc up
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
    //TODO: FIX after adding new context family containers
    //Should only include the

    std::set<std::shared_ptr<Node>> nodesToInspect;

    if(contextRoot == nullptr){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Tried to Rewire ContextRoot that is null", getSharedPointer()));
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
            throw std::runtime_error(ErrorHelpers::genErrorStr("When rewiring, an enabled subsystem was encountered that had nodes not yet moved into a context", getSharedPointer()));
        }
    }else{
        throw std::runtime_error(ErrorHelpers::genErrorStr("When rewiring, a context root was encountered which is not yet implemented", getSharedPointer()));
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

std::set<GraphMLParameter> ContextFamilyContainer::graphMLParameters() {
    std::set<GraphMLParameter> parameters;
    return parameters;
}

xercesc::DOMElement *
ContextFamilyContainer::emitGraphML(xercesc::DOMDocument *doc, xercesc::DOMElement *graphNode, bool include_block_node_type) {
    //Create Node
    xercesc::DOMElement* thisNode = SubSystem::emitGraphML(doc, graphNode, false);

    GraphMLHelper::addDataNode(doc, thisNode, "block_function", "ContextFamilyContainer");

    return thisNode;
}

const std::map<int, std::shared_ptr<ContextFamilyContainer>> ContextFamilyContainer::getSiblingContainers() const {
    return siblingContainers;
}

void ContextFamilyContainer::setSiblingContainers(
        const std::map<int, std::shared_ptr<ContextFamilyContainer>> &siblingContainers) {
    ContextFamilyContainer::siblingContainers = siblingContainers;
}

std::string ContextFamilyContainer::typeNameStr(){
    return "ContextFamilyContainer";
}

std::shared_ptr<DummyReplica> ContextFamilyContainer::getDummyNode() const {
    return dummyNode;
}

void ContextFamilyContainer::setDummyNode(const std::shared_ptr<DummyReplica> &dummyNode) {
    ContextFamilyContainer::dummyNode = dummyNode;
}

std::string ContextFamilyContainer::getFullyQualifiedOrigName(bool sanitize, std::string delim) {
    std::shared_ptr<Node> asNode = GeneralHelper::isType<ContextRoot, Node>(contextRoot);

    if(asNode){
        std::string origName = asNode->getFullyQualifiedOrigName(sanitize, delim);
        if(!origName.empty()){
            origName += delim;
        }
        origName += typeNameStr();
        return origName;
    }else{
        throw std::runtime_error(ErrorHelpers::genErrorStr("Context Root was not a node", getSharedPointer()));
    }
}
