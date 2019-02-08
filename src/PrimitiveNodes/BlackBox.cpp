//
// Created by Christopher Yarp on 2019-01-28.
//

#include <General/GeneralHelper.h>
#include "BlackBox.h"
#include "General/GraphAlgs.h"

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
        throw std::runtime_error("Unable to parse ReturnMethod: " + str);
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
        default : throw std::runtime_error("Unknown ReturnMethod");
    }
}

BlackBox::BlackBox() : stateful(false), returnMethod(BlackBox::ReturnMethod::NONE), previouslyEmitted(false) {

}

BlackBox::BlackBox(std::shared_ptr<SubSystem> parent) : PrimitiveNode(parent), stateful(false), returnMethod(BlackBox::ReturnMethod::NONE), previouslyEmitted(false) {

}

BlackBox::BlackBox(std::shared_ptr<SubSystem> parent, BlackBox *orig) : PrimitiveNode(parent, orig),
    exeCombinationalName(orig->exeCombinationalName), exeCombinationalRtnType(orig->exeCombinationalRtnType),
    stateUpdateName(orig->stateUpdateName), cppHeaderContent(orig->cppHeaderContent),
    cppBodyContent(orig->cppBodyContent), stateful(orig->stateful), registeredOutputPorts(orig->registeredOutputPorts),
    returnMethod(orig->returnMethod), outputAccessRe(orig->outputAccessRe), outputAccessIm(orig->outputAccessIm),
    previouslyEmitted(orig->previouslyEmitted)
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

std::vector<std::string> BlackBox::getOutputAccessRe() const {
    return outputAccessRe;
}

void BlackBox::setOutputAccessRe(const std::vector<std::string> &outputAccessRe) {
    BlackBox::outputAccessRe = outputAccessRe;
}

std::vector<std::string> BlackBox::getOutputAccessIm() const {
    return outputAccessIm;
}

void BlackBox::setOutputAccessIm(const std::vector<std::string> &outputAccessIm) {
    BlackBox::outputAccessIm = outputAccessIm;
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

std::set<GraphMLParameter> BlackBox::graphMLParameters() {
    std::set<GraphMLParameter> parameters;

    parameters.insert(GraphMLParameter("ExeCombinationalName", "string", true));
    parameters.insert(GraphMLParameter("StateUpdateName", "string", true));
    parameters.insert(GraphMLParameter("CppHeaderContent", "string", true));
    parameters.insert(GraphMLParameter("CppBodyContent", "string", true));
    parameters.insert(GraphMLParameter("Stateful", "bool", true));
    parameters.insert(GraphMLParameter("OutputPortsFromState", "string", true));

    return parameters;
}

std::shared_ptr<BlackBox>
BlackBox::createFromGraphML(int id, std::string name, std::map<std::string, std::string> dataKeyValueMap,
                            std::shared_ptr<SubSystem> parent, GraphMLDialect dialect) {
    std::shared_ptr<BlackBox> newNode = NodeFactory::createNode<BlackBox>(parent);

    newNode->populatePropertiesFromGraphML(id, name, dataKeyValueMap, parent, dialect);

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

    if(dialect == GraphMLDialect::VITIS){
        //Vitis Name -- InputSigns
        exeCombinationalNameStr = dataKeyValueMap.at("ExeCombinationalName");  //Required

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

    } else
    {
        //Simulink import not yet supported.
        throw std::runtime_error("Unsupported Dialect when parsing XML - BlackBox");
    }

    exeCombinationalName = exeCombinationalNameStr;
    stateUpdateName = stateUpdateNameStr;
    cppHeaderContent = cppHeaderContentStr;
    cppBodyContentStr = cppBodyContentStr;

    std::vector<NumericValue> statefulNumeric = NumericValue::parseXMLString(statefulStr);
    if(statefulNumeric.size()!=1){
        throw std::runtime_error("Error Parsing XML for BlackBox - Stateful");
    }else{
        if((!statefulNumeric[0].isFractional()) && (!statefulNumeric[0].isComplex())){
            stateful = (statefulNumeric[0].getRealInt() != 0);
        }else{
            throw std::runtime_error("Error Parsing XML for BlackBox - Stateful");
        }
    }

    std::vector<NumericValue> registeredOutputPortsNumeric = NumericValue::parseXMLString(registeredOutputPortsStr);
    for(unsigned long i = 0; i<registeredOutputPortsNumeric.size(); i++){
        if(registeredOutputPortsNumeric[i].isFractional() || registeredOutputPortsNumeric[i].isComplex()){
            throw std::runtime_error("Error Parsing XML for BlackBox - Stateful");
        }else{
            registeredOutputPorts.push_back(registeredOutputPortsNumeric[i].getRealInt());
        }
    }
}

std::string BlackBox::labelStr() {
    std::string label = Node::labelStr();

    label += "\nFunction: BlackBox\nExeCombinationalName:" + exeCombinationalName + "\nStateful: " +
            GeneralHelper::to_string(stateful) + "\nStateUpdateName: " + stateUpdateName;

    return label;
}

void BlackBox::setResetName(const std::string &resetName) {
    BlackBox::resetName = resetName;
}

xercesc::DOMElement *
BlackBox::emitGraphML(xercesc::DOMDocument *doc, xercesc::DOMElement *graphNode, bool include_block_node_type) {
    xercesc::DOMElement* thisNode = emitGraphMLBasics(doc, graphNode);
    if(include_block_node_type) {
        GraphMLHelper::addDataNode(doc, thisNode, "block_node_type", "Standard");
    }

    GraphMLHelper::addDataNode(doc, thisNode, "block_function", "BlackBox");

    GraphMLHelper::addDataNode(doc, thisNode, "ExeCombinationalName", exeCombinationalName);
    GraphMLHelper::addDataNode(doc, thisNode, "StateUpdateName", stateUpdateName);
    GraphMLHelper::addDataNode(doc, thisNode, "CppHeaderContent", cppHeaderContent);
    GraphMLHelper::addDataNode(doc, thisNode, "CppBodyContent", cppBodyContent);
    GraphMLHelper::addDataNode(doc, thisNode, "Stateful", GeneralHelper::to_string(stateful));
    GraphMLHelper::addDataNode(doc, thisNode, "RegisteredOutputPorts", GeneralHelper::vectorToString(registeredOutputPorts));

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
        std::string callExpr = "";

        if(returnMethod != BlackBox::ReturnMethod::NONE && returnMethod != BlackBox::ReturnMethod::EXT){
            callExpr = exeCombinationalRtnType + " " + rtnVarName + " = ";
        }

        callExpr += exeCombinationalName + "(";

        for(unsigned long i = 0; i < numInputPorts; i++){
            if(i != 0){
                callExpr += ", ";
            }

            callExpr += inputExprsRe[i];

            if(getInputPort(i)->getDataType().isComplex()){
                callExpr += ", " + inputExprsIm[i];
            }
        }

        callExpr += ");";

        cStatementQueue.push_back(callExpr);

        previouslyEmitted = true;
    }

    //==== Return output ====
    if(returnMethod == BlackBox::ReturnMethod::SCALAR){
        return CExpr(rtnVarName, true);
    }else if(returnMethod == BlackBox::ReturnMethod::SCALAR_POINTER){
        return CExpr("*"+rtnVarName, true);
    }else if(returnMethod == BlackBox::ReturnMethod::OBJECT){
        return CExpr(rtnVarName + "." + (imag ? outputAccessIm[outputPortNum] : outputAccessRe[outputPortNum]), true);
    }else if(returnMethod == BlackBox::ReturnMethod::OBJECT_POINTER){
        return CExpr(rtnVarName + "->" + (imag ? outputAccessIm[outputPortNum] : outputAccessRe[outputPortNum]), true);
    }else if(returnMethod == BlackBox::ReturnMethod::EXT){
        return CExpr(imag ? outputAccessIm[outputPortNum] : outputAccessRe[outputPortNum], true);
    }else{
        throw std::runtime_error("Error During BlackBox Emit - Unexpected Return Type");
    }

}

void BlackBox::validate() {
    Node::validate();

    //TODO: Implement Vector Support
    for(unsigned long i = 0; i<inputPorts.size(); i++) {
        if(getInputPort(i)->getDataType().getWidth() > 1) {
            throw std::runtime_error("C Emit Error - BlackBox Support for Vector Types has Not Yet Been Implemented");
        }
    }
    for(unsigned long i = 0; i<outputPorts.size(); i++) {
        if(getOutputPort(i)->getDataType().getWidth() > 1) {
            throw std::runtime_error("C Emit Error - BlackBox Support for Vector Types has Not Yet Been Implemented");
        }
    }

    if(returnMethod == BlackBox::ReturnMethod::NONE && outputPorts.size()>0){
        throw std::runtime_error("C Emit Error - BlackBox Return Type is NONE but had >0 Output Ports");
    }else if((returnMethod == BlackBox::ReturnMethod::SCALAR || returnMethod == BlackBox::ReturnMethod::SCALAR_POINTER) && outputPorts.size()>1){
        throw std::runtime_error("C Emit Error - BlackBox Return Type is SCALAR or SCALAR_POINTER but had >1 Output Ports");
    }
}

void BlackBox::emitCExprNextState(std::vector<std::string> &cStatementQueue, SchedParams::SchedType schedType) {
    //In general, this method does nothing for blackbox.
    //This is because, even though the blackbox may have state, there is no seperated functions which
}

void BlackBox::emitCStateUpdate(std::vector<std::string> &cStatementQueue, SchedParams::SchedType schedType) {
    //If this node is stateful, emit the state update
    if(stateful) {
        cStatementQueue.push_back(stateUpdateName+"();");
    }
}

bool BlackBox::createStateUpdateNode(std::vector<std::shared_ptr<Node>> &new_nodes,
                                     std::vector<std::shared_ptr<Node>> &deleted_nodes,
                                     std::vector<std::shared_ptr<Arc>> &new_arcs,
                                     std::vector<std::shared_ptr<Arc>> &deleted_arcs) {
    //Create the state update node only if this block is stateful
    if(stateful){
        return GraphAlgs::createStateUpdateNodeDelayStyle(getSharedPointer(), new_nodes, deleted_nodes, new_arcs, deleted_arcs);
    }

    return false;
}


