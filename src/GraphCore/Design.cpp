//
// Created by Christopher Yarp on 6/27/18.
//

#include "Design.h"

#include <algorithm>
#include <iostream>
#include <fstream>
#include "General/GeneralHelper.h"
#include "General/GraphAlgs.h"
#include "General/ErrorHelpers.h"

#include "NodeFactory.h"
#include "MasterNodes/MasterInput.h"
#include "MasterNodes/MasterOutput.h"
#include "MasterNodes/MasterUnconnected.h"

#include "GraphCore/Node.h"
#include "GraphCore/Arc.h"
#include "GraphCore/Variable.h"
#include "PrimitiveNodes/Constant.h"
#include "PrimitiveNodes/Mux.h"
#include "PrimitiveNodes/BlackBox.h"
#include "ContextContainer.h"
#include "ContextFamilyContainer.h"
#include "ContextRoot.h"

#include "GraphCore/StateUpdate.h"

#include "GraphMLTools/GraphMLExporter.h"

//==== Constructors
Design::Design() {
    inputMaster = NodeFactory::createNode<MasterInput>();
    outputMaster = NodeFactory::createNode<MasterOutput>();
    visMaster = NodeFactory::createNode<MasterOutput>();
    unconnectedMaster = NodeFactory::createNode<MasterUnconnected>();
    terminatorMaster = NodeFactory::createNode<MasterOutput>();

//    std::vector<std::shared_ptr<Node>> nodes; //A vector of nodes in the design
//    std::vector<std::shared_ptr<Arc>> arcs; //A vector of arcs in the design
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
    parameters.insert(GraphMLParameter("block_partition_num", "int", true));
    parameters.insert(GraphMLParameter("block_sched_order", "int", true));

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
        } else if(parameter->getType() == "bool" || parameter->getType() == "boolean"){
            defaultVal = "false";
        } else{
            throw std::runtime_error("Unexpected attribute type: " + parameter->getType());
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
        std::shared_ptr<ExpandedNode> expandedPtr = (*node)->expand(newNodes, deletedNodes, newArcs, deletedArcs, unconnectedMaster);
        if(expandedPtr != nullptr){
            expanded = true;
        }
    }

    addRemoveNodesAndArcs(newNodes, deletedNodes, newArcs, deletedArcs);

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

void Design::addRemoveNodesAndArcs(std::vector<std::shared_ptr<Node>> &new_nodes,
                           std::vector<std::shared_ptr<Node>> &deleted_nodes,
                           std::vector<std::shared_ptr<Arc>> &new_arcs,
                           std::vector<std::shared_ptr<Arc>> &deleted_arcs){
    //Add new nodes first, then delete old ones
    for(auto node = new_nodes.begin(); node != new_nodes.end(); node++){
        nodes.push_back(*node);

        //If node has no parent, add it to the list of top level nodes (matches the check for deleted nodes to remove top level nodes)
        if((*node)->getParent() == nullptr){
            topLevelNodes.push_back(*node);
        }
    }

    for(auto node = deleted_nodes.begin(); node != deleted_nodes.end(); node++){
        //Remove the node from it's parent's children list (if it has a parent) -> this is important when the graph is traversed hierarchically)
        if((*node)->getParent()){
            (*node)->getParent()->removeChild(*node);
        }

        //Erase pattern using iterators from https://en.cppreference.com/w/cpp/container/vector/erase
        for(auto candidate = nodes.begin(); candidate != nodes.end(); ){//Will handle iteration in body since erase returns next iterator pos
            if((*candidate) == (*node)){
                candidate = nodes.erase(candidate);
            }else{
                candidate++;
            }
        }

        //TODO: make this check for top level nodes more intelligent.  Right now, all deletions are checked against the top level node list.  Really, only top level node should be considered.  Consider changing top level nodes to a set.
        for (auto candidate = topLevelNodes.begin(); candidate != topLevelNodes.end();) {//Will handle iteration in body since erase returns next iterator pos
            if ((*candidate) == (*node)) {
                candidate = topLevelNodes.erase(candidate);
            } else {
                candidate++;
            }
        }
    }

    //Add new arcs first, then delete old ones
    for(auto arc = new_arcs.begin(); arc != new_arcs.end(); arc++){
        arcs.push_back(*arc);
    }

    for(auto arc = deleted_arcs.begin(); arc != deleted_arcs.end(); arc++){
        //Erase pattern using iterators from https://en.cppreference.com/w/cpp/container/vector/erase
        for(auto candidate = arcs.begin(); candidate != arcs.end(); ){//Will handle iteration in body since erase returns next iterator pos
            if((*candidate) == (*arc)){
                candidate = arcs.erase(candidate);
            }else{
                candidate++;
            }
        }
    }
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

std::string Design::getCFunctionArgPrototype(bool forceArray) {
    std::string prototype = "";

    std::vector<Variable> inputVars = getCInputVariables();
    unsigned long numInputVars = inputVars.size();

    //TODO: Assuming port numbers do not have a discontinuity.  Validate this assumption.
    if(numInputVars>0){
        Variable var = inputVars[0];

        if(forceArray){
            DataType varType = var.getDataType();
            varType.setWidth(2);
            var.setDataType(varType);
        }

        prototype += "const " + var.getCVarDecl(false, false, false, true);

        //Check if complex
        if(var.getDataType().isComplex()){
            prototype += ", const " + var.getCVarDecl(true, false, false, true);
        }
    }

    for(unsigned long i = 1; i<numInputVars; i++){
        Variable var = inputVars[i];

        if(forceArray){
            DataType varType = var.getDataType();
            varType.setWidth(2);
            var.setDataType(varType);
        }

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

std::string Design::getCInputPortStructDefn(int blockSize){
    std::string prototype = "typedef struct {\n";

    std::vector<Variable> inputVars = getCInputVariables();
    unsigned long numInputVars = inputVars.size();

    for(unsigned long i = 0; i<numInputVars; i++){
        Variable var = inputVars[i];

        DataType varType = var.getDataType();
        varType.setWidth(varType.getWidth()*blockSize);
        var.setDataType(varType);

        prototype += "\t" + var.getCVarDecl(false, true, false, true) + ";\n";

        //Check if complex
        if(var.getDataType().isComplex()){
            prototype += "\t" + var.getCVarDecl(true, true, false, true) + ";\n";
        }
    }

    prototype += "} InputType;";

    return prototype;
}

std::string Design::getCOutputStructDefn(int blockSize) {
    //Emit struct header
    std::string str = "typedef struct {";

    unsigned long numPorts = outputMaster->getInputPorts().size();

    for(unsigned long i = 0; i<numPorts; i++){
        std::shared_ptr<InputPort> output = outputMaster->getInputPort(i); //input of output node

        DataType portDataType = output->getDataType();

        portDataType.setWidth(portDataType.getWidth()*blockSize);

        Variable var = Variable(outputMaster->getCOutputName(i), portDataType);

        str += "\n\t" + var.getCVarDecl(false, true, false, true) + ";";

        //Check if complex
        if(portDataType.isComplex()){
            str += "\n\t" + var.getCVarDecl(true, true, false, true) + ";";
        }
    }

    str += "\n} OutputType;";

    return str;
}

void Design::generateSingleThreadedC(std::string outputDir, std::string designName, SchedParams::SchedType schedType, TopologicalSortParameters topoSortParams, unsigned long blockSize, bool emitGraphMLSched, bool printNodeSched){
    if(schedType == SchedParams::SchedType::BOTTOM_UP)
        emitSingleThreadedC(outputDir, designName, designName, schedType, blockSize);
    else if(schedType == SchedParams::SchedType::TOPOLOGICAL) {
        scheduleTopologicalStort(topoSortParams, true, false, designName, outputDir, printNodeSched);
        verifyTopologicalOrder();

        if(emitGraphMLSched) {
            //Export GraphML (for debugging)
            std::cout << "Emitting GraphML Schedule File: " << outputDir << "/" << designName
                      << "_scheduleGraph.graphml" << std::endl;
            GraphMLExporter::exportGraphML(outputDir + "/" + designName + "_scheduleGraph.graphml", *this);
        }

        emitSingleThreadedC(outputDir, designName, designName, schedType, blockSize);
    }else if(schedType == SchedParams::SchedType::TOPOLOGICAL_CONTEXT){
        prune(true);
        expandEnabledSubsystemContexts();
        //Quick check to make sure vis node has no outgoing arcs (required for createEnabledOutputsForEnabledSubsysVisualization).
        //Is ok for them to exist (specifically order constraint arcs) after state update node and context encapsulation
        if(visMaster->outDegree() > 0){
            throw std::runtime_error(ErrorHelpers::genErrorStr("Visualization Master is Not Expected to Have an Out Degree > 0, has " + GeneralHelper::to_string(visMaster->outDegree())));
        }
        createEnabledOutputsForEnabledSubsysVisualization();
        assignNodeIDs(); //Need to assign node IDs since the node ID is used for set ordering and new nodes may have been added
        assignArcIDs();

        createContextVariableUpdateNodes(); //Create after expanding the subcontext so that any movement of EnableInput and EnableOutput nodes
        assignNodeIDs(); //Need to assign node IDs since the node ID is used for set ordering and new nodes may have been added
        assignArcIDs();

        createStateUpdateNodes(); //Done after EnabledSubsystem Contexts are expanded to avoid issues with deleting and re-wiring EnableOutputs
        assignNodeIDs(); //Need to assign node IDs since the node ID is used for set ordering and new nodes may have been added
        assignArcIDs();

        discoverAndMarkContexts();
//        assignNodeIDs(); //Need to assign node IDs since the node ID is used for set ordering and new nodes may have been added
//        assignArcIDs();

        //Order constraining zero input nodes in enabled subsystems is not nessisary as rewireArcsToContexts can wire the enable
        //line as a depedency for the enable context to be emitted.  This is currently done in the scheduleTopoloicalSort method called below
        //TODO: re-introduce orderConstrainZeroInputNodes if the entire enable context is not scheduled hierarchically
        //orderConstrainZeroInputNodes(); //Do this after the contexts being marked since this constraint should not have an impact on contexts˚

        encapsulateContexts();
        assignNodeIDs(); //Need to assign node IDs since the node ID is used for set ordering and new nodes may have been added
        assignArcIDs();

        //Export GraphML (for debugging)
//        std::cout << "Emitting GraphML pre-Schedule File: " << outputDir << "/" << designName << "_preSortGraph.graphml" << std::endl;
//        GraphMLExporter::exportGraphML(outputDir+"/"+designName+"_preSchedGraph.graphml", *this);

        scheduleTopologicalStort(topoSortParams, false, true, designName, outputDir, printNodeSched); //Pruned before inserting state update nodes
        verifyTopologicalOrder();

        if(emitGraphMLSched) {
            //Export GraphML (for debugging)
            std::cout << "Emitting GraphML Schedule File: " << outputDir << "/" << designName
                      << "_scheduleGraph.graphml" << std::endl;
            GraphMLExporter::exportGraphML(outputDir + "/" + designName + "_scheduleGraph.graphml", *this);
        }

        emitSingleThreadedC(outputDir, designName, designName, schedType, blockSize);
    }else{
        throw std::runtime_error("Unknown SCHED Type");
    }
}

void Design::emitSingleThreadedOpsBottomUp(std::ofstream &cFile, std::vector<std::shared_ptr<Node>> &nodesWithState, SchedParams::SchedType schedType){
    //Emit compute next states
    cFile << std::endl << "//==== Compute Next States ====" << std::endl;
    unsigned long numNodesWithState = nodesWithState.size();
    for(unsigned long i = 0; i<numNodesWithState; i++){
        std::vector<std::string> nextStateExprs;
        nodesWithState[i]->emitCExprNextState(nextStateExprs, schedType);
        cFile << std::endl << "//---- Compute Next States " << nodesWithState[i]->getFullyQualifiedName() <<" ----" << std::endl;

        unsigned long numNextStateExprs = nextStateExprs.size();
        for(unsigned long j = 0; j<numNextStateExprs; j++){
            cFile << nextStateExprs[j] << std::endl;
        }
    }

    //Assign each of the outputs (and emit any expressions that preceed it)
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
        std::string expr_re = srcNode->emitC(cStatements_re, schedType, srcNodeOutputPortNum, false);
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
            std::string expr_im = srcNode->emitC(cStatements_im, schedType, srcNodeOutputPortNum, true);
            //emit the expressions
            unsigned long numStatements_im = cStatements_im.size();
            for(unsigned long j = 0; j<numStatements_im; j++){
                cFile << cStatements_im[j] << std::endl;
            }

            //emit the assignment
            cFile << "output[0]." << outputVar.getCVarName(true) << " = " << expr_im << ";" << std::endl;
        }
    }
}

void Design::emitSingleThreadedOpsSched(std::ofstream &cFile, SchedParams::SchedType schedType){

    cFile << std::endl << "//==== Compute Operators ====" << std::endl;

    //Sort nodes by schedOrder.
    std::vector<std::shared_ptr<Node>> orderedNodes = nodes;
    //Add the output master to the scheduled node list
    orderedNodes.push_back(outputMaster);
    std::sort(orderedNodes.begin(), orderedNodes.end(), Node::lessThanSchedOrder);

    std::shared_ptr<Node> zeroSchedNodeCmp = NodeFactory::createNode<MasterUnconnected>(); //Need a node to compare to
    zeroSchedNodeCmp->setSchedOrder(0);

    auto schedIt = std::lower_bound(orderedNodes.begin(), orderedNodes.end(), zeroSchedNodeCmp, Node::lessThanSchedOrder); //Binary search for the first node to be emitted (schedOrder 0)

    //Itterate through the schedule and emit
    for(auto it = schedIt; it != orderedNodes.end(); it++){

//        std::cout << "Writing " << (*it)->getFullyQualifiedName() << std::endl;

        if(*it == outputMaster){
            //Emit output (using same basic code as bottom up except forcing fanout - all results will be availible as temp vars)
            unsigned long numOutputs = outputMaster->getInputPorts().size();
            for(unsigned long i = 0; i<numOutputs; i++){
                std::shared_ptr<InputPort> output = outputMaster->getInputPort(i);
                cFile << std::endl << "//---- Assign Output " << i << ": " << output->getName() <<" ----" << std::endl;

                //Get the arc connected to the output
                std::shared_ptr<Arc> outputArc = *(output->getArcs().begin());

                DataType outputDataType = outputArc->getDataType();

                std::shared_ptr<OutputPort> srcOutputPort = outputArc->getSrcPort();
                int srcNodeOutputPortNum = srcOutputPort->getPortNum();

                std::shared_ptr<Node> srcNode = srcOutputPort->getParent();

                cFile << "//-- Assign Real Component --" << std::endl;
                std::vector<std::string> cStatements_re;
                std::string expr_re = srcNode->emitC(cStatements_re, schedType, srcNodeOutputPortNum, false, true, true);
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
                    cFile << std::endl << "//-- Assign Imag Component --" << std::endl;
                    std::vector<std::string> cStatements_im;
                    std::string expr_im = srcNode->emitC(cStatements_im, schedType, srcNodeOutputPortNum, true, true, true);
                    //emit the expressions
                    unsigned long numStatements_im = cStatements_im.size();
                    for(unsigned long j = 0; j<numStatements_im; j++){
                        cFile << cStatements_im[j] << std::endl;
                    }

                    //emit the assignment
                    cFile << "output[0]." << outputVar.getCVarName(true) << " = " << expr_im << ";" << std::endl;
                }
            }
        }else if((*it)->hasState()){
            //Call emit for state element input
            //The state element output is treateded similarly to a constant and a variable name is always returned
            //The output has no dependencies within the cycle
            //The input can have dependencies

            //Emit comment
            cFile << std::endl << "//---- Calculate " << (*it)->getFullyQualifiedName() << " Inputs ----" << std::endl;

            std::vector<std::string> nextStateExprs;
            (*it)->emitCExprNextState(nextStateExprs, schedType);

            for(unsigned long j = 0; j<nextStateExprs.size(); j++){
                cFile << nextStateExprs[j] << std::endl;
            }

        }else{
            //Emit standard node

            //Emit comment
            cFile << std::endl << "//---- Calculate " << (*it)->getFullyQualifiedName() << " ----" << std::endl;

            unsigned long numOutputPorts = (*it)->getOutputPorts().size();
            //Emit each output port
            //TODO: check for unconnected output ports and do not emit them
            for(unsigned long i = 0; i<numOutputPorts; i++){
                std::vector<std::string> cStatementsRe;
                //Emit real component (force fanout)
                (*it)->emitC(cStatementsRe, schedType, i, false, true, true); //We actually do not use the returned expression.  Dependent nodes will get this by calling the emit function of this block again.

                for(unsigned long j = 0; j<cStatementsRe.size(); j++){
                    cFile << cStatementsRe[j] << std::endl;
                }

                if((*it)->getOutputPort(i)->getDataType().isComplex()) {
                    //Emit imag component (force fanout)
                    std::vector<std::string> cStatementsIm;
                    //Emit real component (force fanout)
                    (*it)->emitC(cStatementsIm, schedType, i, true, true, true); //We actually do not use the returned expression.  Dependent nodes will get this by calling the emit function of this block again.

                    for(unsigned long j = 0; j<cStatementsIm.size(); j++){
                        cFile << cStatementsIm[j] << std::endl;
                    }
                }
            }
        }

    }
}

void Design::emitSingleThreadedOpsSchedStateUpdateContext(std::ofstream &cFile, SchedParams::SchedType schedType, int blockSize, std::string indVarName){

    cFile << std::endl << "//==== Compute Operators ====" << std::endl;

    //Sort nodes by schedOrder.
    std::vector<std::shared_ptr<Node>> orderedNodes = nodes;
    //Add the output master to the scheduled node list
    orderedNodes.push_back(outputMaster);
    std::sort(orderedNodes.begin(), orderedNodes.end(), Node::lessThanSchedOrder);

    std::shared_ptr<Node> zeroSchedNodeCmp = NodeFactory::createNode<MasterUnconnected>(); //Need a node to compare to
    zeroSchedNodeCmp->setSchedOrder(0);

    auto schedIt = std::lower_bound(orderedNodes.begin(), orderedNodes.end(), zeroSchedNodeCmp, Node::lessThanSchedOrder); //Binary search for the first node to be emitted (schedOrder 0)

    std::vector<std::shared_ptr<Node>> toBeEmittedInThisOrder;
    std::copy(schedIt, orderedNodes.end(), std::back_inserter(toBeEmittedInThisOrder));

    emitOpsStateUpdateContext(cFile, schedType, toBeEmittedInThisOrder, blockSize, indVarName);
}

void Design::emitOpsStateUpdateContext(std::ofstream &cFile, SchedParams::SchedType schedType, std::vector<std::shared_ptr<Node>> orderedNodes, int blockSize,
                                       std::string indVarName) {
    //Keep a context stack of the last emitted statement.  This is used to check for context changes.  Also used to check if the 'first' entry should be used.  If first entry is used (ie. previous context at this level in the stack was not in the same famuly, and the subContext emit count is not 0, then contexts are not contiguous -> ie. switch cannot be used)
    std::vector<Context> lastEmittedContext;

    //True if the context that was emitted was opened with the 'first' method
    std::vector<bool> contextFirst;

    //Keep a set of contexts that have been emitted (and exited).  For now, we do not allow re-entry into an already emitted sub context.
    std::set<Context> alreadyEmittedSubContexts;

    //Keep a count of how many subContexts of a contextRoot have been emitted, allows if/else if/else statements to not be contiguous
    std::map<std::shared_ptr<ContextRoot>, int> subContextEmittedCount;

    //Itterate through the schedule and emit
    for(auto it = orderedNodes.begin(); it != orderedNodes.end(); it++){

//        std::cout << "Writing " << (*it)->getFullyQualifiedName() << std::endl;

        //Check if the context has changed
        std::vector<Context> nodeContext = (*it)->getContext();

        std::vector<std::string> contextStatements;

        if(nodeContext != lastEmittedContext) {

            //Check if previous context(s) should be closed (run up the stack until the contexts are the same - we can do this because contexts are in a strict hierarchy [ie. tree])
            for(unsigned long i = 0; i<lastEmittedContext.size(); i++){
                unsigned long ind = lastEmittedContext.size()-1-i;

                if(ind >= nodeContext.size()){
                    //The new context is less deep than this.  Therefore, we should close this context
                    if(subContextEmittedCount.find(lastEmittedContext[ind].getContextRoot()) == subContextEmittedCount.end()){
                        subContextEmittedCount[lastEmittedContext[ind].getContextRoot()] = 0;
                    }

                    if(contextFirst.size() - 1 < ind || contextFirst[ind]){
                        //This context was created with a call to first
                        lastEmittedContext[ind].getContextRoot()->emitCContextCloseFirst(contextStatements, schedType, lastEmittedContext[ind].getSubContext());
                    }else if(subContextEmittedCount[lastEmittedContext[ind].getContextRoot()] >= lastEmittedContext[ind].getContextRoot()->getNumSubContexts()-1 ){
                        lastEmittedContext[ind].getContextRoot()->emitCContextCloseLast(contextStatements, schedType, lastEmittedContext[ind].getSubContext());
                    }else{
                        lastEmittedContext[ind].getContextRoot()->emitCContextCloseMid(contextStatements, schedType, lastEmittedContext[ind].getSubContext());
                    }

                    //Remove this level of the contextFirst stack because the family was exited
                    contextFirst.pop_back();

                    //Add that context to the exited context set
                    alreadyEmittedSubContexts.insert(lastEmittedContext[ind]);

                    //Increment emitted count by 1
                    subContextEmittedCount[lastEmittedContext[ind].getContextRoot()]++;

                }else if(lastEmittedContext[ind] != nodeContext[ind]){
                    //TODO: change if split contexts are introduced -> would need to check for first context change, then emit all context closes up to that point
                    //could be either a first, mid, or last
                    if(contextFirst[ind]) { //contextFirst is guarenteed to exist because this level existed in the last emitted context
                        lastEmittedContext[ind].getContextRoot()->emitCContextCloseFirst(contextStatements, schedType, lastEmittedContext[ind].getSubContext());
                    }else if(subContextEmittedCount[lastEmittedContext[ind].getContextRoot()] >= lastEmittedContext[ind].getContextRoot()->getNumSubContexts()-1 ){
                        lastEmittedContext[ind].getContextRoot()->emitCContextCloseLast(contextStatements, schedType, lastEmittedContext[ind].getSubContext());
                    }else{
                        lastEmittedContext[ind].getContextRoot()->emitCContextCloseMid(contextStatements, schedType, lastEmittedContext[ind].getSubContext());
                    }

                    if(lastEmittedContext[ind].getContextRoot() == nodeContext[ind].getContextRoot()){
                        //In this case, the next context is in the same family, set the contextFirst to false and do not remove it
                        contextFirst[ind] = false;
                    }else{
                        //In this case, the context family is being left, remove the contextFirst entry for the family
                        contextFirst.pop_back();
                    }

                    //Add that context to the exited context set
                    alreadyEmittedSubContexts.insert(lastEmittedContext[ind]);

                    //Increment emitted count by 1
                    subContextEmittedCount[lastEmittedContext[ind].getContextRoot()]++;
                }else{
                    break; //We found the point root of both the old and new contexts, we can stop here
                }
            }

            //If new context(s) have been entered, emit the conditionals (run down the stack to find the root of both, then emit the context entries)
            for(unsigned long i = 0; i<nodeContext.size(); i++){
                if(i>=lastEmittedContext.size()){
                    //This context level is new, it must be the first time it has been entered

                    //Declare vars
                    std::vector<Variable> contextVars = nodeContext[i].getContextRoot()->getCContextVars();
                    for(unsigned long j = 0; j<contextVars.size(); j++) {
                        contextStatements.push_back(contextVars[j].getCVarDecl(false, true, false, true) + ";\n");

                        if (contextVars[j].getDataType().isComplex()) {
                            contextStatements.push_back(contextVars[j].getCVarDecl(true, true, false, true) + ";\n");
                        }
                    }

                    //Emit context
                    nodeContext[i].getContextRoot()->emitCContextOpenFirst(contextStatements, schedType, nodeContext[i].getSubContext());

                    //push back onto contextFirst
                    contextFirst.push_back(true);
                }else if(lastEmittedContext[i] != nodeContext[i]){  //Because contexts must be purely hierarchical, new context is only entered if the context at this level is different from the previously emitted context.
                    //TODO: change if split contexts are introduced -> would need to check for first context change, then emit all context opens after that
                    //This context level already existed, could either be an first, mid, or last

                    //Check if the previous context had the same root.  If so, this is the first
                    if(lastEmittedContext[i].getContextRoot() != nodeContext[i].getContextRoot()){
                        //Emit context vars
                        std::vector<Variable> contextVars = nodeContext[i].getContextRoot()->getCContextVars();
                        for(unsigned long j = 0; j<contextVars.size(); j++) {
                            contextStatements.push_back(contextVars[j].getCVarDecl(false, true, false, true) + ";\n");

                            if (contextVars[j].getDataType().isComplex()) {
                                contextStatements.push_back(contextVars[j].getCVarDecl(true, true, false, true) + ";\n");
                            }
                        }

                        //Emit context
                        nodeContext[i].getContextRoot()->emitCContextOpenFirst(contextStatements, schedType, nodeContext[i].getSubContext());

                        //This is a new family, push back
                        contextFirst.push_back(true);
                    }else{
                        if(subContextEmittedCount[nodeContext[i].getContextRoot()] >= nodeContext[i].getContextRoot()->getNumSubContexts()-1){ //Check if this is the last in the context family
                            nodeContext[i].getContextRoot()->emitCContextOpenLast(contextStatements, schedType, nodeContext[i].getSubContext());
                        }else{
                            nodeContext[i].getContextRoot()->emitCContextOpenMid(contextStatements, schedType, nodeContext[i].getSubContext());
                        }

                        //Contexts are in the same family
                        contextFirst[i] = false;
                    }

                }
                //Else, we have not yet found where the context stacks diverge (if they in fact do) --> continue
            }

            //Check to see if this is the first, last, or other conditional being emitted (if there is only 1 context, default to first call)
                //Check to see if the previous context at this level (if one existed) was in the same family: if so, this is either the middle or end, if not, this is a first
                    //Check the count
                //Check to see if the count of emitted subContests for this context root is max# contexts -1.  If so, it is last.  Else, it is a middle

            //If the first time, call the context preparation function (ie. for declaring outputs outside of context)

            //Set contextFirst to true (add it to the stack) if this was the first context, false otherwise

            //Emit proper conditional
        }

        for(unsigned long i = 0; i<contextStatements.size(); i++){
            cFile << contextStatements[i];
        }

        if(*it == outputMaster) {
            //Emit output (using same basic code as bottom up except forcing fanout - all results will be availible as temp vars)
            unsigned long numOutputs = outputMaster->getInputPorts().size();
            for (unsigned long i = 0; i < numOutputs; i++) {
                std::shared_ptr<InputPort> output = outputMaster->getInputPort(i);
                cFile << std::endl << "//---- Assign Output " << i << ": " << output->getName() << " ----" << std::endl;

                //Get the arc connected to the output
                std::shared_ptr<Arc> outputArc = *(output->getArcs().begin());

                DataType outputDataType = outputArc->getDataType();

                std::shared_ptr<OutputPort> srcOutputPort = outputArc->getSrcPort();
                int srcNodeOutputPortNum = srcOutputPort->getPortNum();

                std::shared_ptr<Node> srcNode = srcOutputPort->getParent();

                cFile << "//-- Assign Real Component --" << std::endl;
                std::vector<std::string> cStatements_re;
                std::string expr_re = srcNode->emitC(cStatements_re, schedType, srcNodeOutputPortNum, false, true, true);
                //emit the expressions
                unsigned long numStatements_re = cStatements_re.size();
                for (unsigned long j = 0; j < numStatements_re; j++) {
                    cFile << cStatements_re[j] << std::endl;
                }

                //emit the assignment
                Variable outputVar = Variable(outputMaster->getCOutputName(i), outputDataType);
                int varWidth = outputVar.getDataType().getWidth();
                int varBytes = outputVar.getDataType().getCPUStorageType().getTotalBits()/8;
                if(varWidth > 1){
                    if(blockSize > 1) {
                        cFile << "memcpy(output[0]." << outputVar.getCVarName(false) << "+" << blockSize
                              << "*" << indVarName << ", " << expr_re << ", " << varWidth * varBytes << ");"
                              << std::endl;
                    }else{
                        cFile << "memcpy(output[0]." << outputVar.getCVarName(false) << ", " << expr_re << ", "
                              << varWidth * varBytes << ");" << std::endl;
                    }
                }else{
                    if(blockSize > 1) {
                        cFile << "output[0]." << outputVar.getCVarName(false) << "[" << indVarName << "] = " << expr_re << ";" << std::endl;
                    }else{
                        cFile << "output[0]." << outputVar.getCVarName(false) << " = " << expr_re << ";" << std::endl;
                    }
                }

                //Emit Imag if Datatype is complex
                if (outputDataType.isComplex()) {
                    cFile << std::endl <<  "//-- Assign Imag Component --" << std::endl;
                    std::vector<std::string> cStatements_im;
                    std::string expr_im = srcNode->emitC(cStatements_im, schedType, srcNodeOutputPortNum, true, true, true);
                    //emit the expressions
                    unsigned long numStatements_im = cStatements_im.size();
                    for (unsigned long j = 0; j < numStatements_im; j++) {
                        cFile << cStatements_im[j] << std::endl;
                    }

                    //emit the assignment
                    if(varWidth > 1){
                        if(blockSize > 1) {
                            cFile << "memcpy(output[0]." << outputVar.getCVarName(true) << "+" << blockSize
                                  << "*" << indVarName << ", " << expr_im << ", " << varWidth * varBytes << ");"
                                  << std::endl;
                        }else{
                            cFile << "memcpy(output[0]." << outputVar.getCVarName(true) << ", " << expr_im << ", "
                                  << varWidth * varBytes << ");" << std::endl;
                        }
                    }else{
                        if(blockSize > 1) {
                            cFile << "output[0]." << outputVar.getCVarName(true) << "[" << indVarName << "] = " << expr_im << ";" << std::endl;
                        }else{
                            cFile << "output[0]." << outputVar.getCVarName(true) << " = " << expr_im << ";" << std::endl;
                        }
                    }
                }
            }
        }else if(GeneralHelper::isType<Node, StateUpdate>(*it) != nullptr){
            std::shared_ptr<StateUpdate> stateUpdateNode = std::dynamic_pointer_cast<StateUpdate>(*it);
            //Emit state update
            cFile << std::endl << "//---- State Update for " << stateUpdateNode->getPrimaryNode()->getFullyQualifiedName() << " ----" << std::endl;

            std::vector<std::string> stateUpdateExprs;
            (*it)->emitCStateUpdate(stateUpdateExprs, schedType);

            for(unsigned long j = 0; j<stateUpdateExprs.size(); j++){
                cFile << stateUpdateExprs[j] << std::endl;
            }

        }else if((*it)->hasState()){
            //Call emit for state element input
            //The state element output is treated similarly to a constant and a variable name is always returned
            //The output has no dependencies within the cycle
            //The input can have dependencies

            //Emit comment
            cFile << std::endl << "//---- Calculate " << (*it)->getFullyQualifiedName() << " Inputs ----" << std::endl;

            std::vector<std::string> nextStateExprs;
            (*it)->emitCExprNextState(nextStateExprs, schedType);

            for(unsigned long j = 0; j<nextStateExprs.size(); j++){
                cFile << nextStateExprs[j] << std::endl;
            }

        }else{
            //Emit standard node

            //Emit comment
            cFile << std::endl << "//---- Calculate " << (*it)->getFullyQualifiedName() << " ----" << std::endl;

            unsigned long numOutputPorts = (*it)->getOutputPorts().size();
            //Emit each output port
            //TODO: check for unconnected output ports and do not emit them
            for(unsigned long i = 0; i<numOutputPorts; i++){
                std::vector<std::string> cStatementsRe;
                //Emit real component (force fanout)
                (*it)->emitC(cStatementsRe, schedType, i, false, true, true); //We actually do not use the returned expression.  Dependent nodes will get this by calling the emit function of this block again.

                for(unsigned long j = 0; j<cStatementsRe.size(); j++){
                    cFile << cStatementsRe[j] << std::endl;
                }

                if((*it)->getOutputPort(i)->getDataType().isComplex()) {
                    //Emit imag component (force fanout)
                    std::vector<std::string> cStatementsIm;
                    //Emit real component (force fanout)
                    (*it)->emitC(cStatementsIm, schedType, i, true, true, true); //We actually do not use the returned expression.  Dependent nodes will get this by calling the emit function of this block again.

                    for(unsigned long j = 0; j<cStatementsIm.size(); j++){
                        cFile << cStatementsIm[j] << std::endl;
                    }
                }
            }
        }

        lastEmittedContext = nodeContext;

    }
}


void Design::emitSingleThreadedC(std::string path, std::string fileName, std::string designName, SchedParams::SchedType sched, unsigned long blockSize) {
    std::string blockIndVar = "";

    //Set the block size and indVarName in the master nodes.  This is used in the emit, specifically in the inputMaster
    inputMaster->setBlockSize(blockSize);
    outputMaster->setBlockSize(blockSize);
    terminatorMaster->setBlockSize(blockSize);
    unconnectedMaster->setBlockSize(blockSize);
    visMaster->setBlockSize(blockSize);

    if(blockSize > 1) {
        blockIndVar = "blkInd";
        inputMaster->setIndVarName(blockIndVar);
        outputMaster->setIndVarName(blockIndVar);
        terminatorMaster->setIndVarName(blockIndVar);
        unconnectedMaster->setIndVarName(blockIndVar);
        visMaster->setIndVarName(blockIndVar);
    }

    //Note: If the blockSize == 1, the function prototype can include scalar arguments.  If blockSize > 1, only pointer
    //types are allowed since multiple values are being passed

    //Get the OutputType struct defn and function prototype

    std::string outputTypeDefn = getCOutputStructDefn(blockSize);
    bool forceArrayArgs = blockSize > 1;
    std::string fctnProtoArgs = getCFunctionArgPrototype(forceArrayArgs);

    std::string fctnProto = "void " + designName + "(" + fctnProtoArgs + ")";

    std::cout << "Emitting C File: " << path << "/" << designName << ".h" << std::endl;
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
    std::vector<std::shared_ptr<Node>> nodesWithState = findNodesWithState();
    std::vector<std::shared_ptr<Node>> nodesWithGlobalDecl = findNodesWithGlobalDecl();
    unsigned long numNodes = nodes.size(); //Iterate through all un-pruned nodes in the design since this is a single threaded emit

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

    //Insert BlackBox Headers
    std::vector<std::shared_ptr<BlackBox>> blackBoxes = findBlackBoxes();
    if(blackBoxes.size() > 0) {
        headerFile << "//==== BlackBox Headers ====" << std::endl;

        for(unsigned long i = 0; i<blackBoxes.size(); i++){
            headerFile << "//**** BEGIN BlackBox " << blackBoxes[i]->getFullyQualifiedName() << " Header ****" << std::endl;
            headerFile << blackBoxes[i]->getCppHeaderContent() << std::endl;
            headerFile << "//**** END BlackBox " << blackBoxes[i]->getFullyQualifiedName() << " Header ****" << std::endl;
        }
    }

    headerFile << "#endif" << std::endl;

    headerFile.close();

    std::cout << "Emitting C File: " << path << "/" << designName << ".c" << std::endl;
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

    //Emit BlackBox C++ functions
    if(blackBoxes.size() > 0) {
        cFile << "//==== BlackBox Functions ====" << std::endl;

        for(unsigned long i = 0; i<blackBoxes.size(); i++){
            cFile << "//**** BEGIN BlackBox " << blackBoxes[i]->getFullyQualifiedName() << " Functions ****" << std::endl;
            cFile << blackBoxes[i]->getCppBodyContent() << std::endl;
            cFile << "//**** END BlackBox " << blackBoxes[i]->getFullyQualifiedName() << " Functions ****" << std::endl;
        }
    }

    cFile << std::endl;

    cFile << "//==== Functions ====" << std::endl;

    cFile << fctnProto << "{" << std::endl;

    //emit inner loop
    DataType blockDT = DataType(false, false, false, (int) std::ceil(std::log2(blockSize)+1), 0, 1);
    if(blockSize > 1) {
        cFile << "for(" + blockDT.getCPUStorageType().toString(DataType::StringStyle::C, false, false) + " " + blockIndVar + " = 0; " + blockIndVar + "<" + GeneralHelper::to_string(blockSize) + "; " + blockIndVar + "++){" << std::endl;
    }

    //Emit operators
    //TODO: Implement other scheduler types
    if(sched == SchedParams::SchedType::BOTTOM_UP){
        emitSingleThreadedOpsBottomUp(cFile, nodesWithState, sched);
    }else if(sched == SchedParams::SchedType::TOPOLOGICAL){
        emitSingleThreadedOpsSched(cFile, sched);
    }else if(sched == SchedParams::SchedType::TOPOLOGICAL_CONTEXT){
        emitSingleThreadedOpsSchedStateUpdateContext(cFile, sched, blockSize, blockIndVar);
    }else{
        throw std::runtime_error("Unknown schedule type");
    }

    //Emit state variable updates (only if state update nodes are not included)
    if(sched != SchedParams::SchedType::TOPOLOGICAL_CONTEXT){
        cFile << std::endl << "//==== Update State Vars ====" << std::endl;
        for(unsigned long i = 0; i<nodesWithState.size(); i++){
            std::vector<std::string> stateUpdateExprs;
            nodesWithState[i]->emitCStateUpdate(stateUpdateExprs, sched);

            unsigned long numStateUpdateExprs = stateUpdateExprs.size();
            for(unsigned long j = 0; j<numStateUpdateExprs; j++){
                cFile << stateUpdateExprs[j] << std::endl;
            }
        }
    }

    if(blockSize > 1) {
        cFile << "}" << std::endl;
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

    //Call the reset functions of blackboxes if they have state
    if(blackBoxes.size() > 0) {
        cFile << "//==== Reset BlackBoxes ====" << std::endl;

        for(unsigned long i = 0; i<blackBoxes.size(); i++){
            cFile << blackBoxes[i]->getResetName() << "();" << std::endl;
        }
    }

    cFile << "}" << std::endl;

    cFile.close();
}

void Design::emitSingleThreadedCBenchmarkingDrivers(std::string path, std::string fileName, std::string designName, int blockSize) {
    //TODO: emit blocked versions
    emitSingleThreadedCBenchmarkingDriverConst(path, fileName, designName, blockSize);
    emitSingleThreadedCBenchmarkingDriverMem(path, fileName, designName, blockSize);
}

void
Design::emitSingleThreadedCBenchmarkingDriverConst(std::string path, std::string fileName, std::string designName, int blockSize) {
    std::string fileNameUpper =  GeneralHelper::toUpper(fileName);

    //#### Emit Kernel File & Header ####
    std::ofstream benchKernel;
    benchKernel.open(path+"/"+fileName+"_benchmark_kernel.cpp", std::ofstream::out | std::ofstream::trunc);

    benchKernel << "#include \"" << fileName << ".h" << "\"" << std::endl;
    benchKernel << "#include \"intrin_bench_default_defines.h\"" << std::endl;
    benchKernel << "void bench_"+fileName+"()\n"
                   "{\n"
                   "\tOutputType output;\n"
                   "\tunsigned long outputCount;\n";

    //Generate loop
    std::vector<Variable> inputVars = getCInputVariables();
    unsigned long numInputVars = inputVars.size();

    std::vector<NumericValue> defaultArgs;
    for (unsigned long i = 0; i < numInputVars; i++) {
        DataType type = inputVars[i].getDataType();
        NumericValue val;
        val.setRealInt(1);
        val.setImagInt(1);
        val.setComplex(type.isComplex());
        val.setFractional(false);

        defaultArgs.push_back(val);
    }

    std::string fctnCall = designName + "(";

    if(blockSize >1) {
        //For a block size greater than 1, constant arrays need to be created
        for (unsigned long i = 0; i < numInputVars; i++) {
            //Expand the size of the variable to account for the block size;
            Variable blockInputVar = inputVars[i];
            DataType blockDT = blockInputVar.getDataType();
            int blockedWidth = blockDT.getWidth()*blockSize;
            blockDT.setWidth(blockedWidth);
            blockInputVar.setDataType(blockDT);
            benchKernel << "\t" << blockInputVar.getCVarDecl(false, true, false, true) << " = {";

            for (unsigned long j = 0; j < blockedWidth; j++){
                if(j>0){
                    benchKernel << ", ";
                }
                benchKernel << defaultArgs[i].toStringComponent(false, blockDT);
            }
            benchKernel << "};" << std::endl;

            if(i>0){
                fctnCall += ", ";
            }
            fctnCall += blockInputVar.getCVarName(false);

            if (inputVars[i].getDataType().isComplex()) {
                benchKernel << "\t" << blockInputVar.getCVarDecl(true, true, false, true) << " = {";

                for (unsigned long j = 0; j < blockedWidth; j++){
                    if(j>0){
                        benchKernel << ", ";
                    }
                    benchKernel << defaultArgs[i].toStringComponent(true, blockDT);
                }
                benchKernel << "};" << std::endl;

                fctnCall += ", "; //This is guarenteed to be needed as the real component will come first
                fctnCall += blockInputVar.getCVarName(true);
            }
        }
    }else{
        //For a block size of 1, the arguments are passed as scalars
        if (numInputVars > 0) {
            fctnCall += defaultArgs[0].toStringComponent(false, inputVars[0].getDataType());
            if (inputVars[0].getDataType().isComplex()) {
                fctnCall += ", " + defaultArgs[0].toStringComponent(true, inputVars[0].getDataType());
            }
        }
        for (unsigned long i = 1; i < numInputVars; i++) {
            fctnCall += ", " + defaultArgs[i].toStringComponent(false, inputVars[i].getDataType());
            if (inputVars[i].getDataType().isComplex()) {
                fctnCall += ", " + defaultArgs[i].toStringComponent(true, inputVars[i].getDataType());
            }
        }
    }

    if (numInputVars > 0) {
        fctnCall += ", ";
    }
    fctnCall += "&output, &outputCount)";
    if(blockSize > 1){
        benchKernel << "\tint iterLimit = STIM_LEN/" << blockSize << ";" << std::endl;
    }else{
        benchKernel << "\tint iterLimit = STIM_LEN;" << std::endl;
    }
    benchKernel << "\tfor(int i = 0; i<iterLimit; i++)\n"
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
    benchDriver << "std::map<std::string, std::map<std::string, Results*>*> runBenchSuite(Profiler* profiler, int* cpu_num_int){\n"
                   "\tstd::map<std::string, std::map<std::string, Results*>*> kernel_results;\n"
                   "\n"
                   "\tstd::map<std::string, Results*>* type_result = new std::map<std::string, Results*>;\n"
                   "\tResults* result = zero_arg_kernel(profiler, &bench_" + fileName + ", *cpu_num_int, \"===== Generated System: " + designName + " =====\");\n"
                   "\t(*type_result)[\"" + typeStr + "\"] = result;\n"
                   "\tkernel_results[\"" + designName + "\"] = type_result;\n"
                   "\tprintf(\"\\n\");\n"
                   "\treturn kernel_results;\n}" << std::endl;

    benchDriver << "void initInput(void* ptr, unsigned long index){}" << std::endl;

    benchDriver.close();

    //#### Emit Makefiles ####
    std::string makefileContent =  "BUILD_DIR=build\n"
                                   "DEPENDS_DIR=./depends\n"
                                   "COMMON_DIR=./common\n"
                                   "SRC_DIR=./intrin\n"
                                   "LIB_DIR=.\n"
                                   "\n"
                                   "#Parameters that can be supplied to this makefile\n"
                                   "USE_PCM=1\n"
                                   "USE_AMDuPROF=1\n"
                                   "\n"
                                   "UNAME:=$(shell uname)\n"
                                   "\n"
                                   "#Compiler Parameters\n"
                                   "#CXX=g++\n"
                                   "#Main Benchmark file is not optomized to avoid timing code being re-organized\n"
                                   "CFLAGS = -O0 -c -g -std=c++11 -march=native -masm=att\n"
                                   "#Generated system should be allowed to optomize - reintepret as c++ file\n"
                                   "SYSTEM_CFLAGS = -O3 -c -g -x c++ -std=c++11 -march=native -masm=att\n"
                                   "#Most kernels are allowed to be optomized.  Most assembly kernels use asm 'volitile' to force execution\n"
                                   "KERNEL_CFLAGS = -O3 -c -g -std=c++11 -march=native -masm=att\n"
                                   "#For kernels that should not be optimized, the following is used\n"
                                   "KERNEL_NO_OPT_CFLAGS = -O0 -c -g -std=c++11 -march=native -masm=att\n"
                                   "INC=-I $(COMMON_DIR) -I $(SRC_DIR)\n"
                                   "LIB_DIRS=-L $(COMMON_DIR)\n"
                                   "LIB=-pthread -lProfilerCommon\n"
                                   "\n"
                                   "DEFINES=\n"
                                   "\n"
                                   "DEPENDS=\n"
                                   "DEPENDS_LIB=$(COMMON_DIR)/libProfilerCommon.a\n"
                                   "\n"
                                   "ifeq ($(USE_PCM), 1)\n"
                                   "DEFINES+= -DUSE_PCM=1\n"
                                   "DEPENDS+= $(DEPENDS_DIR)/pcm/Makefile\n"
                                   "DEPENDS_LIB+= $(DEPENDS_DIR)/pcm/libPCM.a\n"
                                   "INC+= -I $(DEPENDS_DIR)/pcm\n"
                                   "LIB_DIRS+= -L $(DEPENDS_DIR)/pcm\n"
                                   "#Need an additional include directory if on MacOS.\n"
                                   "#Using the technique in pcm makefile to detect MacOS\n"
                                   "UNAME:=$(shell uname)\n"
                                   "ifeq ($(UNAME), Darwin)\n"
                                   "INC+= -I $(DEPENDS_DIR)/pcm/MacMSRDriver\n"
                                   "LIB+= -lPCM -lPcmMsr\n"
                                   "APPLE_DRIVER = $(DEPENDS_DIR)/pcm/MacMSRDriver/build/Release/libPcmMsr.dylib\n"
                                   "LIB_DIRS+= -L $(DEPENDS_DIR)/pcm/MacMSRDriver/build/Release\n"
                                   "else\n"
                                   "LIB+= -lrt -lPCM\n"
                                   "APPLE_DRIVER = \n"
                                   "endif\n"
                                   "else\n"
                                   "DEFINES+= -DUSE_PCM=0\n"
                                   "endif\n"
                                   "\n"
                                   "ifeq ($(USE_AMDuPROF), 1)\n"
                                   "DEFINES+= -DUSE_AMDuPROF=1\n"
                                   "LIB+= -lAMDPowerProfileAPI\n"
                                   "else\n"
                                   "DEFINES+= -DUSE_AMDuPROF=0\n"
                                   "endif\n"
                                   "\n"
                                   "MAIN_FILE = benchmark_throughput_test.cpp\n"
                                   "LIB_SRCS = " + fileName + "_benchmark_driver.cpp #These files are not optimized. micro_bench calls the kernel runner (which starts the timers by calling functions in the profilers).  Re-ordering code is not desired\n"
                                   "SYSTEM_SRC = " + fileName + ".c\n"
                                   "KERNEL_SRCS = " + fileName + "_benchmark_kernel.cpp\n"
                                   "KERNEL_NO_OPT_SRCS = \n"
                                   "\n"
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
                                   "benchmark_" + fileName + "_const: $(OBJS) $(SYSTEM_OBJS) $(LIB_OBJS) $(KERNEL_OBJS) $(KERNEL_NO_OPT_OBJS) $(DEPENDS_LIB) \n"
                                   "\t$(CXX) $(INC) $(LIB_DIRS) -o benchmark_" + fileName + "_const $(OBJS) $(SYSTEM_OBJS) $(LIB_OBJS) $(KERNEL_OBJS) $(KERNEL_NO_OPT_OBJS) $(LIB)\n"
                                   "\n"
                                   "$(KERNEL_NO_OPT_OBJS): $(BUILD_DIR)/%.o : $(SRC_DIR)/%.cpp | $(BUILD_DIR)/ $(DEPENDS)\n"
                                   "\t$(CXX) $(KERNEL_NO_OPT_CFLAGS) $(INC) $(DEFINES) -o $@ $<\n"
                                   "\n"
                                   "$(SYSTEM_OBJS): $(BUILD_DIR)/%.o : $(LIB_DIR)/%.c | $(BUILD_DIR)/ $(DEPENDS)\n"
                                   "\t$(CXX) $(SYSTEM_CFLAGS) $(INC) $(DEFINES) -o $@ $<\n"
                                   "\n"
                                   "$(KERNEL_OBJS): $(BUILD_DIR)/%.o : $(LIB_DIR)/%.cpp | $(BUILD_DIR)/ $(DEPENDS)\n"
                                   "\t$(CXX) $(KERNEL_CFLAGS) $(INC) $(DEFINES) -o $@ $<\n"
                                   "\n"
                                   "$(LIB_OBJS): $(BUILD_DIR)/%.o : $(LIB_DIR)/%.cpp | $(BUILD_DIR)/ $(DEPENDS)\n"
                                   "\t$(CXX) $(CFLAGS) $(INC) $(DEFINES) -o $@ $<\n"
                                   "\n"
                                   "$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp | $(BUILD_DIR)/ $(DEPENDS)\n"
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
                                   "$(COMMON_DIR)/libProfilerCommon.a:\n"
                                   "\tcd $(COMMON_DIR); make USE_PCM=$(USE_PCM) USE_AMDuPROF=$(USE_AMDuPROF)\n"
                                   "\t\n"
                                   "clean:\n"
                                   "\trm -f benchmark_" + fileName + "_const\n"
                                   "\trm -rf build/*\n"
                                   "\n"
                                   ".PHONY: clean\n";

    std::ofstream makefile;
    makefile.open(path+"/Makefile_" + fileName + "_const", std::ofstream::out | std::ofstream::trunc);
    makefile << makefileContent;
    makefile.close();
}

void Design::emitSingleThreadedCBenchmarkingDriverMem(std::string path, std::string fileName, std::string designName, int blockSize) {
    std::string fileNameUpper =  GeneralHelper::toUpper(fileName);

    //#### Emit Kernel File & Header (Memory)
    std::ofstream benchKernelMem;
    benchKernelMem.open(path+"/"+fileName+"_benchmark_kernel_mem.cpp", std::ofstream::out | std::ofstream::trunc);

    benchKernelMem << "#include \"" << fileName << ".h\"" << std::endl;
    benchKernelMem << "#include \"intrin_bench_default_defines.h\"" << std::endl;
    benchKernelMem << "void bench_"+fileName+"_mem(void** in, void** out)" << std::endl;
    benchKernelMem << "{\n";

    //Cast the input and output arrays to the correct type
    std::vector<Variable> inputVars = getCInputVariables();
    unsigned long numInputVars = inputVars.size();
    int inCur = 0; //This is used to track the current index in the input array since it may not allign with i (due to mixed real and imag inputs)
    for(unsigned long i = 0; i<numInputVars; i++){
        std::string inputDTStr = inputVars[i].getDataType().toString(DataType::StringStyle::C, false, false);
        benchKernelMem << "\t" << inputDTStr << "* " << inputVars[i].getCVarName(false) << " = (" << inputDTStr << "*) in[" << inCur << "];" << std::endl;
        inCur++;
        if(inputVars[i].getDataType().isComplex()){
            benchKernelMem << "\t" << inputDTStr << "* " << inputVars[i].getCVarName(true) << " = (" << inputDTStr << "*) in[" << inCur << "];" << std::endl;
            inCur++;
        }
    }

    //Cast output array
    benchKernelMem << "\tOutputType* outArr = (OutputType*) out[0];" << std::endl;

    //Generate loop
    benchKernelMem << "\tunsigned long outputCount;\n";
    if(blockSize>1){
        benchKernelMem << "\tint iterLimit = STIM_LEN/" << blockSize << ";\n";
    }else{
        benchKernelMem << "\tint iterLimit = STIM_LEN;\n";
    }
    benchKernelMem << "\tfor(int i = 0; i<iterLimit; i++)\n";
    benchKernelMem << "\t{\n";

    if(blockSize>1){
        benchKernelMem << "\t\tint j = i*" << blockSize << ";\n";
    }

    benchKernelMem << "\t\t" << designName << "(";
    for(unsigned long i = 0; i<numInputVars; i++){
        if(i > 0){
            benchKernelMem << ", ";
        }
        benchKernelMem << inputVars[i].getCVarName(false);
        if(blockSize>1){
            benchKernelMem << "+j";
        }else{
            benchKernelMem << "[i]";
        }
        if(inputVars[i].getDataType().isComplex()){
            benchKernelMem << ", " << inputVars[i].getCVarName(true);
            if(blockSize>1){
                benchKernelMem << "+j";
            }else{
                benchKernelMem << "[i]";
            }
        }
    }
    if(numInputVars>=1){
        benchKernelMem << ", ";
    }
    benchKernelMem << "outArr+i, &outputCount);" << std::endl;

    benchKernelMem << "\t}" << std::endl;
    benchKernelMem << "}" << std::endl;
    benchKernelMem.close();

    std::ofstream benchKernelHeaderMem;
    benchKernelHeaderMem.open(path+"/"+fileName+"_benchmark_kernel_mem.h", std::ofstream::out | std::ofstream::trunc);

    benchKernelHeaderMem << "#ifndef " << fileNameUpper << "_BENCHMARK_KERNEL_H" << std::endl;
    benchKernelHeaderMem << "#define " << fileNameUpper << "_BENCHMARK_KERNEL_H" << std::endl;
    benchKernelHeaderMem << "void bench_"+fileName+"_mem(void** in, void** out);" << std::endl;
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
    benchMemDriver << "std::map<std::string, std::map<std::string, Results*>*> runBenchSuite(Profiler* profiler, int* cpu_num_int){\n"
                      "\tstd::map<std::string, std::map<std::string, Results*>*> kernel_results;\n"
                      "\n"
                      "\tstd::map<std::string, Results*>* type_result = new std::map<std::string, Results*>;\n";

    //Create vectors for input, output sizing
    std::string inSizeStr = "";
    std::string inElementsStr = "";
    for(unsigned long i = 0; i<numInputVars; i++){
        if(i > 0){
            inSizeStr += ", ";
            inElementsStr += ", ";
        }
        DataType dt = inputVars[i].getDataType().getCPUStorageType();
        inSizeStr += GeneralHelper::to_string(dt.getTotalBits()/8);
        inElementsStr += "STIM_LEN";

        if(dt.isComplex()){
            inSizeStr += ", " + GeneralHelper::to_string(dt.getTotalBits()/8);
            inElementsStr += ", STIM_LEN";
        }
    }

    benchMemDriver << "\tstd::vector<int> inSizes = {" << inSizeStr << "};" << std::endl;
    benchMemDriver << "\tstd::vector<int> numInElements = {" << inElementsStr << "};" << std::endl;

    std::string outputEntriesStr;
    if(blockSize>1) {
        outputEntriesStr = "STIM_LEN/" + GeneralHelper::to_string(blockSize);
    }else{
        outputEntriesStr = "STIM_LEN";
    }

    benchMemDriver << "\tstd::vector<int> outSizes = {sizeof(OutputType)};" << std::endl;
    benchMemDriver << "\tstd::vector<int> numOutElements = {" << outputEntriesStr << "};" <<std::endl;

    benchMemDriver << "\tResults* result = load_store_arb_init_kernel<__m256>(profiler, &bench_" + fileName + "_mem, "
                      "inSizes, outSizes, numInElements, numOutElements, "
                      "*cpu_num_int, \"===== Generated System: " + designName + " =====\");\n"
                      "\t(*type_result)[\"" + typeStr + "\"] = result;\n"
                      "\tkernel_results[\"" + designName + "\"] = type_result;\n"
                      "\tprintf(\"\\n\");\n"
                      "\treturn kernel_results;\n}" << std::endl;

    //==== Generate init ====
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

    benchMemDriver << "void initInput(void* in, unsigned long index){" << std::endl;

    //Cast the input and output arrays to the correct type
    benchMemDriver << "\tvoid** in_cast = (void**) in;" << std::endl;
    inCur = 0;
    for(unsigned long i = 0; i<numInputVars; i++){
        std::string inputDTStr = inputVars[i].getDataType().toString(DataType::StringStyle::C, false, false);
        benchMemDriver << "\t" << inputDTStr << "* " << inputVars[i].getCVarName(false) << " = (" << inputDTStr << "*) in_cast[" << inCur << "];" << std::endl;
        inCur++;
        if(inputVars[i].getDataType().isComplex()){
            benchMemDriver << "\t" << inputDTStr << "* " << inputVars[i].getCVarName(true) << " = (" << inputDTStr << "*) in_cast[" << inCur << "];" << std::endl;
            inCur++;
        }
    }

    for(unsigned long i = 0; i<numInputVars; i++){
        benchMemDriver << "\t" << inputVars[i].getCVarName(false) << "[index] = " << defaultArgs[i].toStringComponent(false, inputVars[i].getDataType()) << ";" << std::endl;
        //Check if complex
        if(inputVars[i].getDataType().isComplex()) {
            benchMemDriver << "\t" << inputVars[i].getCVarName(true) << "[index] = " << defaultArgs[i].toStringComponent(true, inputVars[i].getDataType()) << ";" << std::endl;
        }
    }
    benchMemDriver << "}" << std::endl;

    benchMemDriver.close();

    //#### Emit Makefiles ####
    std::string makefileContent =  "BUILD_DIR=build\n"
                                   "DEPENDS_DIR=./depends\n"
                                   "COMMON_DIR=./common\n"
                                   "SRC_DIR=./intrin\n"
                                   "LIB_DIR=.\n"
                                   "\n"
                                   "#Parameters that can be supplied to this makefile\n"
                                   "USE_PCM=1\n"
                                   "USE_AMDuPROF=1\n"
                                   "\n"
                                   "UNAME:=$(shell uname)\n"
                                   "\n"
                                   "#Compiler Parameters\n"
                                   "#CXX=g++\n"
                                   "#Main Benchmark file is not optomized to avoid timing code being re-organized\n"
                                   "CFLAGS = -O0 -c -g -std=c++11 -march=native -masm=att\n"
                                   "#Generated system should be allowed to optomize - reintepret as c++ file\n"
                                   "SYSTEM_CFLAGS = -O3 -c -g -x c++ -std=c++11 -march=native -masm=att\n"
                                   "#Most kernels are allowed to be optomized.  Most assembly kernels use asm 'volitile' to force execution\n"
                                   "KERNEL_CFLAGS = -O3 -c -g -std=c++11 -march=native -masm=att\n"
                                   "#For kernels that should not be optimized, the following is used\n"
                                   "KERNEL_NO_OPT_CFLAGS = -O0 -c -g -std=c++11 -march=native -masm=att\n"
                                   "INC=-I $(COMMON_DIR) -I $(SRC_DIR)\n"
                                   "LIB_DIRS=-L $(COMMON_DIR)\n"
                                   "LIB=-pthread -lProfilerCommon\n"
                                   "\n"
                                   "DEFINES=\n"
                                   "\n"
                                   "DEPENDS=\n"
                                   "DEPENDS_LIB=$(COMMON_DIR)/libProfilerCommon.a\n"
                                   "\n"
                                   "ifeq ($(USE_PCM), 1)\n"
                                   "DEFINES+= -DUSE_PCM=1\n"
                                   "DEPENDS+= $(DEPENDS_DIR)/pcm/Makefile\n"
                                   "DEPENDS_LIB+= $(DEPENDS_DIR)/pcm/libPCM.a\n"
                                   "INC+= -I $(DEPENDS_DIR)/pcm\n"
                                   "LIB_DIRS+= -L $(DEPENDS_DIR)/pcm\n"
                                   "#Need an additional include directory if on MacOS.\n"
                                   "#Using the technique in pcm makefile to detect MacOS\n"
                                   "UNAME:=$(shell uname)\n"
                                   "ifeq ($(UNAME), Darwin)\n"
                                   "INC+= -I $(DEPENDS_DIR)/pcm/MacMSRDriver\n"
                                   "LIB+= -lPCM -lPcmMsr\n"
                                   "APPLE_DRIVER = $(DEPENDS_DIR)/pcm/MacMSRDriver/build/Release/libPcmMsr.dylib\n"
                                   "LIB_DIRS+= -L $(DEPENDS_DIR)/pcm/MacMSRDriver/build/Release\n"
                                   "else\n"
                                   "LIB+= -lrt -lPCM\n"
                                   "APPLE_DRIVER = \n"
                                   "endif\n"
                                   "else\n"
                                   "DEFINES+= -DUSE_PCM=0\n"
                                   "endif\n"
                                   "\n"
                                   "ifeq ($(USE_AMDuPROF), 1)\n"
                                   "DEFINES+= -DUSE_AMDuPROF=1\n"
                                   "LIB+= -lAMDPowerProfileAPI\n"
                                   "else\n"
                                   "DEFINES+= -DUSE_AMDuPROF=0\n"
                                   "endif\n"
                                   "\n"
                                   "MAIN_FILE = benchmark_throughput_test.cpp\n"
                                   "LIB_SRCS = " + fileName + "_benchmark_driver_mem.cpp #These files are not optimized. micro_bench calls the kernel runner (which starts the timers by calling functions in the profilers).  Re-ordering code is not desired\n"
                                   "SYSTEM_SRC = " + fileName + ".c\n"
                                   "KERNEL_SRCS = " + fileName + "_benchmark_kernel_mem.cpp\n"
                                   "KERNEL_NO_OPT_SRCS = \n"
                                   "\n"
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
                                   "benchmark_" + fileName + "_mem: $(OBJS) $(SYSTEM_OBJS) $(LIB_OBJS) $(KERNEL_OBJS) $(KERNEL_NO_OPT_OBJS) $(DEPENDS_LIB) \n"
                                   "\t$(CXX) $(INC) $(LIB_DIRS) -o benchmark_" + fileName + "_mem $(OBJS) $(SYSTEM_OBJS) $(LIB_OBJS) $(KERNEL_OBJS) $(KERNEL_NO_OPT_OBJS) $(LIB)\n"
                                   "\n"
                                   "$(KERNEL_NO_OPT_OBJS): $(BUILD_DIR)/%.o : $(SRC_DIR)/%.cpp | $(BUILD_DIR)/ $(DEPENDS)\n"
                                   "\t$(CXX) $(KERNEL_NO_OPT_CFLAGS) $(INC) $(DEFINES) -o $@ $<\n"
                                   "\n"
                                   "$(SYSTEM_OBJS): $(BUILD_DIR)/%.o : $(LIB_DIR)/%.c | $(BUILD_DIR)/ $(DEPENDS)\n"
                                   "\t$(CXX) $(SYSTEM_CFLAGS) $(INC) $(DEFINES) -o $@ $<\n"
                                   "\n"
                                   "$(KERNEL_OBJS): $(BUILD_DIR)/%.o : $(LIB_DIR)/%.cpp | $(BUILD_DIR)/ $(DEPENDS)\n"
                                   "\t$(CXX) $(KERNEL_CFLAGS) $(INC) $(DEFINES) -o $@ $<\n"
                                   "\n"
                                   "$(LIB_OBJS): $(BUILD_DIR)/%.o : $(LIB_DIR)/%.cpp | $(BUILD_DIR)/ $(DEPENDS)\n"
                                   "\t$(CXX) $(CFLAGS) $(INC) $(DEFINES) -o $@ $<\n"
                                   "\n"
                                   "$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp | $(BUILD_DIR)/ $(DEPENDS)\n"
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
                                   "$(COMMON_DIR)/libProfilerCommon.a:\n"
                                   "\tcd $(COMMON_DIR); make USE_PCM=$(USE_PCM) USE_AMDuPROF=$(USE_AMDuPROF)\n"
                                   "\t\n"
                                   "clean:\n"
                                   "\trm -f benchmark_" + fileName + "_mem\n"
                                   "\trm -rf build/*\n"
                                   "\n"
                                   ".PHONY: clean\n";

    std::ofstream makefile;
    makefile.open(path+"/Makefile_" + fileName + "_mem", std::ofstream::out | std::ofstream::trunc);
    makefile << makefileContent;
    makefile.close();
}

Design Design::copyGraph(std::map<std::shared_ptr<Node>, std::shared_ptr<Node>> &origToCopyNode,
                         std::map<std::shared_ptr<Node>, std::shared_ptr<Node>> &copyToOrigNode,
                         std::map<std::shared_ptr<Arc>, std::shared_ptr<Arc>> &origToCopyArc,
                         std::map<std::shared_ptr<Arc>, std::shared_ptr<Arc>> &copyToOrigArc) {
    Design designCopy;

    //==== Create new master nodes and add them to the node copies list and maps ====
    designCopy.inputMaster = std::dynamic_pointer_cast<MasterInput>(inputMaster->shallowClone(nullptr));
    designCopy.inputMaster->copyPortNames(inputMaster);
    designCopy.outputMaster = std::dynamic_pointer_cast<MasterOutput>(outputMaster->shallowClone(nullptr));
    designCopy.outputMaster->copyPortNames(outputMaster);
    designCopy.visMaster = std::dynamic_pointer_cast<MasterOutput>(visMaster->shallowClone(nullptr));
    designCopy.visMaster->copyPortNames(visMaster);
    designCopy.unconnectedMaster = std::dynamic_pointer_cast<MasterUnconnected>(unconnectedMaster->shallowClone(nullptr));
    designCopy.unconnectedMaster->copyPortNames(unconnectedMaster);
    designCopy.terminatorMaster = std::dynamic_pointer_cast<MasterOutput>(terminatorMaster->shallowClone(nullptr));
    designCopy.terminatorMaster->copyPortNames(terminatorMaster);

    origToCopyNode[inputMaster] = designCopy.inputMaster;
    origToCopyNode[outputMaster] = designCopy.outputMaster;
    origToCopyNode[visMaster] = designCopy.visMaster;
    origToCopyNode[unconnectedMaster] = designCopy.unconnectedMaster;
    origToCopyNode[terminatorMaster] = designCopy.terminatorMaster;

    copyToOrigNode[designCopy.inputMaster] = inputMaster;
    copyToOrigNode[designCopy.outputMaster] = outputMaster;
    copyToOrigNode[designCopy.visMaster] = visMaster;
    copyToOrigNode[designCopy.unconnectedMaster] = unconnectedMaster;
    copyToOrigNode[designCopy.terminatorMaster] = terminatorMaster;

    //==== Copy nodes ====
    std::vector<std::shared_ptr<Node>> nodeCopies;

    //Emit nodes hierarchically starting from the top level nodes (so that hierarchy is preserved)
    unsigned long numTopLevelNodes = topLevelNodes.size();
    for (unsigned long i = 0; i < numTopLevelNodes; i++) {
        topLevelNodes[i]->shallowCloneWithChildren(nullptr, nodeCopies, origToCopyNode, copyToOrigNode);
    }

    designCopy.nodes=nodeCopies;

//    //Copy the node list in the same order
//    std::vector<std::shared_ptr<Node>> nodeCopiesOrdered;
//    for(unsigned long i = 0; i<nodes.size(); i++){
//        if(origToCopyNode.find(nodes[i]) == origToCopyNode.end()){
//            throw std::runtime_error("While cloning design, found an node that has no clone");
//        }
//
//        nodeCopiesOrdered.push_back(origToCopyNode[nodes[i]]);
//    }
//    designCopy.nodes=nodeCopiesOrdered;

    //==== Set StateUpdate nodes and contexts for any node, primary node pointers for any StateUpdate nodes, and context node vectors for ContextRoots
    for(unsigned long i = 0; i<nodeCopies.size(); i++){
        //General Nodes
        //Copy context stack
        std::shared_ptr<Node> origNode = copyToOrigNode[nodeCopies[i]];
        std::vector<Context> origContext = origNode->getContext();
        std::vector<Context> newContext;
        for(unsigned long j = 0; j<origContext.size(); j++){
            std::shared_ptr<ContextRoot> origContextRoot = origContext[j].getContextRoot();
            std::shared_ptr<Node> origContextRootAsNode = GeneralHelper::isType<ContextRoot, Node>(origContextRoot);
            if(origContextRootAsNode){
                std::shared_ptr<Node> newContextRoot = origToCopyNode[origContextRootAsNode];

                //TODO: Refactor ContextRoot inheritance
                std::shared_ptr<ContextRoot> newContextRootAsContextRoot = GeneralHelper::isType<Node, ContextRoot>(newContextRoot);
                if(newContextRoot){
                    newContext.push_back(Context(newContextRootAsContextRoot, origContext[j].getSubContext()));
                }else{
                    throw std::runtime_error("Node encountered durring Graph Copy which was expected to be a context root");
                }
            }else{
                throw std::runtime_error("ContextRoot encountered which is not a Node during Graph Copy");
            }
        }
        nodeCopies[i]->setContext(newContext);

        //Copy StateUpdate (if not null)
        if(origNode->getStateUpdateNode() != nullptr){
            std::shared_ptr<Node> stateUpdateNode = origToCopyNode[origNode->getStateUpdateNode()];
            std::shared_ptr<StateUpdate> stateUpdateNodeAsStateUpdate = GeneralHelper::isType<Node, StateUpdate>(stateUpdateNode);

            if(stateUpdateNodeAsStateUpdate) {
                nodeCopies[i]->setStateUpdateNode(stateUpdateNodeAsStateUpdate);
            }else{
                throw std::runtime_error("Node encountered durring Graph Copy which was expected to be a StateUpdate");
            }
        }

        if(GeneralHelper::isType<Node, StateUpdate>(nodeCopies[i]) != nullptr){
            //StateUpdates
            std::shared_ptr<StateUpdate> stateUpdateCopy = std::dynamic_pointer_cast<StateUpdate>(nodeCopies[i]);
            std::shared_ptr<StateUpdate> stateUpdateOrig = std::dynamic_pointer_cast<StateUpdate>(copyToOrigNode[stateUpdateCopy]);

            if(stateUpdateOrig->getPrimaryNode() != nullptr){
                stateUpdateCopy->setPrimaryNode(origToCopyNode[stateUpdateOrig->getPrimaryNode()]);
            }else{
                stateUpdateCopy->setPrimaryNode(nullptr);
            }
        }

        if(GeneralHelper::isType<Node, ContextVariableUpdate>(nodeCopies[i]) != nullptr){
            //ContextVariableUpdate
            std::shared_ptr<ContextVariableUpdate> contextVariableUpdateCopy = std::dynamic_pointer_cast<ContextVariableUpdate>(nodeCopies[i]);
            std::shared_ptr<ContextVariableUpdate> contextVariableUpdateOrig = std::dynamic_pointer_cast<ContextVariableUpdate>(copyToOrigNode[contextVariableUpdateCopy]);

            if(contextVariableUpdateOrig->getContextRoot() != nullptr){
                //Fix diamond inheritance
                std::shared_ptr<Node> origContextRootAsNode = GeneralHelper::isType<ContextRoot, Node>(contextVariableUpdateOrig->getContextRoot());
                if(origContextRootAsNode == nullptr){
                    throw std::runtime_error("When cloning ContextVariableUpdate, could not cast a ContextRoot to a Node");
                }
                std::shared_ptr<Node> copyContextRootAsNode = origToCopyNode[origContextRootAsNode];

                std::shared_ptr<ContextRoot> copyContextRoot = GeneralHelper::isType<Node, ContextRoot>(copyContextRootAsNode);
                if(copyContextRoot == nullptr){
                    throw std::runtime_error("When cloning ContextVariableUpdate, could not cast a Node to a ContextRoot");
                }

                contextVariableUpdateCopy->setContextRoot(copyContextRoot);
            }else{
                contextVariableUpdateCopy->setContextRoot(nullptr);
            }
        }

        if(GeneralHelper::isType<Node, ContextRoot>(nodeCopies[i]) != nullptr){
            //ContextRoots
            std::shared_ptr<ContextRoot> nodeCopyAsContextRoot = std::dynamic_pointer_cast<ContextRoot>(nodeCopies[i]); //Don't think you can cross cast with a static cast TODO: check

            //TODO: Refactor ContextRoot inheritance
            std::shared_ptr<ContextRoot> origNodeAsContextRoot = GeneralHelper::isType<Node, ContextRoot>(origNode);

            if(origNodeAsContextRoot){

                //Copy ContextFamilyContainerMap
                std::map<int, std::shared_ptr<ContextFamilyContainer>> origContextFamilyContainerMap = origNodeAsContextRoot->getContextFamilyContainers();
                std::map<int, std::shared_ptr<ContextFamilyContainer>> copyContextFamilyContainerMap;
                for(auto it = origContextFamilyContainerMap.begin(); it != origContextFamilyContainerMap.end(); it++){
                    std::shared_ptr<Node> copyContextFamilyContainerAsNode = origToCopyNode[it->second];
                    std::shared_ptr<ContextFamilyContainer> copyContextFamilyContainer = GeneralHelper::isType<Node, ContextFamilyContainer>(copyContextFamilyContainerAsNode);
                    copyContextFamilyContainerMap[it->first] = copyContextFamilyContainer;
                }
                nodeCopyAsContextRoot->setContextFamilyContainers(copyContextFamilyContainerMap);

                //Copy SubContextNodes
                int numSubContexts = nodeCopyAsContextRoot->getNumSubContexts();
                for(int j = 0; j<numSubContexts; j++){
                    std::vector<std::shared_ptr<Node>> origContextNodes = origNodeAsContextRoot->getSubContextNodes(j);

                    for(unsigned long k = 0; k<origContextNodes.size(); k++){
                        nodeCopyAsContextRoot->addSubContextNode(j, origToCopyNode[origContextNodes[k]]);
                    }
                }
            }else{
                throw std::runtime_error("Node encountered durring Graph Copy which was expected to be a ContextRoot");
            }

        }

        if(GeneralHelper::isType<Node, ContextFamilyContainer>(nodeCopies[i]) != nullptr){
            //ContextFamilyContainer
            std::shared_ptr<ContextFamilyContainer> contextFamilyContainerCopy = std::dynamic_pointer_cast<ContextFamilyContainer>(nodeCopies[i]);
            std::shared_ptr<ContextFamilyContainer> contextFamilyContainerOrig = std::dynamic_pointer_cast<ContextFamilyContainer>(copyToOrigNode[contextFamilyContainerCopy]);

            //translate the ContextRoot pointer
            if(contextFamilyContainerOrig->getContextRoot() != nullptr){
                //Fix diamond inheritance
                std::shared_ptr<Node> origContextRootAsNode = GeneralHelper::isType<ContextRoot, Node>(contextFamilyContainerOrig->getContextRoot());
                if(origContextRootAsNode == nullptr){
                    throw std::runtime_error("When cloning ContextFamilyContainer, could not cast a ContextRoot to a Node");
                }
                std::shared_ptr<Node> copyContextRootAsNode = origToCopyNode[origContextRootAsNode];

                std::shared_ptr<ContextRoot> copyContextRoot = GeneralHelper::isType<Node, ContextRoot>(copyContextRootAsNode);
                if(copyContextRoot == nullptr){
                    throw std::runtime_error("When cloning ContextFamilyContainer, could not cast a Node to a ContextRoot");
                }

                contextFamilyContainerCopy->setContextRoot(copyContextRoot);
            }else{
                contextFamilyContainerCopy->setContextRoot(nullptr);
            }

            //subContextContainers are handled in the clone method
        }
    }

    //==== Copy arcs ====
    std::vector<std::shared_ptr<Arc>> arcCopies;

    //Iterate through the list of nodes and copy input arcs
    //All arcs should be copied because even arcs from sources should be connected to another node or to the unconnected master node
    unsigned long numNodes = nodes.size();
    for (unsigned long i = 0; i < numNodes; i++) {
        nodes[i]->cloneInputArcs(arcCopies, origToCopyNode, origToCopyArc, copyToOrigArc);
    }

    //copy input arcs into master nodes
    outputMaster->cloneInputArcs(arcCopies, origToCopyNode, origToCopyArc, copyToOrigArc);
    visMaster->cloneInputArcs(arcCopies, origToCopyNode, origToCopyArc, copyToOrigArc);
    unconnectedMaster->cloneInputArcs(arcCopies, origToCopyNode, origToCopyArc, copyToOrigArc);
    terminatorMaster->cloneInputArcs(arcCopies, origToCopyNode, origToCopyArc, copyToOrigArc);

//    designCopy.arcs = arcCopies;

    //Copy the arc list in the same order
    std::vector<std::shared_ptr<Arc>> arcCopiesOrdered;
    for(unsigned long i = 0; i<arcs.size(); i++){
        if(origToCopyArc.find(arcs[i]) == origToCopyArc.end()){
            throw std::runtime_error("While cloning design, found an arc that has no clone");
        }

        arcCopiesOrdered.push_back(origToCopyArc[arcs[i]]);
    }

    designCopy.arcs = arcCopiesOrdered;

    //Copy topLevelNode entries
    for(unsigned long i = 0; i<numTopLevelNodes; i++){
        designCopy.topLevelNodes.push_back(origToCopyNode[topLevelNodes[i]]);
    }

    return designCopy;
}

void Design::removeNode(std::shared_ptr<Node> node) {
    //Disconnect arcs from the node
    std::set<std::shared_ptr<Arc>> disconnectedArcs = node->disconnectNode();

    //Check if the node is a master node
    if(GeneralHelper::isType<Node, MasterNode>(node) != nullptr){
        //Master Node
        if(node == inputMaster){
            inputMaster = nullptr;
        }else if(node == outputMaster){
            outputMaster = nullptr;
        }else if(node == visMaster){
            visMaster = nullptr;
        }else if(node == unconnectedMaster){
            unconnectedMaster = nullptr;
        }else if(node == terminatorMaster){
            terminatorMaster = nullptr;
        }else{
            throw std::runtime_error("Removing an unexpected master node: " + node->getName());
        }
    }else{
        //Remove the node from the node array (and the top level nodes array if applicable)
        nodes.erase(std::remove(nodes.begin(), nodes.end(), node));
        nodes.erase(std::remove(topLevelNodes.begin(), topLevelNodes.end(), node));
    }

    //Remove the arcs
    for(auto it = disconnectedArcs.begin(); it != disconnectedArcs.end(); it++){
        std::shared_ptr<Arc> arcToDelete = *it;
        arcs.erase(std::remove(arcs.begin(), arcs.end(), arcToDelete));
    }
}

unsigned long Design::prune(bool includeVisMaster) {
    //TODO: Check connected components to make sure that there is a link to an output.  If not, prune connected component.  This eliminates cycles that do no useful work.

    //Want sets to have a consistent ordering across runs.  Therefore, will not use pointer address for set comparison

    //Find nodes with 0 out-degree when not counting the various master nodes
    std::set<std::shared_ptr<Node>> nodesToIgnore; //This can be a standard set as order is unimportant for checking against the ignore set
    nodesToIgnore.insert(unconnectedMaster);
    nodesToIgnore.insert(terminatorMaster);
    if(includeVisMaster){
        nodesToIgnore.insert(visMaster);
    }

    std::set<std::shared_ptr<Node>, Node::PtrID_Compare> nodesWithZeroOutDeg;
    for(unsigned long i = 0; i<nodes.size(); i++){
        //Do not remove subsystems or state updates (they will have outdeg 0)
        if(GeneralHelper::isType<Node, SubSystem>(nodes[i]) == nullptr && GeneralHelper::isType<Node, StateUpdate>(nodes[i]) == nullptr ) {
            if (nodes[i]->outDegreeExclusingConnectionsTo(nodesToIgnore) == 0) {
                nodesWithZeroOutDeg.insert(nodes[i]);
            }
        }
    }

    //Get the candidate nodes which may have out-deg 0 after removing the nodes
    //Note that all of the connected nodes have zero out degree so connected nodes are either connected to the input ports or are the master nodes
    std::set<std::shared_ptr<Node>, Node::PtrID_Compare> candidateNodes;
    for(auto it = nodesWithZeroOutDeg.begin(); it != nodesWithZeroOutDeg.end(); it++){
        std::set<std::shared_ptr<Node>> moreCandidates = (*it)->getConnectedNodes(); //Should be ok if this is ordered by ptr since it is being inserted into the set ordered by ID
        candidateNodes.insert(moreCandidates.begin(), moreCandidates.end());
    }

    //Remove the master nodes from the candidate list as well as any nodes that are about to be removed
    candidateNodes.erase(unconnectedMaster);
    candidateNodes.erase(terminatorMaster);
    candidateNodes.erase(visMaster);
    candidateNodes.erase(inputMaster);
    candidateNodes.erase(outputMaster);

    //Erase
    std::vector<std::shared_ptr<Node>> nodesDeleted;
    std::vector<std::shared_ptr<Arc>> arcsDeleted;

//    for(auto it = nodesWithZeroOutDeg.begin(); it != nodesWithZeroOutDeg.end(); it++){
//        std::cout << "Node with Indeg 0: " <<  (*it)->getFullyQualifiedName() << std::endl;
//    }

    while(!nodesWithZeroOutDeg.empty()){
        //Disconnect, erase nodes, remove from candidate set (if it is included)
        for(auto it = nodesWithZeroOutDeg.begin(); it != nodesWithZeroOutDeg.end(); it++){
            std::set<std::shared_ptr<Arc>> arcsToRemove = (*it)->disconnectNode();
            //Check for the corner case that the node is an enabled input or output
            //If so, it needs to be removed from the parent's enabled port lists
            std::shared_ptr<EnableInput> asEnableInput = GeneralHelper::isType<Node, EnableInput>(*it);
            if(asEnableInput){
                std::shared_ptr<EnabledSubSystem> parentAsEnabledSubsystem = GeneralHelper::isType<Node, EnabledSubSystem>(asEnableInput->getParent());
                if(parentAsEnabledSubsystem){
                    parentAsEnabledSubsystem->removeEnableInput(asEnableInput);
                }else{
                    throw std::runtime_error(ErrorHelpers::genErrorStr("When pruning EnabledInput node, could not remove node from EnableNode lists of parent", (*it)->getSharedPointer()));
                }
            }else{
                std::shared_ptr<EnableOutput> asEnableOutput = GeneralHelper::isType<Node, EnableOutput>(*it);
                if(asEnableOutput){
                    std::shared_ptr<EnabledSubSystem> parentAsEnabledSubsystem = GeneralHelper::isType<Node, EnabledSubSystem>(asEnableOutput->getParent());
                    if(parentAsEnabledSubsystem){
                        parentAsEnabledSubsystem->removeEnableOutput(asEnableOutput);
                    }else{
                        throw std::runtime_error(ErrorHelpers::genErrorStr("When pruning EnabledOutput node, could not remove node from EnableNode lists of parent", (*it)->getSharedPointer()));
                    }
                }
            }

            arcsDeleted.insert(arcsDeleted.end(), arcsToRemove.begin(), arcsToRemove.end());
            nodesDeleted.push_back(*it);
            candidateNodes.erase(*it);
        }

        //Reset nodes with zero out degree
        nodesWithZeroOutDeg.clear();

        //Find nodes with zero out degree from candidates list
        for(auto it = candidateNodes.begin(); it != candidateNodes.end(); it++){
            if((*it)->outDegreeExclusingConnectionsTo(nodesToIgnore) == 0){
                nodesWithZeroOutDeg.insert(*it);
            }
        }

        //Update candidates list
        for(auto it = nodesWithZeroOutDeg.begin(); it != nodesWithZeroOutDeg.end(); it++){
            std::set<std::shared_ptr<Node>> newCandidates = (*it)->getConnectedNodes();
            candidateNodes.insert(newCandidates.begin(), newCandidates.end());
        }

        //Remove master nodes from candidates list
        candidateNodes.erase(unconnectedMaster);
        candidateNodes.erase(terminatorMaster);
        candidateNodes.erase(visMaster);
        candidateNodes.erase(inputMaster);
        candidateNodes.erase(outputMaster);
    }

    //Delete nodes and arcs from design
    for(auto it = nodesDeleted.begin(); it != nodesDeleted.end(); it++){
        std::shared_ptr<Node> nodeToDelete = *it;
        std::cout << "Pruned Node: " << nodeToDelete->getFullyQualifiedName(true) << " [ID: " << nodeToDelete->getId() << "]" << std::endl;
        //Remove the node from it's parent's children list (if it has a parent) -> this is important when the graph is traversed hierarchically)
        if(nodeToDelete->getParent()){
            nodeToDelete->getParent()->removeChild(nodeToDelete);
        }
        nodes.erase(std::remove(nodes.begin(), nodes.end(), nodeToDelete), nodes.end());
        topLevelNodes.erase(std::remove(topLevelNodes.begin(), topLevelNodes.end(), nodeToDelete), topLevelNodes.end()); //Also remove from top lvl node (if applicable)
    }

    for(auto it = arcsDeleted.begin(); it != arcsDeleted.end(); it++){
        arcs.erase(std::remove(arcs.begin(), arcs.end(), *it), arcs.end());
    }

    //Connect unconnected arcs to unconnected master node
    for(unsigned long i = 0; i<nodes.size(); i++){
        std::vector<std::shared_ptr<Arc>> newArcs =
        nodes[i]->connectUnconnectedPortsToNode(unconnectedMaster, unconnectedMaster, 0, 0);

        //Add newArcs to arc vector
        for(unsigned long j = 0; j<newArcs.size(); j++){
            arcs.push_back(newArcs[j]);
        }
    }

    return nodesDeleted.size();
}

void Design::verifyTopologicalOrder() {
    //First, check that the output is scheduled (so long as there are output arcs)
    if(outputMaster->inDegree() > 0){
        if(outputMaster->getSchedOrder() == -1){
            throw std::runtime_error("Topological Order Validation: Output was not scheduled even though the system does have outputs.");
        }
    }

    for(unsigned long i = 0; i<arcs.size(); i++){
        //Check if the src node is a constant or the input master node
        //If so, do not check the ordering
        std::shared_ptr<Node> srcNode = arcs[i]->getSrcPort()->getParent();

        //The one case that needs to be checked if the src has state is the StateUpdate node for the given state element
        //Otherwise the outputs of nodes with state are considered constants
        //Another exception is EnableOutput nodes which should be checked
        std::shared_ptr<Node> dstNode = arcs[i]->getDstPort()->getParent();
        std::shared_ptr<StateUpdate> dstNodeAsStateUpdate = GeneralHelper::isType<Node, StateUpdate>(dstNode);

        bool shouldCheck;
        if(srcNode != inputMaster && srcNode != unconnectedMaster && srcNode != terminatorMaster && GeneralHelper::isType<Node, Constant>(srcNode) == nullptr) {
            if (GeneralHelper::isType<Node, BlackBox>(srcNode) != nullptr) {
                std::shared_ptr<BlackBox> asBlackBox = std::dynamic_pointer_cast<BlackBox>(srcNode);

                //We need to treat black boxes a little differently.  They may have state but their outputs may not
                //be registered (could be cominationally dependent on the input signals).
                //We should only disconnect arcs to outputs that are registered.

                std::vector<int> registeredPortNumbers = asBlackBox->getRegisteredOutputPorts();

                shouldCheck = std::find(registeredPortNumbers.begin(), registeredPortNumbers.end(), arcs[i]->getSrcPort()->getPortNum()) == registeredPortNumbers.end(); //Check if output port is unregistered
            }else{
                shouldCheck = !srcNode->hasState() || (srcNode->hasState() && srcNode->hasCombinationalPath())
                        || (srcNode->hasState() && dstNodeAsStateUpdate != nullptr); //Check arcs going into state update nodes

            }
        }else{
            shouldCheck = false;
        }

        if(shouldCheck){
            //It is allowed for the destination node to have order -1 (ie. not emitted) but the reverse is not OK
            //The srcNode can only be -1 if the destination is also -1
            if (srcNode->getSchedOrder() == -1) {
                //Src node is unscheduled
                if (dstNode->getSchedOrder() != -1) {
                    //dst node is scheduled
                    throw std::runtime_error(
                            "Topological Order Validation: Src Node (" + srcNode->getFullyQualifiedName() +
                            ") [Sched Order: " + GeneralHelper::to_string(srcNode->getSchedOrder()) + ", ID: " +
                            GeneralHelper::to_string(srcNode->getId()) +
                            "] is Unscheduled but Dst Node (" + dstNode->getFullyQualifiedName() +
                            ") [Sched Order: " + GeneralHelper::to_string(dstNode->getSchedOrder()) + ", ID: " +
                            GeneralHelper::to_string(dstNode->getId()) + "] is Scheduled");
                }
                //otherwise, there is no error here as both nodes are unscheduled
            } else {
                //Src node is scheduled
                if (dstNode->getSchedOrder() != -1) {
                    //Dst node is scheduled
                    if (srcNode->getSchedOrder() >= dstNode->getSchedOrder()) {
                        throw std::runtime_error(
                                "Topological Order Validation: Src Node (" + srcNode->getFullyQualifiedName() +
                                ") [Sched Order: " + GeneralHelper::to_string(srcNode->getSchedOrder()) + ", ID: " +
                                GeneralHelper::to_string(srcNode->getId()) +
                                "] is not Scheduled before Dst Node (" + dstNode->getFullyQualifiedName() +
                                ") [Sched Order: " + GeneralHelper::to_string(dstNode->getSchedOrder()) + ", ID: " +
                                GeneralHelper::to_string(dstNode->getId()) + "]");
                    }
                }
                //Dst node unscheduled is OK
            }
        }
    }

    //For each node, check its context stack and make sure it is scheduled after the driver node
    for(unsigned long i = 0; i<nodes.size(); i++){
        if(nodes[i]->getSchedOrder() != -1) {

            std::vector<Context> context = nodes[i]->getContext();

            //Check if this is a context root, add its context to the context stack
            std::shared_ptr<ContextRoot> asContextRoot = GeneralHelper::isType<Node, ContextRoot>(nodes[i]);
            if(asContextRoot) {
                context.push_back(Context(asContextRoot, -1));
            }

            for(unsigned long j = 0; j < context.size(); j++){
                std::vector<std::shared_ptr<Arc>> drivers = context[j].getContextRoot()->getContextDecisionDriver();

                for(unsigned long k = 0; k < drivers.size(); k++){
                    std::shared_ptr<Node> driverSrc = drivers[k]->getSrcPort()->getParent();

                    //Only check if the context driver node is not a constant or a node with state (with the exception of
                    //Enabled output ports)

                    if(GeneralHelper::isType<Node, Constant>(driverSrc) == nullptr && (!driverSrc->hasState() || (driverSrc->hasState() && GeneralHelper::isType<Node, EnableOutput>(driverSrc) != nullptr))) {
                        if (driverSrc->getSchedOrder() >= nodes[i]->getSchedOrder()) {
                            throw std::runtime_error(
                                    "Topological Order Validation: Context Driver Node (" +
                                    driverSrc->getFullyQualifiedName() +
                                    ") [Sched Order: " + GeneralHelper::to_string(driverSrc->getSchedOrder()) +
                                    ", ID: " +
                                    GeneralHelper::to_string(driverSrc->getId()) +
                                    "] is not Scheduled before Context Node (" + nodes[i]->getFullyQualifiedName() +
                                    ") [Sched Order: " + GeneralHelper::to_string(nodes[i]->getSchedOrder()) +
                                    ", ID: " +
                                    GeneralHelper::to_string(nodes[i]->getId()) + "]");
                        }
                    }
                }
            }
        }
    }
}

std::vector<std::shared_ptr<Node>> Design::topologicalSortDestructive(std::string designName, std::string dir, TopologicalSortParameters params) {
    std::vector<std::shared_ptr<Arc>> arcsToDelete;
    std::vector<std::shared_ptr<Node>> topLevelContextNodes = GraphAlgs::findNodesStopAtContextFamilyContainers(topLevelNodes);
    topLevelContextNodes.push_back(outputMaster);

//    std::cout << "Emitting: " + dir + "/" + designName + "_sort_error_scheduleGraph.graphml" << std::endl;
//    GraphMLExporter::exportGraphML(dir + "/" + designName + "_sort_error_scheduleGraph.graphml", *this);

//    std::cerr << "Top Lvl Nodes to Sched:" << std::endl;
//    for(unsigned long i = 0; i<topLevelContextNodes.size(); i++){
//        std::cerr << topLevelContextNodes[i]->getFullyQualifiedName(true) << std::endl;
//    }

    std::vector<std::shared_ptr<Node>> sortedNodes;


    bool failed = false;
    std::exception err;
    try {
        sortedNodes = GraphAlgs::topologicalSortDestructive(params, topLevelContextNodes, arcsToDelete, outputMaster,
                                                            inputMaster, terminatorMaster, unconnectedMaster,
                                                            visMaster);
    }catch(const std::exception &e){
        failed = true;
        err = e;
    }

    //Delete the arcs
    //TODO: Remove this?  May not be needed for most applications.  We are probably going to distroy the design anyway
    for(auto it = arcsToDelete.begin(); it != arcsToDelete.end(); it++){
        arcs.erase(std::remove(arcs.begin(), arcs.end(), *it), arcs.end());
    }

    if(failed) {
        std::cout << "Emitting: " + dir + "/" + designName + "_sort_error_scheduleGraph.graphml" << std::endl;
        GraphMLExporter::exportGraphML(dir + "/" + designName + "_sort_error_scheduleGraph.graphml", *this);

        throw err;
    }

    return sortedNodes;
}

unsigned long Design::scheduleTopologicalStort(TopologicalSortParameters params, bool prune, bool rewireContexts, std::string designName, std::string dir, bool printNodeSched) {
    std::map<std::shared_ptr<Node>, std::shared_ptr<Node>> origToClonedNodes;
    std::map<std::shared_ptr<Node>, std::shared_ptr<Node>> clonedToOrigNodes;
    std::map<std::shared_ptr<Arc>, std::shared_ptr<Arc>> origToClonedArcs;
    std::map<std::shared_ptr<Arc>, std::shared_ptr<Arc>> clonedToOrigArcs;

    //Make a copy of the design to conduct the destructive topological sort on
    Design designClone = copyGraph(origToClonedNodes, clonedToOrigNodes, origToClonedArcs, clonedToOrigArcs);

//    std::cerr << "Node Count: " << designClone.nodes.size() << std::endl;
    unsigned long numNodesPruned=0;
    if(prune){
        numNodesPruned = designClone.prune(true);
        std::cerr << "Nodes Pruned: " << numNodesPruned << std::endl;
    }

//    std::cerr << "Node Count: " << designClone.nodes.size() << std::endl;

    //==== Remove input master and constants.  Disconnect output arcs from nodes with state (that do not contain combo paths)====
    //++++ Note: excpetions explained below ++++
    std::set<std::shared_ptr<Arc>> arcsToDelete = designClone.inputMaster->disconnectNode();
    std::set<std::shared_ptr<Node>> nodesToRemove;

    for(unsigned long i = 0; i<designClone.nodes.size(); i++){
        if(GeneralHelper::isType<Node, Constant>(designClone.nodes[i]) != nullptr) {
            //This is a constant node, disconnect it and remove it from the graph to be ordered
            std::set<std::shared_ptr<Arc>> newArcsToDelete = designClone.nodes[i]->disconnectNode();
            arcsToDelete.insert(newArcsToDelete.begin(), newArcsToDelete.end());

            nodesToRemove.insert(designClone.nodes[i]);
        }else if(GeneralHelper::isType<Node, BlackBox>(designClone.nodes[i]) != nullptr){
            std::shared_ptr<BlackBox> asBlackBox = std::dynamic_pointer_cast<BlackBox>(designClone.nodes[i]);

            //We need to treat black boxes a little differently.  They may have state but their outputs may not
            //be registered (could be cominationally dependent on the input signals).
            //We should only disconnect arcs to outputs that are registered.

            std::vector<int> registeredPortNumbers = asBlackBox->getRegisteredOutputPorts();

            std::vector<std::shared_ptr<OutputPort>> outputPorts = designClone.nodes[i]->getOutputPorts();
            for(unsigned long j = 0; j<outputPorts.size(); j++){
                //Check if the port number is in the list of registered output ports
                if(std::find(registeredPortNumbers.begin(), registeredPortNumbers.end(), outputPorts[j]->getPortNum()) != registeredPortNumbers.end()){
                    //This port is registered, its output arcs can be removed

                    std::set<std::shared_ptr<Arc>> outputArcs = outputPorts[j]->getArcs();
                    for(auto it = outputArcs.begin(); it != outputArcs.end(); it++){
                        std::shared_ptr<StateUpdate> dstAsStateUpdate = GeneralHelper::isType<Node, StateUpdate>((*it)->getDstPort()->getParent());
                        if(dstAsStateUpdate == nullptr || (dstAsStateUpdate != nullptr && dstAsStateUpdate->getPrimaryNode() != (*it)->getSrcPort()->getParent())){
                            (*it)->disconnect();
                            arcsToDelete.insert(*it);
                        }
                        //Else, this is a State node connected to its StateUpdate node and should not be removed
                    }
                }
            }

        }else if(designClone.nodes[i]->hasState() && !designClone.nodes[i]->hasCombinationalPath()){
            //Do not disconnect enabled outputs even though they have state.  They are actually more like transparent latches and do pass signals directly (within the same cycle) when the subsystem is enabled.  Otherwise, the pass the previous output
            //Disconnect output arcs (we still need to calculate the inputs to the state element, however the output of the output appears like a constant from a scheduling perspecitve)

            //Note, arcs to state update nodes should not be removed.

            std::set<std::shared_ptr<Arc>> outputArcs = designClone.nodes[i]->getOutputArcs();
            for(auto it = outputArcs.begin(); it != outputArcs.end(); it++){
                //Check if the dst is a state update.  Keep arcs to state update nodes
                std::shared_ptr<StateUpdate> dstAsStateUpdate = GeneralHelper::isType<Node, StateUpdate>((*it)->getDstPort()->getParent());
                if(dstAsStateUpdate == nullptr){
                    (*it)->disconnect();
                    arcsToDelete.insert(*it);
                }
                //Else, this is a State node connected to its StateUpdate node and should not be removed
            }
        }
    }

    for(auto it = nodesToRemove.begin(); it != nodesToRemove.end(); it++){
        if((*it)->getParent()){
            (*it)->getParent()->removeChild(*it);
        }

        designClone.nodes.erase(std::remove(designClone.nodes.begin(), designClone.nodes.end(), *it), designClone.nodes.end());
        designClone.topLevelNodes.erase(std::remove(designClone.topLevelNodes.begin(), designClone.topLevelNodes.end(), *it), designClone.topLevelNodes.end());
    }
    for(auto it = arcsToDelete.begin(); it != arcsToDelete.end(); it++){
        designClone.arcs.erase(std::remove(designClone.arcs.begin(), designClone.arcs.end(), *it), designClone.arcs.end());
    }

    //Rewire the remaining arcs in the designClone
    if(rewireContexts){
        std::vector<std::shared_ptr<Arc>> origArcs;
        std::vector<std::vector<std::shared_ptr<Arc>>> newArcsGrouped;

        designClone.rewireArcsToContexts(origArcs, newArcsGrouped);

        std::vector<std::shared_ptr<Arc>> newArcs;
        for(unsigned long i = 0; i<newArcsGrouped.size(); i++){
            for(unsigned long j = 0; j<newArcsGrouped[i].size(); j++){
                newArcs.push_back(newArcsGrouped[i][j]);
            }
        }

        //Dicsonnect and Remove the orig arcs for scheduling
        std::vector<std::shared_ptr<Node>> emptyNodeVector;
        std::vector<std::shared_ptr<Arc>> emptyArcVector;
        for(unsigned long i = 0; i<origArcs.size(); i++){
            origArcs[i]->disconnect();
        }
        designClone.addRemoveNodesAndArcs(emptyNodeVector, emptyNodeVector, newArcs, origArcs);
        designClone.assignArcIDs();
    }

//    std::cout << "Emitting: ./cOut/context_scheduleGraph.graphml" << std::endl;
//    GraphMLExporter::exportGraphML("./cOut/context_scheduleGraph.graphml", designClone);

    //==== Topological Sort (Destructive) ====
    std::vector<std::shared_ptr<Node>> schedule = designClone.topologicalSortDestructive(designName, dir, params);

    if(printNodeSched) {
        std::cout << "Schedule" << std::endl;
        for(unsigned long i = 0; i<schedule.size(); i++){
            std::cout << i << ": " << schedule[i]->getFullyQualifiedName() << std::endl;
        }
    }

    //==== Back Propagate Schedule ====
    for(unsigned long i = 0; i<schedule.size(); i++){
        //Index is the schedule number
        std::shared_ptr<Node> origNode = clonedToOrigNodes[schedule[i]];
        origNode->setSchedOrder(i);
    }

//    std::cout << "Schedule - All Nodes" << std::endl;
//    for(unsigned long i = 0; i<nodes.size(); i++){
//        std::cout << nodes[i]->getSchedOrder() << ": " << nodes[i]->getFullyQualifiedName() << std::endl;
//    }

    return numNodesPruned;
}

void Design::expandEnabledSubsystemContexts(){
    std::vector<std::shared_ptr<Node>> newNodes;
    std::vector<std::shared_ptr<Node>> deletedNodes;
    std::vector<std::shared_ptr<Arc>> newArcs;
    std::vector<std::shared_ptr<Arc>> deletedArcs;

    std::vector<std::shared_ptr<SubSystem>> childSubsystems;
    for(auto topLvlNode = topLevelNodes.begin(); topLvlNode != topLevelNodes.end(); topLvlNode++){
        if(GeneralHelper::isType<Node, SubSystem>(*topLvlNode) != nullptr){
            childSubsystems.push_back(std::dynamic_pointer_cast<SubSystem>(*topLvlNode));
        }
    }

    for(unsigned long i = 0; i<childSubsystems.size(); i++){
        //Adding a condition to check if the subsystem still has this node as parent.
        //TODO: remove condition if subsystems are never moved
        if(childSubsystems[i]->getParent() == nullptr){ //We are at the top level
            childSubsystems[i]->extendEnabledSubsystemContext(newNodes, deletedNodes, newArcs, deletedArcs);
        }else{
            throw std::runtime_error("Subsystem moved during enabled subsystem context expansion.  This was unexpected.");
        }
    }

    addRemoveNodesAndArcs(newNodes, deletedNodes, newArcs, deletedArcs);
}

void Design::discoverAndMarkContexts() {
    std::vector<std::shared_ptr<Mux>> discoveredMuxes;
    std::vector<std::shared_ptr<EnabledSubSystem>> discoveredEnabledSubsystems;
    std::vector<std::shared_ptr<Node>> discoveredGeneral;

    //The top level context stack has no entries
    std::vector<Context> initialStack;

    //Discover contexts at the top layer (and below non-enabled subsystems).  Also set the contexts of these top level nodes
    GraphAlgs::discoverAndUpdateContexts(topLevelNodes, initialStack, discoveredMuxes, discoveredEnabledSubsystems,
                                         discoveredGeneral);

    //Add context nodes (muxes and enabled subsystems) to the topLevelContextRoots list
    for(unsigned long i = 0; i<discoveredMuxes.size(); i++){
        topLevelContextRoots.push_back(discoveredMuxes[i]);
    }
    for(unsigned long i = 0; i<discoveredEnabledSubsystems.size(); i++){
        topLevelContextRoots.push_back(discoveredEnabledSubsystems[i]);
    }

    //Get and mark the Mux contexts
    Mux::discoverAndMarkMuxContextsAtLevel(discoveredMuxes);

    //Recursivly call on the discovered enabled subsystems
    for(unsigned long i = 0; i<discoveredEnabledSubsystems.size(); i++) {
        discoveredEnabledSubsystems[i]->discoverAndMarkContexts(initialStack);
    }
}

void Design::createStateUpdateNodes() {
    //Find nodes with state in design
    std::vector<std::shared_ptr<Node>> nodesWithState = findNodesWithState();

    for(unsigned long i = 0; i<nodesWithState.size(); i++){
        std::vector<std::shared_ptr<Node>> newNodes;
        std::vector<std::shared_ptr<Node>> deletedNodes;
        std::vector<std::shared_ptr<Arc>> newArcs;
        std::vector<std::shared_ptr<Arc>> deletedArcs;

        nodesWithState[i]->createStateUpdateNode(newNodes, deletedNodes, newArcs, deletedArcs);

        addRemoveNodesAndArcs(newNodes, deletedNodes, newArcs, deletedArcs);
    }
}

void Design::createContextVariableUpdateNodes() {
    //Find nodes with state in design
    std::vector<std::shared_ptr<ContextRoot>> contextRoots = findContextRoots();

    for(unsigned long i = 0; i<contextRoots.size(); i++){
        std::vector<std::shared_ptr<Node>> newNodes;
        std::vector<std::shared_ptr<Node>> deletedNodes;
        std::vector<std::shared_ptr<Arc>> newArcs;
        std::vector<std::shared_ptr<Arc>> deletedArcs;

        contextRoots[i]->createContextVariableUpdateNodes(newNodes, deletedNodes, newArcs, deletedArcs);

        addRemoveNodesAndArcs(newNodes, deletedNodes, newArcs, deletedArcs);
    }
}

std::vector<std::shared_ptr<Node>> Design::findNodesWithState() {
    std::vector<std::shared_ptr<Node>> nodesWithState;

    for(unsigned long i = 0; i<nodes.size(); i++) {
        if (nodes[i]->hasState()) {
            nodesWithState.push_back(nodes[i]);
        }
    }

    return nodesWithState;
}

std::vector<std::shared_ptr<Node>> Design::findNodesWithGlobalDecl() {
    std::vector<std::shared_ptr<Node>> nodesWithGlobalDecl;

    for(unsigned long i = 0; i<nodes.size(); i++){
        if(nodes[i]->hasGlobalDecl()){
            nodesWithGlobalDecl.push_back(nodes[i]);
        }
    }

    return nodesWithGlobalDecl;
}

//TODO: Check
void Design::encapsulateContexts() {
    //context expansion stops at enabled subsystem boundaries.  Therefore, we cannot have enabled subsystems nested in muxes
    //However, we can have enabled subsystems nested in enabled subsystems, muxes nested in enabled subsystems, and muxes nested in muxes

    //**** <Change>Do this for all context roots</Change> *****
    //<Delete>Start at the top level of the hierarchy and find the nodes with context</Delete>

    //Create the appropriate ContextContainer of ContextFamilyContainer for each and create a map of nodes to ContextContainer
    std::vector<std::shared_ptr<ContextRoot>> contextRootNodes;
    std::vector<std::shared_ptr<Node>> nodesInContext;

    for(unsigned long i = 0; i<nodes.size(); i++){
        if(GeneralHelper::isType<Node, ContextRoot>(nodes[i]) != nullptr){
            contextRootNodes.push_back(std::dynamic_pointer_cast<ContextRoot>(nodes[i]));
        }

        //A context root can also be in a context
        if(nodes[i]->getContext().size() > 0){
            nodesInContext.push_back(nodes[i]);
        }
    }

    //***** Create set from all context nodes ****

    //<Delete>
    //Create a set of nodes in contexts.  Creating the set prevents redundant operations since nodes can be in multiple contexts.
    //Itterate though the set and move the nodes into the corresponding subsystem. (add as a child)
    //Note that context nodes in enabled subsystems are not added.  They are added by the enabled subsystem recursion


    //Recurse into the enabled subsytems and repeat

    //Create a helper function which can be called upon here and in the enabled subsystem version.  This function will take the list of nodes with context within it
    //In the enabled subsystem version, a difference is that the enabled subsystem has nodes in its own context that may not be in mux contexts
    //</Delete>

    //Move context nodes under ContextContainers
    for(unsigned long i = 0; i<nodesInContext.size(); i++){
        std::shared_ptr<SubSystem> origParent = nodesInContext[i]->getParent();

        if(origParent == nullptr) {
            //If has no parent, remove from topLevelNodes
            topLevelNodes.erase(std::remove(topLevelNodes.begin(), topLevelNodes.end(), nodesInContext[i]), topLevelNodes.end());

            if(GeneralHelper::isType<Node, ContextRoot>(nodesInContext[i]) != nullptr){
                //If is a context root, remove from top level context roots
                std::shared_ptr<ContextRoot> toRemove = std::dynamic_pointer_cast<ContextRoot>(nodesInContext[i]);
                contextRootNodes.erase(std::remove(contextRootNodes.begin(), contextRootNodes.end(), toRemove), contextRootNodes.end());
            }
        }else{
            //Remove from orig parent
            origParent->removeChild(nodesInContext[i]);
        }

        //Discover appropriate (sub)context container
        std::vector<Context> contextStack = nodesInContext[i]->getContext();

        //Create a ContextFamilyContainer for this partition at each level of the context stack if one has not already been created
        //TODO: refactor
        for(int j = 0; j<contextStack.size()-1; j++){ //-1 because the inner context is handled explicitally below
            getContextFamilyContainerCreateIfNotNoParent(contextStack[j].getContextRoot(), nodesInContext[i]->getPartitionNum());
        }

        Context innerContext = contextStack[contextStack.size()-1];
        std::shared_ptr<ContextFamilyContainer> contextFamily = getContextFamilyContainerCreateIfNotNoParent(innerContext.getContextRoot(), nodesInContext[i]->getPartitionNum());
        std::shared_ptr<ContextContainer> contextContainer = contextFamily->getSubContextContainer(innerContext.getSubContext());

        //Set new parent to be the context container.  Also add it as a child of the context container.
        contextContainer->addChild(nodesInContext[i]);
        nodesInContext[i]->setParent(contextContainer);
    }

    //Move context roots into their ContextFamilyContainers.  Note, the ContextStack of the ContextRootNode does not include its own context
    for(unsigned long i = 0; i<contextRootNodes.size(); i++){
        std::shared_ptr<Node> asNode = std::dynamic_pointer_cast<Node>(contextRootNodes[i]); //TODO: fix diamond inheritance issue
        std::shared_ptr<SubSystem> origParent = asNode->getParent();

        if(origParent != nullptr){
            origParent->removeChild(asNode);
        }

        std::shared_ptr<ContextFamilyContainer> contextFamilyContainer = getContextFamilyContainerCreateIfNotNoParent(contextRootNodes[i], asNode->getPartitionNum());
        contextFamilyContainer->addChild(asNode);
        contextFamilyContainer->setContextRoot(contextRootNodes[i]);
        asNode->setParent(contextFamilyContainer);
    }

    //Iterate through the ContextFamilyContainers (which should all be created now) and set the parent based on the context.
    for(unsigned long i = 0; i<contextRootNodes.size(); i++){
        //Get the context stack
        std::shared_ptr<ContextRoot> asContextRoot = contextRootNodes[i];
        std::shared_ptr<Node> asNode = std::dynamic_pointer_cast<Node>(asContextRoot); //TODO: Fix inheritance diamond issue

        std::vector<Context> context = asNode->getContext();
        if(context.size() > 0){
            //This node is in a context, move it's containers under the appropriate container.
            Context innerContext = context[context.size()-1];

            //Itterate through the ContextFamilyContainers for this ContextRoot
            std::map<int, std::shared_ptr<ContextFamilyContainer>> contextFamilyContainers = asContextRoot->getContextFamilyContainers();
            for(auto it = contextFamilyContainers.begin(); it != contextFamilyContainers.end(); it++){
                std::map<int, std::shared_ptr<ContextFamilyContainer>> newParentFamilyContainers = innerContext.getContextRoot()->getContextFamilyContainers();
                if(newParentFamilyContainers.find(it->second->getPartition()) == newParentFamilyContainers.end()){
                    throw std::runtime_error(ErrorHelpers::genErrorStr("Error while encapsulating contexts.  Unable to find parent ContextFamilyContainer for specified partition", asNode));
                }
                std::shared_ptr<ContextContainer> newParent = newParentFamilyContainers[it->second->getPartition()]->getSubContextContainer(innerContext.getSubContext());

                std::shared_ptr<ContextFamilyContainer> toBeMoved = it->second;

                toBeMoved->setParent(newParent);
                newParent->addChild(toBeMoved);
            }

        }else{
            //This node is not in a context, its ContextFamily containers should exist at the top level
            std::map<int, std::shared_ptr<ContextFamilyContainer>> contextFamilyContainers = asContextRoot->getContextFamilyContainers();
            for(auto it = contextFamilyContainers.begin(); it != contextFamilyContainers.end(); it++) {
                topLevelNodes.push_back(it->second);
            }
        }
    }
}

void Design::orderConstrainZeroInputNodes(){

    std::vector<std::shared_ptr<Node>> predecessorNodes;
    std::vector<std::shared_ptr<Node>> new_nodes;
    std::vector<std::shared_ptr<Node>> deleted_nodes;
    std::vector<std::shared_ptr<Arc>> new_arcs;
    std::vector<std::shared_ptr<Arc>> deleted_arcs;

    //Call the recursive function on any subsystem at the top level
    for(unsigned long i = 0; i<topLevelNodes.size(); i++){
        std::shared_ptr<SubSystem> nodeAsSubSystem = GeneralHelper::isType<Node, SubSystem>(topLevelNodes[i]);

        if(nodeAsSubSystem){
            nodeAsSubSystem->orderConstrainZeroInputNodes(predecessorNodes, new_nodes, deleted_nodes, new_arcs, deleted_arcs);
        }
    }

    addRemoveNodesAndArcs(new_nodes, deleted_nodes, new_arcs, deleted_arcs);
}

std::vector<std::shared_ptr<ContextRoot>> Design::findContextRoots() {
    std::vector<std::shared_ptr<ContextRoot>> contextRoots;

    for(unsigned long i = 0; i<nodes.size(); i++){
        std::shared_ptr<ContextRoot> nodeAsContextRoot = GeneralHelper::isType<Node, ContextRoot>(nodes[i]);

        if(nodeAsContextRoot){
            contextRoots.push_back(nodeAsContextRoot);
        }
    }

    return contextRoots;
}

std::vector<std::shared_ptr<BlackBox>> Design::findBlackBoxes(){
    std::vector<std::shared_ptr<BlackBox>> blackBoxes;

    for(unsigned long i = 0; i<nodes.size(); i++){
        std::shared_ptr<BlackBox> nodeAsBlackBox = GeneralHelper::isType<Node, BlackBox>(nodes[i]);

        if(nodeAsBlackBox){
            blackBoxes.push_back(nodeAsBlackBox);
        }
    }

    return blackBoxes;
}

//TODO: Check
void Design::rewireArcsToContexts(std::vector<std::shared_ptr<Arc>> &origArcs,
                                  std::vector<std::vector<std::shared_ptr<Arc>>> &contextArcs) {
    //First, let's rewire the ContextRoot driver arcs
    std::vector<std::shared_ptr<ContextRoot>> contextRoots = findContextRoots();
    std::set<std::shared_ptr<Arc>, Arc::PtrID_Compare> contextRootOrigArcs, contextRootRewiredArcs; //Want Arc orders to be consistent across runs

    //For each discovered context root
    for(unsigned long i = 0; i<contextRoots.size(); i++){
        //Rewire Drivers
        std::vector<std::shared_ptr<Arc>> driverArcs = contextRoots[i]->getContextDecisionDriver();

        //Note: All of these share the same destination context root, so checking the destination context root is not required
        std::map<std::shared_ptr<OutputPort>, std::vector<std::shared_ptr<Arc>>> srcPortToNewArcs; //Use this to check for duplicate new arcs (before creating them).  Return the pointer from the map instead.  This is used when

        for(unsigned long j = 0; j<driverArcs.size(); j++){
            //The driver arcs must have the destination re-wired.  At a minimum, they should be rewired to
            //the context container node.  However, if they exist inside of a nested context, the destination
            //may need to be rewired to another context family container higher in the hierarchy

            origArcs.push_back(driverArcs[j]);
            contextRootOrigArcs.insert(driverArcs[j]);

            //Check if the driver src / destination contextRoot pair have been encountered before
            std::shared_ptr<OutputPort> driverSrc = driverArcs[j]->getSrcPort();

            std::vector<std::shared_ptr<Arc>> rewiredArcs;
            if(srcPortToNewArcs.find(driverSrc) != srcPortToNewArcs.end()){
                //This src port has been seen before and has already been re-wired to the context family containers
                //return the new arcs
                rewiredArcs = srcPortToNewArcs[driverSrc];
            }else{
                //Have not seen this driver source for this particular ContextRoot before
                //Create new rewiredArcs, one for each ContextFamilyContainer associated with this ContextRoot
                std::shared_ptr<Node> contextRootAsNode = GeneralHelper::isType<ContextRoot, Node>(contextRoots[i]);

                std::vector<Context> srcContext = driverSrc->getParent()->getContext();
                std::vector<Context> dstContext = contextRootAsNode->getContext();
                //Add the context family container to the context stack of the destination (the context root is not in its own context
                dstContext.push_back(Context(contextRoots[i], -1)); //The subcontext number is a dummy

                //Note, the src may be different for different ContextFamilyContainers due to partition differences
                std::map<int, std::shared_ptr<ContextFamilyContainer>> dstContextFamilyContainers = contextRoots[i]->getContextFamilyContainers();
                for(auto it = dstContextFamilyContainers.begin(); it != dstContextFamilyContainers.end(); it++) {
                    std::shared_ptr<Node> newSrc = driverSrc->getParent(); //Default to orig
                    //Check if the src should be changed
                    if ((!Context::isEqOrSubContext(dstContext, srcContext)) ||
                        (driverSrc->getParent()->getPartitionNum() != it->first)) { //it->first is the partition number
                        //Need to pick new src
                        /* Since we are now checking for partitions, it is possible for the source and destination to
                         * actually be in the same context but in different partitions (different ContextFamilyContainers)
                         * We need to pick the appropriate ContextFamilyContainer partition
                         */
                        long commonContextInd = Context::findMostSpecificCommonContext(srcContext, dstContext);

                        std::shared_ptr<ContextRoot> newSrcAsContextRoot;

                        if(commonContextInd == -1 && srcContext.empty()){
                            //These nodes have no common context and the src node is not even in a context
                            //This only happens if the nodes are in different partitions.  Otherwise, the destination
                            //would be in an equal or sub context of the source

                            //Leave the src unchanged.
                        }
                        else {
                            if (commonContextInd + 1 == srcContext.size()) {
                                //The destination is in the same context or below as the src, just in a different partiton
                                //Will re-wire the src to the ContextFamilyContainer of the src
                                newSrcAsContextRoot = srcContext[commonContextInd].getContextRoot();
                            }else if (commonContextInd + 1 > srcContext.size()) {
                                throw std::runtime_error(ErrorHelpers::genErrorStr("Unexpected common context found when re-wiring arcs to source", contextRootAsNode));
                            }else {
                                //The nodes share a context at a higher level of the context stack.
                                //Set the source to be the ContextFamilyContainer one level down in the context stack
                                //(the src node's context must finish executing before the result is ready to drive this context)
                                newSrcAsContextRoot = srcContext[commonContextInd + 1].getContextRoot();
                            }

                            //TODO: fix diamond inheritcance

                            newSrc = newSrcAsContextRoot->getContextFamilyContainers()[driverSrc->getParent()->getPartitionNum()]; //Get the src ContextFamilyContainer corresponding to the src partition
                        }
                    }

                    //The dst needs to be rewired (at least to the ContextFamily container of the context root
                    //However, may be higher up in the higherarchy.
                    //Determine new dst node
                    long commonContextInd = Context::findMostSpecificCommonContext(srcContext, dstContext);

                    std::shared_ptr<ContextRoot> newDstAsContextRoot;

                    if (commonContextInd + 1 > dstContext.size()) {
                        throw std::runtime_error(ErrorHelpers::genErrorStr("Unexpected common context found when re-wiring arcs to destination", contextRootAsNode));
                    }else if(commonContextInd + 1 == dstContext.size()){
                        //The src is in the same context or below as the dst, just in a different partiton
                        //Will re-wire the dst to the ContextFamilyContainer of the dst

                        newDstAsContextRoot = dstContext[commonContextInd].getContextRoot();
                    }else{
                        newDstAsContextRoot = dstContext[commonContextInd + 1].getContextRoot();
                    }

                    //TODO: fix diamond inheritcance

                    std::shared_ptr<Node> newDest = newDstAsContextRoot->getContextFamilyContainers()[it->first]; //it->first is the partition of the origional ContextFamilyContainer destination for this ContextRoot

                    std::shared_ptr<Arc> rewiredArc = Arc::connectNodesOrderConstraint(newSrc, newDest, driverArcs[j]->getDataType(),
                                                                  driverArcs[j]->getSampleTime());
                    rewiredArcs.push_back(rewiredArc);
                }
            }

            //Add to the map and the set of contextRootRewiredArcs
            srcPortToNewArcs[driverSrc] = rewiredArcs;

            for(int rewiredArcInd = 0; rewiredArcInd < rewiredArcs.size(); rewiredArcInd++) {
                contextRootRewiredArcs.insert(rewiredArcs[rewiredArcInd]);
            }

            contextArcs.push_back(rewiredArcs);
        }
    }

    //Create a copy of the arc list and remove the orig ContextRoot arcs
    std::vector<std::shared_ptr<Arc>> candidateArcs = arcs;
    GeneralHelper::removeAll(candidateArcs, contextRootOrigArcs);

    //This is redundant with the add/remove method that occurs outside of the function.  This function does not add the other
    //arcs re-wired below so it does not make much sense to add the re-wired drivers here.
//    //Add the contextRootRewiredArcs to the design (we do not need to rewire these again so they are not included in the candidate arcs)
//    for(auto rewiredIt = contextRootRewiredArcs.begin(); rewiredIt != contextRootRewiredArcs.end(); rewiredIt++){
//        arcs.push_back(*rewiredIt);
//    }

    //Run through the remaining arcs and check if they should be rewired.
    for(unsigned long i = 0; i<candidateArcs.size(); i++){
        std::shared_ptr<Arc> candidateArc = candidateArcs[i];

        std::vector<Context> srcContext = candidateArc->getSrcPort()->getParent()->getContext();
        std::vector<Context> dstContext = candidateArc->getDstPort()->getParent()->getContext();

        //ContextRoots are not within their own contexts.  However, we need to make sure the inputs and output
        //arcs to the ContextRoots are elevated to the ContextFamily container for that ContextRoot as if it were in its
        //own subcontext.  We will therefore check for context roots and temporarily insert a dummy context entry
        std::shared_ptr<ContextRoot> srcAsContextRoot = GeneralHelper::isType<Node, ContextRoot>(candidateArc->getSrcPort()->getParent());
        if(srcAsContextRoot){
            srcContext.push_back(Context(srcAsContextRoot, -1));
        }
        std::shared_ptr<ContextRoot> dstAsContextRoot = GeneralHelper::isType<Node, ContextRoot>(candidateArc->getDstPort()->getParent());
        if(dstAsContextRoot){
            dstContext.push_back(Context(dstAsContextRoot, -1));
        }

        bool rewireSrc = false;
        bool rewireDst = false;

        /* Since we are now checking for partitions, it is possible for the source and destination to
         * actually be in the same context but in different partitions (different ContextFamilyContainers)
         * We need to pick the appropriate ContextFamilyContainer partition
         */

        //Check if the src should be rewired
        //TODO: Refactor since we may not actually re-wire one of the sides if the partitions don't match
        //We do not re-wire the src if the src in dst are in different partitions if the src is not in a context.  In this case, the src remains the same (the origional node in the general context)
        //We do re-wire the src if it is in a different partition and is in a context.  We then need to elevate the arc to the ContextFamilyContainer of the particular partition
        if((!Context::isEqOrSubContext(dstContext, srcContext)) || ((candidateArc->getSrcPort()->getParent()->getPartitionNum() != candidateArc->getDstPort()->getParent()->getPartitionNum()) && !srcContext.empty())){
            rewireSrc = true;
        }

        //Check if the dst should be rewired
        //Similarly to the src, the dest is not re-wired if is in a different partition but is not within a context
        if((!Context::isEqOrSubContext(srcContext, dstContext)) || ((candidateArc->getSrcPort()->getParent()->getPartitionNum() != candidateArc->getDstPort()->getParent()->getPartitionNum()) && !dstContext.empty())){
            rewireDst = true;
        }

        if(rewireSrc || rewireDst){
            std::shared_ptr<OutputPort> srcPort;
            std::shared_ptr<InputPort> dstPort;

            if(rewireSrc){
                long commonContextInd = Context::findMostSpecificCommonContext(srcContext, dstContext);
                std::shared_ptr<ContextRoot> newSrcAsContextRoot;

                if(commonContextInd+1 > srcContext.size()){
                    throw std::runtime_error(ErrorHelpers::genErrorStr("Unexpected common context found when re-wiring arcs to src", candidateArc->getSrcPort()->getParent()));
                }else if(commonContextInd+1 == srcContext.size()){
                    //The dst is at or below the src context but they are in different partitions
                    //Need to re-wire the src to be the ContextFamilyContainer for its particular
                    newSrcAsContextRoot = srcContext[commonContextInd].getContextRoot();
                }else{
                    //The dst is above the src in context, need to rewire
                    newSrcAsContextRoot = srcContext[commonContextInd+1].getContextRoot();
                }//The case of the src not having a context is handled when checking if src rewiring is needed


                //TODO: fix diamond inheritcance

                srcPort = newSrcAsContextRoot->getContextFamilyContainers()[candidateArc->getSrcPort()->getParent()->getPartitionNum()]->getOrderConstraintOutputPort();
            }else{
                srcPort = candidateArc->getSrcPort();
            }

            if(rewireDst){
                long commonContextInd = Context::findMostSpecificCommonContext(srcContext, dstContext);

                std::shared_ptr<ContextRoot> newDstAsContextRoot;
                if(commonContextInd+1 > dstContext.size()){
                    throw std::runtime_error(ErrorHelpers::genErrorStr("Unexpected common context found when re-wiring arcs to dst", candidateArc->getDstPort()->getParent()));
                }else if(commonContextInd+1 == dstContext.size()){
                    //The src is at or below the dest context but they are in different partitions
                    //Need to re-wire to the ContextFamilyContainer for the partition
                    newDstAsContextRoot = dstContext[commonContextInd].getContextRoot();
                }else{
                    newDstAsContextRoot = dstContext[commonContextInd+1].getContextRoot();
                }

                //TODO: fix diamond inheritcance

                dstPort = newDstAsContextRoot->getContextFamilyContainers()[candidateArc->getDstPort()->getParent()->getPartitionNum()]->getOrderConstraintInputPort();
            }else{
                dstPort = candidateArc->getDstPort();
            }

            //Handle the special case when going between subcontexts under the same ContextFamilyContainer.  This should
            //only occur when the dst is the context root for the ContextFamilyContainer.  In this case, do not rewire
            if(srcPort->getParent() == dstPort->getParent()){
                std::shared_ptr<ContextRoot> origDstAsContextRoot = GeneralHelper::isType<Node, ContextRoot>(candidateArc->getDstPort()->getParent());
                if(origDstAsContextRoot == nullptr){
                    throw std::runtime_error("Attempted to Rewire a Context Arc into a Self Loop");
                }

                if(origDstAsContextRoot->getContextFamilyContainers()[candidateArc->getDstPort()->getParent()->getPartitionNum()] != dstPort->getParent()){
                    throw std::runtime_error("Attempted to Rewire a Context Arc into a Self Loop");
                }
            }else {
                std::shared_ptr<Arc> rewiredArc = Arc::connectNodes(srcPort, dstPort, candidateArc->getDataType(),
                                                                    candidateArc->getSampleTime());

                //Add the orig and rewired arcs to the vectors to return
                origArcs.push_back(candidateArc);
                contextArcs.push_back({rewiredArc});
            }
        }
    }
}

void Design::createEnabledOutputsForEnabledSubsysVisualization() {
    //Find in nodes that have arcs to the Vis Master
    std::set<std::shared_ptr<Arc>> visDriverArcsPtrOrder = visMaster->getInputArcs();
    std::set<std::shared_ptr<Arc>, Arc::PtrID_Compare> visDriverArcs; //Order by ID for consistency across runs
    visDriverArcs.insert(visDriverArcsPtrOrder.begin(), visDriverArcsPtrOrder.end());

    for(auto driverArc = visDriverArcs.begin(); driverArc != visDriverArcs.end(); driverArc++){
        std::shared_ptr<Node> srcNode = (*driverArc)->getSrcPort()->getParent();

        std::shared_ptr<OutputPort> portToRewireTo = (*driverArc)->getSrcPort();

        //Run through the hierarchy of the node (context stack not built yet)

        for(std::shared_ptr<SubSystem> parent = srcNode->getParent(); parent != nullptr; parent=parent->getParent()){
            //Check if the context root is an enabled subsystem
            std::shared_ptr<EnabledSubSystem> contextAsEnabledSubsys = GeneralHelper::isType<SubSystem, EnabledSubSystem>(parent);

            if(contextAsEnabledSubsys){
                //This is an enabled subsystem context, create an EnableOutput port for this vis
                std::shared_ptr<EnableOutput> visOutput = NodeFactory::createNode<EnableOutput>(contextAsEnabledSubsys);
                contextAsEnabledSubsys->addEnableOutput(visOutput);
                addNode(visOutput);

                visOutput->setName("VisEnableOutput");

                std::shared_ptr<Arc> enableDriverArcOrig = contextAsEnabledSubsys->getEnableDriverArc();
                //Copy enable driver arc
                std::shared_ptr<Arc> newEnDriverArc = Arc::connectNodes(enableDriverArcOrig->getSrcPort(), visOutput, enableDriverArcOrig->getDataType(), enableDriverArcOrig->getSampleTime());
                addArc(newEnDriverArc);

                //Create a new node from the portToRewireTo to this new output node
                std::shared_ptr<Arc> newStdDriverArc = Arc::connectNodes(portToRewireTo, visOutput->getInputPortCreateIfNot(0), (*driverArc)->getDataType(), (*driverArc)->getSampleTime());
                addArc(newStdDriverArc);

                //Update the portToRewireTo to be the output port of this node.
                portToRewireTo = visOutput->getOutputPortCreateIfNot(0);
            }
        }

        //rewire the arc to the nodeToRewireTo
        (*driverArc)->setSrcPortUpdateNewUpdatePrev(portToRewireTo);

    }
}

std::map<int, std::vector<std::shared_ptr<Node>>> Design::findPartitions() {
    //Go through the list of nodes in the design and place them in the appropriate partition
    std::map<int, std::vector<std::shared_ptr<Node>>> partitions;

    for(unsigned long i = 0; i<nodes.size(); i++){
        int nodePartition = nodes[i]->getPartitionNum();
        partitions[nodePartition].push_back(nodes[i]); //The map returns a reference to the vector
    }

    return partitions;
}

std::map<std::pair<int, int>, std::vector<std::shared_ptr<Arc>>> Design::getPartitionCrossings(bool checkForToFromNoPartition) {
    //Iterate through nodes and their out - arcs to discover partition crossings
    std::map<std::pair<int, int>, std::vector<std::shared_ptr<Arc>>> partitionCrossings;

    for(unsigned long i = 0; i<nodes.size(); i++){
        int srcPartition = nodes[i]->getPartitionNum();

        std::set<std::shared_ptr<Arc>> outArcs = nodes[i]->getOutputArcs();
        for(auto outArc = outArcs.begin(); outArc != outArcs.end(); outArc++){
            int dstPartition = (*outArc)->getDstPort()->getParent()->getPartitionNum();
            if(srcPartition != dstPartition){
                if(checkForToFromNoPartition && (srcPartition == -1 || dstPartition == -1)){
                    throw std::runtime_error(ErrorHelpers::genErrorStr("Found an arc going to/from partition -1"));
                }

                partitionCrossings[std::pair<int, int>(srcPartition, dstPartition)].push_back(*outArc);
            }
        }
    }

    return partitionCrossings;
}

std::map<std::pair<int, int>, std::vector<std::vector<std::shared_ptr<Arc>>>> Design::getGroupableCrossings(bool checkForToFromNoPartition) {
    //Iterate through nodes and their out - arcs to discover partition crossings
    //Check for multiple destinations in a particular partition which could be grouped

    std::map<std::pair<int, int>, std::vector<std::vector<std::shared_ptr<Arc>>>> partitionCrossings;

    for(unsigned long i = 0; i<nodes.size(); i++){
        int srcPartition = nodes[i]->getPartitionNum();

        std::set<std::shared_ptr<Arc>> outArcs = nodes[i]->getOutputArcs();

        //For each node, we create a map of other partitions which contains a vector of the arcs to that partition
        //Arcs from a given node to the same partition can be grouped
        std::map<int, std::vector<std::shared_ptr<Arc>>> currentGroups;

        for(auto outArc = outArcs.begin(); outArc != outArcs.end(); outArc++){
            //Note that arcs are not necessarily ordered according to partitions
            int dstPartition = (*outArc)->getDstPort()->getParent()->getPartitionNum();
            if(srcPartition != dstPartition){
                if(checkForToFromNoPartition && (srcPartition == -1 || dstPartition == -1)){
                    throw std::runtime_error(ErrorHelpers::genErrorStr("Found an arc going to/from partition -1"));
                }

                currentGroups[dstPartition].push_back(*outArc);
            }
        }

        //Add discovered groups to partitionCrossing map
        for(auto it = currentGroups.begin(); it != currentGroups.end(); it++){
            int dstPartition = it->first;
            partitionCrossings[std::pair<int, int>(srcPartition, dstPartition)].push_back(it->second);
        }
    }

    return partitionCrossings;
}

void Design::emitMultiThreadedC(std::string path, std::string fileName, std::string designName,
                                SchedParams::SchedType schedType) {

    //The code below if adapted from the single threaded emitter

    prune(true);
    expandEnabledSubsystemContexts();
    //Quick check to make sure vis node has no outgoing arcs (required for createEnabledOutputsForEnabledSubsysVisualization).
    //Is ok for them to exist (specifically order constraint arcs) after state update node and context encapsulation
    if(visMaster->outDegree() > 0){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Visualization Master is Not Expected to Have an Out Degree > 0, has " + GeneralHelper::to_string(visMaster->outDegree())));
    }
    createEnabledOutputsForEnabledSubsysVisualization();
    assignNodeIDs(); //Need to assign node IDs since the node ID is used for set ordering and new nodes may have been added
    assignArcIDs();

    //Partition Here

    //FIFO insert Here (before context variable updates or state variable updates created

    createContextVariableUpdateNodes(); //Create after expanding the subcontext so that any movement of EnableInput and EnableOutput nodes
    assignNodeIDs(); //Need to assign node IDs since the node ID is used for set ordering and new nodes may have been added
    assignArcIDs();

    createStateUpdateNodes(); //Done after EnabledSubsystem Contexts are expanded to avoid issues with deleting and re-wiring EnableOutputs
    assignNodeIDs(); //Need to assign node IDs since the node ID is used for set ordering and new nodes may have been added
    assignArcIDs();

    //TODO: Modify to not stop at FIFOs for
    discoverAndMarkContexts();
//        assignNodeIDs(); //Need to assign node IDs since the node ID is used for set ordering and new nodes may have been added
//        assignArcIDs();

    //Order constraining zero input nodes in enabled subsystems is not nessisary as rewireArcsToContexts can wire the enable
    //line as a depedency for the enable context to be emitted.  This is currently done in the scheduleTopoloicalSort method called below
    //TODO: re-introduce orderConstrainZeroInputNodes if the entire enable context is not scheduled hierarchically
    //orderConstrainZeroInputNodes(); //Do this after the contexts being marked since this constraint should not have an impact on contexts˚

    encapsulateContexts();
    assignNodeIDs(); //Need to assign node IDs since the node ID is used for set ordering and new nodes may have been added
    assignArcIDs();

    //Multithreaded specific

    //This function needs to perform the expansion, context expansion, and context encapsulation that the single threaded
    //emitters already do.  Note that the encapsulation into ContextFamilyContainers now needs to be partition aware.
    //Seperate ContextFamilyContainers should be created for each partition.
    //Context family containers should also be in the partition they are assigned to and should be included in the list
    //of nodes to schedule when a particular partition is being scheduled
    //When arcs are elevated for scheduling, the context/partition pair should now be considered rather than just the partition (this happens in the scheduler method by calling rewireArcsToContexts)
    //Also, the context driver arc should also be duplicated for each ContextFamilyContainer

    //These changes should not effect the single thread

    //Need to modify discover and mark contexts to not stop at FIFOs even though they have state (special case

    //Discover partitions
    std::map<int, std::vector<std::shared_ptr<Node>>> partitions = findPartitions();

    //Discover groupable partition crossings
    std::map<std::pair<int, int>, std::vector<std::vector<std::shared_ptr<Arc>>>> partitionCrossings = getGroupableCrossings();

    //Insert FIFOs into the crossings.  Note, the FIFO's partition should reside in the upstream partition of the arc(s) it is being placed in the middle of.
    //They should also reside inside of the source context family container (double check)

    //We've added nodes and arcs and therefore need to assign their ID numbers
    assignNodeIDs();
    assignArcIDs();

        //Emit each partition's function (except partition -1)
            //Discover partition function prototype from crossing FIFOs
            //Need to emit the output seperatly? (No, by supplying the correct emitCNextState statement, the output can be set

        //Emit each partition's driver function

    //Emit I/O
        //For now, is a seperate partition (0)
        //Get function prototype from Input/Output master and FIFOs

        //For now, provide a constant test case

    //Emit startup function
}

//For now, the emitter will not track if a temp variable's declaration needs to be moved to allow it to be used outside the local context.  This could occur with contexts not being scheduled together.  It could be aleviated by the emitter checking for context breaks.  However, this will be a future todo for now.

std::shared_ptr<ContextFamilyContainer> Design::getContextFamilyContainerCreateIfNotNoParent(std::shared_ptr<ContextRoot> contextRoot, int partition){
    std::map<int, std::shared_ptr<ContextFamilyContainer>> contextFamilyContainers = contextRoot->getContextFamilyContainers();

    if(contextFamilyContainers.find(partition) == contextFamilyContainers.end()){
        //Does not exist for given partition
        std::shared_ptr<Node> asNode = std::dynamic_pointer_cast<Node>(contextRoot);
        std::shared_ptr<ContextFamilyContainer> familyContainer = NodeFactory::createNode<ContextFamilyContainer>(nullptr); //Temporarily set parent to nullptr
        nodes.push_back(familyContainer);
        familyContainer->setName("ContextFamilyContainer_For_"+asNode->getFullyQualifiedName(true, "::")+"_Partition_"+GeneralHelper::to_string(partition));
        familyContainer->setPartition(partition);

        //Set context of FamilyContainer (since it is a node in the graph as well which may be scheduled)
        std::vector<Context> origContext = asNode->getContext();
        familyContainer->setContext(origContext);

        //Connect to ContextRoot and to siblings
        familyContainer->setSiblingContainers(contextFamilyContainers);
        for(auto it = contextFamilyContainers.begin(); it != contextFamilyContainers.end(); it++){
            std::map<int, std::shared_ptr<ContextFamilyContainer>> siblingMap = it->second->getSiblingContainers();
            siblingMap[partition] = familyContainer;
            it->second->setSiblingContainers(siblingMap);
        }
        contextFamilyContainers[partition] = familyContainer;
        contextRoot->setContextFamilyContainers(contextFamilyContainers);

        //Create Context Containers inside of the context Family container;
        int numContexts = contextRoot->getNumSubContexts();

        std::vector<std::shared_ptr<ContextContainer>> subContexts;

        for(unsigned long j = 0; j<numContexts; j++){
            std::shared_ptr<ContextContainer> contextContainer = NodeFactory::createNode<ContextContainer>(familyContainer);
            contextContainer->setName("ContextContainer_For_"+asNode->getFullyQualifiedName(true, "::")+"_Partition_"+GeneralHelper::to_string(partition)+"_Subcontext_"+GeneralHelper::to_string(j));
            nodes.push_back(contextContainer);
            //Add this to the container order
            subContexts.push_back(contextContainer);

            Context context = Context(contextRoot, j);
            contextContainer->setContainerContext(context);
            contextContainer->setContext(origContext); //We give the ContextContainer the same Context stack as the ContextFamily container, the containerContext sets the next level of the context for the nodes contained within it
        }
        familyContainer->setSubContextContainers(subContexts);

        return familyContainer;
    }else{
        return contextFamilyContainers[partition];
    }
}