//
// Created by Christopher Yarp on 5/2/20.
//

#include "WrappingCounter.h"
#include "General/ErrorHelpers.h"
#include "GraphCore/Variable.h"
#include "General/GraphAlgs.h"

int WrappingCounter::getCountTo() const {
    return countTo;
}

void WrappingCounter::setCountTo(int countTo) {
    WrappingCounter::countTo = countTo;
}

int WrappingCounter::getInitCondition() const {
    return initCondition;
}

void WrappingCounter::setInitCondition(int initCondition) {
    WrappingCounter::initCondition = initCondition;
}

Variable WrappingCounter::getCStateVar() const {
    return cStateVar;
}

void WrappingCounter::setCStateVar(const Variable &cStateVar) {
    WrappingCounter::cStateVar = cStateVar;
}

WrappingCounter::WrappingCounter() : countTo(0), initCondition(0) {

}

WrappingCounter::WrappingCounter(std::shared_ptr<SubSystem> parent) : PrimitiveNode(parent), countTo(0), initCondition(0) {

}

WrappingCounter::WrappingCounter(std::shared_ptr<SubSystem> parent, WrappingCounter *orig) : PrimitiveNode(parent, orig), countTo(orig->countTo), initCondition(orig->initCondition), cStateVar(orig->cStateVar
) {

}

std::shared_ptr<WrappingCounter>
WrappingCounter::createFromGraphML(int id, std::string name, std::map<std::string, std::string> dataKeyValueMap,
                                   std::shared_ptr<SubSystem> parent, GraphMLDialect dialect) {
    std::shared_ptr<WrappingCounter> newNode = NodeFactory::createNode<WrappingCounter>(parent);
    newNode->setId(id);
    newNode->setName(name);

    if (dialect != GraphMLDialect::VITIS) {
        //This is a vitis only node
        throw std::runtime_error(ErrorHelpers::genErrorStr("Unsupported Dialect when parsing XML - WrappingCounter", newNode));
    }

    //==== Import important properties ====
    std::string countToStr;
    std::string initialConditionStr;

    //Vitis Names -- CountTo, InitialCondition
    countToStr = dataKeyValueMap.at("CountTo");
    initialConditionStr = dataKeyValueMap.at("InitialCondition");

    int countTo = std::stoi(countToStr);
    newNode->setCountTo(countTo);

    int initialCondition = std::stoi(initialConditionStr);
    newNode->setInitCondition(initialCondition);

    return newNode;
}

std::set<GraphMLParameter> WrappingCounter::graphMLParameters() {
    std::set<GraphMLParameter> parameters;

    //TODO: Declaring types as string so that complex can be stored.  Re-evaluate this
    parameters.insert(GraphMLParameter("CountTo", "string", true));
    parameters.insert(GraphMLParameter("InitialCondition", "string", true));

    return parameters;
}

xercesc::DOMElement *
WrappingCounter::emitGraphML(xercesc::DOMDocument *doc, xercesc::DOMElement *graphNode, bool include_block_node_type) {
    xercesc::DOMElement* thisNode = emitGraphMLBasics(doc, graphNode);
    if(include_block_node_type) {
        GraphMLHelper::addDataNode(doc, thisNode, "block_node_type", "Standard");
    }

    GraphMLHelper::addDataNode(doc, thisNode, "block_function", "WrappingCounter");

    GraphMLHelper::addDataNode(doc, thisNode, "CountTo", GeneralHelper::to_string(countTo));

    GraphMLHelper::addDataNode(doc, thisNode, "InitialCondition", GeneralHelper::to_string(initCondition));

    return thisNode;
}

std::string WrappingCounter::typeNameStr() {
    return "WrappingCounter";
}

std::string WrappingCounter::labelStr() {
    std::string label = Node::labelStr();

    label += "\nFunction: " + typeNameStr() + "\nCountTo:" + GeneralHelper::to_string(countTo) + "\nInitialCondition: " + GeneralHelper::to_string(initCondition);

    return label;
}

void WrappingCounter::validate() {
    Node::validate();

    //Should have 0 input ports and 1 output port
    if(inputPorts.size() != 0){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - WrappingCounter - Should Have No Input Ports", getSharedPointer()));
    }

    if(outputPorts.size() != 1){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - WrappingCounter - Should Have Exactly 1 Output Port", getSharedPointer()));
    }

    //Check that input port and the output port have the same type
    DataType outType = getOutputPort(0)->getDataType();

    if(outType.isFloatingPt()){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - WrappingCounter - Output should not be a floating point type", getSharedPointer()));
    }

    if(log2(countTo+1) + (outType.isSignedType() ? 1 : 0) > outType.getTotalBits()){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - WrappingCounter - Not enough bits in output", getSharedPointer()));
    }
}

bool WrappingCounter::hasState() {
    return true;
}

bool WrappingCounter::hasCombinationalPath() {
    return false;
}

std::vector<Variable> WrappingCounter::getCStateVars() {
    std::vector<Variable> vars;

    //There is a single state variable for the counter.
    DataType stateType(false, false, false, log2(countTo+1), 0, {1});
    stateType = stateType.getCPUStorageType();

    std::string varName = name+"_n"+GeneralHelper::to_string(id)+"_state";

    NumericValue initCond((long int) initCondition);

    Variable var = Variable(varName, stateType, {initCond}, false, true);
    cStateVar = var;
    vars.push_back(var);

    return vars;
}

void WrappingCounter::emitCExprNextState(std::vector<std::string> &cStatementQueue, SchedParams::SchedType schedType) {
    //No need to emit anything for the next state since there are no inputs
    //The increment and bounds check will occure inside the state update
}

CExpr WrappingCounter::emitCExpr(std::vector<std::string> &cStatementQueue, SchedParams::SchedType schedType,
                                 int outputPortNum, bool imag) {
    //TODO: Implement Vector Support
    if(!getOutputPort(0)->getDataType().isScalar()){
        throw std::runtime_error(ErrorHelpers::genErrorStr("C Emit Error - Delay Support for Vector Types has Not Yet Been Implemented", getSharedPointer()));
    }

    return CExpr(cStateVar.getCVarName(imag) + "==0", CExpr::ExprType::SCALAR_EXPR);
}

void WrappingCounter::emitCStateUpdate(std::vector<std::string> &cStatementQueue, SchedParams::SchedType schedType, std::shared_ptr<StateUpdate> stateUpdateSrc) {
    cStatementQueue.push_back(cStateVar.getCVarName(false) + " = " +
                    cStateVar.getCVarName(false) + " < " + GeneralHelper::to_string(countTo-1) + " ? " +
                    cStateVar.getCVarName(false) + " + 1 : 0;");
}

std::shared_ptr<Node> WrappingCounter::shallowClone(std::shared_ptr<SubSystem> parent) {
    return NodeFactory::shallowCloneNode<WrappingCounter>(parent, this);
}

bool WrappingCounter::createStateUpdateNode(std::vector<std::shared_ptr<Node>> &new_nodes,
                                            std::vector<std::shared_ptr<Node>> &deleted_nodes,
                                            std::vector<std::shared_ptr<Arc>> &new_arcs,
                                            std::vector<std::shared_ptr<Arc>> &deleted_arcs, bool includeContext) {
    //This node uses the same basic pattern as the delay node when it comes to creating a state update node
    return GraphAlgs::createStateUpdateNodeDelayStyle(getSharedPointer(), new_nodes, deleted_nodes, new_arcs, deleted_arcs, includeContext);
}
