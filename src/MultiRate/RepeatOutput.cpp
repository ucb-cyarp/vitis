//
// Created by Christopher Yarp on 4/29/20.
//

#include "RepeatOutput.h"
#include "General/ErrorHelpers.h"
#include "MultiRateHelpers.h"
#include "GraphCore/StateUpdate.h"
#include "Blocking/BlockingHelpers.h"

RepeatOutput::RepeatOutput() : useCompressedOutputType(true) {

}

RepeatOutput::RepeatOutput(std::shared_ptr<SubSystem> parent) : Repeat(parent), useCompressedOutputType(true) {

}

RepeatOutput::RepeatOutput(std::shared_ptr<SubSystem> parent, RepeatOutput *orig) : Repeat(parent, orig), stateVar(orig->stateVar), initCondition(orig->initCondition), useCompressedOutputType(orig->useCompressedOutputType) {

}

Variable RepeatOutput::getStateVar() const {
    return stateVar;
}

void RepeatOutput::setStateVar(const Variable &stateVar) {
    RepeatOutput::stateVar = stateVar;
}

std::vector<NumericValue> RepeatOutput::getInitCondition() const {
    return initCondition;
}

void RepeatOutput::setInitCondition(const std::vector<NumericValue> &initCondition) {
    RepeatOutput::initCondition = initCondition;
}

xercesc::DOMElement *
RepeatOutput::emitGraphML(xercesc::DOMDocument *doc, xercesc::DOMElement *graphNode, bool include_block_node_type) {
    xercesc::DOMElement* thisNode = emitGraphMLBasics(doc, graphNode);
    if(include_block_node_type) {
        GraphMLHelper::addDataNode(doc, thisNode, "block_node_type", "RateChange");
    }

    emitGraphMLProperties(doc, thisNode);

    GraphMLHelper::addDataNode(doc, thisNode, "InitialCondition", NumericValue::toString(initCondition));

    return thisNode;
}

std::set<GraphMLParameter> RepeatOutput::graphMLParameters() {
    std::set<GraphMLParameter> params = Repeat::graphMLParameters();

    //TODO: Declaring types as string so that complex can be stored.  Re-evaluate this
    params.insert(GraphMLParameter("InitialCondition", "string", true));

    return params;
}


std::shared_ptr<RepeatOutput>
RepeatOutput::createFromGraphML(int id, std::string name, std::map<std::string, std::string> dataKeyValueMap,
                                std::shared_ptr<SubSystem> parent, GraphMLDialect dialect) {
    std::shared_ptr<RepeatOutput> newNode = NodeFactory::createNode<RepeatOutput>(parent);

    newNode->populateRepeatParametersFromGraphML(id, name, dataKeyValueMap, dialect);

    std::string initialConditionStr;

    //This is a vitis only node
    if (dialect == GraphMLDialect::VITIS) {
        //Vitis Names -- DownsampleRatio
        initialConditionStr = dataKeyValueMap.at("InitialCondition");
    } else {
        throw std::runtime_error(ErrorHelpers::genErrorStr("Unsupported Dialect when parsing XML - RepeatOutput", newNode));
    }

    newNode->setInitCondition(NumericValue::parseXMLString(initialConditionStr));

    return newNode;
}

std::string RepeatOutput::typeNameStr() {
    return "RepeatOutput";
}

std::string RepeatOutput::labelStr() {
    std::string label = Repeat::labelStr();

    label += "\nInitialCondition: " + NumericValue::toString(initCondition);

    return label;
}

void RepeatOutput::validate() {
    Repeat::validate();

    std::shared_ptr<RepeatOutput> thisAsRepeatOutput = std::static_pointer_cast<RepeatOutput>(getSharedPointer());
    MultiRateHelpers::validateRateChangeOutput_SetMasterRates(thisAsRepeatOutput, false);
}

std::shared_ptr<Node> RepeatOutput::shallowClone(std::shared_ptr<SubSystem> parent) {
    return NodeFactory::shallowCloneNode<RepeatOutput>(parent, this);
}

bool RepeatOutput::hasState() {
    if(useVectorSamplingMode){
        //TODO: Change if phase can be configured to not be 0.  Otherwise, some state will be needed to cary an element over
        return false;
    }else {
        return true;
    }
}

bool RepeatOutput::hasCombinationalPath() {
    return true;
}

bool RepeatOutput::createStateUpdateNode(std::vector<std::shared_ptr<Node>> &new_nodes,
                                         std::vector<std::shared_ptr<Node>> &deleted_nodes,
                                         std::vector<std::shared_ptr<Arc>> &new_arcs,
                                         std::vector<std::shared_ptr<Arc>> &deleted_arcs, bool includeContext) {
    //TODO: Remove Check
    if(isUsingVectorSamplingMode()){
        throw std::runtime_error(ErrorHelpers::genErrorStr("When using useVectorSamplingMode, state update nodes should not be created", getSharedPointer()));
    }

    //The state update node will not be directly dependent on this port since
    //any singal feeding into the EnableOutput had to pass through an enable input

    //Do not check Node::passesThroughInputs since this StateUpdate is used to implement Latch like behavior with the destination being the state element

    std::shared_ptr<StateUpdate> stateUpdate = NodeFactory::createNode<StateUpdate>(getParent());
    stateUpdate->setName("StateUpdate-For-"+getName());
    stateUpdate->setPartitionNum(partitionNum);
    stateUpdate->setBaseSubBlockingLen(baseSubBlockingLen);
    stateUpdate->setPrimaryNode(getSharedPointer());
    addStateUpdateNode(stateUpdate); //Set the state update node pointer in this node

    if(includeContext) {
        //Set context to be the same as the primary node
        std::vector<Context> primarayContext = getContext();
        stateUpdate->setContext(primarayContext);

        //Add node to the lowest level context (if such a context exists
        if (!primarayContext.empty()) {
            Context specificContext = primarayContext[primarayContext.size() - 1];
            specificContext.getContextRoot()->addSubContextNode(specificContext.getSubContext(), stateUpdate);
        }
    }

    new_nodes.push_back(stateUpdate);

    //rewire input arc
    std::set<std::shared_ptr<Arc>> inputArcs = getInputPort(0)->getArcs();
    if(inputArcs.size() == 0){
        throw std::runtime_error("Encountered RepeatOutput node with no inputs when creating state update");
    }

    std::shared_ptr<InputPort> origDestPort = (*inputArcs.begin())->getDstPort();
    DataType origDataType = (*inputArcs.begin())->getDataType();
    double origSampleTime = (*inputArcs.begin())->getSampleTime();

    (*inputArcs.begin())->setDstPortUpdateNewUpdatePrev(stateUpdate->getInputPortCreateIfNot(0));

    //Create new
    std::shared_ptr<Arc> newArc = Arc::connectNodes(stateUpdate->getOutputPortCreateIfNot(0), origDestPort, origDataType, origSampleTime);

    new_arcs.push_back(newArc);

    return true;
}

CExpr
RepeatOutput::emitCExpr(std::vector<std::string> &cStatementQueue, SchedParams::SchedType schedType, int outputPortNum,
                        bool imag) {
    if(useVectorSamplingMode) {

        std::shared_ptr<OutputPort> srcOutputPort = getInputPort(0)->getSrcOutputPort();
        int srcOutputPortNum = srcOutputPort->getPortNum();
        std::shared_ptr<Node> srcNode = srcOutputPort->getParent();
        CExpr inputExpr = srcNode->emitC(cStatementQueue, schedType, srcOutputPortNum, imag);
        DataType inputDT = getInputPort(0)->getDataType();

        Variable outputVar = getVectorModeOutputVariable();
        DataType outputDT = getOutputPort(0)->getDataType();

        if(outputDT.isScalar()){
            //TODO: remove check
            if(!inputDT.isScalar()){
                throw std::runtime_error(ErrorHelpers::genErrorStr("Unexpected input/output dimensions", getSharedPointer()));
            }
            //Just pass the input value (which must be a scalar)
            //Need to assign to output var explicitly instead of just returning the expression because
            //the result needs to be accessible outside of the clock domain
            cStatementQueue.insert(cStatementQueue.end(), outputVar.getCVarName(imag) + "=" + inputExpr.getExpr() + ";");

            return CExpr(outputVar.getCVarName(imag),CExpr::ExprType::SCALAR_VAR);
        }else{
            //Unlike upsample, we can't pull the trick of setting the output to zero then filling in the
            if(inputDT.isScalar()){
                //TODO: Remove check
                if(!outputDT.isVector()){
                    throw std::runtime_error(ErrorHelpers::genErrorStr("Unexpected input/output dimensions", getSharedPointer()));
                }

                if(useCompressedOutputType){
                    //Copy input
                    std::string outputAssign = outputVar.getCVarName(imag) + "=" + inputExpr.getExpr() + ";";
                    cStatementQueue.insert(cStatementQueue.end(), outputAssign);

                    int blockSize = outputDT.getDimensions()[0];
                    return CExpr(outputVar.getCVarName(imag), blockSize, upsampleRatio, CExpr::ExprType::SCALAR_VAR_REPEAT);
                }else {
                    //If input is scalar, simply copy the value all positions of the array
                    std::tuple<std::vector<std::string>, std::vector<std::string>, std::vector<std::string>> forLoopStrs =
                            EmitterHelpers::generateVectorMatrixForLoops(outputDT.getDimensions());
                    std::vector<std::string> forLoopOpen = std::get<0>(forLoopStrs);
                    std::vector<std::string> forLoopIndexVars = std::get<1>(forLoopStrs);
                    std::vector<std::string> forLoopClose = std::get<2>(forLoopStrs);
                    cStatementQueue.insert(cStatementQueue.end(), forLoopOpen.begin(), forLoopOpen.end());

                    std::string outputAssign =
                            outputVar.getCVarName(imag) + EmitterHelpers::generateIndexOperation(forLoopIndexVars) +
                            "=" + inputExpr.getExpr() + ";";
                    cStatementQueue.insert(cStatementQueue.end(), outputAssign);

                    cStatementQueue.insert(cStatementQueue.end(), forLoopClose.begin(), forLoopClose.end());
                }
            }else{
                if(useCompressedOutputType) {
                    //Copy input into output var.  The variable has the same type as the input
                    std::tuple<std::vector<std::string>, std::vector<std::string>, std::vector<std::string>> forLoopStrs =
                            EmitterHelpers::generateVectorMatrixForLoops(inputDT.getDimensions());
                    std::vector<std::string> forLoopOpen = std::get<0>(forLoopStrs);
                    std::vector<std::string> forLoopIndexVars = std::get<1>(forLoopStrs);
                    std::vector<std::string> forLoopClose = std::get<2>(forLoopStrs);
                    cStatementQueue.insert(cStatementQueue.end(), forLoopOpen.begin(), forLoopOpen.end());

                    std::string outputAssign =
                            outputVar.getCVarName(imag) + EmitterHelpers::generateIndexOperation(forLoopIndexVars) +
                            "=" + inputExpr.getExprIndexed(forLoopIndexVars, true) + ";";
                    cStatementQueue.insert(cStatementQueue.end(), outputAssign);
                    cStatementQueue.insert(cStatementQueue.end(), forLoopClose.begin(), forLoopClose.end());

                    int blockSize = outputDT.getDimensions()[0];
                    return CExpr(outputVar.getCVarName(imag), blockSize, upsampleRatio, CExpr::ExprType::ARRAY_REPEAT);
                }else {
                    //Otherwise, loop over the input and write into the output, replicating the output according to the upsample rate
                    int copies = upsampleRatio;

                    //NOTE: There are 2 scenarios which can occur and the output type vs. input type will reveal which case is occurring.
                    //      - If the sub-blocking within the clock domain is 1 (the degenerate case), the vector/matrix at the input is the
                    //        primitive being repeated.  The dimensionality at the output should be expanded and the copies should be made
                    //      - If the sub-blocking within the clock domain is >1 (the normal case), the outer dimension is expanded by the
                    //        number of copies

                    bool degenerateCase;
                    std::vector<int> degenerateCaseExpectedOutputDims = inputDT.getDimensions();
                    degenerateCaseExpectedOutputDims.insert(degenerateCaseExpectedOutputDims.begin(), copies);
                    std::vector<int> standardCaseExpectedOutputDims = inputDT.getDimensions();
                    standardCaseExpectedOutputDims[0] *= copies;
                    if (degenerateCaseExpectedOutputDims == outputDT.getDimensions()) {
                        degenerateCase = true;
                    } else if (standardCaseExpectedOutputDims == outputDT.getDimensions()) {
                        degenerateCase = false;
                    } else {
                        throw std::runtime_error(ErrorHelpers::genErrorStr("Unexpected input and output dimensions",
                                                                           getSharedPointer()));
                    }

                    std::vector<int> forLoopDims = inputDT.getDimensions();
                    //Will make 2nd loop (index 1 in the loop vars) iterate over the copies
                    forLoopDims.insert(forLoopDims.begin(), copies);
                    std::tuple<std::vector<std::string>, std::vector<std::string>, std::vector<std::string>> forLoopStrs =
                            EmitterHelpers::generateVectorMatrixForLoops(forLoopDims);
                    std::vector<std::string> forLoopOpen = std::get<0>(forLoopStrs);
                    std::vector<std::string> forLoopIndexVars = std::get<1>(forLoopStrs);
                    std::vector<std::string> forLoopClose = std::get<2>(forLoopStrs);
                    cStatementQueue.insert(cStatementQueue.end(), forLoopOpen.begin(), forLoopOpen.end());

                    std::vector<std::string> forLoopIndexVarsInput = forLoopIndexVars;
                    //Ignore the copy loop entirely when it comes to indexing
                    forLoopIndexVarsInput.erase(forLoopIndexVarsInput.begin());

                    std::vector<std::string> forLoopIndexVarsOutput = forLoopIndexVars;
                    if (!degenerateCase) {
                        forLoopIndexVarsOutput[1] += "*" + GeneralHelper::to_string(copies) + "+" +
                                                     forLoopIndexVarsOutput[0]; //When writing into the output, the stride of the 1st dimension is set by the upsample rate
                        forLoopIndexVarsOutput.erase(forLoopIndexVarsOutput.begin());
                    }
                    //Otherwise, copy into the outer dimension (keep the additional copy dimension added to the input type)

                    std::string outputAssign = outputVar.getCVarName(imag) +
                                               EmitterHelpers::generateIndexOperation(forLoopIndexVarsOutput) + "=" +
                                               inputExpr.getExpr() +
                                               EmitterHelpers::generateIndexOperation(forLoopIndexVarsInput) + ";";
                    cStatementQueue.insert(cStatementQueue.end(), outputAssign);

                    cStatementQueue.insert(cStatementQueue.end(), forLoopClose.begin(), forLoopClose.end());
                }
            }

            return CExpr(outputVar.getCVarName(imag),CExpr::ExprType::ARRAY);
        }
    }else{
        return CExpr(stateVar.getCVarName(imag), stateVar.getDataType().isScalar() ? CExpr::ExprType::SCALAR_VAR : CExpr::ExprType::ARRAY); //This is a variable name therefore inform the cEmit function
    }
}

std::vector<Variable> RepeatOutput::getCStateVars() {
    std::vector<Variable> vars;

    DataType stateType = getInputPort(0)->getDataType();

    std::string varName = name+"_n"+GeneralHelper::to_string(id)+"_state";
    Variable var = Variable(varName, stateType, initCondition, false, true);
    stateVar = var;

    vars.push_back(var);

    return vars;
}

void RepeatOutput::emitCStateUpdate(std::vector<std::string> &cStatementQueue, SchedParams::SchedType schedType, std::shared_ptr<StateUpdate> stateUpdateSrc) {
    //TODO: Remove Check
    if(isUsingVectorSamplingMode()){
        throw std::runtime_error(ErrorHelpers::genErrorStr("When using useVectorSamplingMode, state update nodes should not be created", getSharedPointer()));
    }

    DataType inputDataType = getInputPort(0)->getDataType();
    std::shared_ptr<OutputPort> srcPort = getInputPort(0)->getSrcOutputPort();
    int srcOutPortNum = srcPort->getPortNum();
    std::shared_ptr<Node> srcNode = srcPort->getParent();

    //Emit the upstream
    CExpr inputExprRe = srcNode->emitC(cStatementQueue, schedType, srcOutPortNum, false);
    CExpr inputExprIm;

    if(inputDataType.isComplex()){
        inputExprIm = srcNode->emitC(cStatementQueue, schedType, srcOutPortNum, true);
    }

    std::vector<std::string> forLoopIndexVars;
    std::vector<std::string> forLoopClose;
    if(!inputDataType.isScalar()){
        //Emit For Loop
        std::tuple<std::vector<std::string>, std::vector<std::string>, std::vector<std::string>> forLoopStrs =
                EmitterHelpers::generateVectorMatrixForLoops(inputDataType.getDimensions());

        std::vector<std::string> forLoopOpen = std::get<0>(forLoopStrs);
        forLoopIndexVars = std::get<1>(forLoopStrs);
        forLoopClose = std::get<2>(forLoopStrs);

        cStatementQueue.insert(cStatementQueue.end(), forLoopOpen.begin(), forLoopOpen.end());
    }

    std::string assignExprRe = stateVar.getCVarName(false) + (inputDataType.isScalar() ? "" : EmitterHelpers::generateIndexOperation(forLoopIndexVars)) +
                             " = " + (inputDataType.isScalar() ? inputExprRe.getExpr() : inputExprRe.getExprIndexed(forLoopIndexVars, true)) + ";";
    cStatementQueue.push_back(assignExprRe);

    if(stateVar.getDataType().isComplex()){
        std::string assignExprIm = stateVar.getCVarName(true) + (inputDataType.isScalar() ? "" : EmitterHelpers::generateIndexOperation(forLoopIndexVars)) +
                                   " = " + (inputDataType.isScalar() ? inputExprIm.getExpr() : inputExprIm.getExprIndexed(forLoopIndexVars, true)) + ";";
        cStatementQueue.push_back(assignExprIm);
    }

    if(!inputDataType.isScalar()) {
        //Close For Loop
        cStatementQueue.insert(cStatementQueue.end(), forLoopClose.begin(), forLoopClose.end());
    }
}

bool RepeatOutput::isSpecialized() {
    return true;
}

Variable RepeatOutput::getVectorModeOutputVariable(){
    DataType outputDT = getOutputPort(0)->getDataType();
    if(useCompressedOutputType){
        //If using compressed types, the output datatype should be the dimension of the input port
        //The input is stored in this variable and indexing with the repeat will be handled at the output
        std::vector<int> unBlockedOutputDTDims = getInputPort(0)->getDataType().getDimensions();
        outputDT.setDimensions(unBlockedOutputDTDims);
    }
    std::string outName = name + "_n" + GeneralHelper::to_string(id) + "_Out";
    return Variable(outName, outputDT);
}

std::vector<Variable> RepeatOutput::getVariablesToDeclareOutsideClockDomain() {
    std::vector<Variable> extVars = RateChange::getVariablesToDeclareOutsideClockDomain();

    if(useVectorSamplingMode){
        //Do not need to declare state (unless phase != 0 is later introduced).  However, need to declare the output
        //outside of the clock domain
        //TODO: Change if phase can be configured to not be 0.  Otherwise, some state will be needed to cary an element over

        extVars.push_back(getVectorModeOutputVariable());
    }

    return extVars;
}

bool RepeatOutput::isUseCompressedOutputType() const {
    return useCompressedOutputType;
}

void RepeatOutput::setUseCompressedOutputType(bool useCompressedOutputType) {
    RepeatOutput::useCompressedOutputType = useCompressedOutputType;
}

void RepeatOutput::specializeForBlocking(int localBlockingLength, int localSubBlockingLength,
                                            std::vector<std::shared_ptr<Node>> &nodesToAdd,
                                            std::vector<std::shared_ptr<Node>> &nodesToRemove,
                                            std::vector<std::shared_ptr<Arc>> &arcsToAdd,
                                            std::vector<std::shared_ptr<Arc>> &arcsToRemove,
                                            std::vector<std::shared_ptr<Node>> &nodesToRemoveFromTopLevel,
                                            std::map<std::shared_ptr<Arc>, std::tuple<int, int, bool, bool>> &arcsWithDeferredBlockingExpansion) {
    if(useVectorSamplingMode){
        //Do nothing, the logic is handled internally in the implementations of RateChange

        //However, need to request expansion of arcs in case the src or dst has a different base sub-blocking length

        std::set<std::shared_ptr<Arc>> inputArcs = getInputArcs();
        for(const std::shared_ptr<Arc> &arc : inputArcs){
            std::tuple<int, int, bool, bool> expansion = {0, 0, false, false};
            if(GeneralHelper::contains(arc, arcsWithDeferredBlockingExpansion)){
                expansion = arcsWithDeferredBlockingExpansion[arc];
            }

            //localBlockingLength is the blocking size inside the blocking domain

            std::get<1>(expansion) = localBlockingLength;
            std::get<3>(expansion) = true;

            arcsWithDeferredBlockingExpansion[arc] = expansion;
        }

        std::set<std::shared_ptr<Arc>> outputArcs = getOutputArcs();
        for(const std::shared_ptr<Arc> &arc : outputArcs){
            std::tuple<int, int, bool, bool> expansion = {0, 0, false, false};
            if(GeneralHelper::contains(arc, arcsWithDeferredBlockingExpansion)){
                expansion = arcsWithDeferredBlockingExpansion[arc];
            }

            //localBlockingLength is the blocking size inside the blocking domain
            int blockingLengthOutsideClkDomain = localBlockingLength*getUpsampleRatio();
            std::get<0>(expansion) = blockingLengthOutsideClkDomain;
            std::get<2>(expansion) = true;

            arcsWithDeferredBlockingExpansion[arc] = expansion;
        }

    }else{
        Node::specializeForBlocking(localBlockingLength,
                                    localSubBlockingLength,
                                    nodesToAdd,
                                    nodesToRemove,
                                    arcsToAdd,
                                    arcsToRemove,
                                    nodesToRemoveFromTopLevel,
                                    arcsWithDeferredBlockingExpansion);
    }
}