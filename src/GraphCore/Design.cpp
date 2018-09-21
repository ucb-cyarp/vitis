//
// Created by Christopher Yarp on 6/27/18.
//

#include "Design.h"

#include <algorithm>
#include <iostream>
#include <fstream>
#include <General/GeneralHelper.h>

#include "NodeFactory.h"
#include "MasterNodes/MasterInput.h"
#include "MasterNodes/MasterOutput.h"
#include "MasterNodes/MasterUnconnected.h"

#include "GraphCore/Node.h"
#include "GraphCore/Arc.h"
#include "GraphCore/Variable.h"

//==== Constructors
Design::Design() {
    inputMaster = NodeFactory::createNode<MasterInput>();
    outputMaster = NodeFactory::createNode<MasterOutput>();
    visMaster = NodeFactory::createNode<MasterOutput>();
    unconnectedMaster = NodeFactory::createNode<MasterUnconnected>();
    terminatorMaster = NodeFactory::createNode<MasterOutput>();

    std::vector<std::shared_ptr<Node>> nodes; ///< A vector of nodes in the design
    std::vector<std::shared_ptr<Arc>> arcs; ///< A vector of arcs in the design
}

//==== Getters/Setters ====
std::shared_ptr<MasterInput> Design::getInputMaster() const {
    return inputMaster;
}

void Design::setInputMaster(const std::shared_ptr<MasterInput> inputMaster) {
    Design::inputMaster = inputMaster;
}

std::shared_ptr<MasterOutput> Design::getOutputMaster() const {
    return outputMaster;
}

void Design::setOutputMaster(const std::shared_ptr<MasterOutput> outputMaster) {
    Design::outputMaster = outputMaster;
}

std::shared_ptr<MasterOutput> Design::getVisMaster() const {
    return visMaster;
}

void Design::setVisMaster(const std::shared_ptr<MasterOutput> visMaster) {
    Design::visMaster = visMaster;
}

std::shared_ptr<MasterUnconnected> Design::getUnconnectedMaster() const {
    return unconnectedMaster;
}

void Design::setUnconnectedMaster(const std::shared_ptr<MasterUnconnected> unconnectedMaster) {
    Design::unconnectedMaster = unconnectedMaster;
}

std::shared_ptr<MasterOutput> Design::getTerminatorMaster() const {
    return terminatorMaster;
}

void Design::setTerminatorMaster(const std::shared_ptr<MasterOutput> terminatorMaster) {
    Design::terminatorMaster = terminatorMaster;
}

std::vector<std::shared_ptr<Node>> Design::getTopLevelNodes() const {
    return topLevelNodes;
}

void Design::setTopLevelNodes(const std::vector<std::shared_ptr<Node>> topLevelNodes) {
    Design::topLevelNodes = topLevelNodes;
}

std::vector<std::shared_ptr<Node>> Design::getNodes() const {
    return nodes;
}

void Design::setNodes(const std::vector<std::shared_ptr<Node>> nodes) {
    Design::nodes = nodes;
}

std::vector<std::shared_ptr<Arc>> Design::getArcs() const {
    return arcs;
}

void Design::setArcs(const std::vector<std::shared_ptr<Arc>> arcs) {
    Design::arcs = arcs;
}

void Design::addTopLevelNode(std::shared_ptr<Node> node) {
    topLevelNodes.push_back(node);
}

void Design::addNode(std::shared_ptr<Node> node) {
    nodes.push_back(node);
}

void Design::addArc(std::shared_ptr<Arc> arc) {
    arcs.push_back(arc);
}

void Design::reNumberNodeIDs() {
    unsigned long numNodes = nodes.size();

    for(unsigned long i = 6; i<numNodes; i++){
        nodes[i]->setId(i);  //Start at ID 6 since there are 5 master nodes that start at index 1
    }
}

void Design::reNumberArcIDs() {
    unsigned long numArcs = arcs.size();

    for(unsigned long i = 0; i<numArcs; i++){
        arcs[i]->setId(i);  //Start at ID 0
    }
}

void Design::assignNodeIDs() {
    int maxID = -1;

    unsigned long numNodes = nodes.size();

    for(unsigned long i = 0; i<numNodes; i++){
        maxID = std::max(maxID, nodes[i]->getId());
    }

    int newID = maxID+1;

    if(newID < 6){
        newID = 6; //This is because there are 5 master node starting with index 1
    }

    for(unsigned long i = 0; i<numNodes; i++){
        if(nodes[i]->getId()<0){
            nodes[i]->setId(newID);
            newID++;
        }
    }
}

void Design::assignArcIDs() {
    int maxID = -1;

    unsigned long numArcs = arcs.size();

    for(unsigned long i = 0; i<numArcs; i++){
        maxID = std::max(maxID, arcs[i]->getId());
    }

    int newID = maxID+1;

    for(unsigned long i = 0; i<numArcs; i++){
        if(arcs[i]->getId()<0){
            arcs[i]->setId(newID);
            newID++;
        }
    }
}

std::set<GraphMLParameter> Design::graphMLParameters() {
    //This node has no parameters itself, however, its children may
    std::set<GraphMLParameter> parameters;

    //Add the static entries for nodes
    parameters.insert(GraphMLParameter("instance_name", "string", true));
    parameters.insert(GraphMLParameter("block_node_type", "string", true));
    parameters.insert(GraphMLParameter("block_function", "string", true));
    parameters.insert(GraphMLParameter("block_label", "string", true));

    //Add the static entries for arcs
    parameters.insert(GraphMLParameter("arc_src_port", "int", false));
    parameters.insert(GraphMLParameter("arc_dst_port", "int", false));
    parameters.insert(GraphMLParameter("arc_dst_port_type", "string", false));
    parameters.insert(GraphMLParameter("arc_disp_label", "string", false));
    parameters.insert(GraphMLParameter("arc_datatype", "string", false));
    parameters.insert(GraphMLParameter("arc_complex", "string", false));
    parameters.insert(GraphMLParameter("arc_width", "string", false));

    std::set<GraphMLParameter> inputParameters = inputMaster->graphMLParameters();
    parameters.insert(inputParameters.begin(), inputParameters.end());

    std::set<GraphMLParameter> outputParameters = outputMaster->graphMLParameters();
    parameters.insert(outputParameters.begin(), outputParameters.end());

    std::set<GraphMLParameter> visParameters = visMaster->graphMLParameters();
    parameters.insert(visParameters.begin(), visParameters.end());

    std::set<GraphMLParameter> unconnectedParameters = unconnectedMaster->graphMLParameters();
    parameters.insert(unconnectedParameters.begin(), unconnectedParameters.end());

    std::set<GraphMLParameter> terminatorParameters = terminatorMaster->graphMLParameters();
    parameters.insert(terminatorParameters.begin(), terminatorParameters.end());

    unsigned long numTopLvlNodes = topLevelNodes.size();
    for(unsigned long i = 0; i < numTopLvlNodes; i++){
        //Note that the call to get graphML node parameters works recursively when called on subsystems.
        std::set<GraphMLParameter> nodeParameters = topLevelNodes[i]->graphMLParameters();
        parameters.insert(nodeParameters.begin(), nodeParameters.end());
    }

    return parameters;
}

void Design::emitGraphML(xercesc::DOMDocument *doc, xercesc::DOMElement *root) {
    //--Emit GraphML Parameters---
    std::set<GraphMLParameter> parameterSet = graphMLParameters();

    for(auto parameter = parameterSet.begin(); parameter != parameterSet.end(); parameter++){
        xercesc::DOMElement* keyElement = GraphMLHelper::createNode(doc, "key");
        GraphMLHelper::setAttribute(keyElement, "id", parameter->getKey());
        std::string forStr = parameter->isNodeParam() ? "node" : "edge";
        GraphMLHelper::setAttribute(keyElement, "for", forStr);
        GraphMLHelper::setAttribute(keyElement, "attr.name", parameter->getKey());
        GraphMLHelper::setAttribute(keyElement, "attr.type", parameter->getType());

        std::string defaultVal;

        if(parameter->getType() == "string"){
            defaultVal = "\"\"";
        } else if(parameter->getType() == "int"){
            defaultVal = "0";
        } else if(parameter->getType() == "double"){
            defaultVal = "0.0";
        } else{
            throw std::runtime_error("Unexpected attribute type");
        }

        xercesc::DOMElement* defaultNode = GraphMLHelper::createEncapulatedTextNode(doc, "default", defaultVal);
        keyElement->appendChild(defaultNode);

        //Add to document
        root->appendChild(keyElement);
    }

    //----Create Top Level Graph Node----
    xercesc::DOMElement* graphElement = GraphMLHelper::createNode(doc, "graph");
    GraphMLHelper::setAttribute(graphElement, "id", "G");
    GraphMLHelper::setAttribute(graphElement, "edgedefault", "directed");

    //Add to doc
    root->appendChild(graphElement);

    //----Emit Each Node----
    //Master Nodes
    inputMaster->emitGraphML(doc, graphElement);
    outputMaster->emitGraphML(doc, graphElement);
    visMaster->emitGraphML(doc, graphElement);
    unconnectedMaster->emitGraphML(doc, graphElement);
    terminatorMaster->emitGraphML(doc, graphElement);

    //Top Level Nodes (will recursivly emit children)
    for(auto node=topLevelNodes.begin(); node!=topLevelNodes.end(); node++){
        (*node)->emitGraphML(doc, graphElement);
    }

    //Arcs
    for(auto arc=arcs.begin(); arc!=arcs.end(); arc++){
        (*arc)->emitGraphML(doc, graphElement);
    }
}

bool Design::canExpand() {
    bool canExpand = false;

    for(auto node = nodes.begin(); node != nodes.end(); node++){
        canExpand |= (*node)->canExpand();
    }

    return canExpand;
}

bool Design::expand() {
    bool expanded = false;

    std::vector<std::shared_ptr<Node>> newNodes;
    std::vector<std::shared_ptr<Node>> deletedNodes;
    std::vector<std::shared_ptr<Arc>> newArcs;
    std::vector<std::shared_ptr<Arc>> deletedArcs;

    for(auto node = nodes.begin(); node != nodes.end(); node++){
        expanded |= (*node)->expand(newNodes, deletedNodes, newArcs, deletedArcs);
    }

    //Add new nodes first, then delete old ones
    for(auto node = newNodes.begin(); node != newNodes.end(); node++){
        nodes.push_back(*node);
    }

    for(auto node = deletedNodes.begin(); node != deletedNodes.end(); node++){
        //Erase pattern using iterators from https://en.cppreference.com/w/cpp/container/vector/erase
        for(auto candidate = nodes.begin(); candidate != nodes.end(); ){//Will handle iteration in body since erase returns next iterator pos
            if((*candidate) == (*node)){
                candidate = nodes.erase(candidate);
            }else{
                candidate++;
            }
        }
    }

    //Add new arcs first, then delete old ones
    for(auto arc = newArcs.begin(); arc != newArcs.end(); arc++){
        arcs.push_back(*arc);
    }

    for(auto arc = deletedArcs.begin(); arc != deletedArcs.end(); arc++){
        //Erase pattern using iterators from https://en.cppreference.com/w/cpp/container/vector/erase
        for(auto candidate = arcs.begin(); candidate != arcs.end(); ){//Will handle iteration in body since erase returns next iterator pos
            if((*candidate) == (*arc)){
                candidate = arcs.erase(candidate);
            }else{
                candidate++;
            }
        }
    }

    return expanded;
}

bool Design::expandToPrimitive() {
    bool expanded = false;

    bool done = false;
    while(!done){
        bool localExpanded = expand();
        expanded |= localExpanded;
        done = !localExpanded;
    }

    return expanded;
}

std::shared_ptr<Node> Design::getNodeByNamePath(std::vector<std::string> namePath) {
    unsigned long pathLen = namePath.size();

    if(pathLen < 1){
        return nullptr;
    }

    //first, search top level nodes
    unsigned long numTopLevel = topLevelNodes.size();
    std::shared_ptr<Node> cursor = nullptr;
    for(unsigned long i = 0; i<numTopLevel; i++){
        if(topLevelNodes[i]->getName() == namePath[0]){
            cursor = topLevelNodes[i];
            break;
        }
    }

    if(cursor == nullptr){
        return nullptr;
    }

    for(unsigned long i = 1; i<pathLen; i++){
        std::set<std::shared_ptr<Node>> children = std::dynamic_pointer_cast<SubSystem>(cursor)->getChildren();

        bool found = false;
        for(auto nodeIter = children.begin(); nodeIter!=children.end(); nodeIter++){
            if((*nodeIter)->getName() == namePath[i]){
                cursor = (*nodeIter);
                found = true;
                break;
            }
        }

        if(!found){
            return nullptr;
        }

    }

    return cursor;
}

std::vector<Variable> Design::getCInputVariables() {
    unsigned long numPorts = inputMaster->getOutputPorts().size();

    std::vector<Variable> inputVars;

    //TODO: Assuming port numbers do not have a discontinuity.  Validate this assumption.
    if(numPorts>0){
        std::shared_ptr<OutputPort> input = inputMaster->getOutputPort(0); //output of input node

        DataType portDataType = input->getDataType();

        Variable var = Variable(inputMaster->getCInputName(0), portDataType);
        inputVars.push_back(var);
    }

    for(unsigned long i = 1; i<numPorts; i++){
        std::shared_ptr<OutputPort> input = inputMaster->getOutputPort(i); //output of input node

        DataType portDataType = input->getDataType();

        Variable var = Variable(inputMaster->getCInputName(i), portDataType);
        inputVars.push_back(var);
    }

    return inputVars;
}

std::string Design::getCFunctionArgPrototype() {
    std::string prototype = "";

    std::vector<Variable> inputVars = getCInputVariables();
    unsigned long numInputVars = inputVars.size();

    //TODO: Assuming port numbers do not have a discontinuity.  Validate this assumption.
    if(numInputVars>0){
        Variable var = inputVars[0];

        prototype += "const " + var.getCVarDecl(false);

        //Check if complex
        if(var.getDataType().isComplex()){
            prototype += ", const " + var.getCVarDecl(true, true, false, true);
        }
    }

    for(unsigned long i = 1; i<numInputVars; i++){
        Variable var = inputVars[i];

        prototype += ", const " + var.getCVarDecl(false);

        //Check if complex
        if(var.getDataType().isComplex()){
            prototype += ", const " + var.getCVarDecl(true);
        }
    }

    //Add output
    if(numInputVars>0){
        prototype += ", ";
    }
    prototype += "OutputType *output";

    //Add output count
    prototype += ", unsigned long *outputCount";

    return prototype;
}

std::string Design::getCInputPortStructDefn(){
    std::string prototype = "typedef struct {\n";

    std::vector<Variable> inputVars = getCInputVariables();
    unsigned long numInputVars = inputVars.size();

    for(unsigned long i = 0; i<numInputVars; i++){
        Variable var = inputVars[i];

        prototype += "\t" + var.getCVarDecl(false, false, false, true) + ";\n";

        //Check if complex
        if(var.getDataType().isComplex()){
            prototype += "\t" + var.getCVarDecl(true, false, false, true) + ";\n";
        }
    }

    prototype += "} InputType;";

    return prototype;
}

std::string Design::getCOutputStructDefn() {
    //Emit struct header
    std::string str = "typedef struct {";

    unsigned long numPorts = outputMaster->getInputPorts().size();

    for(unsigned long i = 0; i<numPorts; i++){
        std::shared_ptr<InputPort> output = outputMaster->getInputPort(i); //input of output node

        DataType portDataType = output->getDataType();

        Variable var = Variable(outputMaster->getCOutputName(i), portDataType);

        str += "\n\t" + var.getCVarDecl(false, false, false, true) + ";";

        //Check if complex
        if(portDataType.isComplex()){
            str += "\n\t" + var.getCVarDecl(true, false, false, true) + ";";
        }
    }

    str += "\n} OutputType;";

    return str;
}

void Design::emitSingleThreadedC(std::string path, std::string fileName, std::string designName) {
    //Get the OutputType struct defn
    std::string outputTypeDefn = getCOutputStructDefn();

    std::string fctnProtoArgs = getCFunctionArgPrototype();

    std::string fctnProto = "void " + designName + "(" + fctnProtoArgs + ")";

    //#### Emit .h file ####
    std::ofstream headerFile;
    headerFile.open(path+"/"+fileName+".h", std::ofstream::out | std::ofstream::trunc);

    std::string fileNameUpper =  GeneralHelper::toUpper(fileName);
    headerFile << "#ifndef " << fileNameUpper << "_H" << std::endl;
    headerFile << "#define " << fileNameUpper << "_H" << std::endl;
    headerFile << "#include <stdint.h>" << std::endl;
    headerFile << "#include <stdbool.h>" << std::endl;
    headerFile << "#include <math.h>" << std::endl;
    //headerFile << "#include <thread.h>" << std::endl;
    headerFile << std::endl;

    //Output the Function Definition
    headerFile << outputTypeDefn << std::endl;
    headerFile << std::endl;
    headerFile << fctnProto << ";" << std::endl;

    //Output the reset function definition
    headerFile << "void " << designName << "_reset();" << std::endl;
    headerFile << std::endl;

    //Find nodes with state & global decls
    std::vector<std::shared_ptr<Node>> nodesWithState;
    std::vector<std::shared_ptr<Node>> nodesWithGlobalDecl;
    unsigned long numNodes = nodes.size(); //Iterate through all un-pruned nodes in the design since this is a single threaded emit

    for(unsigned long i = 0; i<numNodes; i++) {
        if (nodes[i]->hasState()) {
            nodesWithState.push_back(nodes[i]);
        }
        if (nodes[i]->hasGlobalDecl()) {
            nodesWithGlobalDecl.push_back(nodes[i]);
        }
    }

    headerFile << "//==== State Variable Definitions ====" << std::endl;
    //We also need to declare the state variables here as extern;

    //Emit Definition
    unsigned long nodesWithStateCount = nodesWithState.size();
    for(unsigned long i = 0; i<nodesWithStateCount; i++){
        std::vector<Variable> stateVars = nodesWithState[i]->getCStateVars();
        //Emit State Vars
        unsigned long numStateVars = stateVars.size();
        for(unsigned long j = 0; j<numStateVars; j++){
            //cFile << "_Thread_local static " << stateVars[j].getCVarDecl(false, true, true) << ";" << std::endl;
            headerFile << "extern " << stateVars[j].getCVarDecl(false, true, false, true) << ";" << std::endl;

            if(stateVars[j].getDataType().isComplex()){
                headerFile << "extern " << stateVars[j].getCVarDecl(true, true, false, true) << ";" << std::endl;
            }
        }
    }

    headerFile << std::endl;

    headerFile << "#endif" << std::endl;

    headerFile.close();

    //#### Emit .c file ####
    std::ofstream cFile;
    cFile.open(path+"/"+fileName+".c", std::ofstream::out | std::ofstream::trunc);

    cFile << "#include \"" << fileName << ".h" << "\"" << std::endl;
    cFile << std::endl;

    //Find nodes with state & Emit state variable declarations
    cFile << "//==== Init State Vars ====" << std::endl;
    for(unsigned long i = 0; i<nodesWithStateCount; i++){
        std::vector<Variable> stateVars = nodesWithState[i]->getCStateVars();
        //Emit State Vars
        unsigned long numStateVars = stateVars.size();
        for(unsigned long j = 0; j<numStateVars; j++){
            //cFile << "_Thread_local static " << stateVars[j].getCVarDecl(false, true, true) << ";" << std::endl;
            cFile << stateVars[j].getCVarDecl(false, true, true, true) << ";" << std::endl;

            if(stateVars[j].getDataType().isComplex()){
                cFile << stateVars[j].getCVarDecl(true, true, true, true) << ";" << std::endl;
            }
        }
    }

    cFile << std::endl;

    cFile << "//==== Global Declarations ====" << std::endl;
    unsigned long nodesWithGlobalDeclCount = nodesWithGlobalDecl.size();
    for(unsigned long i = 0; i<nodesWithGlobalDeclCount; i++){
        cFile << nodesWithGlobalDecl[i]->getGlobalDecl() << std::endl;
    }

    cFile << std::endl;

    cFile << "//==== Functions ====" << std::endl;

    cFile << fctnProto << "{" << std::endl;

    //Emit compute next states
    cFile << std::endl << "//==== Compute Next States ====" << std::endl;
    unsigned long numNodesWithState = nodesWithState.size();
    for(unsigned long i = 0; i<numNodesWithState; i++){
        std::vector<std::string> nextStateExprs;
        nodesWithState[i]->emitCExprNextState(nextStateExprs);
        cFile << std::endl << "//---- Compute Next States " << nodesWithState[i]->getFullyQualifiedName() <<" ----" << std::endl;

        unsigned long numNextStateExprs = nextStateExprs.size();
        for(unsigned long j = 0; j<numNextStateExprs; j++){
            cFile << nextStateExprs[j] << std::endl;
        }
    }

    //Assign each of the outputs (and emit any expressions that preceed it
    cFile << std::endl << "//==== Compute Outputs ====" << std::endl;
    unsigned long numOutputs = outputMaster->getInputPorts().size();
    for(unsigned long i = 0; i<numOutputs; i++){
        std::shared_ptr<InputPort> output = outputMaster->getInputPort(i);
        cFile << std::endl << "//---- Compute Output " << i << ": " << output->getName() <<" ----" << std::endl;

        //Get the arc connected to the output
        std::shared_ptr<Arc> outputArc = *(output->getArcs().begin());

        DataType outputDataType = outputArc->getDataType();

        std::shared_ptr<OutputPort> srcOutputPort = outputArc->getSrcPort();
        int srcNodeOutputPortNum = srcOutputPort->getPortNum();

        std::shared_ptr<Node> srcNode = srcOutputPort->getParent();

        cFile << "//-- Compute Real Component --" << std::endl;
        std::vector<std::string> cStatements_re;
        std::string expr_re = srcNode->emitC(cStatements_re, srcNodeOutputPortNum, false);
        //emit the expressions
        unsigned long numStatements_re = cStatements_re.size();
        for(unsigned long j = 0; j<numStatements_re; j++){
            cFile << cStatements_re[j] << std::endl;
        }

        //emit the assignment
        Variable outputVar = Variable(outputMaster->getCOutputName(i), outputDataType);
        cFile << "output[0]." << outputVar.getCVarName(false) << " = " << expr_re << ";" << std::endl;

        //Emit Imag if Datatype is complex
        if(outputDataType.isComplex()){
            cFile << std::endl << "//-- Compute Imag Component --" << std::endl;
            std::vector<std::string> cStatements_im;
            std::string expr_im = srcNode->emitC(cStatements_im, srcNodeOutputPortNum, true);
            //emit the expressions
            unsigned long numStatements_im = cStatements_im.size();
            for(unsigned long j = 0; j<numStatements_im; j++){
                cFile << cStatements_im[j] << std::endl;
            }

            //emit the assignment
            cFile << "output[0]." << outputVar.getCVarName(true) << " = " << expr_im << ";" << std::endl;
        }
    }

    //Emit state variable updates
    cFile << std::endl << "//==== Update State Vars ====" << std::endl;
    for(unsigned long i = 0; i<numNodesWithState; i++){
        std::vector<std::string> stateUpdateExprs;
        nodesWithState[i]->emitCStateUpdate(stateUpdateExprs);

        unsigned long numStateUpdateExprs = stateUpdateExprs.size();
        for(unsigned long j = 0; j<numStateUpdateExprs; j++){
            cFile << stateUpdateExprs[j] << std::endl;
        }
    }

    cFile << std::endl << "//==== Return Number Outputs Generated ====" << std::endl;
    cFile << "*outputCount = 1;" << std::endl;

    cFile << "}" << std::endl;

    cFile << std::endl;

    cFile << "void " << designName << "_reset(){" << std::endl;
    cFile << "//==== Reset State Vars ====" << std::endl;
    for(unsigned long i = 0; i<nodesWithStateCount; i++){
        std::vector<Variable> stateVars = nodesWithState[i]->getCStateVars();
        //Emit State Vars
        unsigned long numStateVars = stateVars.size();
        for(unsigned long j = 0; j<numStateVars; j++){
            //cFile << "_Thread_local static " << stateVars[j].getCVarDecl(false, true, true) << ";" << std::endl;
            DataType initDataType = stateVars[j].getDataType();
            std::vector<NumericValue> initVals = stateVars[j].getInitValue();
            unsigned long initValsLen = initVals.size();
            if(initValsLen == 1){
                cFile << stateVars[j].getCVarName(false) << " = " << initVals[0].toStringComponent(false, initDataType) << ";" << std::endl;
            }else{
                for(unsigned long k = 0; k < initValsLen; k++){
                    cFile << stateVars[j].getCVarName(false) << "[" << k << "] = " << initVals[k].toStringComponent(false, initDataType) << ";" << std::endl;
                }
            }

            if(stateVars[j].getDataType().isComplex()){
                if(initValsLen == 1){
                    cFile << stateVars[j].getCVarName(true) << " = " << initVals[0].toStringComponent(true, initDataType) << ";" << std::endl;
                }else{
                    for(unsigned long k = 0; k < initValsLen; k++){
                        cFile << stateVars[j].getCVarName(true) << "[" << k << "] = " << initVals[k].toStringComponent(true, initDataType) << ";" << std::endl;
                    }
                }
            }
        }
    }

    cFile << "}" << std::endl;

    cFile.close();
}

void Design::emitSingleThreadedCBenchmarkingDrivers(std::string path, std::string fileName, std::string designName) {
    emitSingleThreadedCBenchmarkingDriverConst(path, fileName, designName);
    emitSingleThreadedCBenchmarkingDriverMem(path, fileName, designName);
}

void
Design::emitSingleThreadedCBenchmarkingDriverConst(std::string path, std::string fileName, std::string designName) {
    std::string fileNameUpper =  GeneralHelper::toUpper(fileName);

    //#### Emit Kernel File & Header ####
    std::ofstream benchKernel;
    benchKernel.open(path+"/"+fileName+"_benchmark_kernel.cpp", std::ofstream::out | std::ofstream::trunc);

    //Generate loop
    std::vector<Variable> inputVars = getCInputVariables();
    unsigned long numInputVars = inputVars.size();

    std::vector<NumericValue> defaultArgs;
    for(unsigned long i = 0; i<numInputVars; i++){
        DataType type = inputVars[i].getDataType();
        NumericValue val;
        val.setRealInt(1);
        val.setImagInt(1);
        val.setComplex(type.isComplex());
        val.setFractional(false);

        defaultArgs.push_back(val);
    }

    std::string fctnCall = designName + "(";
    if(numInputVars>0){
        fctnCall += defaultArgs[0].toStringComponent(false, inputVars[0].getDataType());
        if(inputVars[0].getDataType().isComplex()){
            fctnCall += ", " + defaultArgs[0].toStringComponent(true, inputVars[0].getDataType());
        }
    }
    for(unsigned long i = 1; i<numInputVars; i++){
        fctnCall += ", " + defaultArgs[i].toStringComponent(false, inputVars[i].getDataType());
        if(inputVars[i].getDataType().isComplex()){
            fctnCall += ", " + defaultArgs[i].toStringComponent(true, inputVars[i].getDataType());
        }
    }
    if(numInputVars>0){
        fctnCall += ", ";
    }
    fctnCall += "&output, &outputCount)";

    benchKernel << "#include \"" << fileName << ".h" << "\"" << std::endl;
    benchKernel << "#include \"intrin_bench_default_defines.h\"" << std::endl;
    benchKernel << "void bench_"+fileName+"()\n"
                   "{\n"
                   "\tOutputType output;\n"
                   "\tunsigned long outputCount;\n"
                   "\tfor(int i = 0; i<STIM_LEN; i++)\n"
                   "\t{\n"
                   "\t\t" + fctnCall + ";\n"
                   "\t}\n"
                   "}" << std::endl;
    benchKernel.close();

    std::ofstream benchKernelHeader;
    benchKernelHeader.open(path+"/"+fileName+"_benchmark_kernel.h", std::ofstream::out | std::ofstream::trunc);

    benchKernelHeader << "#ifndef " << fileNameUpper << "_BENCHMARK_KERNEL_H" << std::endl;
    benchKernelHeader << "#define " << fileNameUpper << "_BENCHMARK_KERNEL_H" << std::endl;
    benchKernelHeader << "void bench_"+fileName+"();" << std::endl;
    benchKernelHeader << "#endif" << std::endl;
    benchKernelHeader.close();

    //#### Emit Driver File ####
    std::ofstream benchDriver;
    benchDriver.open(path+"/"+fileName+"_benchmark_driver.cpp", std::ofstream::out | std::ofstream::trunc);

    benchDriver << "#include <map>" << std::endl;
    benchDriver << "#include <string>" << std::endl;
    benchDriver << "#include \"intrin_bench_default_defines.h\"" << std::endl;
    benchDriver << "#include \"benchmark_throughput_test.h\"" << std::endl;
    benchDriver << "#include \"kernel_runner.h\"" << std::endl;
    benchDriver << "#include \"" + fileName + "_benchmark_kernel.h\"" << std::endl;

    //If performing a load, exe, store benchmark, define an input structure
    //benchDriver << getCInputPortStructDefn() << std::endl;

    //Driver will define a zero arg kernel that sets reasonable inputs and repeatedly runs the function.
    //The function will be compiled in a seperate object file and should not be in-lined (potentially resulting in eronious
    //optimizations for the purpose of benchmarking since we are feeding dummy data in).  This should be the case so long as
    //the compiler flag -flto is not used durring compile and linkign (https://stackoverflow.com/questions/35922966/lto-with-llvm-and-cmake)
    //(https://llvm.org/docs/LinkTimeOptimization.html), (https://clang.llvm.org/docs/CommandGuide/clang.html),
    //(https://gcc.gnu.org/wiki/LinkTimeOptimization), (https://gcc.gnu.org/onlinedocs/gccint/LTO-Overview.html).

    //Emit name, file, and units string
    benchDriver << "std::string getBenchSuiteName(){\n\treturn \"Generated System: " + designName + "\";\n}" << std::endl;
    benchDriver << "std::string getReportFileName(){\n\treturn \"" + fileName + "_benchmarking_report\";\n}" << std::endl;
    benchDriver << "std::string getReportUnitsName(){\n\treturn \"STIM_LEN: \" + std::to_string(STIM_LEN) + \" (Samples/Vector/Trial), TRIALS: \" + std::to_string(TRIALS);\n}" << std::endl;

    //Emit Benchmark Report Selection
    benchDriver << "void getBenchmarksToReport(std::vector<std::string> &kernels, std::vector<std::string> &vec_ext){\n"
                   "\tkernels.push_back(\"" + designName + "\");\tvec_ext.push_back(\"Single Threaded\");\n}" << std::endl;

    //Emit Benchmark Type Report Selection
    std::string typeStr = "";
    if(numInputVars > 0){
        typeStr = inputVars[0].getDataType().toString(DataType::StringStyle::C);
        typeStr += inputVars[0].getDataType().isComplex() ? " (c)" : " (r)";
    }

    for(unsigned long i = 1; i<numInputVars; i++){
        typeStr += ", " + inputVars[i].getDataType().toString(DataType::StringStyle::C);
        typeStr += inputVars[i].getDataType().isComplex() ? " (c)" : " (r)";
    }

    benchDriver << "std::vector<std::string> getVarientsToReport(){\n"
                   "\tstd::vector<std::string> types;\n"
                   "\ttypes.push_back(\"" + typeStr + "\");\n"
                                                      "\treturn types;\n}" << std::endl;

    //Generate call to loop
    benchDriver << "std::map<std::string, std::map<std::string, Results*>*> runBenchSuite(PCM* pcm, int* cpu_num_int){\n"
                   "\tstd::map<std::string, std::map<std::string, Results*>*> kernel_results;\n"
                   "\n"
                   "\tstd::map<std::string, Results*>* type_result = new std::map<std::string, Results*>;\n"
                   "\tResults* result = zero_arg_kernel(pcm, &bench_" + fileName + ", *cpu_num_int, \"===== Generated System: " + designName + " =====\");\n"
                   "\t(*type_result)[\"" + typeStr + "\"] = result;\n"
                   "\tkernel_results[\"" + designName + "\"] = type_result;\n"
                   "\tprintf(\"\\n\");\n"
                   "\treturn kernel_results;\n}" << std::endl;

    benchDriver << "void initInput(void* ptr, unsigned long index){}" << std::endl;

    benchDriver.close();

    //#### Emit Makefiles ####
    std::string makefileTop = "BUILD_DIR=build\n"
                              "DEPENDS_DIR=./depends\n"
                              "COMMON_DIR=./common\n"
                              "SRC_DIR=./intrin\n"
                              "LIB_DIR=.\n"
                              "\n"
                              "UNAME:=$(shell uname)\n"
                              "\n"
                              "#Compiler Parameters\n"
                              "CXX=g++\n"
                              "#Main Benchmark file is not optomized to avoid timing code being re-organized\n"
                              "CFLAGS = -O0 -c -g -std=c++11 -march=native -masm=att\n"
                              "#Generated system should be allowed to optomize - reintepret as c++ file\n"
                              "SYSTEM_CFLAGS = -O3 -c -g -x c++ -std=c++11 -march=native -masm=att\n"
                              "#Most kernels are allowed to be optomized.  Most assembly kernels use asm 'volitile' to force execution\n"
                              "KERNEL_CFLAGS = -O3 -c -g -std=c++11 -march=native -masm=att\n"
                              "#For kernels that should not be optimized, the following is used\n"
                              "KERNEL_NO_OPT_CFLAGS = -O0 -c -g -std=c++11 -march=native -masm=att\n"
                              "INC=-I $(DEPENDS_DIR)/pcm -I $(COMMON_DIR) -I $(SRC_DIR)\n"
                              "LIB_DIRS=-L $(DEPENDS_DIR)/pcm -L $(COMMON_DIR)\n"
                              "ifeq ($(UNAME), Darwin)\n"
                              "LIB=-pthread -lpcmHelper -lPCM -lPcmMsr\n"
                              "LIB_DIRS+= -L $(DEPENDS_DIR)/pcm/MacMSRDriver/build/Release\n"
                              "else\n"
                              "LIB=-pthread -lrt -lpcmHelper -lPCM\n"
                              "endif\n";
    std::string noPCMDefine = "DEFINES = \n";
    std::string PCMDefine = "DEFINES = -DUSE_PCM=0\n";

    std::string makefileMid = "#Need an additional include directory if on MacOS.\n"
                              "#Using the technique in pcm makefile to detect MacOS\n"
                              "ifeq ($(UNAME), Darwin)\n"
                              "INC+= -I $(DEPENDS_DIR)/pcm/MacMSRDriver\n"
                              "endif\n"
                              "\n";

    std::string makefileFiles =  "MAIN_FILE = benchmark_throughput_test.cpp\n"
                                 "LIB_SRCS = " + fileName + "_benchmark_driver.cpp\n"
                                 "SYSTEM_SRC = " + fileName + ".c\n"
                                 "KERNEL_SRCS = " + fileName + "_benchmark_kernel.cpp\n"
                                 "KERNEL_NO_OPT_SRCS = \n";

    std::string makefileBottom = "\n"
                                 "SRCS=$(MAIN_FILE)\n"
                                 "OBJS=$(patsubst %.cpp,$(BUILD_DIR)/%.o,$(SRCS))\n"
                                 "LIB_OBJS=$(patsubst %.cpp,$(BUILD_DIR)/%.o,$(LIB_SRCS))\n"
                                 "SYSTEM_OBJS=$(patsubst %.c,$(BUILD_DIR)/%.o,$(SYSTEM_SRC))\n"
                                 "KERNEL_OBJS=$(patsubst %.cpp,$(BUILD_DIR)/%.o,$(KERNEL_SRCS))\n"
                                 "KERNEL_NO_OPT_OBJS=$(patsubst %.cpp,$(BUILD_DIR)/%.o,$(KERNEL_NO_OPT_SRCS))\n"
                                 "\n"
                                 "#Production\n"
                                 "all: benchmark_" + fileName +"_const\n"
                                 "\n"
                                 "benchmark_" + fileName + "_const: $(OBJS) $(SYSTEM_OBJS) $(LIB_OBJS) $(KERNEL_OBJS) $(KERNEL_NO_OPT_OBJS) $(COMMON_DIR)/libpcmHelper.a $(DEPENDS_DIR)/pcm/libPCM.a \n"
                                 "\t$(CXX) $(INC) $(LIB_DIRS) -o benchmark_" + fileName + "_const $(OBJS) $(SYSTEM_OBJS) $(LIB_OBJS) $(KERNEL_OBJS) $(KERNEL_NO_OPT_OBJS) $(LIB)\n"
                                 "\n"
                                 "$(KERNEL_NO_OPT_OBJS): $(BUILD_DIR)/%.o : $(SRC_DIR)/%.cpp | $(BUILD_DIR)/ $(DEPENDS_DIR)/pcm/Makefile\n"
                                 "\t$(CXX) $(KERNEL_NO_OPT_CFLAGS) $(INC) $(DEFINES) -o $@ $<\n"
                                 "\n"
                                 "$(SYSTEM_OBJS): $(BUILD_DIR)/%.o : $(LIB_DIR)/%.c | $(BUILD_DIR)/ $(DEPENDS_DIR)/pcm/Makefile\n"
                                 "\t$(CXX) $(SYSTEM_CFLAGS) $(INC) $(DEFINES) -o $@ $<\n"
                                 "\n"
                                 "$(KERNEL_OBJS): $(BUILD_DIR)/%.o : $(LIB_DIR)/%.cpp | $(BUILD_DIR)/ $(DEPENDS_DIR)/pcm/Makefile\n"
                                 "\t$(CXX) $(KERNEL_CFLAGS) $(INC) $(DEFINES) -o $@ $<\n"
                                 "\n"
                                 "$(LIB_OBJS): $(BUILD_DIR)/%.o : $(LIB_DIR)/%.cpp | $(BUILD_DIR)/ $(DEPENDS_DIR)/pcm/Makefile\n"
                                 "\t$(CXX) $(CFLAGS) $(INC) $(DEFINES) -o $@ $<\n"
                                 "\n"
                                 "$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp | $(BUILD_DIR)/ $(DEPENDS_DIR)/pcm/Makefile\n"
                                 "\t$(CXX) $(CFLAGS) $(INC) $(DEFINES) -o $@ $<\n"
                                 "\n"
                                 "$(BUILD_DIR)/:\n"
                                 "\tmkdir -p $@\n"
                                 "\n"
                                 "$(DEPENDS_DIR)/pcm/Makefile:\n"
                                 "\tgit submodule update --init --recursive $(DEPENDS_DIR)/pcm\n"
                                 "\n"
                                 "$(DEPENDS_DIR)/pcm/libPCM.a: $(DEPENDS_DIR)/pcm/Makefile\n"
                                 "\tcd $(DEPENDS_DIR)/pcm; make libPCM.a\n"
                                 "\n"
                                 "$(COMMON_DIR)/libpcmHelper.a: $(DEPENDS_DIR)/pcm/Makefile $(DEPENDS_DIR)/pcm/libPCM.a\n"
                                 "\tcd $(COMMON_DIR); make\n"
                                 "\t\n"
                                 "clean:\n"
                                 "\trm -f benchmark_" + fileName + "\n"
                                 "\trm -rf build/*\n"
                                 "\n"
                                 ".PHONY: clean\n";

    std::ofstream makefile;
    makefile.open(path+"/Makefile_" + fileName + "_const", std::ofstream::out | std::ofstream::trunc);
    std::ofstream makefileNoPCM;
    makefileNoPCM.open(path+"/Makefile_noPCM_" + fileName + "_const", std::ofstream::out | std::ofstream::trunc);


    makefile << makefileTop << noPCMDefine << makefileMid << makefileFiles << makefileBottom;
    makefileNoPCM << makefileTop << PCMDefine << makefileMid << makefileFiles << makefileBottom;

    makefile.close();
    makefileNoPCM.close();
}

void Design::emitSingleThreadedCBenchmarkingDriverMem(std::string path, std::string fileName, std::string designName) {
    std::string fileNameUpper =  GeneralHelper::toUpper(fileName);

    //#### Emit Kernel File & Header (Memory)
    std::ofstream benchKernelMem;
    benchKernelMem.open(path+"/"+fileName+"_benchmark_kernel_mem.cpp", std::ofstream::out | std::ofstream::trunc);

    //Generate loop
    std::vector<Variable> inputVars = getCInputVariables();
    unsigned long numInputVars = inputVars.size();

    std::string fctnCall = designName + "(";
    if(numInputVars>0){
        fctnCall += "in[i]." + inputVars[0].getCVarName(false);
        if(inputVars[0].getDataType().isComplex()){
            fctnCall += ", in[i]." + inputVars[0].getCVarName(true);
        }
    }
    for(unsigned long i = 1; i<numInputVars; i++){
        fctnCall += ", in[i]." + inputVars[i].getCVarName(false);
        if(inputVars[i].getDataType().isComplex()){
            fctnCall += ", in[i]." + inputVars[i].getCVarName(true);
        }
    }
    if(numInputVars>0){
        fctnCall += ", ";
    }
    fctnCall += "out+i, &outputCount)";

    benchKernelMem << "#include \"" << fileName << ".c" << "\" //Entire function is included to allow inlining" << std::endl;
    benchKernelMem << "#include \"" << fileName << "_benchmark_kernel_mem.h" << "\"" << std::endl;
    benchKernelMem << "#include \"intrin_bench_default_defines.h\"" << std::endl;
    benchKernelMem << "void bench_"+fileName+"_mem(InputType* in, OutputType* out)\n"
                      "{\n"
                      "\tunsigned long outputCount;\n"
                      "\tfor(int i = 0; i<STIM_LEN; i++)\n"
                      "\t{\n"
                      "\t\t" + fctnCall + ";\n"
                      "\t}\n"
                      "}" << std::endl;
    benchKernelMem.close();

    std::ofstream benchKernelHeaderMem;
    benchKernelHeaderMem.open(path+"/"+fileName+"_benchmark_kernel_mem.h", std::ofstream::out | std::ofstream::trunc);

    benchKernelHeaderMem << "#ifndef " << fileNameUpper << "_BENCHMARK_KERNEL_H" << std::endl;
    benchKernelHeaderMem << "#define " << fileNameUpper << "_BENCHMARK_KERNEL_H" << std::endl;
    benchKernelHeaderMem << "#include \"" << fileName << ".h\"" << std::endl;
    //Emit a structure for the input (for array allocation)
    benchKernelHeaderMem << getCInputPortStructDefn() << std::endl;
    benchKernelHeaderMem << "void bench_"+fileName+"_mem(InputType* in, OutputType* out);" << std::endl;
    benchKernelHeaderMem << "#endif" << std::endl;
    benchKernelHeaderMem.close();

    //#### Emit Driver File (Mem) ####
    std::ofstream benchMemDriver;
    benchMemDriver.open(path+"/"+fileName+"_benchmark_driver_mem.cpp", std::ofstream::out | std::ofstream::trunc);

    benchMemDriver << "#include <map>" << std::endl;
    benchMemDriver << "#include <string>" << std::endl;
    benchMemDriver << "#include \"intrin_bench_default_defines.h\"" << std::endl;
    benchMemDriver << "#include \"benchmark_throughput_test.h\"" << std::endl;
    benchMemDriver << "#include \"kernel_runner.h\"" << std::endl;
    benchMemDriver << "#include \"" + fileName + "_benchmark_kernel_mem.h\"" << std::endl;
    benchMemDriver << "#include \"" + fileName + ".h\"" << std::endl;

    //If performing a load, exe, store benchmark, define an input structure
    //benchDriver << getCInputPortStructDefn() << std::endl;

    //Driver will define a zero arg kernel that sets reasonable inputs and repeatedly runs the function.
    //The function will be compiled in a seperate object file and should not be in-lined (potentially resulting in eronious
    //optimizations for the purpose of benchmarking since we are feeding dummy data in).  This should be the case so long as
    //the compiler flag -flto is not used durring compile and linkign (https://stackoverflow.com/questions/35922966/lto-with-llvm-and-cmake)
    //(https://llvm.org/docs/LinkTimeOptimization.html), (https://clang.llvm.org/docs/CommandGuide/clang.html),
    //(https://gcc.gnu.org/wiki/LinkTimeOptimization), (https://gcc.gnu.org/onlinedocs/gccint/LTO-Overview.html).

    //Emit name, file, and units string
    benchMemDriver << "std::string getBenchSuiteName(){\n\treturn \"Generated System: " + designName + "\";\n}" << std::endl;
    benchMemDriver << "std::string getReportFileName(){\n\treturn \"" + fileName + "_benchmarking_report\";\n}" << std::endl;
    benchMemDriver << "std::string getReportUnitsName(){\n\treturn \"STIM_LEN: \" + std::to_string(STIM_LEN) + \" (Samples/Vector/Trial), TRIALS: \" + std::to_string(TRIALS);\n}" << std::endl;

    //Emit Benchmark Report Selection
    benchMemDriver << "void getBenchmarksToReport(std::vector<std::string> &kernels, std::vector<std::string> &vec_ext){\n"
                      "\tkernels.push_back(\"" + designName + "\");\tvec_ext.push_back(\"Single Threaded\");\n}" << std::endl;

    //Emit Benchmark Type Report Selection
    std::string typeStr = "";
    if(numInputVars > 0){
        typeStr = inputVars[0].getDataType().toString(DataType::StringStyle::C);
        typeStr += inputVars[0].getDataType().isComplex() ? " (c)" : " (r)";
    }

    for(unsigned long i = 1; i<numInputVars; i++){
        typeStr += ", " + inputVars[i].getDataType().toString(DataType::StringStyle::C);
        typeStr += inputVars[i].getDataType().isComplex() ? " (c)" : " (r)";
    }

    benchMemDriver << "std::vector<std::string> getVarientsToReport(){\n"
                      "\tstd::vector<std::string> types;\n"
                      "\ttypes.push_back(\"" + typeStr + "\");\n" //This was calculated
                      "\treturn types;\n}" << std::endl;

    //Generate call to loop
    benchMemDriver << "std::map<std::string, std::map<std::string, Results*>*> runBenchSuite(PCM* pcm, int* cpu_num_int){\n"
                      "\tstd::map<std::string, std::map<std::string, Results*>*> kernel_results;\n"
                      "\n"
                      "\tstd::map<std::string, Results*>* type_result = new std::map<std::string, Results*>;\n"
                      "\tResults* result = load_store_two_arg_init_kernel<InputType, OutputType, __m256>(pcm, &bench_" + fileName + "_mem, *cpu_num_int, \"===== Generated System: " + designName + " =====\");\n"
                      "\t(*type_result)[\"" + typeStr + "\"] = result;\n"
                      "\tkernel_results[\"" + designName + "\"] = type_result;\n"
                      "\tprintf(\"\\n\");\n"
                      "\treturn kernel_results;\n}" << std::endl;

    //Generate init
    std::vector<NumericValue> defaultArgs;
    for(unsigned long i = 0; i<numInputVars; i++){
        DataType type = inputVars[i].getDataType();
        NumericValue val;
        val.setRealInt(1);
        val.setImagInt(1);
        val.setComplex(type.isComplex());
        val.setFractional(false);

        defaultArgs.push_back(val);
    }

    benchMemDriver << "void initInput(void* ptr, unsigned long index){\n"
                   "\tInputType* castPtr = (InputType*) ptr;" << std::endl;

    for(unsigned long i = 0; i<numInputVars; i++){
        benchMemDriver << "\tcastPtr->" << inputVars[i].getCVarName(false) << " = " << defaultArgs[i].toStringComponent(false, inputVars[i].getDataType()) << ";" << std::endl;
        //Check if complex
        if(inputVars[i].getDataType().isComplex()) {
            benchMemDriver << "\tcastPtr->" << inputVars[i].getCVarName(true) << " = " << defaultArgs[i].toStringComponent(true, inputVars[i].getDataType()) << ";" << std::endl;
        }
    }
    benchMemDriver << "}" << std::endl;

    benchMemDriver.close();

    //#### Emit Makefiles ####
    std::string makefileTop = "BUILD_DIR=build\n"
                              "DEPENDS_DIR=./depends\n"
                              "COMMON_DIR=./common\n"
                              "SRC_DIR=./intrin\n"
                              "LIB_DIR=.\n"
                              "\n"
                              "UNAME:=$(shell uname)\n"
                              "\n"
                              "#Compiler Parameters\n"
                              "CXX=g++\n"
                              "#Main Benchmark file is not optomized to avoid timing code being re-organized\n"
                              "CFLAGS = -O0 -c -g -std=c++11 -march=native -masm=att\n"
                              "#Generated system should be allowed to optomize - reintepret as c++ file\n"
                              "SYSTEM_CFLAGS = -O3 -c -g -x c++ -std=c++11 -march=native -masm=att\n"
                              "#Most kernels are allowed to be optomized.  Most assembly kernels use asm 'volitile' to force execution\n"
                              "KERNEL_CFLAGS = -O3 -c -g -std=c++11 -march=native -masm=att\n"
                              "#For kernels that should not be optimized, the following is used\n"
                              "KERNEL_NO_OPT_CFLAGS = -O0 -c -g -std=c++11 -march=native -masm=att\n"
                              "INC=-I $(DEPENDS_DIR)/pcm -I $(COMMON_DIR) -I $(SRC_DIR)\n"
                              "LIB_DIRS=-L $(DEPENDS_DIR)/pcm -L $(COMMON_DIR)\n"
                              "ifeq ($(UNAME), Darwin)\n"
                              "LIB=-pthread -lpcmHelper -lPCM -lPcmMsr\n"
                              "LIB_DIRS+= -L $(DEPENDS_DIR)/pcm/MacMSRDriver/build/Release\n"
                              "else\n"
                              "LIB=-pthread -lrt -lpcmHelper -lPCM\n"
                              "endif\n";
    std::string noPCMDefine = "DEFINES = \n";
    std::string PCMDefine = "DEFINES = -DUSE_PCM=0\n";

    std::string makefileMid = "#Need an additional include directory if on MacOS.\n"
                              "#Using the technique in pcm makefile to detect MacOS\n"
                              "ifeq ($(UNAME), Darwin)\n"
                              "INC+= -I $(DEPENDS_DIR)/pcm/MacMSRDriver\n"
                              "endif\n"
                              "\n";

    std::string makefileFiles =  "MAIN_FILE = benchmark_throughput_test.cpp\n"
                                 "LIB_SRCS = " + fileName + "_benchmark_driver_mem.cpp\n"
                                 "SYSTEM_SRC = \n"
                                 "KERNEL_SRCS = " + fileName + "_benchmark_kernel_mem.cpp\n"
                                 "KERNEL_NO_OPT_SRCS = \n";

    std::string makefileBottom = "\n"
                                 "SRCS=$(MAIN_FILE)\n"
                                 "OBJS=$(patsubst %.cpp,$(BUILD_DIR)/%.o,$(SRCS))\n"
                                 "LIB_OBJS=$(patsubst %.cpp,$(BUILD_DIR)/%.o,$(LIB_SRCS))\n"
                                 "SYSTEM_OBJS=$(patsubst %.c,$(BUILD_DIR)/%.o,$(SYSTEM_SRC))\n"
                                 "KERNEL_OBJS=$(patsubst %.cpp,$(BUILD_DIR)/%.o,$(KERNEL_SRCS))\n"
                                 "KERNEL_NO_OPT_OBJS=$(patsubst %.cpp,$(BUILD_DIR)/%.o,$(KERNEL_NO_OPT_SRCS))\n"
                                 "\n"
                                 "#Production\n"
                                 "all: benchmark_" + fileName +"_mem\n"
                                 "\n"
                                 "benchmark_" + fileName + "_mem: $(OBJS) $(SYSTEM_OBJS) $(LIB_OBJS) $(KERNEL_OBJS) $(KERNEL_NO_OPT_OBJS) $(COMMON_DIR)/libpcmHelper.a $(DEPENDS_DIR)/pcm/libPCM.a \n"
                                 "\t$(CXX) $(INC) $(LIB_DIRS) -o benchmark_" + fileName + "_mem $(OBJS) $(SYSTEM_OBJS) $(LIB_OBJS) $(KERNEL_OBJS) $(KERNEL_NO_OPT_OBJS) $(LIB)\n"
                                 "\n"
                                 "$(KERNEL_NO_OPT_OBJS): $(BUILD_DIR)/%.o : $(SRC_DIR)/%.cpp | $(BUILD_DIR)/ $(DEPENDS_DIR)/pcm/Makefile\n"
                                 "\t$(CXX) $(KERNEL_NO_OPT_CFLAGS) $(INC) $(DEFINES) -o $@ $<\n"
                                 "\n"
                                 "$(SYSTEM_OBJS): $(BUILD_DIR)/%.o : $(LIB_DIR)/%.c | $(BUILD_DIR)/ $(DEPENDS_DIR)/pcm/Makefile\n"
                                 "\t$(CXX) $(SYSTEM_CFLAGS) $(INC) $(DEFINES) -o $@ $<\n"
                                 "\n"
                                 "$(KERNEL_OBJS): $(BUILD_DIR)/%.o : $(LIB_DIR)/%.cpp | $(BUILD_DIR)/ $(DEPENDS_DIR)/pcm/Makefile\n"
                                 "\t$(CXX) $(KERNEL_CFLAGS) $(INC) $(DEFINES) -o $@ $<\n"
                                 "\n"
                                 "$(LIB_OBJS): $(BUILD_DIR)/%.o : $(LIB_DIR)/%.cpp | $(BUILD_DIR)/ $(DEPENDS_DIR)/pcm/Makefile\n"
                                 "\t$(CXX) $(CFLAGS) $(INC) $(DEFINES) -o $@ $<\n"
                                 "\n"
                                 "$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp | $(BUILD_DIR)/ $(DEPENDS_DIR)/pcm/Makefile\n"
                                 "\t$(CXX) $(CFLAGS) $(INC) $(DEFINES) -o $@ $<\n"
                                 "\n"
                                 "$(BUILD_DIR)/:\n"
                                 "\tmkdir -p $@\n"
                                 "\n"
                                 "$(DEPENDS_DIR)/pcm/Makefile:\n"
                                 "\tgit submodule update --init --recursive $(DEPENDS_DIR)/pcm\n"
                                 "\n"
                                 "$(DEPENDS_DIR)/pcm/libPCM.a: $(DEPENDS_DIR)/pcm/Makefile\n"
                                 "\tcd $(DEPENDS_DIR)/pcm; make libPCM.a\n"
                                 "\n"
                                 "$(COMMON_DIR)/libpcmHelper.a: $(DEPENDS_DIR)/pcm/Makefile $(DEPENDS_DIR)/pcm/libPCM.a\n"
                                 "\tcd $(COMMON_DIR); make\n"
                                 "\t\n"
                                 "clean:\n"
                                 "\trm -f benchmark_" + fileName + "\n"
                                 "\trm -rf build/*\n"
                                 "\n"
                                 ".PHONY: clean\n";

    std::ofstream makefile;
    makefile.open(path+"/Makefile_" + fileName + "_mem", std::ofstream::out | std::ofstream::trunc);
    std::ofstream makefileNoPCM;
    makefileNoPCM.open(path+"/Makefile_noPCM_" + fileName + "_mem", std::ofstream::out | std::ofstream::trunc);


    makefile << makefileTop << noPCMDefine << makefileMid << makefileFiles << makefileBottom;
    makefileNoPCM << makefileTop << PCMDefine << makefileMid << makefileFiles << makefileBottom;

    makefile.close();
    makefileNoPCM.close();
}

void Design::copyGraph(std::vector<std::shared_ptr<Node>> &nodeCopies, std::vector<std::shared_ptr<Arc>> &arcCopies,
                       std::map<std::shared_ptr<Node>, std::shared_ptr<Node>> &origToCopyNode,
                       std::map<std::shared_ptr<Node>, std::shared_ptr<Node>> &copyToOrigNode,
                       std::map<std::shared_ptr<Arc>, std::shared_ptr<Arc>> &origToCopyArc,
                       std::map<std::shared_ptr<Arc>, std::shared_ptr<Arc>> &copyToOrigArc) {



}
