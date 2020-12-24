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
    }else if(str == "PTR_ARG" || str == "ptr_arg"){
        return BlackBox::ReturnMethod::PTR_ARG;
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
        case BlackBox::ReturnMethod::PTR_ARG : return "PTR_ARG";
        default : throw std::runtime_error(ErrorHelpers::genErrorStr("Unknown ReturnMethod"));
    }
}

BlackBox::BlackBox() : stateful(false), returnMethod(BlackBox::ReturnMethod::NONE), previouslyEmitted(false) {

}

BlackBox::BlackBox(std::shared_ptr<SubSystem> parent) : PrimitiveNode(parent), stateful(false), returnMethod(BlackBox::ReturnMethod::NONE), previouslyEmitted(false) {

}

BlackBox::BlackBox(std::shared_ptr<SubSystem> parent, BlackBox *orig) : PrimitiveNode(parent, orig),
    exeCombinationalName(orig->exeCombinationalName), exeCombinationalRtnType(orig->exeCombinationalRtnType),
    stateUpdateName(orig->stateUpdateName), resetName(orig->resetName),
    cppHeaderContent(orig->cppHeaderContent), cppBodyContent(orig->cppBodyContent), stateful(orig->stateful),
    reSuffix(orig->reSuffix), imSuffix(orig->imSuffix), inputMethods(orig->inputMethods),
    inputAccess(orig->inputAccess), registeredOutputPorts(orig->registeredOutputPorts),
    returnMethod(orig->returnMethod), outputAccess(orig->outputAccess), previouslyEmitted(orig->previouslyEmitted),
    additionalArgsComboFctn(orig->additionalArgsComboFctn),
    additionalArgsStateUpdateFctn(orig->additionalArgsStateUpdateFctn),
    additionalArgsResetFctn(orig->additionalArgsResetFctn), outputTypes(orig->outputTypes)
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

    //TODO: Add more parameters when more than just the Simulink FSM uses the black box interface.  Need to go through all the args
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
        std::vector<CExpr> inputExprsRe;
        std::vector<CExpr> inputExprsIm;

        int expectedInputArgs = 0;

        unsigned long numInputPorts = inputPorts.size();
        for (unsigned long i = 0; i < numInputPorts; i++) {
            std::shared_ptr<OutputPort> srcOutputPort = getInputPort(i)->getSrcOutputPort();
            int srcOutputPortNum = srcOutputPort->getPortNum();
            std::shared_ptr<Node> srcNode = srcOutputPort->getParent();

            inputExprsRe.push_back(srcNode->emitC(cStatementQueue, schedType, srcOutputPortNum, false));

            if(getInputPort(i)->getDataType().isComplex()){
                inputExprsIm.push_back(srcNode->emitC(cStatementQueue, schedType, srcOutputPortNum, true));
            }else{
                inputExprsIm.push_back(CExpr("", inputExprsRe[inputExprsRe.size()-1].getExprType()));
            }

            if(inputMethods[i] != InputMethod::EXT){
                expectedInputArgs++;
            }
        }

        //==== Check if the output type is ARG_PTR and declare temporary outputs which will be passed to the black box
        int expectedOutputArgs = 0;
        if(returnMethod == ReturnMethod::PTR_ARG){
            unsigned long numOutputPorts = outputPorts.size();
            for(unsigned long i = 0; i<numOutputPorts; i++){
                std::shared_ptr<OutputPort> outputPort = getOutputPort(i);
                DataType dt = outputTypes[i];

                //Will use the rtnVarName + the output port number
                Variable outputTmp = Variable(rtnVarName+"_port"+GeneralHelper::to_string(outputPort->getPortNum()), dt);

                cStatementQueue.push_back(outputTmp.getCVarDecl(false, true, false, true, false) + ";");

                if(dt.isComplex()){
                    cStatementQueue.push_back(outputTmp.getCVarDecl(true, true, false, true, false) + ";");
                }

                expectedOutputArgs++;
            }
        }

        //==== Issue the call to the BlackBoxed function ====
        //Check for external inputs and set them if they exist before the fctn call
        for(unsigned long i = 0; i<numInputPorts; i++){
            DataType inputDataType = getInputPort(i)->getDataType();
            if(inputMethods[i] == InputMethod::EXT){
                cStatementQueue.push_back((inputDataType.isComplex() ? inputAccess[i]+reSuffix : inputAccess[i]) + " = " + inputExprsRe[i].getExpr() + ";");

                if (getInputPort(i)->getDataType().isComplex()) {
                    cStatementQueue.push_back(inputAccess[i]+imSuffix + " = " + inputExprsIm[i].getExpr() + ";");
                }
            }
        }

        //Sort additional args
        std::vector<std::pair<std::string, int>> sortedAdditionalArgs  = additionalArgsComboFctn;
        std::sort(sortedAdditionalArgs.begin(),sortedAdditionalArgs.end(),
                  [](std::pair<std::string, int> a, std::pair<std::string, int> b) -> bool {
                      return a.second < b.second;
                  });

        std::string callExpr = "";

        if(returnMethod != BlackBox::ReturnMethod::NONE && returnMethod != BlackBox::ReturnMethod::EXT &&
           returnMethod != BlackBox::ReturnMethod::PTR_ARG){
            callExpr = exeCombinationalRtnType + " " + rtnVarName + " = ";
        }

        callExpr += exeCombinationalName + "(";


        int inputArgCount = 0;
        int inputArgIdx = 0; //Need a seperate counter because inputs are allowed to be split between args and ext
        int outputArgCount = 0;
        int additionalArgCount = 0;

        while(inputArgCount<expectedInputArgs || outputArgCount<expectedOutputArgs || additionalArgCount<sortedAdditionalArgs.size()){
            int idx = inputArgCount+outputArgCount+additionalArgCount;

            if(idx != 0){
                callExpr += ", ";
            }

            if(additionalArgCount < sortedAdditionalArgs.size()){
                //Check if the additional arg should be inserted into the current index
                if(sortedAdditionalArgs[additionalArgCount].second == idx){
                    //Yes, this is the place for the additional argument
                    callExpr += sortedAdditionalArgs[additionalArgCount].first;
                    additionalArgCount++;
                }else if(inputArgCount>=expectedInputArgs && outputArgCount>=expectedOutputArgs){
                    throw std::runtime_error(ErrorHelpers::genErrorStr("Blackbox combinational additional argument has an index > than the index after all preceding arguments (input, output, and additional) have been emitted.", getSharedPointer()));
                }
            }else if(inputArgCount<expectedInputArgs) {
                //Emit input args first.  We are still emitting input args
                if (inputMethods[inputArgIdx] == InputMethod::FUNCTION_ARG) {

                    callExpr += inputExprsRe[inputArgIdx].getExpr();

                    if (getInputPort(inputArgIdx)->getDataType().isComplex()) {
                        callExpr += ", " + inputExprsIm[inputArgIdx].getExpr();
                    }

                    inputArgCount++;
                }
                inputArgIdx++;
            }else if(outputArgCount<expectedOutputArgs){
                //Done emitting input args, Emitting output args

                //Note, it should not be possible to get here unless returnMethod == ReturnMethod::PTR_ARG
                //Also, we can use the outputArgCount as the index

                std::shared_ptr<OutputPort> outputPort = getOutputPort(outputArgCount);
                DataType dt = outputTypes[outputArgCount];

                //Will use the rtnVarName + the output port number
                Variable outputTmp = Variable(rtnVarName+"_port"+GeneralHelper::to_string(outputPort->getPortNum()), dt);

                callExpr += (dt.isScalar() ? "&" : "") + outputTmp.getCVarName(false);

                if(dt.isComplex()){
                    callExpr += std::string(", ") + (dt.isScalar() ? "&" : "") + outputTmp.getCVarName(true);
                }

                outputArgCount++;
            }else{
                //Something has gone horribly wrong because the loop still ran but we already included all the expected input, output, and additional args
                throw std::runtime_error(ErrorHelpers::genErrorStr("Something went wrong when creating the blackbox combinational function call", getSharedPointer()));
            }
        }

        callExpr += ");";

        cStatementQueue.push_back(callExpr);

        previouslyEmitted = true;
    }

    //==== Return output ====

    DataType outputType = getOutputPort(outputPortNum)->getDataType();

    //TODO: Assuming passthrough of input variable directly to output (in terms of pointer or reference not by value)
    //      is not possible given that call is made in standard C and not C++ which allows pass by reference.
    //      As such, will assume any passed through value is effectively copied.  This may not be true when vector support
    //      is implemented as pointers will be passed.  This could introduce problems if the BlackBox is attached directly
    //      to state at its input.  The state could be updated before the result of this block is used.
    //      When StateUpdate insertion logic improved to track passthroughs, this becomes less of a problem and BlackBoxes
    //      will need to be assumed to cary passthrough values.

    //TODO: Update cExprType if vector support implemented
    if(returnMethod == BlackBox::ReturnMethod::SCALAR){
        return CExpr(rtnVarName, CExpr::ExprType::SCALAR_VAR);
    }else if(returnMethod == BlackBox::ReturnMethod::SCALAR_POINTER){
        return CExpr("*"+rtnVarName, CExpr::ExprType::SCALAR_VAR);
    }else if(returnMethod == BlackBox::ReturnMethod::OBJECT){
        return CExpr(rtnVarName + "." + (imag ? outputAccess[outputPortNum]+imSuffix : (outputType.isComplex() ? outputAccess[outputPortNum]+reSuffix : outputAccess[outputPortNum])), CExpr::ExprType::SCALAR_VAR);
    }else if(returnMethod == BlackBox::ReturnMethod::OBJECT_POINTER){
        return CExpr(rtnVarName + "->" + (imag ? outputAccess[outputPortNum]+imSuffix : (outputType.isComplex() ? outputAccess[outputPortNum]+reSuffix : outputAccess[outputPortNum])), CExpr::ExprType::SCALAR_VAR);
    }else if(returnMethod == BlackBox::ReturnMethod::EXT) {
        return CExpr(imag ? outputAccess[outputPortNum] + imSuffix : (outputType.isComplex() ? outputAccess[outputPortNum] + reSuffix : outputAccess[outputPortNum]),CExpr::ExprType::SCALAR_VAR);
    }else if(returnMethod == BlackBox::ReturnMethod::PTR_ARG){
        std::shared_ptr<OutputPort> outputPort = getOutputPort(outputPortNum);
        DataType dt = outputPort->getDataType();

        //Will use the rtnVarName + the output port number
        Variable outputTmp = Variable(rtnVarName+"_port"+GeneralHelper::to_string(outputPort->getPortNum()), dt);

        return CExpr(outputTmp.getCVarName(imag), dt.isScalar() ? CExpr::ExprType::SCALAR_VAR : CExpr::ExprType::ARRAY);
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

        //Sort additional args
        std::vector<std::pair<std::string, int>> sortedAdditionalArgs  = additionalArgsStateUpdateFctn;
        std::sort(sortedAdditionalArgs.begin(),sortedAdditionalArgs.end(),
                  [](std::pair<std::string, int> a, std::pair<std::string, int> b) -> bool {
                      return a.second < b.second;
                  });

        std::string call = stateUpdateName + "(";

        //The additional arguments should not have any discontinuity in their index numbers
        for(int i = 0; i<sortedAdditionalArgs.size(); i++){
            if (sortedAdditionalArgs[i].second != i){
                throw std::runtime_error(ErrorHelpers::genErrorStr("State update args must not have any discontinuity in index", getSharedPointer()));
            }

            if(i>0){
                call += ", ";
            }

            call += sortedAdditionalArgs[i].first;
        }

        call += ");";
        cStatementQueue.push_back(call);
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

std::vector<std::pair<std::string, int>> BlackBox::getAdditionalArgsComboFctn() const {
    return additionalArgsComboFctn;
}

void BlackBox::setAdditionalArgsComboFctn(const std::vector<std::pair<std::string, int>> &additionalArgsComboFctn) {
    BlackBox::additionalArgsComboFctn = additionalArgsComboFctn;
}

std::vector<std::pair<std::string, int>> BlackBox::getAdditionalArgsStateUpdateFctn() const {
    return additionalArgsStateUpdateFctn;
}

void BlackBox::setAdditionalArgsStateUpdateFctn(
        const std::vector<std::pair<std::string, int>> &additionalArgsStateUpdateFctn) {
    BlackBox::additionalArgsStateUpdateFctn = additionalArgsStateUpdateFctn;
}

std::vector<std::pair<std::string, int>> BlackBox::getAdditionalArgsResetFctn() const {
    return additionalArgsResetFctn;
}

void BlackBox::setAdditionalArgsResetFctn(const std::vector<std::pair<std::string, int>> &additionalArgsResetFctn) {
    BlackBox::additionalArgsResetFctn = additionalArgsResetFctn;
}

std::vector<Variable> BlackBox::getStateVars() const {
    return stateVars;
}

void BlackBox::setStateVars(const std::vector<Variable> &stateVars) {
    BlackBox::stateVars = stateVars;
}

std::vector<Variable> BlackBox::getCStateVars() {
    return stateVars;
}

std::string BlackBox::getResetFunctionCall() {
    //Sort additional args
    std::vector<std::pair<std::string, int>> sortedAdditionalArgs  = additionalArgsResetFctn;
    std::sort(sortedAdditionalArgs.begin(),sortedAdditionalArgs.end(),
              [](std::pair<std::string, int> a, std::pair<std::string, int> b) -> bool {
                  return a.second < b.second;
              });

    std::string call = resetName + "(";

    //The additional arguments should not have any discontinuity in their index numbers
    for(int i = 0; i<sortedAdditionalArgs.size(); i++){
        if (sortedAdditionalArgs[i].second != i){
            throw std::runtime_error(ErrorHelpers::genErrorStr("Reset args must not have any discontinuity in index", getSharedPointer()));
        }

        if(i>0){
            call += ", ";
        }

        call += sortedAdditionalArgs[i].first;
    }

    call += ")";

    return call;
}

std::vector<DataType> BlackBox::getOutputTypes() const {
    return outputTypes;
}

void BlackBox::setOutputTypes(const std::vector<DataType> &outputTypes) {
    BlackBox::outputTypes = outputTypes;
}

void BlackBox::propagateProperties() {
    Node::propagateProperties();

    std::vector<DataType> outputTypeLocal;

    for(int i = 0; i<outputPorts.size(); i++){
        //TODO: assumes no discontinuity in ports and are in sorted order
        DataType dt = outputPorts[i]->getDataType();
        int portNum = outputPorts[i]->getPortNum();
        if(portNum != i){
            throw std::runtime_error(ErrorHelpers::genErrorStr("Unexpected port numbering "));
        }

        outputTypeLocal.push_back(dt);
    }

    setOutputTypes(outputTypeLocal);
}


