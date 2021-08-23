//
// Created by Christopher Yarp on 4/29/20.
//

#include "RepeatOutput.h"
#include "General/ErrorHelpers.h"
#include "MultiRateHelpers.h"
#include "GraphCore/StateUpdate.h"

RepeatOutput::RepeatOutput() {

}

RepeatOutput::RepeatOutput(std::shared_ptr<SubSystem> parent) : Repeat(parent) {

}

RepeatOutput::RepeatOutput(std::shared_ptr<SubSystem> parent, RepeatOutput *orig) : Repeat(parent, orig), stateVar(orig->stateVar), initCondition(orig->initCondition) {

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
    return true;
}

bool RepeatOutput::hasCombinationalPath() {
    return true;
}

bool RepeatOutput::createStateUpdateNode(std::vector<std::shared_ptr<Node>> &new_nodes,
                                         std::vector<std::shared_ptr<Node>> &deleted_nodes,
                                         std::vector<std::shared_ptr<Arc>> &new_arcs,
                                         std::vector<std::shared_ptr<Arc>> &deleted_arcs, bool includeContext) {
    //The state update node will not be directly dependent on this port since
    //any singal feeding into the EnableOutput had to pass through an enable input

    std::shared_ptr<StateUpdate> stateUpdate = NodeFactory::createNode<StateUpdate>(getParent());
    stateUpdate->setName("StateUpdate-For-"+getName());
    stateUpdate->setPartitionNum(partitionNum);
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
    return CExpr(stateVar.getCVarName(imag), stateVar.getDataType().isScalar() ? CExpr::ExprType::SCALAR_VAR : CExpr::ExprType::ARRAY); //This is a variable name therefore inform the cEmit function
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
