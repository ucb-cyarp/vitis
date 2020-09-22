//
// Created by Christopher Yarp on 6/26/18.
//

#include "EnableOutput.h"
#include "GraphMLTools/GraphMLHelper.h"
#include "EnabledSubSystem.h"
#include "NodeFactory.h"
#include "StateUpdate.h"
#include "General/GeneralHelper.h"
#include <iostream>
#include "General/GeneralHelper.h"
#include "General/ErrorHelpers.h"
#include "General/EmitterHelpers.h"

EnableOutput::EnableOutput() {

}

EnableOutput::EnableOutput(std::shared_ptr<SubSystem> parent) : EnableNode(parent) {

}

std::shared_ptr<EnableOutput> EnableOutput::createFromGraphML(int id, std::string name,
                                                std::map<std::string, std::string> dataKeyValueMap,
                                                std::shared_ptr<SubSystem> parent, GraphMLDialect dialect) {
    std::shared_ptr<EnableOutput> newNode = NodeFactory::createNode<EnableOutput>(parent);
    newNode->setId(id);
    newNode->setName(name);

    //==== Import important properties ====
    std::string initialConditionStr;

    if (dialect == GraphMLDialect::VITIS) {
        //Vitis Names -- InitialCondition
        initialConditionStr = dataKeyValueMap.at("InitialCondition");
    } else if (dialect == GraphMLDialect::SIMULINK_EXPORT) {
        //Simulink Names -- Numeric.InitialOutput
        initialConditionStr = dataKeyValueMap.at("Numeric.InitialOutput");
    } else
    {
        throw std::runtime_error("Unsupported Dialect when parsing XML - EnableOutput");
    }

    std::vector<NumericValue> initialConds = NumericValue::parseXMLString(initialConditionStr);

    if(initialConds.empty()){
        initialConds.push_back(NumericValue());

        std::cerr << "Warning: Enabled Output " << newNode->getFullyQualifiedName(true) << " Did not Specify an Initial Value ... Assuming Default: " << initialConds[0] << std::endl;
    }

    newNode->setInitCondition(initialConds);

    return newNode;
}


xercesc::DOMElement *EnableOutput::emitGraphML(xercesc::DOMDocument *doc, xercesc::DOMElement *graphNode, bool include_block_node_type) {
    xercesc::DOMElement* thisNode = emitGraphMLBasics(doc, graphNode);
    if(include_block_node_type) {
        GraphMLHelper::addDataNode(doc, thisNode, "block_node_type", "Special Output Port");
    }

    GraphMLHelper::addDataNode(doc, thisNode, "InitialCondition", NumericValue::toString(initCondition));

    return thisNode;
}

std::set<GraphMLParameter> EnableOutput::graphMLParameters() {
    std::set<GraphMLParameter> parameters = EnableNode::graphMLParameters();

    parameters.insert(GraphMLParameter("InitialCondition", "string", true));

    return parameters;
}

std::string EnableOutput::typeNameStr(){
    return "Enable Output";
};

std::string EnableOutput::labelStr() {
    std::string label = Node::labelStr();

    label += "\nType: " + typeNameStr();

    return label;
}

void EnableOutput::validate() {
    EnableNode::validate();

    //Parent checked above to be an Enabled SubSystem
    std::shared_ptr<EnabledSubSystem> parentEnabled = std::dynamic_pointer_cast<EnabledSubSystem>(parent);

    std::vector<std::shared_ptr<EnableOutput>> parentOutputNodes = parentEnabled->getEnableOutputs();

    unsigned long numEnableOutputs = parentOutputNodes.size();
    bool found = false;
    for(unsigned long i = 0; i<numEnableOutputs; i++){
        if(parentOutputNodes[i] == getSharedPointer()){
            found = true;
            break;
        }
    }

    if(!found){
        throw std::runtime_error("EnableOutput not found in parent EnabledOutput list");
    }

    if(inputPorts.size() != 1){
        throw std::runtime_error("EnableOutput should have exactly 1 input port");
    }

    if(outputPorts.size() != 1){
        throw std::runtime_error("EnableOutput should have exactly 1 output port");
    }
}

EnableOutput::EnableOutput(std::shared_ptr<SubSystem> parent, EnableOutput* orig) : EnableNode(parent, orig), stateVar(orig->stateVar), initCondition(orig->initCondition) { }
//EnableOutput::EnableOutput(std::shared_ptr<SubSystem> parent, EnableOutput* orig) : EnableNode(parent, orig), stateVar(orig->stateVar), nextStateVar(orig->nextStateVar) { }

std::shared_ptr<Node> EnableOutput::shallowClone(std::shared_ptr<SubSystem> parent) {
    return NodeFactory::shallowCloneNode<EnableOutput>(parent, this);
}

bool EnableOutput::hasState() {
    return true;
}

bool EnableOutput::createStateUpdateNode(std::vector<std::shared_ptr<Node>> &new_nodes,
                                         std::vector<std::shared_ptr<Node>> &deleted_nodes,
                                         std::vector<std::shared_ptr<Arc>> &new_arcs,
                                         std::vector<std::shared_ptr<Arc>> &deleted_arcs,
                                         bool includeContext){

    //Note, the enable port is a special port.  The state update node will not be directly dependent on this port since
    //any singal feeding into the EnableOutput had to pass through an enable input
    //TODO: check this assumption that EnableInputs will not be removed

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
        throw std::runtime_error("Encountered EnabledOutput node with no inputs when creating state update");
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

CExpr EnableOutput::emitCExpr(std::vector<std::string> &cStatementQueue, SchedParams::SchedType schedType, int outputPortNum, bool imag) {
    //TODO: Implement Vector Support
    if(!getInputPort(0)->getDataType().isScalar()){
        throw std::runtime_error("C Emit Error - EnableOutput Support for Vector Types has Not Yet Been Implemented");
    }

    //Return the state var name as the expression
    //Return the simple name (no index needed as it is not an array
    return CExpr(stateVar.getCVarName(imag), true); //This is a variable name therefore inform the cEmit function
}

//Because the EnableOutput is preceeded by it's corresponding state update node, that node will create a temporary
//to pass any value through.  If it is a scalar, the temporary will be created by the scheduler.  If a vector, the
//StateUpdateNode creates an explicit variable.

void EnableOutput::emitCStateUpdate(std::vector<std::string> &cStatementQueue, SchedParams::SchedType schedType, std::shared_ptr<StateUpdate> stateUpdateSrc) {
    DataType inputDataType = getInputPort(0)->getDataType();
    std::shared_ptr<OutputPort> srcPort = getInputPort(0)->getSrcOutputPort();
    int srcOutPortNum = srcPort->getPortNum();
    std::shared_ptr<Node> srcNode = srcPort->getParent();

    //Emit the upstream
    std::string inputExprRe = srcNode->emitC(cStatementQueue, schedType, srcOutPortNum, false);
    std::string inputExprIm;

    if(inputDataType.isComplex()){
        inputExprIm = srcNode->emitC(cStatementQueue, schedType, srcOutPortNum, true);
    }

    //If the output is a vector, construct a for loop
    std::vector<std::string> forLoopIndexVars;
    std::vector<std::string> forLoopClose;
    if(!inputDataType.isScalar()){
        //Create nested loops for a given array
        std::tuple<std::vector<std::string>, std::vector<std::string>, std::vector<std::string>> forLoopStrs =
                EmitterHelpers::generateVectorMatrixForLoops(inputDataType.getDimensions());

        std::vector<std::string> forLoopOpen = std::get<0>(forLoopStrs);
        forLoopIndexVars = std::get<1>(forLoopStrs);
        forLoopClose = std::get<2>(forLoopStrs);
        cStatementQueue.insert(cStatementQueue.end(), forLoopOpen.begin(), forLoopOpen.end());
    }

    //The state variable is not an array
    cStatementQueue.push_back(stateVar.getCVarName(false) + (inputDataType.isScalar() ? "" : EmitterHelpers::generateIndexOperation(forLoopIndexVars)) + " = " +
    inputExprRe + (inputDataType.isScalar() ? "" : EmitterHelpers::generateIndexOperation(forLoopIndexVars)) + ";");

    if(stateVar.getDataType().isComplex()){
        cStatementQueue.push_back(stateVar.getCVarName(true) + (inputDataType.isScalar() ? "" : EmitterHelpers::generateIndexOperation(forLoopIndexVars)) + " = " +
        inputExprIm + (inputDataType.isScalar() ? "" : EmitterHelpers::generateIndexOperation(forLoopIndexVars)) + ";");
    }

    //Close for loop
    if(!inputDataType.isScalar()){
        cStatementQueue.insert(cStatementQueue.end(), forLoopClose.begin(), forLoopClose.end());
    }
}

std::vector<Variable> EnableOutput::getCStateVars() {
    std::vector<Variable> vars;

    DataType stateType = getInputPort(0)->getDataType();

    std::string varName = name+"_n"+GeneralHelper::to_string(id)+"_state";
    Variable var = Variable(varName, stateType, initCondition);
    stateVar = var;

    vars.push_back(var);

    return vars;
}

Variable EnableOutput::getStateVar() const {
    return stateVar;
}

void EnableOutput::setStateVar(const Variable &stateVar) {
    EnableOutput::stateVar = stateVar;
}

//Variable EnableOutput::getNextStateVar() const {
//    return nextStateVar;
//}
//
//void EnableOutput::setNextStateVar(const Variable &nextStateVar) {
//    EnableOutput::nextStateVar = nextStateVar;
//}

std::vector<NumericValue> EnableOutput::getInitCondition() const {
    return initCondition;
}

void EnableOutput::setInitCondition(const std::vector<NumericValue> &initCondition) {
    EnableOutput::initCondition = initCondition;
}

bool EnableOutput::hasCombinationalPath() {
    return true; //When transparent, latch has a combinational path
}

void EnableOutput::removeKnownReferences() {
    Node::removeKnownReferences();

    //Remove from EnableSubsystem EnableInput List
    std::shared_ptr<EnabledSubSystem> parentAsEnabledSubsystem = GeneralHelper::isType<Node, EnabledSubSystem>(parent);
    if(parentAsEnabledSubsystem){
        std::shared_ptr<EnableOutput> this_cast = std::static_pointer_cast<EnableOutput>(getSharedPointer());
        parentAsEnabledSubsystem->removeEnableOutput(this_cast);
    }else{
        throw std::runtime_error(ErrorHelpers::genErrorStr("When removing EnableOutput node, could not remove node from EnableNode lists of parent", getSharedPointer()));
    }
}