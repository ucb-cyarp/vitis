//
// Created by Christopher Yarp on 8/24/21.
//

#include<iostream>
#include "BlockingHelpers.h"
#include "BlockingDomain.h"
#include "General/GraphAlgs.h"
#include "GraphCore/ContextRoot.h"
#include "GraphCore/ContextFamilyContainer.h"
#include "General/ErrorHelpers.h"
#include "MultiRate/MultiRateHelpers.h"
#include "GraphCore/ExpandedNode.h"

std::set<std::shared_ptr<Node>> BlockingHelpers::getNodesInBlockingDomainHelper(const std::set<std::shared_ptr<Node>> nodesToSearch){
    return GraphAlgs::getNodesInDomainHelperFilter<BlockingDomain, Node>(nodesToSearch);
}

std::set<std::shared_ptr<BlockingInput>> BlockingHelpers::getBlockingInputsInBlockingDomainHelper(const std::set<std::shared_ptr<Node>> nodesToSearch){
    return GraphAlgs::getNodesInDomainHelperFilter<BlockingDomain, BlockingInput>(nodesToSearch);
}
std::set<std::shared_ptr<BlockingOutput>> BlockingHelpers::getBlockingOutputsInBlockingDomainHelper(const std::set<std::shared_ptr<Node>> nodesToSearch){
    return GraphAlgs::getNodesInDomainHelperFilter<BlockingDomain, BlockingOutput>(nodesToSearch);
}

std::shared_ptr<BlockingDomain> BlockingHelpers::findBlockingDomain(std::shared_ptr<Node> node){
    return GraphAlgs::findDomain<BlockingDomain>(node);
}

bool BlockingHelpers::isOutsideBlockingDomain(std::shared_ptr<BlockingDomain> a, std::shared_ptr<BlockingDomain> b){
    return GraphAlgs::isOutsideDomain(a, b);
}

bool BlockingHelpers::isBlockingDomainOrOneLvlNested(std::shared_ptr<BlockingDomain> a, std::shared_ptr<BlockingDomain> b){
    return GraphAlgs::isDomainOrOneLvlNested(a, b);
}

bool BlockingHelpers::areWithinOneBlockingDomainOfEachOther(std::shared_ptr<BlockingDomain> a, std::shared_ptr<BlockingDomain> b){
    return GraphAlgs::areWithinOneDomainOfEachOther(a, b);
}

std::vector<std::shared_ptr<BlockingDomain>> BlockingHelpers::findBlockingDomainStack(std::shared_ptr<Node> node){
    std::vector<std::shared_ptr<BlockingDomain>> stack = GraphAlgs::getDomainHierarchy<BlockingDomain>(node);

    return stack;
}

std::vector<int> BlockingHelpers::blockingDomainDimensionReduce(std::vector<int> outerDimensions, int blockingLength, int subBlockingLength){
    //Only do the reduction if the blocking length is > 1.  Otherwise, we are in the degenerate case - which can happen
    std::vector<int> innerDim;
    if(blockingLength>1) {
        int outerDim = outerDimensions[0];
        if(outerDimensions[0] != blockingLength){
            throw std::runtime_error(ErrorHelpers::genErrorStr("Outer Dimension not equal to provided blocking length"));
        }
        if (subBlockingLength > outerDim) {
            throw std::runtime_error(
                    ErrorHelpers::genErrorStr("sub-blocking length must be less than the outer dimension"));
        }
        if (outerDim % subBlockingLength != 0) {
            throw std::runtime_error(
                    ErrorHelpers::genErrorStr("sub-blocking length must be a factor of outer dimension"));
        }

        if (subBlockingLength == 1) {
            bool inputIsVectorOrScalar = outerDimensions.size() == 1 && outerDimensions[0] >= 1;
            if (inputIsVectorOrScalar) {
                //The dimensionality is not decreased but is set to the sub-blocking length
                innerDim.push_back(subBlockingLength);
            } else {
                //The outer dimension is removed
                innerDim.insert(innerDim.end(), outerDimensions.begin() + 1, outerDimensions.end());
            }
        } else if (subBlockingLength > 1) {
            //Set the outer dimension to be the sub-blocking length
            innerDim.push_back(subBlockingLength);
            innerDim.insert(innerDim.end(), outerDimensions.begin() + 1, outerDimensions.end());
        } else {
            throw std::runtime_error(ErrorHelpers::genErrorStr("sub-blocking length must be positive"));
        }
    }else{
        innerDim = outerDimensions;
    }

    return innerDim;
}

void BlockingHelpers::createBlockingDomainHelper(std::set<std::shared_ptr<Node>> nodesToMove,
                                              std::set<std::shared_ptr<Arc>> arcsIntoDomain,
                                              std::set<std::shared_ptr<Arc>> arcsOutOfDomain,
                                              std::shared_ptr<SubSystem> blockingDomainParent,
                                              int blockingLength,
                                              int subBlockingLength,
                                              int baseSubBlockingLength,
                                              std::string blockingName,
                                              std::vector<std::shared_ptr<Node>> &nodesToAdd,
                                              std::vector<std::shared_ptr<Arc>> &arcsToAdd,
                                              std::vector<std::shared_ptr<Node>> &nodesToRemoveFromTopLevel,
                                              std::map<std::shared_ptr<Arc>, std::tuple<int, int, bool, bool>>
                                                  &arcsWithDeferredBlockingExpansion){

    //Create Blocking Domain
    std::shared_ptr<BlockingDomain> blockingDomain = NodeFactory::createNode<BlockingDomain>(blockingDomainParent);
    nodesToAdd.push_back(blockingDomain);
    blockingDomain->setName(blockingName);
    blockingDomain->setBlockingLen(blockingLength);
    blockingDomain->setSubBlockingLen(subBlockingLength);

    //Move nodes under blocking domain
    for(const std::shared_ptr<Node> &nodeToMove : nodesToMove) {
        if(nodeToMove->getParent() == nullptr){
            //Remove from the top level nodes of the design
            nodesToRemoveFromTopLevel.push_back(nodeToMove);
        }
        GraphAlgs::moveNodePreserveHierarchy(nodeToMove, blockingDomain, nodesToAdd);
    }

    //It is possible that all of the nodes to move are subsystems without a partition (especially at the top level)
    //If this is the case, traverse hierarchically into children
    int partNum = GraphAlgs::findPartitionInSubSystem(blockingDomain);
    if(partNum == -1){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Could not find partition when creating Blocking Domain", blockingDomain));
    }
    blockingDomain->setPartitionNum(partNum);
    blockingDomain->setBaseSubBlockingLen(baseSubBlockingLength);

    //Create Blocking Inputs and Outputs
    std::map<std::pair<std::shared_ptr<OutputPort>, int>, std::shared_ptr<BlockingInput>> outputPortsToBlockingInputs; //Crete Seperate Blocking Inputs For Different Partitions
    for(const std::shared_ptr<Arc> &inputArc : arcsIntoDomain) {

        //      We create blocking inputs for All I/O Input Arcs EXCEPT ones destined for clock domains that
        //      cannot operate in vector mode (or clock domains nested in clock domains that cannot execute in vector mode)
        //      The blocking Input nodes are scaled by the rate of the destination clock domain.
        //      Indexing will be handled by the context of these Blocking Boundaries

        //      These additional Blocking Inputs will be created in a later pass due to the need for different block sizes

        if(!MultiRateHelpers::arcIsIOAndNotAtBaseRate(inputArc)) {
            //Insert the blocking Input unless the Arc is I/O and Is Going to a Clock Domain which is unable to operate in vector mode
            std::shared_ptr<OutputPort> srcPort = inputArc->getSrcPort();
            std::shared_ptr<InputPort> inputPort = inputArc->getDstPort();
            std::shared_ptr<Node> nodeInDomain = inputPort->getParent();
            int nodeInDomainPartitionNum = nodeInDomain->getPartitionNum();
            int nodeInDomainBaseSubBlockingLen = nodeInDomain->getBaseSubBlockingLen();
            if(nodeInDomainBaseSubBlockingLen != baseSubBlockingLength){
                throw std::runtime_error(ErrorHelpers::genErrorStr("Disagreement between provided baseSubBlockingLen and one discovered from node during creation of Blocking Domain", nodeInDomain));
            }

            std::shared_ptr<BlockingInput> blockingInput;
            std::shared_ptr<Arc> blockingInputInArc;

            std::tuple<int, int, bool, bool> origArcExpansion = {0, 0, false, false};
            if(GeneralHelper::contains(inputArc, arcsWithDeferredBlockingExpansion)){
                origArcExpansion = arcsWithDeferredBlockingExpansion[inputArc];
            }

            if(GeneralHelper::contains(std::pair<std::shared_ptr<OutputPort>, int> (srcPort, nodeInDomainPartitionNum), outputPortsToBlockingInputs)){
                blockingInput = outputPortsToBlockingInputs[{srcPort, nodeInDomainPartitionNum}];
                blockingInputInArc = *(blockingInput->getInputPort(0)->getArcs().begin());
            }else {
                blockingInput = NodeFactory::createNode<BlockingInput>(blockingDomain);
                nodesToAdd.push_back(blockingInput);
                outputPortsToBlockingInputs[{srcPort, nodeInDomainPartitionNum}] = blockingInput;
                blockingDomain->addBlockInput(blockingInput);
                blockingInput->setBlockingLen(blockingLength);
                blockingInput->setSubBlockingLen(subBlockingLength);

                blockingInput->setPartitionNum(nodeInDomainPartitionNum); //Set the partition of the blocking input node to the partition of the node it is connected to in the domain
                blockingInput->setBaseSubBlockingLen(nodeInDomainBaseSubBlockingLen);

                blockingInput->setName("BlockingDomainInputFrom_" + srcPort->getParent()->getName() + "_n" +
                                       GeneralHelper::to_string(srcPort->getParent()->getId()) + "_p" +
                                       GeneralHelper::to_string(srcPort->getPortNum()) + "_dst" + GeneralHelper::to_string(nodeInDomain->getId()));

                std::shared_ptr<Arc> blockingInputConnection = Arc::connectNodes(srcPort,
                        blockingInput->getInputPortCreateIfNot(0),
                        inputArc->getDataType(),
                        inputArc->getSampleTime());
                blockingInputInArc = blockingInputConnection;

                std::tuple<int, int, bool, bool> newArcExpansion = {0, 0, false, false};
                std::get<1>(newArcExpansion) = blockingLength; //Need to expand the arc going into the blocking input by the block size
                std::get<3>(newArcExpansion) = true; //Need to expand the arc going into the blocking input by the block size
                //Will copy expansion properties from existing arc below
                arcsWithDeferredBlockingExpansion[blockingInputConnection] = newArcExpansion;

                arcsToAdd.push_back(blockingInputConnection);
            }

            //Copy over the beginning arc expansion properties (if it exists) to the new arc
            //This new arc comes from outside the blocking domain into the blocking input.  When inserted, the destination request was added, need to copy the src request from the orig arc
            //Need to capture if this arc requires a BlockingDomainBridge
            if(std::get<2>(origArcExpansion)) {
                std::tuple<int, int, bool, bool> newArcExpansion = arcsWithDeferredBlockingExpansion[blockingInputInArc];
                std::get<0>(newArcExpansion) = std::get<0>(origArcExpansion);
                std::get<2>(newArcExpansion) = std::get<2>(origArcExpansion);
                arcsWithDeferredBlockingExpansion[blockingInputInArc] = newArcExpansion;
            }

            //The original input arc is now attached to the blocking input.  Its expansion request from the src side needs to be updated (below)
            inputArc->setSrcPortUpdateNewUpdatePrev(blockingInput->getOutputPortCreateIfNot(0));

            std::get<0>(origArcExpansion) = subBlockingLength;  //Need to expand the arc going into the node input by the sub blocking size.  This would be done by a nested blocking domain except that it does work if the nested blocking domains are created before outer blocing domains
            std::get<2>(origArcExpansion) = true;

            arcsWithDeferredBlockingExpansion[inputArc] = origArcExpansion;
        }
    }

    std::map<std::shared_ptr<OutputPort>, std::shared_ptr<BlockingOutput>> outputPortsToBlockingOutputs; //Do not need partition number since the blocking output is in the partition of the src port
    for(const std::shared_ptr<Arc> &outputArc : arcsOutOfDomain){
        if(!MultiRateHelpers::arcIsIOAndNotAtBaseRate(outputArc)) {
            //Only insert the blocking Input if the Arc is Not To/From I/O or is To/From I/O At the Base Rate

            //Check if we have already created a blocking output for this output port
            std::shared_ptr<OutputPort> origOutputPort = outputArc->getSrcPort();
            std::shared_ptr<BlockingOutput> blockingOutput;
            std::shared_ptr<Arc> blockingOutputInArc;

            std::tuple<int, int, bool, bool> origArcExpansion = {0, 0, false, false};
            if(GeneralHelper::contains(outputArc, arcsWithDeferredBlockingExpansion)){
                origArcExpansion = arcsWithDeferredBlockingExpansion[outputArc];
            }

            if (GeneralHelper::contains(origOutputPort, outputPortsToBlockingOutputs)) {
                blockingOutput = outputPortsToBlockingOutputs[origOutputPort];
                blockingOutputInArc = *(blockingOutput->getInputPort(0)->getArcs().begin());
            } else {
                //Create a new blocking output
                blockingOutput = NodeFactory::createNode<BlockingOutput>(blockingDomain);
                nodesToAdd.push_back(blockingOutput);
                outputPortsToBlockingOutputs[origOutputPort] = blockingOutput;
                blockingDomain->addBlockOutput(blockingOutput);
                blockingOutput->setBlockingLen(blockingLength);
                blockingOutput->setSubBlockingLen(subBlockingLength);
                std::shared_ptr<Node> nodeInDomain = origOutputPort->getParent();
                int nodeInDomainBaseSubBlockingLen = nodeInDomain->getBaseSubBlockingLen();
                if(nodeInDomainBaseSubBlockingLen != baseSubBlockingLength){
                    throw std::runtime_error(ErrorHelpers::genErrorStr("Disagreement between provided baseSubBlockingLen and one discovered from node during creation of Blocking Domain", nodeInDomain));
                }

                blockingOutput->setPartitionNum(
                        nodeInDomain->getPartitionNum()); //Set the partition of the blocking output node to the partition of the node it is connected to in the domain
                blockingOutput->setBaseSubBlockingLen(nodeInDomainBaseSubBlockingLen);
                blockingOutput->setName("BlockingDomainOutputFor_" + nodeInDomain->getName() + "_n" +
                                        GeneralHelper::to_string(nodeInDomain->getId()) + "_p" +
                                        GeneralHelper::to_string(origOutputPort->getPortNum()));

                //Connect the blocking output
                std::shared_ptr<Arc> blockingOutputConnection = Arc::connectNodes(origOutputPort,
                                                                                  blockingOutput->getInputPortCreateIfNot(
                                                                                          0), outputArc->getDataType(),
                                                                                  outputArc->getSampleTime());
                blockingOutputInArc = blockingOutputConnection;

                std::tuple<int, int, bool, bool> newArcExpansion = {0, 0, false, false};
                std::get<1>(newArcExpansion) = subBlockingLength; //Need to expand the arc going into the input by the sub blocking size.  This would be done by a nested blocking domain except that it does work if the nested blocking domains are created before outer blocing domains
                std::get<3>(newArcExpansion) = true;

                //Copy the original arc expansion request from the src side of the original arc
                //This original arc will be re-wired to the output of the blocking output
                std::get<0>(newArcExpansion) = std::get<0>(origArcExpansion);
                std::get<2>(newArcExpansion) = std::get<2>(origArcExpansion);

                arcsWithDeferredBlockingExpansion[blockingOutputConnection] = newArcExpansion;
                arcsToAdd.push_back(blockingOutputConnection);
            }

            //The output arc should be updated by the blocking length (at the src side).
            outputArc->setSrcPortUpdateNewUpdatePrev(blockingOutput->getOutputPortCreateIfNot(0));
            std::get<0>(origArcExpansion) = blockingLength; //Need to expand the arc going into the input by the block size
            std::get<2>(origArcExpansion) = true;
            arcsWithDeferredBlockingExpansion[outputArc] = origArcExpansion;
        }
    }
}

void BlockingHelpers::propagateSubBlockingFromSubsystemsToChildren(std::set<std::shared_ptr<Node>>& nodes, int baseSubBlockingLength){
    for(auto it = nodes.begin(); it != nodes.end(); it++){
        //Do this first since expanded nodes are also subsystems
        std::shared_ptr<ExpandedNode> asExpandedNode = GeneralHelper::isType<Node, ExpandedNode>(*it);
        if(asExpandedNode){
            //This is a special case where the subsystem takes the partition of the parent and not itself
            if(baseSubBlockingLength != -1) {
                asExpandedNode->getOrigNode()->setBaseSubBlockingLen(baseSubBlockingLength);
                asExpandedNode->setBaseSubBlockingLen(baseSubBlockingLength);
            }

            std::set<std::shared_ptr<Node>> children = asExpandedNode->getChildren();
            propagateSubBlockingFromSubsystemsToChildren(children, baseSubBlockingLength); //still at same level as before with same partition
        }else{
            std::shared_ptr<SubSystem> asSubsystem = GeneralHelper::isType<Node, SubSystem>(*it);
            if(asSubsystem){
                int nextBaseSubBlockingLength = asSubsystem->getBaseSubBlockingLen();
                if(nextBaseSubBlockingLength == -1){
                    nextBaseSubBlockingLength = baseSubBlockingLength;
                    asSubsystem->setBaseSubBlockingLen(baseSubBlockingLength);
                }
                std::set<std::shared_ptr<Node>> children = asSubsystem->getChildren();
                propagateSubBlockingFromSubsystemsToChildren(children, nextBaseSubBlockingLength);
            }else{
                //Standard node, set baseSubBlockingLen if !firstLevel
                if(baseSubBlockingLength != -1){
                    (*it)->setBaseSubBlockingLen(baseSubBlockingLength);
                }
            }
        }
    }
}

void BlockingHelpers::requestDeferredBlockingExpansionOfNodeArcs(std::shared_ptr<Node> node, int inExpansion, int outExpansion, std::map<std::shared_ptr<Arc>, std::tuple<int, int, bool, bool>> &arcsWithDeferredBlockingExpansion){
    std::set<std::shared_ptr<Arc>> inArcs = node->getDirectInputArcs();
    std::set<std::shared_ptr<Arc>> outArcs = node->getDirectOutputArcs();
    for (const std::shared_ptr<Arc> &inArc: inArcs) {
        //Input to Node is the end of the arc.  Set the second entry
        std::tuple<int, int, bool, bool> expansion = {0, 0, false, false};
        if(GeneralHelper::contains(inArc, arcsWithDeferredBlockingExpansion)){
            expansion = arcsWithDeferredBlockingExpansion[inArc];
        }

        std::get<1>(expansion) = inExpansion;
        std::get<3>(expansion) = true;

        arcsWithDeferredBlockingExpansion[inArc] = expansion;
    }

    for (const std::shared_ptr<Arc> &outArc: outArcs) {
        //Output of Node is the beginning of the arc.  Set the first entry
        std::tuple<int, int, bool, bool> expansion = {0, 0, false, false};
        if(GeneralHelper::contains(outArc, arcsWithDeferredBlockingExpansion)){
            expansion = arcsWithDeferredBlockingExpansion[outArc];
        }

        std::get<0>(expansion) = outExpansion;
        std::get<2>(expansion) = true;

        arcsWithDeferredBlockingExpansion[outArc] = expansion;
    }
}
