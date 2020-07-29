//
// Created by Christopher Yarp on 2019-01-28.
//

#include <General/GeneralHelper.h>
#include "BlackBox.h"
#include "General/GraphAlgs.h"
#include "General/ErrorHelpers.h"

BlackBox::ReturnMethod BlackBox::parseReturnMethodStr(std::string str) {
    if(str == "NONE" || str == "none"){
        return BlackBox::ReturnMethod::NONE;
    } else if(str == "SCALAR" || str == "scalar"){
        return BlackBox::ReturnMethod::SCALAR;
    } else if(str == "OBJECT" || str == "object") {
        return BlackBox::ReturnMethod::OBJECT;
    } else if(str == "SCALAR_POINTER" || str == "scalar_pointer"){
        return BlackBox::ReturnMethod::SCALAR_POINTER;
    }else if(str == "OBJECT_POINTER" || str == "object_pointer"){
        return BlackBox::ReturnMethod::OBJECT_POINTER;
    }else if(str == "EXT" || str == "ext"){
        return BlackBox::ReturnMethod::EXT;
    }else{
        throw std::runtime_error(ErrorHelpers::genErrorStr("Unable to parse ReturnMethod: " + str));
    }
}

std::string BlackBox::returnMethodToStr(BlackBox::ReturnMethod returnMethod) {
    switch(returnMethod){
        case BlackBox::ReturnMethod::NONE : return "NONE";
        case BlackBox::ReturnMethod::SCALAR : return "SCALAR";
        case BlackBox::ReturnMethod::OBJECT : return "OBJECT";
        case BlackBox::ReturnMethod::SCALAR_POINTER : return "SCALAR_POINTER";
        case BlackBox::ReturnMethod::OBJECT_POINTER : return "OBJECT_POINTER";
        case BlackBox::ReturnMethod::EXT : return "EXT";
        default : throw std::runtime_error(ErrorHelpers::genErrorStr("Unknown ReturnMethod"));
    }
}

BlackBox::BlackBox() : stateful(false), returnMethod(BlackBox::ReturnMethod::NONE), previouslyEmitted(false) {

}

BlackBox::BlackBox(std::shared_ptr<SubSystem> parent) : PrimitiveNode(parent), stateful(false), returnMethod(BlackBox::ReturnMethod::NONE), previouslyEmitted(false) {

}

BlackBox::BlackBox(std::shared_ptr<SubSystem> parent, BlackBox *orig) : PrimitiveNode(parent, orig),
    exeCombinationalName(orig->exeCombinationalName), exeCombinationalRtnType(orig->exeCombinationalRtnType),
    stateUpdateName(orig->stateUpdateName), resetName(orig->resetName), cppHeaderContent(orig->cppHeaderContent),
    cppBodyContent(orig->cppBodyContent), stateful(orig->stateful), reSuffix(orig->reSuffix), imSuffix(orig->imSuffix),
    inputMethods(orig->inputMethods), inputAccess(orig->inputAccess), registeredOutputPorts(orig->registeredOutputPorts),
    returnMethod(orig->returnMethod), outputAccess(orig->outputAccess), previouslyEmitted(orig->previouslyEmitted)
    {
}

std::string BlackBox::getExeCombinationalName() const {
    return exeCombinationalName;
}

void BlackBox::setExeCombinationalName(const std::string &exeCombinationalName) {
    BlackBox::exeCombinationalName = exeCombinationalName;
}

std::string BlackBox::getExeCombinationalRtnType() const {
    return exeCombinationalRtnType;
}

void BlackBox::setExeCombinationalRtnType(const std::string &exeCombinationalRtnType) {
    BlackBox::exeCombinationalRtnType = exeCombinationalRtnType;
}

std::string BlackBox::getStateUpdateName() const {
    return stateUpdateName;
}

void BlackBox::setStateUpdateName(const std::string &stateUpdateName) {
    BlackBox::stateUpdateName = stateUpdateName;
}

std::shared_ptr<Node> BlackBox::shallowClone(std::shared_ptr<SubSystem> parent) {
    return NodeFactory::shallowCloneNode<BlackBox>(parent, this);
}

std::string BlackBox::getResetName() const {
    return resetName;
}

std::string BlackBox::getCppHeaderContent() const {
    return cppHeaderContent;
}

void BlackBox::setCppHeaderContent(const std::string &cppHeaderContent) {
    BlackBox::cppHeaderContent = cppHeaderContent;
}

std::string BlackBox::getCppBodyContent() const {
    return cppBodyContent;
}

void BlackBox::setCppBodyContent(const std::string &cppBodyContent) {
    BlackBox::cppBodyContent = cppBodyContent;
}

bool BlackBox::isStateful() const {
    return stateful;
}

void BlackBox::setStateful(bool stateful) {
    BlackBox::stateful = stateful;
}

std::vector<int> BlackBox::getRegisteredOutputPorts() const {
    return registeredOutputPorts;
}

void BlackBox::setRegisteredOutputPorts(const std::vector<int> &registeredOutputPorts) {
    BlackBox::registeredOutputPorts = registeredOutputPorts;
}

BlackBox::ReturnMethod BlackBox::getReturnMethod() const {
    return returnMethod;
}

void BlackBox::setReturnMethod(BlackBox::ReturnMethod returnMethod) {
    BlackBox::returnMethod = returnMethod;
}

std::vector<std::string> BlackBox::getOutputAccess() const {
    return outputAccess;
}

void BlackBox::setOutputAccess(const std::vector<std::string> &outputAccess) {
    BlackBox::outputAccess = outputAccess;
}

bool BlackBox::isPreviouslyEmitted() const {
    return previouslyEmitted;
}

void BlackBox::setPreviouslyEmitted(bool previouslyEmitted) {
    BlackBox::previouslyEmitted = previouslyEmitted;
}

bool BlackBox::hasState() {
    return stateful;
}

std::set<GraphMLParameter> BlackBox::graphMLParametersCommon(){
    std::set<GraphMLParameter> parameters;

    parameters.insert(GraphMLParameter("ExeCombinationalName", "string", true));
    parameters.insert(GraphMLParameter("StateUpdateName", "string", true));
    parameters.insert(GraphMLParameter("CppHeaderContent", "string", true));
    parameters.insert(GraphMLParameter("CppBodyContent", "string", true));
    parameters.insert(GraphMLParameter("Stateful", "boolean", true));
    parameters.insert(GraphMLParameter("RegisteredOutputPorts", "string", true));

    return parameters;
}

std::set<GraphMLParameter> BlackBox::graphMLParameters() {
    std::set<GraphMLParameter> parameters = graphMLParametersCommon();

    //TODO: Finish with input/output port access import

    return parameters;
}

std::shared_ptr<BlackBox>
BlackBox::createFromGraphML(int id, std::string name, std::map<std::string, std::string> dataKeyValueMap,
                            std::shared_ptr<SubSystem> parent, GraphMLDialect dialect) {
    std::shared_ptr<BlackBox> newNode = NodeFactory::createNode<BlackBox>(parent);

    newNode->populatePropertiesFromGraphML(id, name, dataKeyValueMap, parent, dialect);

    //TODO: Finish with input/output port access import

    return newNode;
}

void
BlackBox::populatePropertiesFromGraphML(int id, std::string name, std::map<std::string, std::string> dataKeyValueMap,
                                        std::shared_ptr<SubSystem> parent, GraphMLDialect dialect) {
    setId(id);
    setName(name);

    //==== Import important properties ====
    std::string exeCombinationalNameStr;
    std::string stateUpdateNameStr;
    std::string cppHeaderContentStr;
    std::string cppBodyContentStr;
    std::string statefulStr;
    std::string registeredOutputPortsStr;

    //Vitis Name -- InputSigns
    if(dataKeyValueMap.find("ExeCombinationalName") != dataKeyValueMap.end()) {
        exeCombinationalNameStr = dataKeyValueMap.at("ExeCombinationalName");  //Required
    }else{
        exeCombinationalNameStr = "";
    }

    if(dataKeyValueMap.find("StateUpdateName") != dataKeyValueMap.end()) {
        stateUpdateNameStr = dataKeyValueMap.at("StateUpdateName");
    }else{
        stateUpdateNameStr = "";
    }

    cppHeaderContentStr = dataKeyValueMap.at("CppHeaderContent"); //Required
    cppBodyContentStr = dataKeyValueMap.at("CppBodyContent"); //Required

    if(dataKeyValueMap.find("Stateful") != dataKeyValueMap.end()) { //stateful bay not be present (is overidden by the SimulinkCoderFSM)
        statefulStr = dataKeyValueMap.at("Stateful");
    }else{
        statefulStr = "false";
    }

    if(dataKeyValueMap.find("RegisteredOutputPorts") != dataKeyValueMap.end()) { //stateful bay not be present (is overidden by the SimulinkCoderFSM)
        registeredOutputPortsStr = dataKeyValueMap.at("RegisteredOutputPorts");
    }else{
        registeredOutputPortsStr = "";
    }

    exeCombinationalName = exeCombinationalNameStr;
    stateUpdateName = stateUpdateNameStr;
    cppHeaderContent = cppHeaderContentStr;
    cppBodyContent = cppBodyContentStr;

    if(statefulStr == "" || statefulStr == "false" || statefulStr == "0"){
        stateful = false;
    }else{
        stateful = true;
    }

    std::vector<NumericValue> registeredOutputPortsNumeric = NumericValue::parseXMLString(registeredOutputPortsStr);
    for(unsigned long i = 0; i<registeredOutputPortsNumeric.size(); i++){
        if(registeredOutputPortsNumeric[i].isFractional() || registeredOutputPortsNumeric[i].isComplex()){
            throw std::runtime_error(ErrorHelpers::genErrorStr("Error Parsing XML for BlackBox - Stateful", getSharedPointer()));
        }else{
            registeredOutputPorts.push_back(registeredOutputPortsNumeric[i].getRealInt());
        }
    }
}

std::string BlackBox::typeNameStr(){
    return "BlackBox";
}

std::string BlackBox::labelStr() {
    std::string label = Node::labelStr();

    label += "\nFunction: " + typeNameStr() + "\nExeCombinationalName:" + exeCombinationalName + "\nStateful: " +
            GeneralHelper::to_string(stateful) + "\nStateUpdateName: " + stateUpdateName;

    return label;
}

void BlackBox::setResetName(const std::string &resetName) {
    BlackBox::resetName = resetName;
}

void BlackBox::emitGraphMLCommon(xercesc::DOMDocument *doc, xercesc::DOMElement *graphNode) {
    GraphMLHelper::addDataNode(doc, graphNode, "ExeCombinationalName", exeCombinationalName);
    GraphMLHelper::addDataNode(doc, graphNode, "StateUpdateName", stateUpdateName);
    GraphMLHelper::addDataNode(doc, graphNode, "CppHeaderContent", cppHeaderContent);
    GraphMLHelper::addDataNode(doc, graphNode, "CppBodyContent", cppBodyContent);
    GraphMLHelper::addDataNode(doc, graphNode, "Stateful", GeneralHelper::to_string(stateful));
    GraphMLHelper::addDataNode(doc, graphNode, "RegisteredOutputPorts", GeneralHelper::vectorToString(registeredOutputPorts));
}

xercesc::DOMElement *
BlackBox::emitGraphML(xercesc::DOMDocument *doc, xercesc::DOMElement *graphNode, bool include_block_node_type) {
    xercesc::DOMElement* thisNode = emitGraphMLBasics(doc, graphNode);
    if(include_block_node_type) {
        GraphMLHelper::addDataNode(doc, thisNode, "block_node_type", "BlackBox");
    }

    GraphMLHelper::addDataNode(doc, thisNode, "block_function", "BlackBox");

    emitGraphMLCommon(doc, thisNode);

    //TODO: Add emit for input/output port access method

    return thisNode;
}

CExpr
BlackBox::emitCExpr(std::vector<std::string> &cStatementQueue, SchedParams::SchedType schedType, int outputPortNum,
                    bool imag) {

    std::string rtnVarName = name+"_n"+GeneralHelper::to_string(id)+"_rtn";

    if(!previouslyEmitted) { //If we have not previously emitted this function call
        //==== Get Input Exprs ====
        //TODO: Implement Vector Support
        //Checked for in Validation

        //Get the expressions for each input
        std::vector<std::string> inputExprsRe;
        std::vector<std::string> inputExprsIm;

        unsigned long numInputPorts = inputPorts.size();
        for (unsigned long i = 0; i < numInputPorts; i++) {
            std::shared_ptr<OutputPort> srcOutputPort = getInputPort(i)->getSrcOutputPort();
            int srcOutputPortNum = srcOutputPort->getPortNum();
            std::shared_ptr<Node> srcNode = srcOutputPort->getParent();

            inputExprsRe.push_back(srcNode->emitC(cStatementQueue, schedType, srcOutputPortNum, false));

            if(getInputPort(i)->getDataType().isComplex()){
                inputExprsIm.push_back(srcNode->emitC(cStatementQueue, schedType, srcOutputPortNum, true));
            }else{
                inputExprsIm.push_back("");
            }
        }

        //==== Issue the call to the BlackBoxed function ====
        //Check for external inputs and set them if they exist before the fctn call
        for(unsigned long i = 0; i<numInputPorts; i++){
            DataType inputDataType = getInputPort(i)->getDataType();
            if(inputMethods[i] == InputMethod::EXT){
                cStatementQueue.push_back((inputDataType.isComplex() ? inputAccess[i]+reSuffix : inputAccess[i]) + " = " + inputExprsRe[i] + ";");

                if (getInputPort(i)->getDataType().isComplex()) {
                    cStatementQueue.push_back(inputAccess[i]+imSuffix + " = " + inputExprsIm[i] + ";");
                }
            }
        }

        std::string callExpr = "";

        if(returnMethod != BlackBox::ReturnMethod::NONE && returnMethod != BlackBox::ReturnMethod::EXT){
            callExpr = exeCombinationalRtnType + " " + rtnVarName + " = ";
        }

        callExpr += exeCombinationalName + "(";

        bool firstArg = true;
        for(unsigned long i = 0; i < numInputPorts; i++){
            if(inputMethods[i] == InputMethod::FUNCTION_ARG) {
                if (!firstArg) {
                    callExpr += ", ";
                } else {
                    firstArg = false;
                }

                callExpr += inputExprsRe[i];

                if (getInputPort(i)->getDataType().isComplex()) {
                    callExpr += ", " + inputExprsIm[i];
                }
            }
        }

        callExpr += ");";

        cStatementQueue.push_back(callExpr);

        previouslyEmitted = true;
    }

    //==== Return output ====

    DataType outputType = getOutputPort(outputPortNum)->getDataType();

    if(returnMethod == BlackBox::ReturnMethod::SCALAR){
        return CExpr(rtnVarName, true);
    }else if(returnMethod == BlackBox::ReturnMethod::SCALAR_POINTER){
        return CExpr("*"+rtnVarName, true);
    }else if(returnMethod == BlackBox::ReturnMethod::OBJECT){
        return CExpr(rtnVarName + "." + (imag ? outputAccess[outputPortNum]+imSuffix : (outputType.isComplex() ? outputAccess[outputPortNum]+reSuffix : outputAccess[outputPortNum])), true);
    }else if(returnMethod == BlackBox::ReturnMethod::OBJECT_POINTER){
        return CExpr(rtnVarName + "->" + (imag ? outputAccess[outputPortNum]+imSuffix : (outputType.isComplex() ? outputAccess[outputPortNum]+reSuffix : outputAccess[outputPortNum])), true);
    }else if(returnMethod == BlackBox::ReturnMethod::EXT){
        return CExpr(imag ? outputAccess[outputPortNum]+imSuffix : (outputType.isComplex() ? outputAccess[outputPortNum]+reSuffix : outputAccess[outputPortNum]), true);
    }else{
        throw std::runtime_error(ErrorHelpers::genErrorStr("Error During BlackBox Emit - Unexpected Return Type", getSharedPointer()));
    }

}

void BlackBox::validate() {
    Node::validate();

    //TODO: Implement Vector Support
    for(unsigned long i = 0; i<inputPorts.size(); i++) {
        if(!getInputPort(i)->getDataType().isScalar()) {
            throw std::runtime_error(ErrorHelpers::genErrorStr("C Emit Error - BlackBox Support for Vector Types has Not Yet Been Implemented", getSharedPointer()));
        }
    }
    for(unsigned long i = 0; i<outputPorts.size(); i++) {
        if(!getOutputPort(i)->getDataType().isScalar()) {
            throw std::runtime_error(ErrorHelpers::genErrorStr("C Emit Error - BlackBox Support for Vector Types has Not Yet Been Implemented", getSharedPointer()));
        }
    }

    if(returnMethod == BlackBox::ReturnMethod::NONE && outputPorts.size()>0){
        throw std::runtime_error(ErrorHelpers::genErrorStr("C Emit Error - BlackBox Return Type is NONE but had >0 Output Ports", getSharedPointer()));
    }else if((returnMethod == BlackBox::ReturnMethod::SCALAR || returnMethod == BlackBox::ReturnMethod::SCALAR_POINTER) && outputPorts.size()>1){
        throw std::runtime_error(ErrorHelpers::genErrorStr("C Emit Error - BlackBox Return Type is SCALAR or SCALAR_POINTER but had >1 Output Ports", getSharedPointer()));
    }
}

void BlackBox::emitCExprNextState(std::vector<std::string> &cStatementQueue, SchedParams::SchedType schedType) {
    //In general, this method does nothing for blackbox.
    //This is because, even though the blackbox may have state, there is no seperated functions which
}

void BlackBox::emitCStateUpdate(std::vector<std::string> &cStatementQueue, SchedParams::SchedType schedType, std::shared_ptr<StateUpdate> stateUpdateSrc) {
    //If this node is stateful, emit the state update
    if(stateful) {
        cStatementQueue.push_back(stateUpdateName+"();");
    }
}

bool BlackBox::createStateUpdateNode(std::vector<std::shared_ptr<Node>> &new_nodes,
                                     std::vector<std::shared_ptr<Node>> &deleted_nodes,
                                     std::vector<std::shared_ptr<Arc>> &new_arcs,
                                     std::vector<std::shared_ptr<Arc>> &deleted_arcs,
                                     bool includeContext) {
    //Create the state update node only if this block is stateful
    if(stateful){
        return GraphAlgs::createStateUpdateNodeDelayStyle(getSharedPointer(), new_nodes, deleted_nodes, new_arcs, deleted_arcs, includeContext);
    }

    return false;
}

std::vector<BlackBox::InputMethod> BlackBox::getInputMethods() const {
    return inputMethods;
}

void BlackBox::setInputMethods(const std::vector<BlackBox::InputMethod> &inputMethods) {
    BlackBox::inputMethods = inputMethods;
}

std::vector<std::string> BlackBox::getInputAccess() const {
    return inputAccess;
}

void BlackBox::setInputAccess(const std::vector<std::string> &inputAccess) {
    BlackBox::inputAccess = inputAccess;
}

std::string BlackBox::getReSuffix() const {
    return reSuffix;
}

void BlackBox::setReSuffix(const std::string &reSuffix) {
    BlackBox::reSuffix = reSuffix;
}

std::string BlackBox::getImSuffix() const {
    return imSuffix;
}

void BlackBox::setImSuffix(const std::string &imSuffix) {
    BlackBox::imSuffix = imSuffix;
}

bool BlackBox::hasCombinationalPath() {
    return registeredOutputPorts.size() != outputPorts.size();
}


