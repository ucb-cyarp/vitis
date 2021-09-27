//
// Created by Christopher Yarp on 8/23/21.
//

#include "BlockingDomain.h"
#include "General/ErrorHelpers.h"
#include "BlockingHelpers.h"
#include "GraphCore/Variable.h"
#include "GraphCore/ContextHelper.h"

BlockingDomain::BlockingDomain() : blockingLen(1), subBlockingLen(1) {

}

BlockingDomain::BlockingDomain(std::shared_ptr<SubSystem> parent) : SubSystem(parent), ContextRoot(),
    blockingLen(1), subBlockingLen(1) {

}

BlockingDomain::BlockingDomain(std::shared_ptr<SubSystem> parent, BlockingDomain *orig) : SubSystem(parent, orig),
    ContextRoot(), blockingLen(orig->blockingLen), subBlockingLen(orig->subBlockingLen) {

}

int BlockingDomain::getBlockingLen() const {
    return blockingLen;
}

void BlockingDomain::setBlockingLen(int blockingLen) {
    BlockingDomain::blockingLen = blockingLen;
}

int BlockingDomain::getSubBlockingLen() const {
    return subBlockingLen;
}

void BlockingDomain::setSubBlockingLen(int subBlockingLen) {
    BlockingDomain::subBlockingLen = subBlockingLen;
}

std::vector<std::shared_ptr<BlockingInput>> BlockingDomain::getBlockInputs() const {
    return blockInputs;
}

void BlockingDomain::setBlockInputs(const std::vector<std::shared_ptr<BlockingInput>> &blockInputs) {
    BlockingDomain::blockInputs = blockInputs;
}

std::vector<std::shared_ptr<BlockingOutput>> BlockingDomain::getBlockOutputs() const {
    return blockOutputs;
}

void BlockingDomain::setBlockOutputs(const std::vector<std::shared_ptr<BlockingOutput>> &blockOutputs) {
    BlockingDomain::blockOutputs = blockOutputs;
}


std::shared_ptr<Node> BlockingDomain::shallowClone(std::shared_ptr<SubSystem> parent) {
    return NodeFactory::shallowCloneNode<BlockingDomain>(parent, this);
}

void BlockingDomain::shallowCloneWithChildren(std::shared_ptr<SubSystem> parent,
                                                 std::vector<std::shared_ptr<Node>> &nodeCopies,
                                                 std::map<std::shared_ptr<Node>,
                                                 std::shared_ptr<Node>> &origToCopyNode,
                                                 std::map<std::shared_ptr<Node>,
                                                 std::shared_ptr<Node>> &copyToOrigNode) {
    //Very similar to the implementation for ClockDomain

    //Copy this node and put into vectors and maps
    std::shared_ptr<BlockingDomain> clonedNode = std::dynamic_pointer_cast<BlockingDomain>(shallowClone(parent));
    nodeCopies.push_back(clonedNode);
    origToCopyNode[shared_from_this()] = clonedNode;
    copyToOrigNode[clonedNode] = shared_from_this();

    //Copy children, including the SubDomainInput and SubDomainOutput nodes which should be nested
    for(auto it = children.begin(); it != children.end(); it++){
        //Recursive call to this function
        (*it)->shallowCloneWithChildren(clonedNode, nodeCopies, origToCopyNode, copyToOrigNode); //Use the copied node as the parent
    }

    //Run through the BlockingBoundary nodes and convert the pointers (should be copied by this point)
    for(auto it = blockInputs.begin(); it != blockInputs.end(); it++){
        std::shared_ptr<BlockingInput> blockInputCopy = std::dynamic_pointer_cast<BlockingInput>(origToCopyNode[*it]);
        clonedNode->addBlockInput(blockInputCopy);
    }

    for(auto it = blockOutputs.begin(); it != blockOutputs.end(); it++){
        std::shared_ptr<BlockingOutput> blockingOutputCopy = std::dynamic_pointer_cast<BlockingOutput>(origToCopyNode[*it]);
        clonedNode->addBlockOutput(blockingOutputCopy);
    }
}

void BlockingDomain::addBlockInput(std::shared_ptr<BlockingInput> input) {
    blockInputs.push_back(input);
}

void BlockingDomain::addBlockOutput(std::shared_ptr<BlockingOutput> output) {
    blockOutputs.push_back(output);
}

void BlockingDomain::validate() {
    Node::validate();

    //Check that the blocking length
    if(blockingLen < 1){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Blocking length must be >=1, is " + GeneralHelper::to_string(blockingLen), getSharedPointer()));
    }

    if(subBlockingLen < 1){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Sub-Blocking length must be >=1, is " + GeneralHelper::to_string(subBlockingLen), getSharedPointer()));
    }

    if(subBlockingLen > blockingLen){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Sub-Blocking length cannot be greater than blocking length", getSharedPointer()));
    }

    if(blockingLen % subBlockingLen != 0){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Blocking length must be a multiple of sub-blocking length", getSharedPointer()));
    }

    //Check that the Blocking boundary nodes have the same blocking configuration factor
    for(const std::shared_ptr<BlockingInput> &input : blockInputs){
        if(input->getBlockingLen() != blockingLen){
            throw std::runtime_error(ErrorHelpers::genErrorStr("Blocking Input (" + input->getFullyQualifiedName() + ") should have the same blocking length as Blocking Domain", getSharedPointer()));
        }
        if(input->getSubBlockingLen() != subBlockingLen){
            throw std::runtime_error(ErrorHelpers::genErrorStr("Blocking Input (" + input->getFullyQualifiedName() + ") should have the same sub-blocking length as Blocking Domain", getSharedPointer()));
        }
    }

    for(const std::shared_ptr<BlockingOutput> &output : blockOutputs){
        if(output->getBlockingLen() != blockingLen){
            throw std::runtime_error(ErrorHelpers::genErrorStr("Blocking Output (" + output->getFullyQualifiedName() + ") should have the same blocking length as Blocking Domain", getSharedPointer()));
        }
        if(output->getSubBlockingLen() != subBlockingLen){
            throw std::runtime_error(ErrorHelpers::genErrorStr("Blocking Output (" + output->getFullyQualifiedName() + ") should have the same sub-blocking length as Blocking Domain", getSharedPointer()));
        }
    }

    //Check that the Blocking boundary nodes are under the BlockingDomain, that there are no extras, and the discovered nodes are the same as the known nodes
    std::set<std::shared_ptr<BlockingInput>> discoveredBlockingInputs = BlockingHelpers::getBlockingInputsInBlockingDomainHelper(
            children);
    std::set<std::shared_ptr<BlockingOutput>> discoveredBlockingOutputs = BlockingHelpers::getBlockingOutputsInBlockingDomainHelper(
            children);

    if(discoveredBlockingInputs.size() != blockInputs.size()){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Number of Blocking Inputs does not match number of discovered Blocking Inputs (" + GeneralHelper::to_string(blockInputs.size()) + " != " + GeneralHelper::to_string(discoveredBlockingInputs.size()) + ")", getSharedPointer()));
    }

    if(discoveredBlockingOutputs.size() != blockOutputs.size()){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Number of Blocking Outputs does not match number of discovered Blocking Outputs (" + GeneralHelper::to_string(blockOutputs.size()) + " != " + GeneralHelper::to_string(discoveredBlockingOutputs.size()) + ")", getSharedPointer()));
    }

    std::set<std::shared_ptr<BlockingInput>> blockInputSet;
    blockInputSet.insert(blockInputs.begin(), blockInputs.end());
    if(discoveredBlockingInputs != blockInputSet){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Discovered Blocking Inputs not the same as Known Blocking Inputs", getSharedPointer()));
    }

    std::set<std::shared_ptr<BlockingOutput>> blockOutputSet;
    blockOutputSet.insert(blockOutputs.begin(), blockOutputs.end());
    if(discoveredBlockingOutputs != blockOutputSet){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Discovered Blocking Outputs not the same as Known Blocking Outputs", getSharedPointer()));
    }

    //Check that all connections between nodes in the blocking domain and nodes outside of the blocking domain
    //are via BlockingBoundary nodes
    std::set<std::shared_ptr<Node>> nodesInBlockingDomain = BlockingHelpers::getNodesInBlockingDomainHelper(children);
    for(const std::shared_ptr<Node> &node : nodesInBlockingDomain){
        if(GeneralHelper::isType<Node, BlockingBoundary>(node)){
            //BlockingBoundary nodes verify their connections are into/out of the Blocking domain as part of their validation functions
        }else{
            //All connections should be to nodes inside of the blocking domain, including those to BlockingBoundary nodes which should also be in the blocking domain
            //Note, can also be from nested blocking domain
            std::set<std::shared_ptr<Arc>> inArcs = node->getInputArcs();
            for(const std::shared_ptr<Arc> &inArc : inArcs){
                std::shared_ptr<Node> srcNode = inArc->getSrcPort()->getParent();
                std::shared_ptr<BlockingDomain> srcDomain = BlockingHelpers::findBlockingDomain(srcNode);
                std::shared_ptr<BlockingDomain> thisAsBlockingDomain = std::dynamic_pointer_cast<BlockingDomain>(getSharedPointer());
                if(BlockingHelpers::isOutsideBlockingDomain(srcDomain, thisAsBlockingDomain)){
                    throw std::runtime_error(ErrorHelpers::genErrorStr("Node in BlockingDomain (" + node->getFullyQualifiedName() + ") is connected to node outside of BlockingDomain (" + srcNode->getFullyQualifiedName() + ")", getSharedPointer()));
                }
            }

            std::set<std::shared_ptr<Arc>> outArcs = node->getOutputArcs();
            for(const std::shared_ptr<Arc> &outArc : outArcs){
                std::shared_ptr<Node> dstNode = outArc->getDstPort()->getParent();
                std::shared_ptr<BlockingDomain> dstDomain = BlockingHelpers::findBlockingDomain(dstNode);
                std::shared_ptr<BlockingDomain> thisAsBlockingDomain = std::dynamic_pointer_cast<BlockingDomain>(getSharedPointer());
                if(BlockingHelpers::isOutsideBlockingDomain(dstDomain, thisAsBlockingDomain)){
                    throw std::runtime_error(ErrorHelpers::genErrorStr("Node in BlockingDomain (" + node->getFullyQualifiedName() + ") is connected to node outside of BlockingDomain (" + dstNode->getFullyQualifiedName() + ")", getSharedPointer()));
                }
            }
        }
    }

    //The BlockingDomain nodes will validate the dimensions of the lines going into/out of them
}

int BlockingDomain::getNumberSubBlocks() {
    if(blockingLen % subBlockingLen != 0){
        //TODO: Remove.  Inserted as an extra assert but should be validated in the validate function
        throw std::runtime_error(ErrorHelpers::genErrorStr("Blocking length must be a multiple of sub-blocking length", getSharedPointer()));
    }

    return blockingLen/subBlockingLen;
}

std::string BlockingDomain::typeNameStr() {
    return "BlockingDomain";
}

xercesc::DOMElement *
BlockingDomain::emitGraphML(xercesc::DOMDocument *doc, xercesc::DOMElement *graphNode, bool include_block_node_type) {
    //Performs the same function as the Subsystem emit GraphML except that the block type is changed

    //Emit the basic info for this node
    xercesc::DOMElement *thisNode = emitGraphMLBasics(doc, graphNode);

    if (include_block_node_type){
        GraphMLHelper::addDataNode(doc, thisNode, "block_node_type", "Blocking Domain");
    }

    GraphMLHelper::addDataNode(doc, thisNode, "blocking_len", GeneralHelper::to_string(blockingLen));
    GraphMLHelper::addDataNode(doc, thisNode, "sub_blocking_len", GeneralHelper::to_string(subBlockingLen));

    emitGramphMLSubgraphAndChildren(doc, thisNode);

    return thisNode;
}

std::set<GraphMLParameter> BlockingDomain::graphMLParameters() {
    std::set<GraphMLParameter> parameters;

    parameters.insert(GraphMLParameter("blocking_len", "int", true));
    parameters.insert(GraphMLParameter("sub_blocking_len", "int", true));

    return parameters;
}

int BlockingDomain::getNumSubContexts() const {
    return 1;
}

std::vector<std::shared_ptr<Arc>> BlockingDomain::getContextDecisionDriver() {
    return std::vector<std::shared_ptr<Arc>>();
}

std::vector<Variable> BlockingDomain::getCContextVars() {
    std::vector<Variable> outputVars;

    for(const std::shared_ptr<BlockingOutput> &outputNode : blockOutputs){
        outputVars.push_back(outputNode->getOutputVar());
    }

    return outputVars;
}

Variable BlockingDomain::getCContextVar(int contextVarIndex) {
    return blockOutputs[contextVarIndex]->getOutputVar();
}

bool BlockingDomain::requiresContiguousContextEmits() {
    return true;
}

Variable BlockingDomain::getBlockIndVar(){
    int numBits = GeneralHelper::numIntegerBits(blockingLen, false);
    DataType dt(false, false, false, numBits, 0, {1});
    Variable indVar("blockDomain_"+GeneralHelper::to_string(id)+"_ind", dt);

    return indVar;
}

void BlockingDomain::emitCContextOpenFirst(std::vector<std::string> &cStatementQueue, SchedParams::SchedType schedType,
                                           int subContextNumber, int partitionNum) {
    if(subContextNumber != 0){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Blocking Domain Only Has 1 Context, Tried to Open Context " + GeneralHelper::to_string(subContextNumber), getSharedPointer()));
    }

    //We are emitting a static for loop.  No context drivers need to be emitted first, the logic is self-contained

    Variable indVar = getBlockIndVar();
    std::string indVarName = indVar.getCVarName(false);

    std::string cExpr = "for(" + indVar.getCVarDecl() + "=0; " + indVarName + "<" + GeneralHelper::to_string(blockingLen/subBlockingLen) + "; " + indVarName + "++){";
    cStatementQueue.push_back(cExpr);
}

void BlockingDomain::emitCContextOpenMid(std::vector<std::string> &cStatementQueue, SchedParams::SchedType schedType,
                                         int subContextNumber, int partitionNum) {
    throw std::runtime_error(ErrorHelpers::genErrorStr("Blocking Domain Requires Contiguous Emit", getSharedPointer()));
}

void BlockingDomain::emitCContextOpenLast(std::vector<std::string> &cStatementQueue, SchedParams::SchedType schedType,
                                          int subContextNumber, int partitionNum) {
    throw std::runtime_error(ErrorHelpers::genErrorStr("Blocking Domain Requires Contiguous Emit", getSharedPointer()));
}

void BlockingDomain::emitCContextCloseFirst(std::vector<std::string> &cStatementQueue, SchedParams::SchedType schedType,
                                            int subContextNumber, int partitionNum) {
    if(subContextNumber != 0){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Blocking Domain Only Has 1 Context, Tried to Close Context " + GeneralHelper::to_string(subContextNumber), getSharedPointer()));
    }

    cStatementQueue.push_back("}");
}

void BlockingDomain::emitCContextCloseMid(std::vector<std::string> &cStatementQueue, SchedParams::SchedType schedType,
                                          int subContextNumber, int partitionNum) {
    throw std::runtime_error(ErrorHelpers::genErrorStr("Blocking Domain Requires Contiguous Emit", getSharedPointer()));
}

void BlockingDomain::emitCContextCloseLast(std::vector<std::string> &cStatementQueue, SchedParams::SchedType schedType,
                                           int subContextNumber, int partitionNum) {
    throw std::runtime_error(ErrorHelpers::genErrorStr("Blocking Domain Requires Contiguous Emit", getSharedPointer()));
}

bool BlockingDomain::shouldReplicateContextDriver() {
    return false;
}

void BlockingDomain::removeBlockInput(std::shared_ptr<BlockingInput> blockInput) {
    blockInputs.erase(std::remove(blockInputs.begin(), blockInputs.end(), blockInput), blockInputs.end());
}

void BlockingDomain::removeBlockOutput(std::shared_ptr<BlockingOutput> blockOutput) {
    blockOutputs.erase(std::remove(blockOutputs.begin(), blockOutputs.end(), blockOutput), blockOutputs.end());
}

std::vector<std::shared_ptr<Node>> BlockingDomain::discoverAndMarkContexts(std::vector<Context> contextStack) {
    //Return all nodes in context (including ones from recursion)
    std::shared_ptr<BlockingDomain> thisAsBlockingDomain = std::dynamic_pointer_cast<BlockingDomain>(getSharedPointer());

    return ContextHelper::discoverAndMarkContexts_SubsystemContextRoots(contextStack, thisAsBlockingDomain);
}

