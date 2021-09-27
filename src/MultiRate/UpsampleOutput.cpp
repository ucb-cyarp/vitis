//
// Created by Christopher Yarp on 4/29/20.
//

#include "UpsampleOutput.h"
#include "General/ErrorHelpers.h"
#include "MultiRateHelpers.h"
#include "GraphCore/StateUpdate.h"
#include "GraphCore/ContextFamilyContainer.h"
#include "GraphCore/ContextContainer.h"

UpsampleOutput::UpsampleOutput() {

}

UpsampleOutput::UpsampleOutput(std::shared_ptr<SubSystem> parent) : Upsample(parent) {

}

UpsampleOutput::UpsampleOutput(std::shared_ptr<SubSystem> parent, UpsampleOutput *orig) : Upsample(parent, orig), stateVar(orig->stateVar) {

}

Variable UpsampleOutput::getStateVar() const {
    return stateVar;
}

void UpsampleOutput::setStateVar(const Variable &stateVar) {
    UpsampleOutput::stateVar = stateVar;
}

xercesc::DOMElement *
UpsampleOutput::emitGraphML(xercesc::DOMDocument *doc, xercesc::DOMElement *graphNode, bool include_block_node_type) {
    xercesc::DOMElement* thisNode = emitGraphMLBasics(doc, graphNode);
    if(include_block_node_type) {
        GraphMLHelper::addDataNode(doc, thisNode, "block_node_type", "RateChange");
    }

    emitGraphMLProperties(doc, thisNode);

    return thisNode;
}

std::set<GraphMLParameter> UpsampleOutput::graphMLParameters() {
    std::set<GraphMLParameter> params = Upsample::graphMLParameters();

    //TODO: Declaring types as string so that complex can be stored.  Re-evaluate this

    return params;
}

std::shared_ptr<UpsampleOutput>
UpsampleOutput::createFromGraphML(int id, std::string name, std::map<std::string, std::string> dataKeyValueMap,
                                  std::shared_ptr<SubSystem> parent, GraphMLDialect dialect) {
    std::shared_ptr<UpsampleOutput> newNode = NodeFactory::createNode<UpsampleOutput>(parent);

    newNode->populateUpsampleParametersFromGraphML(id, name, dataKeyValueMap, dialect);

    std::string initialConditionStr;

    return newNode;
}

std::string UpsampleOutput::typeNameStr() {
    return "UpsampleOutput";
}

std::string UpsampleOutput::labelStr() {
    std::string label = Upsample::labelStr();

    return label;
}

void UpsampleOutput::validate() {
    Upsample::validate();

    std::shared_ptr<UpsampleOutput> thisAsUpsampleOutput = std::static_pointer_cast<UpsampleOutput>(getSharedPointer());
    MultiRateHelpers::validateRateChangeOutput_SetMasterRates(thisAsUpsampleOutput, false);
}

std::shared_ptr<Node> UpsampleOutput::shallowClone(std::shared_ptr<SubSystem> parent) {
    return NodeFactory::shallowCloneNode<UpsampleOutput>(parent, this);
}

bool UpsampleOutput::hasState() {
    if(useVectorSamplingMode){
        //TODO: Change if phase can be configured to not be 0.  Otherwise, some state will be needed to cary an element over
        return false;
    }else {
        return true;
    }
}

bool UpsampleOutput::hasCombinationalPath() {
    return true;
}

bool UpsampleOutput::createStateUpdateNode(std::vector<std::shared_ptr<Node>> &new_nodes,
                                           std::vector<std::shared_ptr<Node>> &deleted_nodes,
                                           std::vector<std::shared_ptr<Arc>> &new_arcs,
                                           std::vector<std::shared_ptr<Arc>> &deleted_arcs, bool includeContext) {
    //TODO: Remove Check
    if(isUsingVectorSamplingMode()){
        throw std::runtime_error(ErrorHelpers::genErrorStr("When using useVectorSamplingMode, state update nodes should not be created", getSharedPointer()));
    }

    if(!includeContext) {
        throw std::runtime_error(ErrorHelpers::genErrorStr("UpsampleOutput StateUpdate creation requires contexts to be present and includeContexts to be enabled", getSharedPointer()));
    }

    //==== Add Latch State Update Node.  This is the same as RepeatOutput ====
    std::shared_ptr<StateUpdate> stateUpdateLatch = NodeFactory::createNode<StateUpdate>(getParent());
    stateUpdateLatch->setName("StateUpdate-For-" + getName() + "-Latch");
    stateUpdateLatch->setPartitionNum(partitionNum);
    stateUpdateLatch->setPrimaryNode(getSharedPointer());
    addStateUpdateNode(stateUpdateLatch); //Set the state update node pointer in this node

    //Set context to be the same as the primary node.  This should be subcontext 0 of the ContextRoot
    std::vector<Context> primarayContext = getContext();
    stateUpdateLatch->setContext(primarayContext);

    //Add node to the lowest level context
    Context specificContext = primarayContext[primarayContext.size() - 1];
    specificContext.getContextRoot()->addSubContextNode(specificContext.getSubContext(), stateUpdateLatch);

    new_nodes.push_back(stateUpdateLatch);

    //rewire input arc
    std::set<std::shared_ptr<Arc>> inputArcs = getInputPort(0)->getArcs();
    if(inputArcs.size() == 0){
        throw std::runtime_error("Encountered UpsampleOutput node with no inputs when creating state update");
    }

    std::shared_ptr<InputPort> origDestPort = (*inputArcs.begin())->getDstPort();
    DataType origDataType = (*inputArcs.begin())->getDataType();
    double origSampleTime = (*inputArcs.begin())->getSampleTime();

    (*inputArcs.begin())->setDstPortUpdateNewUpdatePrev(stateUpdateLatch->getInputPortCreateIfNot(0));

    //Create new
    std::shared_ptr<Arc> newArc = Arc::connectNodes(stateUpdateLatch->getOutputPortCreateIfNot(0), origDestPort, origDataType, origSampleTime);

    new_arcs.push_back(newArc);

    //==== Add State Update Node to ContextFamilyContainer subcontext 1 ====
    //This node has no order constraint arcs
    std::vector<Context> zeroFillContext = primarayContext;
    //Change the subcontext of the lowest level
    zeroFillContext[zeroFillContext.size()-1].setSubContext(1);

    //Find parent
    std::shared_ptr<SubSystem> zeroParentNode;
    std::shared_ptr<ContextRoot> clockDomain = zeroFillContext[zeroFillContext.size()-1].getContextRoot();
    std::map<int, std::shared_ptr<ContextFamilyContainer>> contextFamilyContainers = clockDomain->getContextFamilyContainers();
    if(clockDomain->getContextFamilyContainers().size() > 0){
        //Encapsulation has already happened, place this node in the contextContainer for subcontext 1
        auto contextContainer = contextFamilyContainers.find(partitionNum);
        if(contextContainer == contextFamilyContainers.end()){
            throw std::runtime_error("Could not find expected ContextFamilyContainer when creating StateUpdateNodes for ");
        }

        zeroParentNode = contextContainer->second->getSubContextContainer(1);
    }else{
        //Encapsulation has not happened yet, use the same parent as this node
        zeroParentNode = parent;
    }

    std::shared_ptr<StateUpdate> stateUpdateZero = NodeFactory::createNode<StateUpdate>(zeroParentNode);
    stateUpdateZero->setName("StateUpdate-For-" + getName() + "-Zero");
    stateUpdateZero->setPartitionNum(partitionNum);
    stateUpdateZero->setContext(zeroFillContext);
    stateUpdateZero->setPrimaryNode(getSharedPointer());
    addStateUpdateNode(stateUpdateZero);

    new_nodes.push_back(stateUpdateZero);

    return true;
}

CExpr UpsampleOutput::emitCExpr(std::vector<std::string> &cStatementQueue, SchedParams::SchedType schedType,
                                int outputPortNum, bool imag) {
    if(useVectorSamplingMode){
        std::shared_ptr<OutputPort> srcOutputPort = getInputPort(0)->getSrcOutputPort();
        int srcOutputPortNum = srcOutputPort->getPortNum();
        std::shared_ptr<Node> srcNode = srcOutputPort->getParent();
        CExpr inputExpr = srcNode->emitC(cStatementQueue, schedType, srcOutputPortNum, imag);
        DataType inputDT = getInputPort(0)->getDataType();

        Variable outputVar = getVectorModeOutputVariable();
        DataType outputDT = outputVar.getDataType();

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
            //Reset output to 0 (Should be able to use optimized set instruction)
            //TODO: Evaluate other method?
            std::tuple<std::vector<std::string>, std::vector<std::string>, std::vector<std::string>> forLoopStrs =
                    EmitterHelpers::generateVectorMatrixForLoops(outputDT.getDimensions());

            std::vector<std::string> forLoopOpen = std::get<0>(forLoopStrs);
            std::vector<std::string> forLoopIndexVars = std::get<1>(forLoopStrs);
            std::vector<std::string> forLoopClose = std::get<2>(forLoopStrs);

            cStatementQueue.insert(cStatementQueue.end(), forLoopOpen.begin(), forLoopOpen.end());
            std::string zeroAssignment = outputVar.getCVarName(imag) + EmitterHelpers::generateIndexOperation(forLoopIndexVars) + "=0;";
            cStatementQueue.insert(cStatementQueue.end(), zeroAssignment);
            cStatementQueue.insert(cStatementQueue.end(), forLoopClose.begin(), forLoopClose.end());

            if(inputDT.isScalar()){
                //TODO: Remove check
                if(!outputDT.isVector()){
                    throw std::runtime_error(ErrorHelpers::genErrorStr("Unexpected input/output dimensions", getSharedPointer()));
                }

                //If input is scalar, simply copy the value to the 0th position
                std::string outputAssign = outputVar.getCVarName(imag) + "[0]=" + inputExpr.getExpr() + ";";
                cStatementQueue.insert(cStatementQueue.end(), outputAssign);
            }else{
                //Otherwise, loop over the input and write into the output abiding by the stride
                std::tuple<std::vector<std::string>, std::vector<std::string>, std::vector<std::string>> forLoopStrs =
                        EmitterHelpers::generateVectorMatrixForLoops(inputDT.getDimensions());
                std::vector<std::string> forLoopOpen = std::get<0>(forLoopStrs);
                std::vector<std::string> forLoopIndexVars = std::get<1>(forLoopStrs);
                std::vector<std::string> forLoopClose = std::get<2>(forLoopStrs);
                cStatementQueue.insert(cStatementQueue.end(), forLoopOpen.begin(), forLoopOpen.end());

                std::vector<std::string> forLoopIndexVarsOutput = forLoopIndexVars;
                int stride = upsampleRatio;
                forLoopIndexVarsOutput[0] += "*" + GeneralHelper::to_string(stride); //When writing into the output, the stride of the 1st dimension is set by the upsample rate

                std::string outputAssign = outputVar.getCVarName(imag) + EmitterHelpers::generateIndexOperation(forLoopIndexVarsOutput) + "=" + inputExpr.getExpr() + EmitterHelpers::generateIndexOperation(forLoopIndexVars) + ";";
                cStatementQueue.insert(cStatementQueue.end(), outputAssign);

                cStatementQueue.insert(cStatementQueue.end(), forLoopClose.begin(), forLoopClose.end());
            }

            return CExpr(outputVar.getCVarName(imag),CExpr::ExprType::ARRAY);
        }
    }else {
        return CExpr(stateVar.getCVarName(imag), stateVar.getDataType().isScalar() ? CExpr::ExprType::SCALAR_VAR
                                                                                   : CExpr::ExprType::ARRAY); //This is a variable name therefore inform the cEmit function
    }
}

std::vector<Variable> UpsampleOutput::getCStateVars() {
    std::vector<Variable> vars;

    DataType stateType = getInputPort(0)->getDataType();

    std::string varName = name+"_n"+GeneralHelper::to_string(id)+"_state";
    Variable var = Variable(varName, stateType, initCond, false, true);
    stateVar = var;

    vars.push_back(var);

    return vars;
}

bool UpsampleOutput::isSpecialized() {
    return true;
}

void UpsampleOutput::emitCStateUpdate(std::vector<std::string> &cStatementQueue, SchedParams::SchedType schedType,
                                      std::shared_ptr<StateUpdate> stateUpdateSrc) {

    //TODO: Remove this sanity check
    if(stateUpdateSrc->getPrimaryNode() != getSharedPointer()){
        throw std::runtime_error(ErrorHelpers::genErrorStr("When emitting the Upsample StateUpdate, the primary node of the StateUpdate node is not the UpsampleOutput node", getSharedPointer()));
    }

    std::vector<Context> updateContext = stateUpdateSrc->getContext();
    if(updateContext.size()<=0){
        throw std::runtime_error(ErrorHelpers::genErrorStr("When emitting the Upsample StateUpdate, the state update node did not have a context", getSharedPointer()));
    }

    int subContextNum = updateContext[updateContext.size()-1].getSubContext();

    DataType inputDataType = getInputPort(0)->getDataType();

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

    if(subContextNum == 0) {
        //This is the latching logic
        std::shared_ptr<OutputPort> srcPort = getInputPort(0)->getSrcOutputPort();
        int srcOutPortNum = srcPort->getPortNum();
        std::shared_ptr<Node> srcNode = srcPort->getParent();

        //Emit the upstream
        CExpr inputExprRe = srcNode->emitC(cStatementQueue, schedType, srcOutPortNum, false);
        CExpr inputExprIm;

        if (inputDataType.isComplex()) {
            inputExprIm = srcNode->emitC(cStatementQueue, schedType, srcOutPortNum, true);
        }

        std::string assignExprRe = stateVar.getCVarName(false) + (inputDataType.isScalar() ? "" : EmitterHelpers::generateIndexOperation(forLoopIndexVars)) +
                                   " = " + (inputDataType.isScalar() ? inputExprRe.getExpr() : inputExprRe.getExprIndexed(forLoopIndexVars, true)) + ";";
        cStatementQueue.push_back(assignExprRe);

        if (stateVar.getDataType().isComplex()) {
            std::string assignExprIm = stateVar.getCVarName(true) + (inputDataType.isScalar() ? "" : EmitterHelpers::generateIndexOperation(forLoopIndexVars)) +
                                       " = " + (inputDataType.isScalar() ? inputExprIm.getExpr() : inputExprIm.getExprIndexed(forLoopIndexVars, true)) + ";";
            cStatementQueue.push_back(assignExprIm);
        }
    }else if(subContextNum == 1){
        //This is the zero fill logic
        NumericValue zeroConst((long) 0);

        //Set the state
        std::string assignExprRe = stateVar.getCVarName(false) + (inputDataType.isScalar() ? "" : EmitterHelpers::generateIndexOperation(forLoopIndexVars)) +
                                   " = " + zeroConst.toStringComponent(false, inputDataType) + ";";
        cStatementQueue.push_back(assignExprRe);

        if (stateVar.getDataType().isComplex()) {
            std::string assignExprIm = stateVar.getCVarName(true) + (inputDataType.isScalar() ? "" : EmitterHelpers::generateIndexOperation(forLoopIndexVars)) +
                                       " = " + zeroConst.toStringComponent(true, inputDataType) + ";";
            cStatementQueue.push_back(assignExprIm);
        }
    }else{
        throw std::runtime_error(ErrorHelpers::genErrorStr("When emitting the Upsample StateUpdate, the state update node had an unexpected subcontext number", getSharedPointer()));
    }

    if(!inputDataType.isScalar()) {
        //Close For Loop
        cStatementQueue.insert(cStatementQueue.end(), forLoopClose.begin(), forLoopClose.end());
    }
}

Variable UpsampleOutput::getVectorModeOutputVariable() {
    DataType outputDT = getOutputPort(0)->getDataType();
    std::string outName = name + "_n" + GeneralHelper::to_string(id) + "_Out";
    return Variable(outName, outputDT);
}

std::vector<Variable> UpsampleOutput::getVariablesToDeclareOutsideClockDomain() {
    std::vector<Variable> extVars = RateChange::getVariablesToDeclareOutsideClockDomain();

    if(useVectorSamplingMode){
        //Do not need to declare state (unless phase != 0 is later introduced).  However, need to declare the output
        //outside of the clock domain
        //TODO: Change if phase can be configured to not be 0.  Otherwise, some state will be needed to cary an element over

        extVars.push_back(getVectorModeOutputVariable());
    }

    return extVars;
}
