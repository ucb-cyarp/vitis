//
// Created by Christopher Yarp on 8/24/21.
//

#include "BlockingOutput.h"
#include "BlockingHelpers.h"
#include "General/ErrorHelpers.h"
#include "General/EmitterHelpers.h"

BlockingOutput::BlockingOutput() : BlockingBoundary() {

}

BlockingOutput::BlockingOutput(std::shared_ptr<SubSystem> parent) : BlockingBoundary(parent) {

}

BlockingOutput::BlockingOutput(std::shared_ptr<SubSystem> parent, BlockingOutput *orig) : BlockingBoundary(parent, orig){

}

void BlockingOutput::validate() {
    BlockingBoundary::validate();

    std::shared_ptr<BlockingDomain> blockingDomain = BlockingHelpers::findBlockingDomain(getSharedPointer());
    //Validated this could be found in Blocking Boundary

    //Check that this node is in the list of BlockingOutputs in the BlockingDomain (was already checked that this node was within the BlockingDomain in its validaiton function).
    // However, this guards against any issue with the findBlockingDomain function
    //TODO: Could potentially be removed
    std::shared_ptr<BlockingOutput> thisAsSubBlockingOutput = std::static_pointer_cast<BlockingOutput>(getSharedPointer());
    std::vector<std::shared_ptr<BlockingOutput>> blockOutputs = blockingDomain->getBlockOutputs();
    if(!GeneralHelper::contains(thisAsSubBlockingOutput, blockOutputs)){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Discovered Blocking Domain does not include BlockingOutput in list of known BlockingOutputs", getSharedPointer()));
    }

    //BlockingOutputs should be connected to nodes outside of the blocking domain at the outputs and connected to nodes inside of the blocking domain at the inputs
    std::set<std::shared_ptr<Arc>> outputArcs = getOutputArcs();
    for(const std::shared_ptr<Arc> &outArc : outputArcs){
        std::shared_ptr<Node> dstNode = outArc->getDstPort()->getParent();
        std::vector<std::shared_ptr<BlockingDomain>> dstDomainStack = BlockingHelpers::findBlockingDomainStack(dstNode);

        //Check that the Dst node is outside of the domain
        if(GeneralHelper::contains(blockingDomain, dstDomainStack)){
            throw std::runtime_error(ErrorHelpers::genErrorStr("Output (" + dstNode->getFullyQualifiedName() + ") from BlockingOutput should be outside of the BlockingDomain", getSharedPointer()));
        }
    }

    std::set<std::shared_ptr<Arc>> inputArcs = getInputArcs();
    for(const std::shared_ptr<Arc> &inArc : inputArcs){
        std::shared_ptr<Node> srcNode = inArc->getSrcPort()->getParent();

        if(GeneralHelper::isType<Node, BlockingOutput>(srcNode)){
            //One excepetion is that the src node is allowed to be a BlockingOutput of a BlockingDomain one level down

            std::vector<std::shared_ptr<BlockingDomain>> srcDomainStack = BlockingHelpers::findBlockingDomainStack(srcNode);
            if(srcDomainStack.size() <= 1){
                //This blocking input cannot be nested below the current BlockingDomain
                throw std::runtime_error(ErrorHelpers::genErrorStr("Input (" + srcNode->getFullyQualifiedName() + ") to BlockingOutput which is a BlockingOutput should be one level below the current BlockingDomain", getSharedPointer()));
            }
            //Look one level up from the bottom of the stack
            if(srcDomainStack[srcDomainStack.size()-2] != blockingDomain){
                throw std::runtime_error(ErrorHelpers::genErrorStr("Input (" + srcNode->getFullyQualifiedName() + ") to BlockingOutput which is a BlockingOutput should be one level below the current BlockingDomain", getSharedPointer()));
            }
        }else{
            std::shared_ptr<BlockingDomain> srcDomain = BlockingHelpers::findBlockingDomain(srcNode);
            if(srcDomain != blockingDomain){
                //Regular output nodes must be in the domain
                throw std::runtime_error(ErrorHelpers::genErrorStr("Input (" + srcNode->getFullyQualifiedName() + ") to BlockingOutput should be inside of the BlockingDomain", getSharedPointer()));
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

    //Number of dimensions is actually not reduced from output to input if subBlockingLen is not 1.
    //This allows for nested blocking without much difficulty

    //The outer dimension on the output side should be the block size and the outer dimension on the input side
    //should be the sub-blocking dimension.  If the subBlockingLen is 1, it expected that the extra dimension is removed
    //on the input side rather than having the degenerate extra dimension of 1 (unless it only has 1 dimension)

    if(blockingLen != outputType.getDimensions()[0]){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Outer dimension of output is expected to be the same as the blockingLen", getSharedPointer()));
    }

    if(subBlockingLen == 1 && outputType.getDimensions().size() > 1){
        //This is a special case where the number of dimension is reduced
        std::vector<int> outputDims = outputType.getDimensions();
        std::vector<int> expectedInDimensions(outputDims.begin()+1, outputDims.end());
        if(inputType.getDimensions() != expectedInDimensions){
            throw std::runtime_error(ErrorHelpers::genErrorStr("With subBlocking Length of 1, expected growth in dimensions from input to output.", getSharedPointer()));
        }
    }else{
        //Number of dimensions is not reduced but the inner dimension should be the sub-blocking factor
        std::vector<int> expectedInDimensions = outputType.getDimensions();
        expectedInDimensions[0] = subBlockingLen;
        if(inputType.getDimensions() != expectedInDimensions){
            throw std::runtime_error(ErrorHelpers::genErrorStr("Input dimension is unexpected", getSharedPointer()));
        }
    }
}

bool BlockingOutput::canExpand() {
    return false;
}

std::shared_ptr<Node> BlockingOutput::shallowClone(std::shared_ptr<SubSystem> parent) {
    return NodeFactory::shallowCloneNode<BlockingOutput>(parent, this);
}

xercesc::DOMElement *
BlockingOutput::emitGraphML(xercesc::DOMDocument *doc, xercesc::DOMElement *graphNode, bool include_block_node_type) {
    xercesc::DOMElement* thisNode = emitGraphMLBasics(doc, graphNode);
    if(include_block_node_type) {
        GraphMLHelper::addDataNode(doc, thisNode, "block_node_type", "Blocking Output");
    }

    emitBoundaryGraphMLParams(doc, thisNode); //Emit the common BlockBoundary parameters

    return thisNode;
}

void BlockingOutput::removeKnownReferences() {
    Node::removeKnownReferences();

    std::shared_ptr<BlockingDomain> blockingDomain = BlockingHelpers::findBlockingDomain(getSharedPointer());

    if(blockingDomain){
        std::shared_ptr<BlockingOutput> sharedThis = std::static_pointer_cast<BlockingOutput>(getSharedPointer());
        blockingDomain->removeBlockOutput(sharedThis);
    }
}

std::string BlockingOutput::typeNameStr() {
    return "Blocking Output";
}

CExpr BlockingOutput::emitCExpr(std::vector<std::string> &cStatementQueue, SchedParams::SchedType schedType,
                                int outputPortNum, bool imag) {
    //Emit the input
    std::shared_ptr<OutputPort> srcOutputPort = getInputPort(outputPortNum)->getSrcOutputPort();
    int srcOutputPortNum = srcOutputPort->getPortNum();
    std::shared_ptr<Node> srcNode = srcOutputPort->getParent();
    DataType inputDT = getInputPort(outputPortNum)->getDataType();
    CExpr inputExpr = srcNode->emitC(cStatementQueue, schedType, srcOutputPortNum, imag);

    DataType outputDT = getOutputPort(outputPortNum)->getDataType();

    std::shared_ptr<BlockingDomain> blockingDomain = BlockingHelpers::findBlockingDomain(getSharedPointer());
    Variable indexVar = blockingDomain->getBlockIndVar();

    //Unlike the BlockingInput, all outputs are written to the output variable
    Variable outputVar = getOutputVar();

    if(outputDT.isScalar()){
        //Check that the input DT is also scalar
        if(!inputDT.isScalar()){
            throw std::runtime_error(ErrorHelpers::genErrorStr("For a scalar output, input must be a scalar", getSharedPointer()));
        }

        cStatementQueue.push_back(outputVar.getCVarName(imag) + "=" + inputExpr.getExpr() + ";");

        //Return the context variable as a scalar
        return CExpr(outputVar.getCVarName(imag), CExpr::ExprType::SCALAR_VAR);
    }else{
        if(inputDT.isScalar()){
            if(!outputDT.isVector()){
                throw std::runtime_error(ErrorHelpers::genErrorStr("For a scalar input, input must be a vector", getSharedPointer()));
            }

            //Do not de-reference the input but need to dereference the output
            std::vector<std::string> outputInd = {indexVar.getCVarName(false) + (subBlockingLen > 1 ? "*" + GeneralHelper::to_string(subBlockingLen) : "")};
            cStatementQueue.push_back(outputVar.getCVarName(imag) + EmitterHelpers::generateIndexOperation(outputInd) + "=" + inputExpr.getExpr() + ";");
        }else{
            //Need to loop over data to copy
            std::tuple<std::vector<std::string>, std::vector<std::string>, std::vector<std::string>> forLoopStrs =
                    EmitterHelpers::generateVectorMatrixForLoops(inputDT.getDimensions());

            std::vector<std::string> forLoopOpen = std::get<0>(forLoopStrs);
            std::vector<std::string> forLoopIndexVars = std::get<1>(forLoopStrs);
            std::vector<std::string> forLoopClose = std::get<2>(forLoopStrs);

            //Open for loop for copy
            cStatementQueue.insert(cStatementQueue.end(), forLoopOpen.begin(), forLoopOpen.end());

            std::vector<std::string> outputIndexVars = forLoopIndexVars;

            //If the sub-blocking length is > 1, providing multiple elements within sub-block indexing is offset
            if(subBlockingLen>1){
                //There is no extra dimension, the index in the output is offset by the index variable
                outputIndexVars[0] = indexVar.getCVarName(false) + "*" + GeneralHelper::to_string(subBlockingLen) + "+" + outputIndexVars[0];
            }else{
                outputIndexVars.insert(outputIndexVars.begin(), indexVar.getCVarName(false));
            }
            cStatementQueue.push_back(outputVar.getCVarName(imag) + EmitterHelpers::generateIndexOperation(outputIndexVars) + "=" + inputExpr.getExprIndexed(forLoopIndexVars, true) + ";");

            //Close for loop for copy
            cStatementQueue.insert(cStatementQueue.end(), forLoopClose.begin(), forLoopClose.end());
        }

        //Return the context variable as an array
        return CExpr(outputVar.getCVarName(imag), CExpr::ExprType::ARRAY);
    }
}

Variable BlockingOutput::getOutputVar() {
    DataType outputType = getOutputPort(0)->getDataType();
    std::string varName = name+"_n"+GeneralHelper::to_string(id)+"_out";
    return Variable(varName, outputType);
}
