//
// Created by Christopher Yarp on 8/24/21.
//

#include "BlockingInput.h"
#include "BlockingDomain.h"
#include "BlockingHelpers.h"
#include "General/ErrorHelpers.h"
#include "General/GeneralHelper.h"
#include "General/EmitterHelpers.h"


BlockingInput::BlockingInput() : BlockingBoundary() {

}

BlockingInput::BlockingInput(std::shared_ptr<SubSystem> parent) : BlockingBoundary(parent) {

}

BlockingInput::BlockingInput(std::shared_ptr<SubSystem> parent, BlockingInput *orig) : BlockingBoundary(parent, orig) {

}

void BlockingInput::validate() {
    BlockingBoundary::validate();

    std::shared_ptr<BlockingDomain> blockingDomain = BlockingHelpers::findBlockingDomain(getSharedPointer());
    //Validated this could be found in Blocking Boundary

    //Check that this node is in the list of BlockingInputs in the BlockingDomain (was already checked that this node was within the BlockingDomain in its validaiton function).
    // However, this guards against any issue with the findBlockingDomain function
    //TODO: Could potentially be removed
    std::shared_ptr<BlockingInput> thisAsSubBlockingInput = std::static_pointer_cast<BlockingInput>(getSharedPointer());
    std::vector<std::shared_ptr<BlockingInput>> blockInputs = blockingDomain->getBlockInputs();
    if(!GeneralHelper::contains(thisAsSubBlockingInput, blockInputs)){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Discovered Blocking Domain does not include BlockingInput in list of known BlockingInputs", getSharedPointer()));
    }

    //BlockingInputs should be connected to nodes outside of the blocking domain at the inputs and connected to nodes inside of the blocking domain at the outputs
    std::set<std::shared_ptr<Arc>> inputArcs = getInputArcs();
    for(const std::shared_ptr<Arc> &inArc : inputArcs){
        std::shared_ptr<Node> srcNode = inArc->getSrcPort()->getParent();
        std::vector<std::shared_ptr<BlockingDomain>> srcDomainStack = BlockingHelpers::findBlockingDomainStack(srcNode);

        //Check that the Src node is outside of the domain
        if(GeneralHelper::contains(blockingDomain, srcDomainStack)){
            throw std::runtime_error(ErrorHelpers::genErrorStr("Input (" + srcNode->getFullyQualifiedName() + ") to BlockingInput should be outside of the BlockingDomain", getSharedPointer()));
        }
    }

    std::set<std::shared_ptr<Arc>> outputArcs = getOutputArcs();
    for(const std::shared_ptr<Arc> &outArc : outputArcs){
        std::shared_ptr<Node> dstNode = outArc->getDstPort()->getParent();

        if(GeneralHelper::isType<Node, BlockingInput>(dstNode)){
            //One excepetion is that the destination node is allowed to be a BlockingInput of a BlockingDomain one level down

            std::vector<std::shared_ptr<BlockingDomain>> dstDomainStack = BlockingHelpers::findBlockingDomainStack(dstNode);
            if(dstDomainStack.size() <= 1){
                //This blocking input cannot be nested below the current BlockingDomain
                throw std::runtime_error(ErrorHelpers::genErrorStr("Output (" + dstNode->getFullyQualifiedName() + ") from BlockingInput which is a BlockingInput should be one level below the current BlockingDomain", getSharedPointer()));
            }
            //Look one level up from the bottom of the stack
            if(dstDomainStack[dstDomainStack.size()-2] != blockingDomain){
                throw std::runtime_error(ErrorHelpers::genErrorStr("Output (" + dstNode->getFullyQualifiedName() + ") from BlockingInput which is a BlockingInput should be one level below the current BlockingDomain", getSharedPointer()));
            }
        }else{
            std::shared_ptr<BlockingDomain> dstDomain = BlockingHelpers::findBlockingDomain(dstNode);
            if(dstDomain != blockingDomain){
                //Regular output nodes must be in the domain
                throw std::runtime_error(ErrorHelpers::genErrorStr("Output (" + dstNode->getFullyQualifiedName() + ") from BlockingInput should be inside of the BlockingDomain", getSharedPointer()));
            }
        }
    }

    //Check the datatype
    DataType inputType = inputPorts[0]->getDataType();
    DataType outputType = outputPorts[0]->getDataType();

    DataType inputBaseType = inputType;
    inputBaseType.setDimensions({1});
    DataType outputBaseType = outputType;
    outputBaseType.setDimensions({1});

    if(inputBaseType != outputBaseType){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Input and Output Type should match", getSharedPointer()));
    }

    //Check the dimensions

    //Number of dimensions is actually not redused from input to output if subBlockingLen is not 1.
    //This allows for nested blocking without much difficulty

    //The outer dimension on the input side should be the block size and the outer dimension on the output side
    //should be the sub-blocking dimension.  If the subBlockingLen is 1, it expected that the extra dimension is removed
    //rather than having the degenerate extra dimension of 1 (unless it only has 1 dimension)

    if(blockingLen != inputType.getDimensions()[0]){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Outer dimension of input is expected to be the same as the blockingLen", getSharedPointer()));
    }

    if(subBlockingLen == 1 && inputType.getDimensions().size() > 1){
        //This is a special case where the number of dimension is reduced
        std::vector<int> inputDims = inputType.getDimensions();
        std::vector<int> expectedOutDimensions(inputDims.begin()+1, inputDims.end());
        if(outputType.getDimensions() != expectedOutDimensions){
            throw std::runtime_error(ErrorHelpers::genErrorStr("With subBlocking Length of 1, expected reduction in dimensions from input to output.", getSharedPointer()));
        }
    }else{
        //Number of dimensions is not reduced but the outer dimension should be the sub-blocking factor
        std::vector<int> expectedOutDimensions = inputType.getDimensions();
        expectedOutDimensions[0] = subBlockingLen;
        if(outputType.getDimensions() != expectedOutDimensions){
            throw std::runtime_error(ErrorHelpers::genErrorStr("Output dimension is unexpected", getSharedPointer()));
        }
    }
}

bool BlockingInput::canExpand() {
    return false;
}

std::shared_ptr<Node> BlockingInput::shallowClone(std::shared_ptr<SubSystem> parent) {
    return NodeFactory::shallowCloneNode<BlockingInput>(parent, this);
}

xercesc::DOMElement *
BlockingInput::emitGraphML(xercesc::DOMDocument *doc, xercesc::DOMElement *graphNode, bool include_block_node_type) {
    xercesc::DOMElement* thisNode = emitGraphMLBasics(doc, graphNode);
    if(include_block_node_type) {
        GraphMLHelper::addDataNode(doc, thisNode, "block_node_type", "Blocking Input");
    }

    emitBoundaryGraphMLParams(doc, thisNode); //Emit the common BlockBoundary parameters

    return thisNode;
}

void BlockingInput::removeKnownReferences() {
    Node::removeKnownReferences();

    std::shared_ptr<BlockingDomain> blockingDomain = BlockingHelpers::findBlockingDomain(getSharedPointer());

    if(blockingDomain){
        std::shared_ptr<BlockingInput> sharedThis = std::static_pointer_cast<BlockingInput>(getSharedPointer());
        blockingDomain->removeBlockInput(sharedThis);
    }
}

CExpr
BlockingInput::emitCExpr(std::vector<std::string> &cStatementQueue, SchedParams::SchedType schedType, int outputPortNum,
                         bool imag) {
    //Emit the input
    std::shared_ptr<OutputPort> srcOutputPort = getInputPort(outputPortNum)->getSrcOutputPort();
    int srcOutputPortNum = srcOutputPort->getPortNum();
    std::shared_ptr<Node> srcNode = srcOutputPort->getParent();
    DataType inputDT = getInputPort(outputPortNum)->getDataType();
    CExpr inputExpr = srcNode->emitC(cStatementQueue, schedType, srcOutputPortNum, imag);

    //Indexes along the first dimension.  If the sub-blocking length is 1, the dimensionality is decreased by 1.
    //Note that if the input is a vector, the number of dimensions is not changed

    DataType outputType = inputDT;
    std::vector<int> outputDim = BlockingHelpers::blockingDomainDimensionReduce(inputDT.getDimensions(), blockingLen, subBlockingLen);
    outputType.setDimensions(outputDim);

    //TODO: Remove Check
    DataType outputTypeFromArc = getOutputPort(0)->getDataType();
    if(outputType != outputTypeFromArc){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Disagreement between expected output port type and output port type", getSharedPointer()));
    }

    std::shared_ptr<BlockingDomain> blockingDomain = BlockingHelpers::findBlockingDomain(getSharedPointer());
    Variable indexVar = blockingDomain->getBlockIndVar();

    if(outputType.isScalar()){
        //We index into the input (if necessary) and return that as a scalar expression.  A temporary variable will be
        //created automatically if used by multiple destinations
        if(inputDT.isScalar()){
            //Do not dereference, simply return the input
            return CExpr(inputExpr.getExpr(), CExpr::ExprType::SCALAR_EXPR);
        }else{
            //The input should be a vector and needs to be dereferenced
            if(!inputDT.isVector()){
                throw std::runtime_error(ErrorHelpers::genErrorStr("For scalar output type, input type is expected to be a vector or scalar", getSharedPointer()));
            }

            std::vector<std::string> indVector = {indexVar.getCVarName(false) + (subBlockingLen > 1 ? "*" + GeneralHelper::to_string(subBlockingLen) : "")}; //Multiply the index by the sub blocking length.  Th
            std::string inputExprDeref = inputExpr.getExprIndexed(indVector, true);
            return CExpr(inputExprDeref, CExpr::ExprType::SCALAR_EXPR);
        }
    }else{
        //Does not create a copy but rather passes through the indexed expression.
        //Note that BlockingInput::passesThroughInputs() was overwritten to return true
        //And state update node creation was updated to

        //It is possible for there to be a degenerate case where the blocking and sub-blocking length are both 1.
        //In that case, it is a simple copy
        std::string outputExpr;
        if(blockingLen >1 ) {
            if (subBlockingLen > 1) {
                //If the sub-blocking length is > 1, providing multiple elements within sub-block indexing is offset
                //Do not dereference the outer dimension as it will be indexed into by the destination node
                std::vector<std::string> indexExpr = {indexVar.getCVarName(false) + "*" + GeneralHelper::to_string(subBlockingLen)};
                outputExpr = inputExpr.getExprIndexed(indexExpr, false);
            } else {
                //Dereference the outer dimension
                std::vector<std::string> indexExpr = {indexVar.getCVarName(false)};
                outputExpr = inputExpr.getExprIndexed(indexExpr, true);
            }
        }else{
            //If blocking length is 1, sub-blocking length must also be 1
            //TODO: Remove check
            if(subBlockingLen != 1){
                throw std::runtime_error(ErrorHelpers::genErrorStr("When blocking length is 1, sub-blocking length must also be 1", getSharedPointer()));
            }
        }

        outputExpr = "(" + outputExpr + ")";

        cStatementQueue.push_back("//Blocking Input Passthrough Expression: " + outputExpr);

        return CExpr(outputExpr, CExpr::ExprType::ARRAY, true);
    }
}

std::string BlockingInput::typeNameStr() {
    return "Blocking Input";
}

bool BlockingInput::passesThroughInputs() {
    return true;
}