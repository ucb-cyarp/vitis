//
// Created by Christopher Yarp on 8/24/21.
//

#include "BlockingBoundary.h"
#include "BlockingHelpers.h"
#include "General/ErrorHelpers.h"
#include "GraphMLTools/GraphMLHelper.h"

BlockingBoundary::BlockingBoundary() : blockingLen(1), subBlockingLen(1) {

}

BlockingBoundary::BlockingBoundary(std::shared_ptr<SubSystem> parent) : Node(parent),
    blockingLen(1), subBlockingLen(1) {

}

BlockingBoundary::BlockingBoundary(std::shared_ptr<SubSystem> parent, BlockingBoundary *orig) : Node(parent, orig),
    blockingLen(orig->blockingLen), subBlockingLen(orig->subBlockingLen) {

}

void BlockingBoundary::validate() {
    Node::validate();

    std::shared_ptr<BlockingDomain> blockingDomain = BlockingHelpers::findBlockingDomain(getSharedPointer());
    if(blockingDomain == nullptr){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Could not find BlockingDomain for this BlockingBoundary", getSharedPointer()));
    }

    //TODO: Could potentially be removed as this is also checked in the validation of BlockingDomain
    if(blockingLen != blockingDomain->getBlockingLen()){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Blocking length does not match that of parent BlockingDomain", getSharedPointer()));
    }
    if(subBlockingLen != blockingDomain->getSubBlockingLen()){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Sub-Blocking length does not match that of parent BlockingDomain", getSharedPointer()));
    }

    //Verify there is only 1 input and 1 output port
    if(inputPorts.size() != 1){
        throw std::runtime_error(ErrorHelpers::genErrorStr("BlockingBoundary should have 1 input port", getSharedPointer()));
    }

    if(outputPorts.size() != 1){
        throw std::runtime_error(ErrorHelpers::genErrorStr("BlockingBoundary should have 1 output port", getSharedPointer()));
    }
}

int BlockingBoundary::getBlockingLen() const {
    return blockingLen;
}

void BlockingBoundary::setBlockingLen(int blockingLen) {
    BlockingBoundary::blockingLen = blockingLen;
}

int BlockingBoundary::getSubBlockingLen() const {
    return subBlockingLen;
}

void BlockingBoundary::setSubBlockingLen(int subBlockingLen) {
    BlockingBoundary::subBlockingLen = subBlockingLen;
}

void BlockingBoundary::emitBoundaryGraphMLParams(xercesc::DOMDocument* doc, xercesc::DOMElement* graphNode){
    GraphMLHelper::addDataNode(doc, graphNode, "blocking_len", GeneralHelper::to_string(blockingLen));
    GraphMLHelper::addDataNode(doc, graphNode, "sub_blocking_len", GeneralHelper::to_string(subBlockingLen));
}

std::set<GraphMLParameter> BlockingBoundary::graphMLParameters() {
    std::set<GraphMLParameter> parameters;

    parameters.insert(GraphMLParameter("blocking_len", "int", true));
    parameters.insert(GraphMLParameter("sub_blocking_len", "int", true));

    return parameters;
}