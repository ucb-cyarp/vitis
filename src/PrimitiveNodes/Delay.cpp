//
// Created by Christopher Yarp on 7/6/18.
//

#include <vector>

#include "Delay.h"
#include "GraphCore/NodeFactory.h"
#include "GraphCore/NumericValue.h"
#include "General/GraphAlgs.h"

#include "General/GeneralHelper.h"
#include "General/ErrorHelpers.h"

Delay::Delay() : delayValue(0){

}

Delay::Delay(std::shared_ptr<SubSystem> parent) : PrimitiveNode(parent), delayValue(0) {

}

int Delay::getDelayValue() const {
    return delayValue;
}

void Delay::setDelayValue(int delayValue) {
    Delay::delayValue = delayValue;
}

std::vector<NumericValue> Delay::getInitCondition() const {
    return initCondition;
}

void Delay::setInitCondition(const std::vector<NumericValue> &initCondition) {
    Delay::initCondition = initCondition;
}

std::shared_ptr<Delay> Delay::createFromGraphML(int id, std::string name,
                                                std::map<std::string, std::string> dataKeyValueMap,
                                                std::shared_ptr<SubSystem> parent, GraphMLDialect dialect) {
    std::shared_ptr<Delay> newNode = NodeFactory::createNode<Delay>(parent);
    newNode->setId(id);
    newNode->setName(name);

    if (dialect == GraphMLDialect::SIMULINK_EXPORT) {
        //==== Check Supported Config (Only if Simulink Import)====
        std::string delayLengthSource = dataKeyValueMap.at("DelayLengthSource");

        if (delayLengthSource != "Dialog") {
            throw std::runtime_error(ErrorHelpers::genErrorStr("Delay block must specify Delay Source as \"Dialog\"", newNode));
        }

        std::string initialConditionSource = dataKeyValueMap.at("InitialConditionSource");
        if (initialConditionSource != "Dialog") {
            throw std::runtime_error(ErrorHelpers::genErrorStr("Delay block must specify Initial Condition Source source as \"Dialog\"", newNode));
        }
    }

    //==== Import important properties ====
    std::string delayStr;
    std::string initialConditionStr;

    if (dialect == GraphMLDialect::VITIS) {
        //Vitis Names -- DelayLength, InitialCondit
        delayStr = dataKeyValueMap.at("DelayLength");
        initialConditionStr = dataKeyValueMap.at("InitialCondition");
    } else if (dialect == GraphMLDialect::SIMULINK_EXPORT) {
        //Simulink Names -- Numeric.DelayLength, Numeric.InitialCondition
        delayStr = dataKeyValueMap.at("Numeric.DelayLength");
        initialConditionStr = dataKeyValueMap.at("Numeric.InitialCondition");
    } else {
        throw std::runtime_error(ErrorHelpers::genErrorStr("Unsupported Dialect when parsing XML - Delay", newNode));
    }

    int delayVal = std::stoi(delayStr);
    newNode->setDelayValue(delayVal);

    std::vector<NumericValue> initialConds = NumericValue::parseXMLString(initialConditionStr);

    if(initialConds.size() == 1 && delayVal > 1){
        //There is a single initial value and the delay length is > 1 ... replicate initial conditions
        NumericValue uniformVal = initialConds[0];

        //Already have one entry, add the rest
        for(unsigned long i = 1; i<delayVal; i++){
            initialConds.push_back(uniformVal);
        }
    }

    newNode->setInitCondition(initialConds);

    return newNode;
}

std::set<GraphMLParameter> Delay::graphMLParameters() {
    std::set<GraphMLParameter> parameters;

    //TODO: Declaring types as string so that complex can be stored.  Re-evaluate this
    parameters.insert(GraphMLParameter("DelayLength", "string", true));
    parameters.insert(GraphMLParameter("InitialCondition", "string", true));

    return parameters;
}

xercesc::DOMElement *
Delay::emitGraphML(xercesc::DOMDocument *doc, xercesc::DOMElement *graphNode, bool include_block_node_type) {
    xercesc::DOMElement* thisNode = emitGraphMLBasics(doc, graphNode);
    if(include_block_node_type) {
        GraphMLHelper::addDataNode(doc, thisNode, "block_node_type", "Standard");
    }

    GraphMLHelper::addDataNode(doc, thisNode, "block_function", "Delay");

    GraphMLHelper::addDataNode(doc, thisNode, "DelayLength", GeneralHelper::to_string(delayValue));

    GraphMLHelper::addDataNode(doc, thisNode, "InitialCondition", NumericValue::toString(initCondition));

    return thisNode;
}

std::string Delay::typeNameStr(){
    return "Delay";
}

std::string Delay::labelStr() {
    std::string label = Node::labelStr();

    label += "\nFunction: " + typeNameStr() + "\nDelayLength:" + GeneralHelper::to_string(delayValue) + "\nInitialCondition: " + NumericValue::toString(initCondition);

    return label;
}

void Delay::validate() {
    Node::validate();

    //Should have 1 input ports and 1 output port
    if(inputPorts.size() != 1){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - Delay - Should Have Exactly 1 Input Port", getSharedPointer()));
    }

    if(outputPorts.size() != 1){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - Delay - Should Have Exactly 1 Output Port", getSharedPointer()));
    }

    //Check that input port and the output port have the same type
    DataType outType = getOutputPort(0)->getDataType();
    DataType inType = getInputPort(0)->getDataType();

    if(inType != outType){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - Delay - DataType of Input Port Does not Match Output Port", getSharedPointer()));
    }

    if(delayValue != 0 && delayValue != initCondition.size()){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - Delay - Delay Length (" + GeneralHelper::to_string(delayValue) + ") Does not Match the Length of Init Condition Vector (" + GeneralHelper::to_string(initCondition.size()) + ")", getSharedPointer()));
    }
}

bool Delay::hasState() {
    if(delayValue > 0) {
        return true;
    }else{
        return false;
    }
}

std::vector<Variable> Delay::getCStateVars() {
    std::vector<Variable> vars;

    //If delayValue < 0 return empty vector
    if(delayValue == 0){
        return vars;
    }else{
        //There is a single state variable for the delay.  However, it is an array if the delay is > 1.

        DataType stateType = getInputPort(0)->getDataType();
        stateType.setWidth(delayValue);

        //TODO: Extend to support vectors (must declare 2D array for state)

        std::string varName = name+"_n"+GeneralHelper::to_string(id)+"_state";
        Variable var = Variable(varName, stateType, initCondition);
        cStateVar = var;
        //Complex variable will be made if needed by the design code based on the data type
        vars.push_back(var);

        return vars;
    }
}

CExpr Delay::emitCExpr(std::vector<std::string> &cStatementQueue, SchedParams::SchedType schedType, int outputPortNum, bool imag) {
    //TODO: Implement Vector Support
    if(getInputPort(0)->getDataType().getWidth()>1){
        throw std::runtime_error(ErrorHelpers::genErrorStr("C Emit Error - Delay Support for Vector Types has Not Yet Been Implemented", getSharedPointer()));
    }

    if(delayValue == 0){
        //If delay = 0, simply pass through
        std::shared_ptr<OutputPort> srcPort = getInputPort(0)->getSrcOutputPort();
        int srcOutPortNum = srcPort->getPortNum();
        std::shared_ptr<Node> srcNode = srcPort->getParent();

        //Emit the upstream
        std::string inputExpr = srcNode->emitC(cStatementQueue, schedType, srcOutPortNum, imag);

        return CExpr(inputExpr, false);
    }else {
        //Return the state var name as the expression
        if (delayValue == 1) {
            //Return the simple name (no index needed as it is not an array_
            return CExpr(cStateVar.getCVarName(imag), true); //This is a variable name therefore inform the cEmit function
        }else{
            return CExpr(cStateVar.getCVarName(imag) + "[0]", true);
        }
    }
}

void Delay::emitCStateUpdate(std::vector<std::string> &cStatementQueue, SchedParams::SchedType schedType, std::shared_ptr<StateUpdate> stateUpdateSrc) {
    //TODO: Implement Vector Support (Need 2D state)
    if(delayValue == 0){
        return; //No state to update
    }else if(delayValue == 1){
        //The state variable is not an array
        cStatementQueue.push_back(cStateVar.getCVarName(false) + " = " + cStateInputVar.getCVarName(false) + ";");

        if(cStateVar.getDataType().isComplex()){
            cStatementQueue.push_back(cStateVar.getCVarName(true) + " = " + cStateInputVar.getCVarName(true) + ";");
        }
    }else{
        //This is a (pair) of arrays
        //Emit a for loop to perform the shift for each
        std::string loopVarName = name+"_n"+GeneralHelper::to_string(id)+"_loopCounter";

        cStatementQueue.push_back("for(unsigned long " + loopVarName + " = 0; " + loopVarName + " < " + GeneralHelper::to_string(delayValue-1) + "; " + loopVarName + "++){");
        cStatementQueue.push_back(cStateVar.getCVarName(false) + "[" + loopVarName + "] = " + cStateVar.getCVarName(false) + "[" + loopVarName + "+1];}");
        cStatementQueue.push_back(cStateVar.getCVarName(false) + "[" + GeneralHelper::to_string(delayValue-1) + "] = " + cStateInputVar.getCVarName(false) + ";");

        if(cStateVar.getDataType().isComplex()){
            cStatementQueue.push_back("for(unsigned long " + loopVarName + " = 0; " + loopVarName + " < " + GeneralHelper::to_string(delayValue-1) + "; " + loopVarName + "++){");
            cStatementQueue.push_back(cStateVar.getCVarName(true) + "[" + loopVarName + "] = " + cStateVar.getCVarName(true) + "[" + loopVarName + "+1];}");
            cStatementQueue.push_back(cStateVar.getCVarName(true) + "[" + GeneralHelper::to_string(delayValue-1) + "] = " + cStateInputVar.getCVarName(true) + ";");
        }
    }

    Node::emitCStateUpdate(cStatementQueue, schedType, stateUpdateSrc);
}

void Delay::emitCExprNextState(std::vector<std::string> &cStatementQueue, SchedParams::SchedType schedType) {
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

    //Assign the expr to a special variable defined here (before the state update)
    std::string stateInputName = name + "_n" + GeneralHelper::to_string(id) + "_state_input";
    Variable stateInputVar = Variable(stateInputName, getInputPort(0)->getDataType());
    cStateInputVar = stateInputVar;

    //TODO: Implement Vector Support (need to loop over input variable indexes (will be stored as a variable due to default behavior of internal fanout

    std::string stateInputDeclAssignRe = stateInputVar.getCVarDecl(false, false, false, false) + " = " + inputExprRe + ";";
    cStatementQueue.push_back(stateInputDeclAssignRe);
    if(inputDataType.isComplex()){
        std::string stateInputDeclAssignIm = stateInputVar.getCVarDecl(true, false, false, false) + " = " + inputExprIm + ";";
        cStatementQueue.push_back(stateInputDeclAssignIm);
    }
}

Delay::Delay(std::shared_ptr<SubSystem> parent, Delay* orig) : PrimitiveNode(parent, orig), delayValue(orig->delayValue), initCondition(orig->initCondition), cStateVar(orig->cStateVar), cStateInputVar(orig->cStateInputVar){

}

std::shared_ptr<Node> Delay::shallowClone(std::shared_ptr<SubSystem> parent) {
    return NodeFactory::shallowCloneNode<Delay>(parent, this);
}

bool Delay::createStateUpdateNode(std::vector<std::shared_ptr<Node>> &new_nodes,
                                  std::vector<std::shared_ptr<Node>> &deleted_nodes,
                                  std::vector<std::shared_ptr<Arc>> &new_arcs,
                                  std::vector<std::shared_ptr<Arc>> &deleted_arcs,
                                  bool includeContext) {

    return GraphAlgs::createStateUpdateNodeDelayStyle(getSharedPointer(), new_nodes, deleted_nodes, new_arcs, deleted_arcs, includeContext);
}

bool Delay::hasCombinationalPath() {
    return false; //This is a pure state element
}
