//
// Created by Christopher Yarp on 2019-01-28.
//

#include "SimulinkCoderFSM.h"

#include "GraphMLTools/GraphMLImporter.h"
#include "GraphMLTools/GraphMLExporter.h"

#include "General/ErrorHelpers.h"

SimulinkCoderFSM::SimulinkCoderFSM() {

}

SimulinkCoderFSM::SimulinkCoderFSM(std::shared_ptr<SubSystem> parent) : BlackBox(parent) {

}

SimulinkCoderFSM::SimulinkCoderFSM(std::shared_ptr<SubSystem> parent, BlackBox *orig) : BlackBox(parent, orig) {

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
    newNode->setResetName(initFctnStr); //It appears that in the Simulink Coder Stateflow Generated C, init accomplishes the same as reset
    //Body and header contents imported in populateProperties method above
    newNode->setStateful(true);
    newNode->setReSuffix(".re"); //In the current simulink settings, complex types are structures
    newNode->setImSuffix(".im");

    //Input ports are external for the current style of Simulink Code Generation
    std::vector<std::shared_ptr<InputPort>> inputPorts = newNode->getInputPorts();
    std::vector<BlackBox::InputMethod> inputMethods;
    std::vector<std::string> inputAccess;
    for(unsigned long i = 0; i<inputPorts.size(); i++){
        inputMethods.push_back(BlackBox::InputMethod::EXT);
        inputAccess.push_back(inputsStructNameStr + "." + newNode->getInputPort(i)->getName()); //Prepend stucture name here
    }
    newNode->setInputMethods(inputMethods);
    newNode->setInputAccess(inputAccess);

    //Output ports are external for the current style of Simulink Code Generation
    std::vector<std::shared_ptr<OutputPort>> outputPorts = newNode->getOutputPorts();
    std::vector<std::string> outputAccess;
    for(unsigned long i = 0; i<outputPorts.size(); i++){
        outputAccess.push_back(outputsStructNameStr + "." + newNode->getOutputPort(i)->getName());
    }
    newNode->setOutputAccess(outputAccess);

    //Assume that no output ports are registered*
    //TODO: refine
    std::vector<int> registeredOutputPorts;
    newNode->setRegisteredOutputPorts(registeredOutputPorts);
    newNode->setReturnMethod(BlackBox::ReturnMethod::EXT);

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

    return parameters;
}

