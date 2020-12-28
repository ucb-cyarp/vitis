//
// Created by Christopher Yarp on 2019-01-28.
//

#include "SimulinkCoderFSM.h"

#include "GraphMLTools/GraphMLImporter.h"
#include "GraphMLTools/GraphMLExporter.h"

#include "General/ErrorHelpers.h"

SimulinkCoderFSM::SimulinkCoderFSM() : simulinkExportReusableFunction(false) {

}

SimulinkCoderFSM::SimulinkCoderFSM(std::shared_ptr<SubSystem> parent) : BlackBox(parent),
    simulinkExportReusableFunction(false) {

}

SimulinkCoderFSM::SimulinkCoderFSM(std::shared_ptr<SubSystem> parent, SimulinkCoderFSM *orig) : BlackBox(parent, orig),
    inputsStructName(orig->inputsStructName), outputsStructName(orig->outputsStructName),
    stateStructName(orig->stateStructName), simulinkExportReusableFunction(orig->simulinkExportReusableFunction),
    rtModelStructType(orig->rtModelStructType), rtModelStatePtrName(orig->rtModelStatePtrName),
    stateStructType(orig->stateStructType), generatedReset(orig->generatedReset) {

}

std::shared_ptr<BlackBox>
SimulinkCoderFSM::createFromGraphML(int id, std::string name, std::map<std::string, std::string> dataKeyValueMap,
                                    std::shared_ptr<SubSystem> parent, GraphMLDialect dialect) {
    std::shared_ptr<SimulinkCoderFSM> newNode = NodeFactory::createNode<SimulinkCoderFSM>(parent);

    newNode->populatePropertiesFromGraphML(id, name, dataKeyValueMap, parent, dialect);

    //Import Port Names (used to set port types)
    GraphMLImporter::importNodePortNames(newNode, dataKeyValueMap, dialect);

    //Get Stateflow specific properties
    std::string initFctnStr;
    std::string outputFunctionStr;
    std::string stateUpdateFunctionStr;
    std::string inputsStructNameStr;
    std::string outputsStructNameStr;
    std::string stateStructNameStr;

    if(dataKeyValueMap.find("init_function") != dataKeyValueMap.end()) {
        initFctnStr = dataKeyValueMap.at("init_function");
    }else{
        throw std::runtime_error(ErrorHelpers::genErrorStr("Stateflow Module Missing Init Function", newNode));
    }

    if(dataKeyValueMap.find("output_function") != dataKeyValueMap.end()) {
        outputFunctionStr = dataKeyValueMap.at("output_function");
    }else{
        throw std::runtime_error(ErrorHelpers::genErrorStr("Stateflow Module Missing Output Function", newNode));
    }

    if(dataKeyValueMap.find("state_update_function") != dataKeyValueMap.end()) {
        stateUpdateFunctionStr = dataKeyValueMap.at("state_update_function");
    }else{
        throw std::runtime_error(ErrorHelpers::genErrorStr("Stateflow Module Missing State Update Function", newNode));
    }

    if(dataKeyValueMap.find("inputs_struct_name") != dataKeyValueMap.end()) {
        inputsStructNameStr = dataKeyValueMap.at("inputs_struct_name");
    }else{
        throw std::runtime_error(ErrorHelpers::genErrorStr("Stateflow Module Missing Input Structure Name", newNode));
    }

    if(dataKeyValueMap.find("outputs_struct_name") != dataKeyValueMap.end()) {
        outputsStructNameStr = dataKeyValueMap.at("outputs_struct_name");
    }else{
        throw std::runtime_error(ErrorHelpers::genErrorStr("Stateflow Module Missing Output Structure Name", newNode));
    }

    if(dataKeyValueMap.find("state_struct_name") != dataKeyValueMap.end()) {
        stateStructNameStr = dataKeyValueMap.at("state_struct_name");
    }else{
        throw std::runtime_error(ErrorHelpers::genErrorStr("Stateflow Module Missing State Structure Name", newNode));
    }

    //Set parameters
    newNode->setInputsStructName(inputsStructNameStr);
    newNode->setOutputsStructName(outputsStructNameStr);
    newNode->setStateStructName(stateStructNameStr);

    //Set BlackBox Parameters from Stateflow Parameters (often one-to-one but with a name change)
    newNode->setExeCombinationalName(outputFunctionStr);
    newNode->setExeCombinationalRtnType(""); //Output is to external global
    newNode->setStateUpdateName(stateUpdateFunctionStr);
    //Body and header contents imported in populateProperties method above
    newNode->setStateful(true);
    newNode->setReSuffix(".re"); //In the current simulink settings, complex types are structures
    newNode->setImSuffix(".im");

    //Set the origionally generated reset function name
    //Note that the reset function in the blackbox superclass is set to a different reset function
    if(dialect == GraphMLDialect::SIMULINK_EXPORT){
        newNode->setGeneratedReset(initFctnStr);
    }else if(dialect == GraphMLDialect::VITIS){
        std::string origResetFctn;
        if(dataKeyValueMap.find("generatedReset") != dataKeyValueMap.end()) {
            origResetFctn = dataKeyValueMap.at("generatedReset");
        }else{
            throw std::runtime_error(ErrorHelpers::genErrorStr("Stateflow Module Missing Orig Reset Function", newNode));
        }

        newNode->setGeneratedReset(origResetFctn);
    }else{
        throw std::runtime_error(ErrorHelpers::genErrorStr("Unknown GraphML Dialect", newNode->getSharedPointer()));
    }

    //Check if the export from simulink included re-usable functions
    bool reusableFctn = false;
    if(dataKeyValueMap.find("exported_reusable_fctns") != dataKeyValueMap.end()) {
        if(dataKeyValueMap["exported_reusable_fctns"] == "yes" || dataKeyValueMap["exported_reusable_fctns"] == "Yes" ||
           dataKeyValueMap["exported_reusable_fctns"] == "YES" || dataKeyValueMap["exported_reusable_fctns"] == "true" ||
           dataKeyValueMap["exported_reusable_fctns"] == "True" || dataKeyValueMap["exported_reusable_fctns"] == "TRUE"){
            reusableFctn = true;
            newNode->setSimulinkExportReusableFunction(true);

            std::string rtModelType;
            if(dataKeyValueMap.find("rtModelType") != dataKeyValueMap.end()) {
                rtModelType = dataKeyValueMap.at("rtModelType");
            }else{
                throw std::runtime_error(ErrorHelpers::genErrorStr("Stateflow Module Missing RT Model Type", newNode));
            }

            std::string stateStructType;
            if(dataKeyValueMap.find("stateStructType") != dataKeyValueMap.end()) {
                stateStructType = dataKeyValueMap.at("stateStructType");
            }else{
                throw std::runtime_error(ErrorHelpers::genErrorStr("Stateflow Module Missing State Struct Type", newNode));
            }

            std::string rtStateMemberPtrName;
            if(dataKeyValueMap.find("rtStateMemberPtrName") != dataKeyValueMap.end()) {
                rtStateMemberPtrName = dataKeyValueMap.at("rtStateMemberPtrName");
            }else{
                throw std::runtime_error(ErrorHelpers::genErrorStr("Stateflow Module Missing RT State Ptr Name", newNode));
            }

            newNode->setRtModelStructType(rtModelType);
            newNode->setStateStructType(stateStructType);
            newNode->setRtModelStatePtrName(rtStateMemberPtrName);
            //Reset name is set in call to getResetFunctionCall
        }else{
            newNode->setSimulinkExportReusableFunction(false);
            newNode->setResetName(initFctnStr); //It appears that in the Simulink Coder Stateflow Generated C, init accomplishes the same as reset
        }
    }else{
        newNode->setSimulinkExportReusableFunction(false);
        newNode->setResetName(initFctnStr); //It appears that in the Simulink Coder Stateflow Generated C, init accomplishes the same as reset
    }

    //Input ports are external for the non-reusable style of Simulink Code Generation
    //Input ports are arguments for reusable style of Simulink Code Generation
    std::vector<std::shared_ptr<InputPort>> inputPorts = newNode->getInputPorts();
    std::vector<BlackBox::InputMethod> inputMethods;
    std::vector<std::string> inputAccess;
    for(unsigned long i = 0; i<inputPorts.size(); i++){
        if(reusableFctn){
            inputMethods.push_back(BlackBox::InputMethod::FUNCTION_ARG);
        }else {
            inputMethods.push_back(BlackBox::InputMethod::EXT);
            inputAccess.push_back(
                    inputsStructNameStr + "." + newNode->getInputPort(i)->getName()); //Prepend stucture name here
        }
    }
    newNode->setInputMethods(inputMethods);
    newNode->setInputAccess(inputAccess);

    //Output ports are external for the non-reusable style of Simulink Code Generation
    //Output ports are pointer arguments for the reusable style of Simulink Code Generation
    //   HOWEVER! Output ports can also be part of the state for stateflow charts!
    //            If a state variable is declared as an output, it (at least with the settings used
    //            in the current incarnation of the Simulink export script) will not be included
    //            in the external state structure.  The output is passed as a pointer but it is
    //            expected by stateflow/simulink that the value is preserved between calls.
    if(reusableFctn){
        std::string imSuffix = "_im";
        std::string reSuffix = "_re";
        newNode->setImSuffix(imSuffix);
        newNode->setReSuffix(reSuffix);

        //Output access used for description
        std::vector<std::shared_ptr<OutputPort>> outputPorts = newNode->getOutputPorts();
        std::vector<std::string> outputAccess;
        for (unsigned long i = 0; i < outputPorts.size(); i++) {
            std::shared_ptr<OutputPort> outputPort = outputPorts[i];
            std::string descr = outputPort->getName();
            if(!descr.empty()){
                descr = "_" + descr;
            }

            outputAccess.push_back(std::string(VITIS_STATE_STRUCT_NAME) + "->blackbox" + GeneralHelper::to_string(outputPort->getParent()->getId()) + "_out" +
                                   descr + "_port" + GeneralHelper::to_string(i));
        }

        newNode->setOutputAccess(outputAccess);
        newNode->setReturnMethod(BlackBox::ReturnMethod::PTR_ARG);
    }else {
        std::vector<std::shared_ptr<OutputPort>> outputPorts = newNode->getOutputPorts();
        std::vector<std::string> outputAccess;
        for (unsigned long i = 0; i < outputPorts.size(); i++) {
            outputAccess.push_back(outputsStructNameStr + "." + newNode->getOutputPort(i)->getName());
        }
        newNode->setOutputAccess(outputAccess);
        newNode->setReturnMethod(BlackBox::ReturnMethod::EXT);
    }

    //Assume that no output ports are registered*
    //TODO: refine
    std::vector<int> registeredOutputPorts;
    newNode->setRegisteredOutputPorts(registeredOutputPorts);

    if(reusableFctn){
        // Set state variables in state structure
        Variable fsmStateVar = Variable(name+"_n"+GeneralHelper::to_string(id)+"_state", DataType(), {}, false, true);
        fsmStateVar.setOverrideType(newNode->getStateStructType());

        Variable fsmRTModelVar = Variable(name+"_n"+GeneralHelper::to_string(id)+"_rtModel", DataType(), {}, false, true);
        fsmRTModelVar.setOverrideType(newNode->getRtModelStructType());

        std::vector<Variable> stateVars = {fsmStateVar, fsmRTModelVar};

        //Need to include outputs as state variables as well
        //However, we need to know their types which we learn in the
        //propogateProperties step
        //defer adding these until the propragateProperties step.

        newNode->setStateVars(stateVars);

        //Add additional args to functions (for passing state)
        //Need to pass a pointer to the rtModel rather than a copy of the structure
        std::vector<std::pair<std::string, int>> additionalArgsComboStateUpdate =
                {std::pair<std::string, int> ("(&(" + fsmRTModelVar.getCVarName(false, true, true) + "))", 0)};

        newNode->setAdditionalArgsComboFctn(additionalArgsComboStateUpdate);
        newNode->setAdditionalArgsStateUpdateFctn(additionalArgsComboStateUpdate);

        std::vector<std::pair<std::string, int>> additionalArgsRst =
                {std::pair<std::string, int> (VITIS_STATE_STRUCT_NAME, 0)};
        newNode->setAdditionalArgsResetFctn(additionalArgsRst);
    }

    //TODO: Add importing/exporting output port types.

    return newNode;
}

std::string SimulinkCoderFSM::getInputsStructName() const {
    return inputsStructName;
}

void SimulinkCoderFSM::setInputsStructName(const std::string &inputsStructName) {
    SimulinkCoderFSM::inputsStructName = inputsStructName;
}

std::string SimulinkCoderFSM::getOutputsStructName() const {
    return outputsStructName;
}

void SimulinkCoderFSM::setOutputsStructName(const std::string &outputsStructName) {
    SimulinkCoderFSM::outputsStructName = outputsStructName;
}

std::string SimulinkCoderFSM::getStateStructName() const {
    return stateStructName;
}

void SimulinkCoderFSM::setStateStructName(const std::string &stateStructName) {
    SimulinkCoderFSM::stateStructName = stateStructName;
}

xercesc::DOMElement *
SimulinkCoderFSM::emitGraphML(xercesc::DOMDocument *doc, xercesc::DOMElement *graphNode, bool include_block_node_type) {
    xercesc::DOMElement* thisNode = emitGraphMLBasics(doc, graphNode);
    if(include_block_node_type) {
        GraphMLHelper::addDataNode(doc, thisNode, "block_node_type", "Stateflow");
    }

    GraphMLHelper::addDataNode(doc, thisNode, "block_function", "Stateflow");

    emitGraphMLCommon(doc, thisNode);

    GraphMLExporter::exportPortNames(getSharedPointer(), doc, thisNode);

    GraphMLHelper::addDataNode(doc, thisNode, "init_function", getResetName());
    GraphMLHelper::addDataNode(doc, thisNode, "output_function", getExeCombinationalName());
    GraphMLHelper::addDataNode(doc, thisNode, "state_update_function", getStateUpdateName());
    GraphMLHelper::addDataNode(doc, thisNode, "inputs_struct_name", getInputsStructName());
    GraphMLHelper::addDataNode(doc, thisNode, "outputs_struct_name", getOutputsStructName());
    GraphMLHelper::addDataNode(doc, thisNode, "state_struct_name", getStateStructName());
    GraphMLHelper::addDataNode(doc, thisNode, "exported_reusable_fctns", isSimulinkExportReusableFunction() ? "Yes" : "No");
    GraphMLHelper::addDataNode(doc, thisNode, "rtModelType", getRtModelStructType());
    GraphMLHelper::addDataNode(doc, thisNode, "stateStructType", getStateStructType());
    GraphMLHelper::addDataNode(doc, thisNode, "rtStateMemberPtrName", getRtModelStatePtrName());
    GraphMLHelper::addDataNode(doc, thisNode, "generatedReset", getGeneratedReset());


    return thisNode;
}

std::set<GraphMLParameter> SimulinkCoderFSM::graphMLParameters() {
    std::set<GraphMLParameter> parameters  = graphMLParametersCommon();

    GraphMLExporter::addPortNameProperties(getSharedPointer(), parameters);

    parameters.insert(GraphMLParameter("init_function", "string", true));
    parameters.insert(GraphMLParameter("output_function", "string", true));
    parameters.insert(GraphMLParameter("state_update_function", "string", true));
    parameters.insert(GraphMLParameter("inputs_struct_name", "string", true));
    parameters.insert(GraphMLParameter("outputs_struct_name", "string", true));
    parameters.insert(GraphMLParameter("state_struct_name", "string", true));
    parameters.insert(GraphMLParameter("exported_reusable_fctns", "string", true));
    parameters.insert(GraphMLParameter("rtModelType", "string", true));
    parameters.insert(GraphMLParameter("stateStructType", "string", true));
    parameters.insert(GraphMLParameter("rtStateMemberPtrName", "string", true));
    parameters.insert(GraphMLParameter("generatedReset", "string", true));

    return parameters;
}

std::string SimulinkCoderFSM::typeNameStr(){
    return "SimulinkCoderFSM";
}

bool SimulinkCoderFSM::isSimulinkExportReusableFunction() const {
    return simulinkExportReusableFunction;
}

void SimulinkCoderFSM::setSimulinkExportReusableFunction(bool simulinkExportReusableFunction) {
    SimulinkCoderFSM::simulinkExportReusableFunction = simulinkExportReusableFunction;
}

std::string SimulinkCoderFSM::getRtModelStructType() const {
    return rtModelStructType;
}

void SimulinkCoderFSM::setRtModelStructType(const std::string &rtModelStructType) {
    SimulinkCoderFSM::rtModelStructType = rtModelStructType;
}

std::string SimulinkCoderFSM::getRtModelStatePtrName() const {
    return rtModelStatePtrName;
}

void SimulinkCoderFSM::setRtModelStatePtrName(const std::string &rtModelStatePtrName) {
    SimulinkCoderFSM::rtModelStatePtrName = rtModelStatePtrName;
}

std::string SimulinkCoderFSM::getStateStructType() const {
    return stateStructType;
}

void SimulinkCoderFSM::setStateStructType(const std::string &stateStructType) {
    SimulinkCoderFSM::stateStructType = stateStructType;
}

std::string SimulinkCoderFSM::getCppHeaderContent() const {
    std::string cppHeader = BlackBox::getCppHeaderContent();

    if(isSimulinkExportReusableFunction()){
        //Add the prototype for the reset function
        cppHeader += "\n#ifndef _H_NODE"+GeneralHelper::to_string(id)+"\n";
        cppHeader += "#define _H_NODE"+GeneralHelper::to_string(id)+"\n";

        cppHeader += getResetFunctionPrototype() + ";\n";

        cppHeader += "#endif\n";
    }

    return cppHeader;
}

std::string SimulinkCoderFSM::getCppBodyContent() const {
    std::string cppBody = BlackBox::getCppBodyContent();

    if(isSimulinkExportReusableFunction()){
        std::string rstName = "node"+GeneralHelper::to_string(id)+"_rst";
        Variable fsmStateVar = Variable(name+"_n"+GeneralHelper::to_string(id)+"_state", DataType(), {}, false, true);
        fsmStateVar.setOverrideType(getStateStructType());
        Variable fsmRTModelVar = Variable(name+"_n"+GeneralHelper::to_string(id)+"_rtModel", DataType(), {}, false, true);
        fsmRTModelVar.setOverrideType(getRtModelStructType());

        cppBody += getResetFunctionPrototype() + "{\n";

        //Set the ptr in rtModel
        cppBody += "\t" + fsmRTModelVar.getCVarName(false) +
                "." + rtModelStatePtrName + " = &(" + fsmStateVar.getCVarName(false) + ");\n"; //Note that these vars were passed as ptrs

        //Call the init function provided by the exported FSM, passing the RT model ptr and dummy variables for the inputs and outputs
        std::vector<Variable> dummyVars;
        for(int i = 0; i<inputPorts.size(); i++){
            //TODO: assumes no discontinuity in ports and are in sorted order
            DataType dt = inputPorts[i]->getDataType();
            int portNum = inputPorts[i]->getPortNum();
            if(portNum != i){
                throw std::runtime_error(ErrorHelpers::genErrorStr("Unexpected port numbering "));
            }

            Variable dummyInVar = Variable("dummyIn" + GeneralHelper::to_string(i), dt);

            dummyInVar.getCVarDecl(false, true, false, true, false, false);

            if(dt.isComplex()){
                dummyInVar.getCVarDecl(true, true, false, true, false, false);
            }

            dummyVars.push_back(dummyInVar);
        }

        std::vector<DataType> outputTypes = getOutputTypes();

        //Only declare dummy output vars if output access is not provided
        std::vector<std::string> outputAccess = getOutputAccess();
        if(outputAccess.empty()) {
            for (int i = 0; i < outputPorts.size(); i++) {
                //TODO: assumes no discontinuity in ports and are in sorted order
                DataType dt = outputTypes[i];
                int portNum = outputPorts[i]->getPortNum();
                if (portNum != i) {
                    throw std::runtime_error(ErrorHelpers::genErrorStr("Unexpected port numbering "));
                }

                Variable dummyOutVar = Variable("dummyOut" + GeneralHelper::to_string(i), dt);

                dummyOutVar.getCVarDecl(false, true, false, true, false, false);

                if (dt.isComplex()) {
                    dummyOutVar.getCVarDecl(true, true, false, true, false, false);
                }

                dummyVars.push_back(dummyOutVar);
            }
        }

        for(int i = 0; i<dummyVars.size(); i++){
            cppBody += "\t" + dummyVars[i].getCVarDecl(false, true, false, true, false, false) + ";\n";

            if(dummyVars[i].getDataType().isComplex()){
                cppBody += "\t" + dummyVars[i].getCVarDecl(true, true, false, true, false, false) + ";\n";
            }
        }

        cppBody += "\t" + getGeneratedReset() + "(";

        cppBody += "&(" + fsmRTModelVar.getCVarName(false) + ")"; //Pass the FSM state

        //Pass the dummy vars
        for(int i = 0; i<dummyVars.size(); i++){
            cppBody += ", &" + dummyVars[i].getCVarName(false);

            if(dummyVars[i].getDataType().isComplex()){
                cppBody += ", &" + dummyVars[i].getCVarName(true);
            }
        }

        //If output access was provided, emit that
        if(!outputAccess.empty()){
            for(int i = 0; i<outputTypes.size(); i++){
                cppBody += ", &(" + outputAccess[i] + getReSuffix() + ")";

                if(outputTypes[i].isComplex()){
                    cppBody += ", &(" + outputAccess[i] + getImSuffix() + ")";
                }
            }
        }

        cppBody += ");\n";
        cppBody += "}\n";
    }

    return cppBody;
}


std::string SimulinkCoderFSM::getResetFunctionCall() {
    if(isSimulinkExportReusableFunction()){
        //Need to set the reset function to be the wrapping reset function emitted by this node.  Need the ID to be set
        //first
        std::string rstName = "node"+GeneralHelper::to_string(id)+"_rst";
        setResetName(rstName);
    }

    return BlackBox::getResetFunctionCall();
}

std::string SimulinkCoderFSM::getResetFunctionPrototype() const{
    //Need the ID to be set before doing this
    std::string rstName = "node"+GeneralHelper::to_string(id)+"_rst";
    std::string typeName = "Partition"+(partitionNum >= 0?GeneralHelper::to_string(partitionNum):"N"+GeneralHelper::to_string(-partitionNum))+"_state_t";

    std::string proto = "void " + rstName + "(";

    //Pass pointers to RT model structure and FSM state structure
    proto += typeName + " *" + VITIS_STATE_STRUCT_NAME;

    proto += ")";

    return proto;
}

std::string SimulinkCoderFSM::getGeneratedReset() const {
    return generatedReset;
}

void SimulinkCoderFSM::setGeneratedReset(const std::string &generatedReset) {
    SimulinkCoderFSM::generatedReset = generatedReset;
}

void SimulinkCoderFSM::propagateProperties() {
    BlackBox::propagateProperties(); //Get the datatypes of the output ports

    if(isSimulinkExportReusableFunction()){
        //Need to add output variables to the state array

        std::vector<Variable> stateVars = getStateVars();
        std::vector<DataType> outputTypes = getOutputTypes();

        int outputPortCount = outputPorts.size();
        for(int i = 0; i<outputPortCount; i++){
            std::shared_ptr<OutputPort> outputPort = getOutputPort(i);
            std::string descr = outputPort->getName();
            if(!descr.empty()){
                descr = "_" + descr;
            }
            stateVars.push_back(Variable("blackbox" + GeneralHelper::to_string(outputPort->getParent()->getId()) + "_out" +
                                descr + "_port" + GeneralHelper::to_string(i), outputTypes[i]));
        }

        setStateVars(stateVars);
    }
}
