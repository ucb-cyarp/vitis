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
