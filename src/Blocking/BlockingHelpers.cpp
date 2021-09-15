//
// Created by Christopher Yarp on 8/24/21.
//

#include "BlockingHelpers.h"
#include "BlockingDomain.h"
#include "General/GraphAlgs.h"
#include "GraphCore/ContextRoot.h"
#include "GraphCore/ContextFamilyContainer.h"
#include "General/ErrorHelpers.h"

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

std::vector<std::shared_ptr<BlockingDomain>> BlockingHelpers::findBlockingDomainStack(std::shared_ptr<Node> node){
    std::vector<std::shared_ptr<BlockingDomain>> stack;

    std::shared_ptr<Node> cursor = node;
    while(cursor != nullptr){
        std::shared_ptr<BlockingDomain> parentDomain = findBlockingDomain(cursor);
        cursor = parentDomain;
        if(parentDomain != nullptr){
            stack.insert(stack.begin(), parentDomain);
        }
    }

    return stack;
}

std::vector<int> BlockingHelpers::blockingDomainDimensionReduce(std::vector<int> outerDimensions, int subBlockingLength){
    int outerDim = outerDimensions[outerDimensions.size() - 1];
    if(subBlockingLength>outerDim){
        throw std::runtime_error(ErrorHelpers::genErrorStr("sub-blocking length must be less than the outer dimension"));
    }
    if(outerDim%subBlockingLength != 0){
        throw std::runtime_error(ErrorHelpers::genErrorStr("sub-blocking length must be a factor of outer dimension"));
    }

    std::vector<int> innerDim;
    if(subBlockingLength == 1){
        bool inputIsVectorOrScalar = outerDimensions.size() == 1 && outerDimensions[0] >= 1;
        if(inputIsVectorOrScalar){
            //The dimensionality is not decreased but is set to the sub-blocking length
            innerDim.push_back(subBlockingLength);
        }else{
            //The outer dimension is removed
            innerDim.insert(innerDim.end(), outerDimensions.begin()+1, outerDimensions.end());
        }
    }else if(subBlockingLength > 1){
        //Set the outer dimension to be the sub-blocking length
        innerDim.push_back(subBlockingLength);
        innerDim.insert(innerDim.end(), outerDimensions.begin()+1, outerDimensions.end());
    }else{
        throw std::runtime_error(ErrorHelpers::genErrorStr("sub-blocking length must be positive"));
    }

    return innerDim;
}

void BlockingHelpers::createBlockingDomainHelper(std::set<std::shared_ptr<Node>> nodesToMove,
                                              std::set<std::shared_ptr<Arc>> arcsIntoDomain,
                                              std::set<std::shared_ptr<Arc>> arcsOutOfDomain,
                                              std::shared_ptr<SubSystem> blockingDomainParent,
                                              int blockingLength,
                                              int subBlockingLength,
                                              std::string blockingName,
                                              std::vector<std::shared_ptr<Node>> &nodesToAdd,
                                              std::vector<std::shared_ptr<Arc>> &arcsToAdd,
                                              std::vector<std::shared_ptr<Node>> &nodesToRemoveFromTopLevel){

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

        if(nodeToMove->getPartitionNum() != -1){
            //Set the partition of the blocking domain to be the partition defined by one of the nodes being wrapped.  During encapsulation, separate context family containers will be created for each partitions nodes within the context exist in.
            blockingDomain->setPartitionNum(nodeToMove->getPartitionNum());
        }
    }

    //Create Blocking Inputs and Outputs
    for(const std::shared_ptr<Arc> &inputArc : arcsIntoDomain) {
        std::shared_ptr<BlockingInput> blockingInput = NodeFactory::createNode<BlockingInput>(blockingDomain);
        nodesToAdd.push_back(blockingInput);
        blockingDomain->addBlockInput(blockingInput);
        blockingInput->setBlockingLen(blockingLength);
        blockingInput->setSubBlockingLen(subBlockingLength);

        std::shared_ptr<InputPort> origInputPort = inputArc->getDstPort();
        std::shared_ptr<Node> nodeInDomain = origInputPort->getParent();
        blockingInput->setPartitionNum(nodeInDomain->getPartitionNum()); //Set the partition of the blocking input node to the partition of the node it is connected to in the domain

        blockingInput->setName("BlockingDomainInputFor_" + nodeInDomain->getName() + "_n" + GeneralHelper::to_string(nodeInDomain->getId()) + "_p" + GeneralHelper::to_string(origInputPort->getPortNum()));
        inputArc->setDstPortUpdateNewUpdatePrev(blockingInput->getInputPortCreateIfNot(0));

        std::shared_ptr<Arc> blockingInputConnection = Arc::connectNodes(blockingInput->getOutputPortCreateIfNot(0), origInputPort, inputArc->getDataType(), inputArc->getSampleTime());
        arcsToAdd.push_back(blockingInputConnection);
    }

    std::map<std::shared_ptr<OutputPort>, std::shared_ptr<BlockingOutput>> outputPortsToBlockingOutputs;
    for(const std::shared_ptr<Arc> &outputArc : arcsOutOfDomain){
        //Check if we have already created a blocking output for this output port
        std::shared_ptr<OutputPort> origOutputPort = outputArc->getSrcPort();
        std::shared_ptr<BlockingOutput> blockingOutput;
        if(GeneralHelper::contains(origOutputPort, outputPortsToBlockingOutputs)){
            blockingOutput = outputPortsToBlockingOutputs[origOutputPort];
        }else{
            //Create a new blocking output
            blockingOutput = NodeFactory::createNode<BlockingOutput>(blockingDomain);
            nodesToAdd.push_back(blockingOutput);
            outputPortsToBlockingOutputs[origOutputPort] = blockingOutput;
            blockingDomain->addBlockOutput(blockingOutput);
            blockingOutput->setBlockingLen(blockingLength);
            blockingOutput->setSubBlockingLen(subBlockingLength);
            std::shared_ptr<Node> nodeInDomain = origOutputPort->getParent();

            blockingOutput->setPartitionNum(nodeInDomain->getPartitionNum()); //Set the partition of the blocking output node to the partition of the node it is connected to in the domain
            blockingOutput->setName("BlockingDomainOutputFor_" + nodeInDomain->getName() + "_n" + GeneralHelper::to_string(nodeInDomain->getId()) + "_p" + GeneralHelper::to_string(origOutputPort->getPortNum()));

            //Connect the blocking output
            std::shared_ptr<Arc> blockingOutputConnection = Arc::connectNodes(origOutputPort, blockingOutput->getInputPortCreateIfNot(0), outputArc->getDataType(), outputArc->getSampleTime());
            arcsToAdd.push_back(blockingOutputConnection);
        }

        outputArc->setSrcPortUpdateNewUpdatePrev(blockingOutput->getOutputPortCreateIfNot(0));
    }
}