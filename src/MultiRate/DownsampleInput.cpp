//
// Created by Christopher Yarp on 4/29/20.
//

#include "DownsampleInput.h"
#include "MultiRateHelpers.h"
#include "General/ErrorHelpers.h"

DownsampleInput::DownsampleInput() {

}

DownsampleInput::DownsampleInput(std::shared_ptr<SubSystem> parent) : Downsample(parent) {

}


DownsampleInput::DownsampleInput(std::shared_ptr<SubSystem> parent, DownsampleInput *orig) : Downsample(parent, orig) {

}

std::set<GraphMLParameter> DownsampleInput::graphMLParameters() {
    return Downsample::graphMLParameters();
}

std::string DownsampleInput::typeNameStr() {
    return "DownsampleInput";
}

std::shared_ptr<DownsampleInput>
DownsampleInput::createFromGraphML(int id, std::string name, std::map<std::string, std::string> dataKeyValueMap,
                                   std::shared_ptr<SubSystem> parent, GraphMLDialect dialect) {
    std::shared_ptr<DownsampleInput> newNode = NodeFactory::createNode<DownsampleInput>(parent);

    newNode->populateDownsampleParametersFromGraphML(id, name, dataKeyValueMap, dialect);

    return newNode;
}

xercesc::DOMElement *
DownsampleInput::emitGraphML(xercesc::DOMDocument *doc, xercesc::DOMElement *graphNode, bool include_block_node_type) {
    xercesc::DOMElement* thisNode = emitGraphMLBasics(doc, graphNode);
    if(include_block_node_type) {
        GraphMLHelper::addDataNode(doc, thisNode, "block_node_type", "RateChange"); //This is a special type of block called a RateChange
    }
    GraphMLHelper::addDataNode(doc, thisNode, "block_function", "DownsampleInput");
    emitGraphMLProperties(doc, thisNode);

    return thisNode;
}

void DownsampleInput::validate() {
    Downsample::validate();

    std::shared_ptr<DownsampleInput> thisAsDownsampleInput = std::static_pointer_cast<DownsampleInput>(getSharedPointer());
    MultiRateHelpers::validateRateChangeInput_SetMasterRates(thisAsDownsampleInput, false);
}

std::shared_ptr<Node> DownsampleInput::shallowClone(std::shared_ptr<SubSystem> parent) {
    return NodeFactory::shallowCloneNode<DownsampleInput>(parent, this);
}

CExpr DownsampleInput::emitCExpr(std::vector<std::string> &cStatementQueue, SchedParams::SchedType schedType,
                                 int outputPortNum, bool imag) {
    //Get the Expression for the input (should only be 1)
    std::shared_ptr<OutputPort> srcOutputPort = getInputPort(0)->getSrcOutputPort();
    int srcOutputPortNum = srcOutputPort->getPortNum();
    std::shared_ptr<Node> srcNode = srcOutputPort->getParent();
    DataType inputDT = getInputPort(0)->getDataType();
    CExpr inputExpr = srcNode->emitC(cStatementQueue, schedType, srcOutputPortNum, imag);

    DataType outputDT = getOutputPort(0)->getDataType();

    if(useVectorSamplingMode){
        //If the input is a scalar, just pass through
        if (inputDT.isScalar()) {
            return CExpr(inputExpr.getExpr(),
                         CExpr::ExprType::SCALAR_EXPR); //This will create a new variable.  If input is a variable, this will create a copy
        }

        //Otherwise, the input is at least an array
        if (outputDT.isScalar()){
            //No array assignment, just get the first element of the input vec
            std::vector<std::string> indexVec = {"0"};
            return CExpr(inputExpr.getExprIndexed({indexVec}, true),
                         CExpr::ExprType::SCALAR_EXPR); //This will create a new variable.  If input is a variable, this will create a copy
        }

        //Array in and array out
        std::vector<int> inputDims = inputDT.getDimensions();
        int stride = downsampleRatio;
        //Phase not currently supported, starts at phase 0

        //NOTE: There are 2 scenarios which can occur and the output type vs. input type will reveal which case is occurring.
        //      - If the sub-blocking within the clock domain is 1 (the degenerate case), the dimensionality of the
        //        input type is reduced
        //      - If the sub-blocking within the clock domain is >1 (the normal case), the output dimension is just
        //        scaled down by the stride factor.

        bool degenerateCase;
        std::vector<int> degenerateCaseExpectedInputDims = outputDT.getDimensions();
        degenerateCaseExpectedInputDims.insert(degenerateCaseExpectedInputDims.begin(), stride);
        std::vector<int> standardCaseExpectedInputDims = outputDT.getDimensions();
        standardCaseExpectedInputDims[0] *= stride;
        if(degenerateCaseExpectedInputDims == inputDT.getDimensions()){
            degenerateCase = true;
        }else if(standardCaseExpectedInputDims == inputDT.getDimensions()){
            degenerateCase = false;
        }else{
            throw std::runtime_error(ErrorHelpers::genErrorStr("Unexpected input and output dimensions", getSharedPointer()));
        }

        //Create output var
        std::string outName = name + "_n" + GeneralHelper::to_string(id) + "_Out";
        Variable outputVar = Variable(outName, outputDT);
        cStatementQueue.push_back(outputVar.getCVarDecl(imag, true, true, true, false) + ";");

        std::tuple<std::vector<std::string>, std::vector<std::string>, std::vector<std::string>> forLoopStrs =
                EmitterHelpers::generateVectorMatrixForLoops(outputDT.getDimensions());

        std::vector<std::string> forLoopOpen = std::get<0>(forLoopStrs);
        std::vector<std::string> forLoopIndexVars = std::get<1>(forLoopStrs);
        std::vector<std::string> forLoopClose = std::get<2>(forLoopStrs);

        cStatementQueue.insert(cStatementQueue.end(), forLoopOpen.begin(), forLoopOpen.end());

        //Subsample the outer dimension of the
        std::vector<std::string> srcIndExprs = forLoopIndexVars;
        if(degenerateCase){
            //Pull from the 0, phase of the outer dimension of the input
            //The inner dimension is reduced from the output dimension
            srcIndExprs.insert(srcIndExprs.begin(), "0");
        }else {
            srcIndExprs[0] += "*" + GeneralHelper::to_string(stride);
        }

        std::string assignExpr =
                outputVar.getCVarName(imag) + EmitterHelpers::generateIndexOperation(forLoopIndexVars) + " = " +
                inputExpr.getExprIndexed(srcIndExprs, true) + ";";
        cStatementQueue.push_back(assignExpr);

        cStatementQueue.insert(cStatementQueue.end(), forLoopClose.begin(), forLoopClose.end());

        return CExpr(outputVar.getCVarName(imag), CExpr::ExprType::ARRAY);
    }else {
        //Just return the input expression

        //Create a temporary variable to avoid issue if this node is directly attached to state
        //at the input.  The state update is placed after this node but the variable from the delay is simply
        //passed through.  This could cause the state to be update before the result is used.
        //TODO: Remove Temporary when StateUpdate insertion logic improved to track passthroughs
        //Accomplished by returning a SCALAR_EXPR instead of a SCALAR_VAR

        if (inputDT.isScalar()) {
            return CExpr(inputExpr.getExpr(),
                         CExpr::ExprType::SCALAR_EXPR); //This will create a new variable.  If input is a variable, this will create a copy
        } else {
            std::string outName = name + "_n" + GeneralHelper::to_string(id) + "_OutMat";
            Variable outputVar = Variable(outName, outputDT);
            cStatementQueue.push_back(outputVar.getCVarDecl(imag, true, true, true, false) + ";");

            std::tuple<std::vector<std::string>, std::vector<std::string>, std::vector<std::string>> forLoopStrs =
                    EmitterHelpers::generateVectorMatrixForLoops(inputDT.getDimensions());

            std::vector<std::string> forLoopOpen = std::get<0>(forLoopStrs);
            std::vector<std::string> forLoopIndexVars = std::get<1>(forLoopStrs);
            std::vector<std::string> forLoopClose = std::get<2>(forLoopStrs);

            cStatementQueue.insert(cStatementQueue.end(), forLoopOpen.begin(), forLoopOpen.end());

            std::string assignExpr =
                    outputVar.getCVarName(imag) + EmitterHelpers::generateIndexOperation(forLoopIndexVars) + " = " +
                    inputExpr.getExprIndexed(forLoopIndexVars, true) + ";";
            cStatementQueue.push_back(assignExpr);

            cStatementQueue.insert(cStatementQueue.end(), forLoopClose.begin(), forLoopClose.end());

            return CExpr(outputVar.getCVarName(imag), CExpr::ExprType::ARRAY);
        }
    }
}

bool DownsampleInput::isSpecialized() {
    return true;
}

bool DownsampleInput::isInput() {
    return true;
}
