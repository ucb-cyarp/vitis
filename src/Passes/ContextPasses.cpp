//
// Created by Christopher Yarp on 9/2/21.
//

#include "ContextPasses.h"
#include "General/GraphAlgs.h"
#include "PrimitiveNodes/Mux.h"
#include "General/ErrorHelpers.h"
#include "GraphCore/DummyReplica.h"

void ContextPasses::discoverAndMarkContexts(Design &design) {
    std::vector<std::shared_ptr<Mux>> discoveredMuxes;
    std::vector<std::shared_ptr<EnabledSubSystem>> discoveredEnabledSubsystems;
    std::vector<std::shared_ptr<ClockDomain>> discoveredClockDomains;
    std::vector<std::shared_ptr<Node>> discoveredGeneral;

    //The top level context stack has no entries
    std::vector<Context> initialStack;

    //Discover contexts at the top layer (and below non-enabled subsystems).  Also set the contexts of these top level nodes
    GraphAlgs::discoverAndUpdateContexts(design.getTopLevelNodes(), initialStack, discoveredMuxes, discoveredEnabledSubsystems,
                                         discoveredClockDomains, discoveredGeneral);

    //Add context nodes (muxes, enabled subsystems, and clock domains) to the topLevelContextRoots list
    for(unsigned long i = 0; i<discoveredMuxes.size(); i++){
        design.addTopLevelContextRoot(discoveredMuxes[i]);
    }
    for(unsigned long i = 0; i<discoveredEnabledSubsystems.size(); i++){
        design.addTopLevelContextRoot(discoveredEnabledSubsystems[i]);
    }
    for(unsigned long i = 0; i<discoveredClockDomains.size(); i++){
        std::shared_ptr<ContextRoot> clkDomainAsContextRoot = std::dynamic_pointer_cast<ContextRoot>(discoveredClockDomains[i]);
        if(clkDomainAsContextRoot == nullptr){
            throw std::runtime_error(ErrorHelpers::genErrorStr("Discovered a clock domain that is not a ContextRoot when discovering and marking contexts", discoveredClockDomains[i]));
        }
        design.addTopLevelContextRoot(clkDomainAsContextRoot);
    }

    //Get and mark the Mux contexts
    Mux::discoverAndMarkMuxContextsAtLevel(discoveredMuxes);

    //Recursivly call on the discovered enabled subsystems
    for(unsigned long i = 0; i<discoveredEnabledSubsystems.size(); i++) {
        discoveredEnabledSubsystems[i]->discoverAndMarkContexts(initialStack);
    }

    //Recursivly call on the discovered clock domains
    for(unsigned long i = 0; i<discoveredClockDomains.size(); i++) {
        discoveredClockDomains[i]->discoverAndMarkContexts(initialStack);
    }

}

void ContextPasses::expandEnabledSubsystemContexts(Design &design){
    std::vector<std::shared_ptr<Node>> newNodes;
    std::vector<std::shared_ptr<Node>> deletedNodes;
    std::vector<std::shared_ptr<Arc>> newArcs;
    std::vector<std::shared_ptr<Arc>> deletedArcs;

    std::vector<std::shared_ptr<SubSystem>> childSubsystems;
    std::vector<std::shared_ptr<Node>> topLevelNodes = design.getTopLevelNodes();
    for(auto topLvlNode = topLevelNodes.begin(); topLvlNode != topLevelNodes.end(); topLvlNode++){
        if(GeneralHelper::isType<Node, SubSystem>(*topLvlNode) != nullptr){
            childSubsystems.push_back(std::dynamic_pointer_cast<SubSystem>(*topLvlNode));
        }
    }

    for(unsigned long i = 0; i<childSubsystems.size(); i++){
        //Adding a condition to check if the subsystem still has this node as parent.
        //TODO: remove condition if subsystems are never moved
        if(childSubsystems[i]->getParent() == nullptr){ //We are at the top level
            childSubsystems[i]->extendEnabledSubsystemContext(newNodes, deletedNodes, newArcs, deletedArcs);
        }else{
            throw std::runtime_error("Subsystem moved during enabled subsystem context expansion.  This was unexpected.");
        }
    }

    design.addRemoveNodesAndArcs(newNodes, deletedNodes, newArcs, deletedArcs);
}

void ContextPasses::createEnabledOutputsForEnabledSubsysVisualization(Design &design) {
    //Find in nodes that have arcs to the Vis Master
    std::set<std::shared_ptr<Arc>> visDriverArcsPtrOrder = design.getVisMaster()->getInputArcs();
    std::set<std::shared_ptr<Arc>, Arc::PtrID_Compare> visDriverArcs; //Order by ID for consistency across runs
    visDriverArcs.insert(visDriverArcsPtrOrder.begin(), visDriverArcsPtrOrder.end());

    for(auto driverArc = visDriverArcs.begin(); driverArc != visDriverArcs.end(); driverArc++){
        std::shared_ptr<Node> srcNode = (*driverArc)->getSrcPort()->getParent();

        std::shared_ptr<OutputPort> portToRewireTo = (*driverArc)->getSrcPort();

        //Run through the hierarchy of the node (context stack not built yet)

        for(std::shared_ptr<SubSystem> parent = srcNode->getParent(); parent != nullptr; parent=parent->getParent()){
            //Check if the context root is an enabled subsystem
            std::shared_ptr<EnabledSubSystem> contextAsEnabledSubsys = GeneralHelper::isType<SubSystem, EnabledSubSystem>(parent);

            if(contextAsEnabledSubsys){
                //This is an enabled subsystem context, create an EnableOutput port for this vis
                std::shared_ptr<EnableOutput> visOutput = NodeFactory::createNode<EnableOutput>(contextAsEnabledSubsys);
                visOutput->setPartitionNum(contextAsEnabledSubsys->getPartitionNum());
                contextAsEnabledSubsys->addEnableOutput(visOutput);
                design.addNode(visOutput);

                visOutput->setName("VisEnableOutput");

                std::shared_ptr<Arc> enableDriverArcOrig = contextAsEnabledSubsys->getEnableDriverArc();
                //Copy enable driver arc
                std::shared_ptr<Arc> newEnDriverArc = Arc::connectNodes(enableDriverArcOrig->getSrcPort(), visOutput, enableDriverArcOrig->getDataType(), enableDriverArcOrig->getSampleTime());
                design.addArc(newEnDriverArc);

                //Create a new node from the portToRewireTo to this new output node
                std::shared_ptr<Arc> newStdDriverArc = Arc::connectNodes(portToRewireTo, visOutput->getInputPortCreateIfNot(0), (*driverArc)->getDataType(), (*driverArc)->getSampleTime());
                design.addArc(newStdDriverArc);

                //Update the portToRewireTo to be the output port of this node.
                portToRewireTo = visOutput->getOutputPortCreateIfNot(0);
            }
        }

        //rewire the arc to the nodeToRewireTo
        (*driverArc)->setSrcPortUpdateNewUpdatePrev(portToRewireTo);

    }
}

void ContextPasses::encapsulateContexts(Design &design) {
    //context expansion stops at enabled subsystem boundaries.  Therefore, we cannot have enabled subsystems nested in muxes
    //However, we can have enabled subsystems nested in enabled subsystems, muxes nested in enabled subsystems, and muxes nested in muxes

    //**** <Change>Do this for all context roots</Change> *****
    //<Delete>Start at the top level of the hierarchy and find the nodes with context</Delete>

    //Create the appropriate ContextContainer of ContextFamilyContainer for each and create a map of nodes to ContextContainer
    std::vector<std::shared_ptr<ContextRoot>> contextRootNodes;
    std::vector<std::shared_ptr<Node>> nodesInContext;

    { //Open scope to get nodes
        std::vector<std::shared_ptr<Node>> nodes = design.getNodes();
        for (unsigned long i = 0; i < nodes.size(); i++) {
            if (GeneralHelper::isType<Node, ContextRoot>(nodes[i]) != nullptr) {
                contextRootNodes.push_back(std::dynamic_pointer_cast<ContextRoot>(nodes[i]));
            }

            //A context root can also be in a context
            if (nodes[i]->getContext().size() > 0) {
                nodesInContext.push_back(nodes[i]);
            }
        }
    }

    //***** Create set from all context nodes ****

    //<Delete>
    //Create a set of nodes in contexts.  Creating the set prevents redundant operations since nodes can be in multiple contexts.
    //Itterate though the set and move the nodes into the corresponding subsystem. (add as a child)
    //Note that context nodes in enabled subsystems are not added.  They are added by the enabled subsystem recursion


    //Recurse into the enabled subsytems and repeat

    //Create a helper function which can be called upon here and in the enabled subsystem version.  This function will take the list of nodes with context within it
    //In the enabled subsystem version, a difference is that the enabled subsystem has nodes in its own context that may not be in mux contexts
    //</Delete>

    //Move context nodes under ContextContainers
    for(unsigned long i = 0; i<nodesInContext.size(); i++){
        std::shared_ptr<SubSystem> origParent = nodesInContext[i]->getParent();

        if(origParent == nullptr) {
            //If has no parent, remove from topLevelNodes
            design.removeTopLevelNode(nodesInContext[i]);

            if(GeneralHelper::isType<Node, ContextRoot>(nodesInContext[i]) != nullptr){
                //If is a context root, remove from top level context roots
                std::shared_ptr<ContextRoot> toRemove = std::dynamic_pointer_cast<ContextRoot>(nodesInContext[i]);
                contextRootNodes.erase(std::remove(contextRootNodes.begin(), contextRootNodes.end(), toRemove), contextRootNodes.end());
            }
        }else{
            //Remove from orig parent
            origParent->removeChild(nodesInContext[i]);
        }

        //Discover appropriate (sub)context container
        std::vector<Context> contextStack = nodesInContext[i]->getContext();

        //Create a ContextFamilyContainer for this partition at each level of the context stack if one has not already been created
        //TODO: refactor
        for(int j = 0; j<contextStack.size()-1; j++){ //-1 because the inner context is handled explicitally below
            int partitionNum = nodesInContext[i]->getPartitionNum();
            getContextFamilyContainerCreateIfNotNoParent(design, contextStack[j].getContextRoot(), partitionNum);
        }

        Context innerContext = contextStack[contextStack.size()-1];
        std::shared_ptr<ContextFamilyContainer> contextFamily = getContextFamilyContainerCreateIfNotNoParent(design, innerContext.getContextRoot(), nodesInContext[i]->getPartitionNum());
        std::shared_ptr<ContextContainer> contextContainer = contextFamily->getSubContextContainer(innerContext.getSubContext());

        //Set new parent to be the context container.  Also add it as a child of the context container.
        //Note, removed from previous parent above
        contextContainer->addChild(nodesInContext[i]);
        nodesInContext[i]->setParent(contextContainer);
    }

    //Move context roots into their ContextFamilyContainers.  Note, the ContextStack of the ContextRootNode does not include its own context
    for(unsigned long i = 0; i<contextRootNodes.size(); i++){
        std::shared_ptr<Node> asNode = std::dynamic_pointer_cast<Node>(contextRootNodes[i]); //TODO: fix diamond inheritance issue
        std::shared_ptr<SubSystem> origParent = asNode->getParent();

        if(origParent != nullptr){
            origParent->removeChild(asNode);
        }else{
            //If has no parent, remove from topLevelNodes
            design.removeTopLevelNode(asNode);
        }

        std::shared_ptr<ContextFamilyContainer> contextFamilyContainer = getContextFamilyContainerCreateIfNotNoParent(design, contextRootNodes[i], asNode->getPartitionNum());
        contextFamilyContainer->addChild(asNode);
        contextFamilyContainer->setContextRoot(contextRootNodes[i]);
        asNode->setParent(contextFamilyContainer);

        //Also move dummy nodes of this ContextRoot into their relevent ContextFamilyContainers
        std::map<int, std::shared_ptr<DummyReplica>> dummyReplicas = contextRootNodes[i]->getDummyReplicas();
        for(auto dummyPair = dummyReplicas.begin(); dummyPair != dummyReplicas.end(); dummyPair++){
            int dummyPartition = dummyPair->first;
            std::shared_ptr<DummyReplica> dummyNode = dummyPair->second;
            std::shared_ptr<SubSystem> dummyOrigParent = dummyNode->getParent();

            //Move the dummy node
            if(dummyOrigParent != nullptr){
                dummyOrigParent->removeChild(dummyNode);
            }

            std::shared_ptr<ContextFamilyContainer> contextFamilyContainer = getContextFamilyContainerCreateIfNotNoParent(design, contextRootNodes[i], dummyPartition);
            contextFamilyContainer->addChild(dummyNode);
            //Set as the dummy node for ContextFamilyContainer
            contextFamilyContainer->setDummyNode(dummyNode);
            dummyNode->setParent(contextFamilyContainer);
        }
    }

    //Iterate through the ContextFamilyContainers (which should all be created now) and set the parent based on the context.
    for(unsigned long i = 0; i<contextRootNodes.size(); i++){
        //Get the context stack
        std::shared_ptr<ContextRoot> asContextRoot = contextRootNodes[i];
        std::shared_ptr<Node> asNode = std::dynamic_pointer_cast<Node>(asContextRoot); //TODO: Fix inheritance diamond issue

        std::vector<Context> context = asNode->getContext();
        if(context.size() > 0){
            //This node is in a context, move it's containers under the appropriate container.
            Context innerContext = context[context.size()-1];

            //Itterate through the ContextFamilyContainers for this ContextRoot
            std::map<int, std::shared_ptr<ContextFamilyContainer>> contextFamilyContainers = asContextRoot->getContextFamilyContainers();
            for(auto it = contextFamilyContainers.begin(); it != contextFamilyContainers.end(); it++){
                std::map<int, std::shared_ptr<ContextFamilyContainer>> newParentFamilyContainers = innerContext.getContextRoot()->getContextFamilyContainers();
                if(newParentFamilyContainers.find(it->second->getPartitionNum()) == newParentFamilyContainers.end()){
                    throw std::runtime_error(ErrorHelpers::genErrorStr("Error while encapsulating contexts.  Unable to find parent ContextFamilyContainer for specified partition", asNode));
                }
                std::shared_ptr<ContextContainer> newParent = newParentFamilyContainers[it->second->getPartitionNum()]->getSubContextContainer(innerContext.getSubContext());

                std::shared_ptr<ContextFamilyContainer> toBeMoved = it->second;

                if(toBeMoved->getParent()){
                    toBeMoved->getParent()->removeChild(toBeMoved);
                }
                toBeMoved->setParent(newParent);
                newParent->addChild(toBeMoved);
            }

        }else{
            //This node is not in a context, its ContextFamily containers should exist at the top level
            std::map<int, std::shared_ptr<ContextFamilyContainer>> contextFamilyContainers = asContextRoot->getContextFamilyContainers();
            for(auto it = contextFamilyContainers.begin(); it != contextFamilyContainers.end(); it++) {
                design.addTopLevelNode(it->second);
            }
        }
    }
}

void ContextPasses::createContextVariableUpdateNodes(Design &design, bool includeContext) {
    //Find nodes with state in design
    std::vector<std::shared_ptr<ContextRoot>> contextRoots = design.findContextRoots();

    for(unsigned long i = 0; i<contextRoots.size(); i++){
        std::vector<std::shared_ptr<Node>> newNodes;
        std::vector<std::shared_ptr<Node>> deletedNodes;
        std::vector<std::shared_ptr<Arc>> newArcs;
        std::vector<std::shared_ptr<Arc>> deletedArcs;

        contextRoots[i]->createContextVariableUpdateNodes(newNodes, deletedNodes, newArcs, deletedArcs, includeContext);

        design.addRemoveNodesAndArcs(newNodes, deletedNodes, newArcs, deletedArcs);
    }
}

void ContextPasses::rewireArcsToContexts(Design &design, std::vector<std::shared_ptr<Arc>> &origArcs, std::vector<std::vector<std::shared_ptr<Arc>>> &contextArcs) {
    //First, let's rewire the ContextRoot driver arcs
    std::vector<std::shared_ptr<ContextRoot>> contextRoots = design.findContextRoots();
    std::set<std::shared_ptr<Arc>, Arc::PtrID_Compare> contextRootOrigArcs, contextRootRewiredArcs; //Want Arc orders to be consistent across runs

    //For each discovered context root
    //Mark the driver arcs to be removed because order constraint arcs representing the drivers for each partition's
    //ContextFamilyContainer should have already been created durring encapsulation.  This should include an arc to the
    //ContextFamilyContainer in the same partition as the ContextRoot.  In the past, this function would create replicas
    //of the context driver arcs for each partition.  However, that could result in undesierable behavior if a context
    //is split across partitions.  Normally, unless the ContextRoot requests driver node replication, FIFOs are inserted
    //at the order constraint arcs introduced during encapsulation that span partitions.  Arcs after these FIFOs are
    //removed before this function is called as part of scheduling because the FIFOs are stateful nodes with no
    //cobinational path.  This prevents the partition context drivers from becoming false cross partition dependencies

    //NOTE: Clock domain

    //Mark the origional (before encapsulation) ContextDriverArcs for deletion
    for(unsigned long i = 0; i<contextRoots.size(); i++){
        std::shared_ptr<ContextRoot> contextRoot = contextRoots[i];
        std::vector<std::shared_ptr<Arc>> driverArcs = contextRoot->getContextDecisionDriver();

        for(unsigned long j = 0; j<driverArcs.size(); j++){
            //The driver arcs must have the destination re-wired.  At a minimum, they should be rewired to
            //the context container node.  However, if they exist inside of a nested context, the destination
            //may need to be rewired to another context family container higher in the hierarchy

            //TODO: Remove check
            if(driverArcs[j] == nullptr){
                throw std::runtime_error(ErrorHelpers::genErrorStr("Null Arc discovered when rewiring driver arcs"));
            }

            std::shared_ptr<Arc> driverArc = driverArcs[j];

            origArcs.push_back(driverArcs[j]);
            contextRootOrigArcs.insert(driverArcs[j]);
            contextArcs.push_back({}); //Replace with nothing
        }
    }

    //Create a copy of the arc list and remove the orig ContextRoot arcs
    std::vector<std::shared_ptr<Arc>> candidateArcs = design.getArcs();
    GeneralHelper::removeAll(candidateArcs, contextRootOrigArcs);

    //This is redundant with the add/remove method that occurs outside of the function.  This function does not add the other
    //arcs re-wired below so it does not make much sense to add the re-wired drivers here.
//    //Add the contextRootRewiredArcs to the design (we do not need to rewire these again so they are not included in the candidate arcs)
//    for(auto rewiredIt = contextRootRewiredArcs.begin(); rewiredIt != contextRootRewiredArcs.end(); rewiredIt++){
//        arcs.push_back(*rewiredIt);
//    }

    //Run through the remaining arcs and check if they should be rewired.
    for(unsigned long i = 0; i<candidateArcs.size(); i++){
        std::shared_ptr<Arc> candidateArc = candidateArcs[i];

        std::vector<Context> srcContext = candidateArc->getSrcPort()->getParent()->getContext();
        std::vector<Context> dstContext = candidateArc->getDstPort()->getParent()->getContext();

        //ContextRoots are not within their own contexts.  However, we need to make sure the inputs and output
        //arcs to the ContextRoots are elevated to the ContextFamily container for that ContextRoot as if it were in its
        //own subcontext.  We will therefore check for context roots and temporarily insert a dummy context entry
        std::shared_ptr<ContextRoot> srcAsContextRoot = GeneralHelper::isType<Node, ContextRoot>(candidateArc->getSrcPort()->getParent());
        std::shared_ptr<DummyReplica> srcAsDummyReplica = GeneralHelper::isType<Node, DummyReplica>(candidateArc->getSrcPort()->getParent());
        if(srcAsContextRoot){
            srcContext.push_back(Context(srcAsContextRoot, -1));
        }else if(srcAsDummyReplica){
            srcContext.push_back(Context(srcAsDummyReplica->getDummyOf(), -1));
        }
        std::shared_ptr<ContextRoot> dstAsContextRoot = GeneralHelper::isType<Node, ContextRoot>(candidateArc->getDstPort()->getParent());
        std::shared_ptr<DummyReplica> dstAsDummyReplica = GeneralHelper::isType<Node, DummyReplica>(candidateArc->getDstPort()->getParent());
        if(dstAsContextRoot){
            dstContext.push_back(Context(dstAsContextRoot, -1));
        }else if(dstAsDummyReplica){
            dstContext.push_back(Context(dstAsDummyReplica->getDummyOf(), -1));
        }

        bool rewireSrc = false;
        bool rewireDst = false;

        //Basically, we need to re-wire the arc if the src and destination are not in the same context for the perpose of scheduling
        //If the the destination is in a subcontext of where the src is, the source does not need to be rewired.
        //However, the destination will need to be re-wired up to the ContextFamilyContainer that resides at the same
        //Context level as the source

        //If the src is in a subcontext compared to the destination, the source needs to be re-wired up to the ContextFamilyContainer
        //that resides at the same level as the destination.  The destination does not need to be re-wired.

        //The case where both the source and destination need to be re-wired is when they are in seperate subcontexts
        //and one is not nested inside the other.

        //We may also need to re-wire the the arc for nodes in the same context if they are in different partitions

        /* Since we are now checking for partitions, it is possible for the source and destination to
         * actually be in the same context but in different partitions (different ContextFamilyContainers)
         * We need to pick the appropriate ContextFamilyContainer partition
         */

        //Check if the src should be rewired
        //TODO: Refactor since we may not actually re-wire one of the sides if the partitions don't match
        //We do not re-wire the src if the src in dst are in different partitions if the src is not in a context.  In this case, the src remains the same (the origional node in the general context)
        //We do re-wire the src if it is in a different partition and is in a context.  We then need to elevate the arc to the ContextFamilyContainer of the particular partition
        if((!Context::isEqOrSubContext(dstContext, srcContext)) || ((candidateArc->getSrcPort()->getParent()->getPartitionNum() != candidateArc->getDstPort()->getParent()->getPartitionNum()) && !srcContext.empty())){
            rewireSrc = true;
        }

        //Check if the dst should be rewired
        //Similarly to the src, the dest is not re-wired if is in a different partition but is not within a context
        if((!Context::isEqOrSubContext(srcContext, dstContext)) || ((candidateArc->getSrcPort()->getParent()->getPartitionNum() != candidateArc->getDstPort()->getParent()->getPartitionNum()) && !dstContext.empty())){
            rewireDst = true;
        }

        if(rewireSrc || rewireDst){
            std::shared_ptr<OutputPort> srcPort;
            std::shared_ptr<InputPort> dstPort;

            if(rewireSrc){
                long commonContextInd = Context::findMostSpecificCommonContext(srcContext, dstContext);
                std::shared_ptr<ContextRoot> newSrcAsContextRoot;

                if(commonContextInd+1 > srcContext.size()){
                    throw std::runtime_error(ErrorHelpers::genErrorStr("Unexpected common context found when re-wiring arcs to src", candidateArc->getSrcPort()->getParent()));
                }else if(commonContextInd+1 == srcContext.size()){
                    //The dst is at or below the src context but they are in different partitions
                    //Need to re-wire the src to be the ContextFamilyContainer for its particular
                    newSrcAsContextRoot = srcContext[commonContextInd].getContextRoot();
                }else{
                    //The dst is above the src in context, need to rewire
                    newSrcAsContextRoot = srcContext[commonContextInd+1].getContextRoot();
                }//The case of the src not having a context is handled when checking if src rewiring is needed

                //TODO: fix diamond inheritcance

                srcPort = newSrcAsContextRoot->getContextFamilyContainers()[candidateArc->getSrcPort()->getParent()->getPartitionNum()]->getOrderConstraintOutputPort();
            }else{
                srcPort = candidateArc->getSrcPort();
            }

            if(rewireDst){
                long commonContextInd = Context::findMostSpecificCommonContext(srcContext, dstContext);

                std::shared_ptr<ContextRoot> newDstAsContextRoot;
                if(commonContextInd+1 > dstContext.size()){
                    throw std::runtime_error(ErrorHelpers::genErrorStr("Unexpected common context found when re-wiring arcs to dst", candidateArc->getDstPort()->getParent()));
                }else if(commonContextInd+1 == dstContext.size()){
                    //The src is at or below the dest context but they are in different partitions
                    //Need to re-wire to the ContextFamilyContainer for the partition
                    newDstAsContextRoot = dstContext[commonContextInd].getContextRoot();
                }else{
                    newDstAsContextRoot = dstContext[commonContextInd+1].getContextRoot();
                }

                //TODO: fix diamond inheritcance

                dstPort = newDstAsContextRoot->getContextFamilyContainers()[candidateArc->getDstPort()->getParent()->getPartitionNum()]->getOrderConstraintInputPort();
            }else{
                dstPort = candidateArc->getDstPort();
            }

            //Handle the special case when going between subcontexts under the same ContextFamilyContainer.  This should
            //only occur when the dst is the context root for the ContextFamilyContainer.  In this case, do not rewire
            if(srcPort->getParent() == dstPort->getParent()){
                std::shared_ptr<ContextRoot> origDstAsContextRoot = GeneralHelper::isType<Node, ContextRoot>(candidateArc->getDstPort()->getParent());
                if(origDstAsContextRoot == nullptr){
                    throw std::runtime_error("Attempted to Rewire a Context Arc into a Self Loop");
                }

                if(origDstAsContextRoot->getContextFamilyContainers()[candidateArc->getDstPort()->getParent()->getPartitionNum()] != dstPort->getParent()){
                    throw std::runtime_error("Attempted to Rewire a Context Arc into a Self Loop");
                }
            }else {
                std::shared_ptr<Arc> rewiredArc = Arc::connectNodes(srcPort, dstPort, candidateArc->getDataType(),
                                                                    candidateArc->getSampleTime());

                //Add the orig and rewired arcs to the vectors to return
                origArcs.push_back(candidateArc);
                contextArcs.push_back({rewiredArc});
            }
        }
    }
}

std::shared_ptr<ContextFamilyContainer> ContextPasses::getContextFamilyContainerCreateIfNotNoParent(
        Design &design, std::shared_ptr<ContextRoot> contextRoot, int partition){
    std::map<int, std::shared_ptr<ContextFamilyContainer>> contextFamilyContainers = contextRoot->getContextFamilyContainers();

    if(contextFamilyContainers.find(partition) == contextFamilyContainers.end()){
        //Does not exist for given partition
        std::shared_ptr<Node> asNode = std::dynamic_pointer_cast<Node>(contextRoot);
        std::shared_ptr<ContextFamilyContainer> familyContainer = NodeFactory::createNode<ContextFamilyContainer>(nullptr); //Temporarily set parent to nullptr
        design.addNode(familyContainer);
        familyContainer->setName("ContextFamilyContainer_For_"+asNode->getFullyQualifiedName(true, "::")+"_Partition_"+GeneralHelper::to_string(partition));
        familyContainer->setPartitionNum(partition);
        familyContainer->setContextRoot(contextRoot);

        //Set context of FamilyContainer (since it is a node in the graph as well which may be scheduled)
        std::vector<Context> origContext = asNode->getContext();
        familyContainer->setContext(origContext);

        //Connect to ContextRoot and to siblings
        familyContainer->setSiblingContainers(contextFamilyContainers);
        for(auto it = contextFamilyContainers.begin(); it != contextFamilyContainers.end(); it++){
            std::map<int, std::shared_ptr<ContextFamilyContainer>> siblingMap = it->second->getSiblingContainers();
            siblingMap[partition] = familyContainer;
            it->second->setSiblingContainers(siblingMap);
        }
        contextFamilyContainers[partition] = familyContainer;
        contextRoot->setContextFamilyContainers(contextFamilyContainers);

        //Create an order constraint arcs based on the context driver arcs.  This ensures that the context driver will be
        //Available in each partition that the context resides in (so long as FIFOs are inserted after this).
        //Only do this if the context root did not request replication
        if(!contextRoot->shouldReplicateContextDriver()) {
            //For cases like enabled subsystems where multiple enable arcs exist for the context but
            //all derive from the same source, we will avoid creating duplicate arcs
            //by checking to see if an equivalent order constraint arc has already been created durring
            //the encapsulation process

            std::set<std::shared_ptr<OutputPort>> driverSrcPortsSeen;

            std::vector<std::shared_ptr<Arc>> driverArcs = contextRoot->getContextDecisionDriver();
            std::vector<std::shared_ptr<Arc>> partitionDrivers;
            for (int i = 0; i < driverArcs.size(); i++){
                std::shared_ptr<OutputPort> driverSrcPort = driverArcs[i]->getSrcPort();

                //Check if we have already seen this src port.  If we have do not create a duplicate
                //order constraint arc
                if(driverSrcPortsSeen.find(driverSrcPort) == driverSrcPortsSeen.end()) {
                    std::shared_ptr<Arc> partitionDriver = Arc::connectNodes(driverSrcPort,
                                                                             familyContainer->getOrderConstraintInputPortCreateIfNot(),
                                                                             driverArcs[i]->getDataType(),
                                                                             driverArcs[i]->getSampleTime());
                    driverSrcPortsSeen.insert(driverSrcPort);
                    partitionDrivers.push_back(partitionDriver);
                    //Also add arc to design
                    design.addArc(partitionDriver);
                }
            }
            contextRoot->addContextDriverArcsForPartition(partitionDrivers, partition);
        }

        //Create Context Containers inside of the context Family container;
        int numContexts = contextRoot->getNumSubContexts();

        std::vector<std::shared_ptr<ContextContainer>> subContexts;

        for(unsigned long j = 0; j<numContexts; j++){
            std::shared_ptr<ContextContainer> contextContainer = NodeFactory::createNode<ContextContainer>(familyContainer);
            contextContainer->setName("ContextContainer_For_"+asNode->getFullyQualifiedName(true, "::")+"_Partition_"+GeneralHelper::to_string(partition)+"_Subcontext_"+GeneralHelper::to_string(j));
            contextContainer->setPartitionNum(partition);
            design.addNode(contextContainer);
            //Add this to the container order
            subContexts.push_back(contextContainer);

            Context context = Context(contextRoot, j);
            contextContainer->setContainerContext(context);
            contextContainer->setContext(origContext); //We give the ContextContainer the same Context stack as the ContextFamily container, the containerContext sets the next level of the context for the nodes contained within it
        }
        familyContainer->setSubContextContainers(subContexts);

        return familyContainer;
    }else{
        return contextFamilyContainers[partition];
    }
}

void ContextPasses::replicateContextRootDriversIfRequested(Design &design){
    //Find ContextRoots that request repliction
    std::vector<std::shared_ptr<ContextRoot>> contextRootsRequestingExpansion;

    {
        std::vector<std::shared_ptr<Node>> nodes = design.getNodes();
        for (int i = 0; i < nodes.size(); i++) {
            std::shared_ptr<ContextRoot> asContextRoot = GeneralHelper::isType<Node, ContextRoot>(nodes[i]);

            if (asContextRoot) {
                if (asContextRoot->shouldReplicateContextDriver()) {
                    contextRootsRequestingExpansion.push_back(asContextRoot);
                }
            }
        }
    }

    for(int i = 0; i<contextRootsRequestingExpansion.size(); i++) {
        std::shared_ptr<ContextRoot> contextRoot = contextRootsRequestingExpansion[i];
        std::set<int> contextPartitions = contextRoot->partitionsInContext();

        std::shared_ptr<Node> contextRootAsNode = std::dynamic_pointer_cast<Node>(contextRoot);
        if(contextRootAsNode == nullptr){
            throw std::runtime_error(ErrorHelpers::genErrorStr("Found a ContextRoot that is not a Node"));
        }
        std::vector<std::shared_ptr<Arc>> driverArcs = contextRoot->getContextDecisionDriver();

        for(int j = 0; j<driverArcs.size(); j++){
            std::shared_ptr<Arc> driverArc = driverArcs[j];
            std::shared_ptr<OutputPort> driverSrcPort = driverArc->getSrcPort();
            std::shared_ptr<Node> driverSrc = driverSrcPort->getParent();

            //Check for conditions of driver src for clone
            //Check that the driver node has no inputs and a single output
            std::set<std::shared_ptr<Arc>> srcInputArcs = driverSrc->getInputArcs();
            if(srcInputArcs.size() != 0){
                throw std::runtime_error(ErrorHelpers::genErrorStr("Context Driver Replication Currently requires the driver source to have no inputs", contextRootAsNode));
            }
            std::set<std::shared_ptr<Arc>> srcOutputArcs = driverSrc->getOutputArcs();
            if(srcOutputArcs.size() != 1){
                throw std::runtime_error(ErrorHelpers::genErrorStr("Context Driver Replication Currently requires the driver source to have only 1 output arc", contextRootAsNode));
            }

            //Replicate for each partition (keep the one that exists and avoid creating a duplicate)
            for(auto partitionIt = contextPartitions.begin(); partitionIt != contextPartitions.end(); partitionIt++){
                int partition = *partitionIt;
                std::shared_ptr<Node> partitionContextDriver;
                std::shared_ptr<Node> partitionContextDst;
                if(driverSrc->getPartitionNum() == partition){
                    //Reuse the existing node
                    //Replicate the arc
                    partitionContextDriver = driverSrc;
                    partitionContextDst = contextRootAsNode;
                }else{
                    //Replicate the node
                    std::shared_ptr<Node> clonedDriver = driverSrc->shallowClone(driverSrc->getParent());
                    //The clone method copies the node ID allong with the other parameters.  When making a copy of the design,
                    //this is desired but, since both nodes will be present in this design, it is not desierable
                    //reset the ID to -1 so that it can be assigned later.
                    //TODO: Make ID copy an option in shallowClone
                    clonedDriver->setId(-1);
                    clonedDriver->setName(driverSrc->getName() + "_Replicated_Partition_" + GeneralHelper::to_string(partition));
                    clonedDriver->setPartitionNum(partition); //Also set the partition number
                    std::vector<Context> origContext = driverSrc->getContext();
                    clonedDriver->setContext(origContext);
                    if(origContext.size()>0){ //Add the replicated node to the ContextRoot if appropriate
                        std::shared_ptr<ContextRoot> contextRoot = origContext[origContext.size()-1].getContextRoot();
                        int subContext = origContext[origContext.size()-1].getSubContext();
                        contextRoot->addSubContextNode(subContext, clonedDriver);
                    }

                    partitionContextDriver = clonedDriver;

                    //Create a dummy node
                    std::shared_ptr<DummyReplica> dummyContextRoot = NodeFactory::createNode<DummyReplica>(contextRootAsNode->getParent());
                    dummyContextRoot->setName(contextRootAsNode->getName() + "_Dummy_Partition_" + GeneralHelper::to_string(partition));
                    dummyContextRoot->setPartitionNum(partition); //Also set the partition number
                    dummyContextRoot->setDummyOf(contextRoot);
                    std::vector<Context> contextRootContext = contextRootAsNode->getContext();
                    dummyContextRoot->setContext(contextRootContext);
                    if(contextRootContext.size()>0){ //Add the replicated node to the ContextRoot if appropriate
                        std::shared_ptr<ContextRoot> contextRoot = contextRootContext[contextRootContext.size()-1].getContextRoot();
                        int subContext = contextRootContext[contextRootContext.size()-1].getSubContext();
                        contextRoot->addSubContextNode(subContext, dummyContextRoot);
                    }
                    contextRoot->setDummyReplica(partition, dummyContextRoot);
                    partitionContextDst = dummyContextRoot;

                    std::vector<std::shared_ptr<Node>> clonedDriverVec = {clonedDriver, dummyContextRoot};
                    std::vector<std::shared_ptr<Node>> emptyNodeVec;
                    std::vector<std::shared_ptr<Arc>> emptyArcVec;

                    //Add replicated node to design
                    design.addRemoveNodesAndArcs(clonedDriverVec, emptyNodeVec, emptyArcVec, emptyArcVec);
                }

                //Create a new arc for the node
                std::shared_ptr<OutputPort> partitionDriverPort = partitionContextDriver->getOutputPortCreateIfNot(driverSrcPort->getPortNum());
                std::shared_ptr<Arc> partitionDriverArc = Arc::connectNodes(partitionDriverPort, partitionContextDst->getOrderConstraintInputPortCreateIfNot(), driverArc->getDataType(), driverArc->getSampleTime());
                std::vector<std::shared_ptr<Arc>> newArcVec = {partitionDriverArc};
                std::vector<std::shared_ptr<Node>> emptyNodeVec;
                std::vector<std::shared_ptr<Arc>> emptyArcVec;
                design.addRemoveNodesAndArcs(emptyNodeVec, emptyNodeVec, newArcVec, emptyArcVec);

                //Add it to the list of partition drivers
                contextRoot->addContextDriverArcsForPartition({partitionDriverArc}, partition);
            }
        }
    }
}

void ContextPasses::placeEnableNodesInPartitions(Design &design) {
    std::vector<std::shared_ptr<Node>> nodesToAdd;
    std::vector<std::shared_ptr<Arc>> arcsToAdd;

    {
        std::vector<std::shared_ptr<Node>> nodes = design.getNodes();
        for (const std::shared_ptr<Node> &node: nodes) {
            if (GeneralHelper::isType<Node, EnableInput>(node)) {
                //Should be validated to only have 1 input
                if (node->getPartitionNum() == -1) {
                    //Reassign Output arcs to Replicas of this Enable Input as appropriate
                    std::shared_ptr<EnableInput> nodeAsEnabledInput = std::dynamic_pointer_cast<EnableInput>(node);
                    std::shared_ptr<Arc> inputArc = *node->getInputPortCreateIfNot(0)->getArcs().begin();
                    std::set<std::shared_ptr<Arc>> driverArcs = nodeAsEnabledInput->getEnablePort()->getArcs();
                    std::set<std::shared_ptr<Arc>> outArcs = node->getOutputPortCreateIfNot(0)->getArcs();
                    std::set<std::shared_ptr<Arc>> orderConstraintInputArcs = node->getOrderConstraintInputArcs();
                    std::set<std::shared_ptr<Arc>> orderConstraintOutputArcs = node->getOrderConstraintOutputArcs();

                    std::map<int, std::shared_ptr<EnableInput>> replicas;
                    //Reassign outputs
                    for (const std::shared_ptr<Arc> &outArc: outArcs) {
                        std::shared_ptr<EnableInput> replicaNode;
                        int dstPartitionNum = outArc->getDstPort()->getParent()->getPartitionNum();

                        if (dstPartitionNum == -1) {
                            if (GeneralHelper::isType<Node, EnableOutput>(outArc->getDstPort()->getParent()) !=
                                nullptr) {
                                throw std::runtime_error(ErrorHelpers::genErrorStr(
                                        "Encountered an EnableInput directly connected to an EnableOutput", node));
                            } else {
                                throw std::runtime_error(ErrorHelpers::genErrorStr(
                                        "Encountered an EnableInput that is connected to a node not in a partition",
                                        node));
                            }
                        }

                        //Check if this is the first
                        if (node->getPartitionNum() == -1) {
                            node->setPartitionNum(dstPartitionNum);
                            replicas[dstPartitionNum] = nodeAsEnabledInput;
                            replicaNode = nodeAsEnabledInput;
                        } else {
                            //Check if there is already a replica
                            if (replicas.find(dstPartitionNum) == replicas.end()) {
                                std::shared_ptr<SubSystem> parent = nodeAsEnabledInput->getParent();
                                if (GeneralHelper::isType<SubSystem, EnabledSubSystem>(parent) == nullptr) {
                                    throw std::runtime_error(ErrorHelpers::genErrorStr(
                                            "Found an Enable node that was not directly under an EnabledSubsystem",
                                            node));
                                }
                                std::shared_ptr<EnabledSubSystem> parentAsEnabledSubsystem = std::dynamic_pointer_cast<EnabledSubSystem>(
                                        parent);
                                //TODO: Remove sanity check
                                std::vector<std::shared_ptr<EnableInput>> parentEnabledInputs = parentAsEnabledSubsystem->getEnableInputs();
                                if (std::find(parentEnabledInputs.begin(), parentEnabledInputs.end(),
                                              nodeAsEnabledInput) == parentEnabledInputs.end()) {
                                    throw std::runtime_error(ErrorHelpers::genErrorStr(
                                            "Found an Enable node that was not directly under its own EnabledSubsystem",
                                            node));
                                }

                                //Need to replicate node
                                replicaNode = NodeFactory::shallowCloneNode<EnableInput>(node->getParent(),
                                                                                         nodeAsEnabledInput.get());
                                parentAsEnabledSubsystem->addEnableInput(replicaNode);
                                replicaNode->setPartitionNum(dstPartitionNum);
                                nodesToAdd.push_back(replicaNode);
                                replicas[dstPartitionNum] = replicaNode;

                                //Replicate input arc
                                std::shared_ptr<Arc> newInputArc = Arc::connectNodes(inputArc->getSrcPort(),
                                                                                     replicaNode->getInputPortCreateIfNot(
                                                                                             inputArc->getDstPort()->getPortNum()),
                                                                                     inputArc->getDataType(),
                                                                                     inputArc->getSampleTime());
                                arcsToAdd.push_back(newInputArc);

                                //Replicate order constraint inputs
                                for (const std::shared_ptr<Arc> &orderConstraintInputArc: orderConstraintInputArcs) {
                                    std::shared_ptr<Arc> newOrderConstraintInputArc = Arc::connectNodes(
                                            inputArc->getSrcPort(),
                                            replicaNode->getInputPortCreateIfNot(inputArc->getDstPort()->getPortNum()),
                                            inputArc->getDataType(), inputArc->getSampleTime());
                                    arcsToAdd.push_back(newOrderConstraintInputArc);
                                }

                            } else {
                                //Get the appropriate replica
                                replicaNode = replicas[dstPartitionNum];
                            }
                        }

                        //Reassign arc
                        if (replicaNode != node) {//Only reassign if nessasary
                            outArc->setSrcPortUpdateNewUpdatePrev(
                                    replicaNode->getOutputPortCreateIfNot(outArc->getSrcPort()->getPortNum()));
                        }
                    }

                    //Reassign order constraint outputs error if an output to the same partiton does not exist
                    for (const std::shared_ptr<Arc> &orderConstraintOutputArc: orderConstraintOutputArcs) {
                        std::shared_ptr<EnableInput> replicaNode;
                        int dstPartitionNum = orderConstraintOutputArc->getDstPort()->getParent()->getPartitionNum();

                        //Check if this is the first
                        if (node->getPartitionNum() == -1) {
                            throw std::runtime_error(ErrorHelpers::genErrorStr(
                                    "Found order constraint to partition where there is no output from EnabledInput",
                                    node));
                        } else {
                            //Check if there is already a replica
                            if (replicas.find(dstPartitionNum) == replicas.end()) {
                                throw std::runtime_error(ErrorHelpers::genErrorStr(
                                        "Found order constraint to partition where there is no output from EnabledInput",
                                        node));
                            } else {
                                //Get the appropriate replica
                                replicaNode = replicas[dstPartitionNum];
                            }
                        }

                        //Reassign arc
                        if (replicaNode != node) {//Only reassign if nessasary
                            orderConstraintOutputArc->setSrcPortUpdateNewUpdatePrev(
                                    replicaNode->getOrderConstraintOutputPortCreateIfNot());
                        }
                    }
                }
            } else if (GeneralHelper::isType<Node, EnableOutput>(node)) {
                if (node->getPartitionNum() == -1) {
                    //Place in the partition of the input
                    std::shared_ptr<Arc> inputArc = *node->getInputPortCreateIfNot(0)->getArcs().begin();
                    int srcPartition = inputArc->getSrcPort()->getParent()->getPartitionNum();

                    if (srcPartition == -1) {
                        if (GeneralHelper::isType<Node, EnableInput>(inputArc->getSrcPort()->getParent()) != nullptr) {
                            throw std::runtime_error(ErrorHelpers::genErrorStr(
                                    "Encountered an EnableInput directly connected to an EnableOutput", node));
                        } else {
                            throw std::runtime_error(ErrorHelpers::genErrorStr(
                                    "Encountered an EnableOutput that is connected to a node not in a partition",
                                    node));
                        }
                    }

                    node->setPartitionNum(srcPartition);
                }
            }
        }
    }

    //Add remove nodes and arcs
    std::vector<std::shared_ptr<Node>> emptyNodes;
    std::vector<std::shared_ptr<Arc>> emptyArcs;
    design.addRemoveNodesAndArcs(nodesToAdd, emptyNodes, arcsToAdd, emptyArcs);

    //Place EnabledSubsystem nodes into partitons
    {
        std::vector<std::shared_ptr<Node>> nodes = design.getNodes(); //Get nodes from design again after addRemoveNodesAndArc
        for (const std::shared_ptr<Node> &node: nodes) {
            if (GeneralHelper::isType<Node, EnabledSubSystem>(node)) {
                if (node->getPartitionNum() == -1) {
                    std::shared_ptr<EnabledSubSystem> nodeAsEnabledSubsystem = std::dynamic_pointer_cast<EnabledSubSystem>(
                            node);
                    if (!nodeAsEnabledSubsystem->getEnableInputs().empty()) {
                        nodeAsEnabledSubsystem->setPartitionNum(
                                nodeAsEnabledSubsystem->getEnableInputs()[0]->getPartitionNum());
                    } else if (!nodeAsEnabledSubsystem->getEnableOutputs().empty()) {
                        nodeAsEnabledSubsystem->setPartitionNum(
                                nodeAsEnabledSubsystem->getEnableOutputs()[0]->getPartitionNum());
                    } else {
                        throw std::runtime_error(
                                ErrorHelpers::genErrorStr("Found enabled subsystem with no inputs or outputs", node));
                    }
                }
            }
        }
    }
}