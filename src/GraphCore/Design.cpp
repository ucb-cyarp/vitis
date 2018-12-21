//
// Created by Christopher Yarp on 6/27/18.
//

#include "Design.h"

#include <algorithm>
#include <iostream>
#include <fstream>
#include "General/GeneralHelper.h"
#include "General/GraphAlgs.h"

#include "NodeFactory.h"
#include "MasterNodes/MasterInput.h"
#include "MasterNodes/MasterOutput.h"
#include "MasterNodes/MasterUnconnected.h"

#include "GraphCore/Node.h"
#include "GraphCore/Arc.h"
#include "GraphCore/Variable.h"
#include "PrimitiveNodes/Constant.h"
#include "PrimitiveNodes/Mux.h"
#include "ContextContainer.h"
#include "ContextFamilyContainer.h"

#include "GraphCore/StateUpdate.h"

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
        std::shared_ptr<ExpandedNode> expandedPtr = (*node)->expand(newNodes, deletedNodes, newArcs, deletedArcs);
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

void Design::generateSingleThreadedC(std::string outputDir, std::string designName, SchedParams::SchedType schedType){
    if(schedType == SchedParams::SchedType::BOTTOM_UP)
        emitSingleThreadedC(outputDir, designName, designName, schedType);
    else if(schedType == SchedParams::SchedType::TOPOLOGICAL) {
        scheduleTopologicalStort(true);
        verifyTopologicalOrder();
        emitSingleThreadedC(outputDir, designName, designName, schedType);
    }else if(schedType == SchedParams::SchedType::TOPOLOGICAL_CONTEXT){
        prune(true);
        createStateUpdateNodes();
        createContextVariableUpdateNodes();
        expandEnabledSubsystemContexts();
        discoverAndMarkContexts();
        orderConstrainZeroInputNodes(); //Do this after the contexts being marked since this constraint should not have an impact on contexts˚
        encapsulateContexts();

        //We have added nodes and arcs.  Assign them IDs
        assignNodeIDs();
        assignArcIDs();

        scheduleTopologicalStort(false); //Pruned before inserting state update nodes
        verifyTopologicalOrder();
        emitSingleThreadedC(outputDir, designName, designName, schedType);
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

void Design::emitSingleThreadedOpsSchedStateUpdateContext(std::ofstream &cFile, SchedParams::SchedType schedType){

    cFile << std::endl << "//==== Compute Operators ====" << std::endl;

    //Sort nodes by schedOrder.
    std::vector<std::shared_ptr<Node>> orderedNodes = nodes;
    //Add the output master to the scheduled node list
    orderedNodes.push_back(outputMaster);
    std::sort(orderedNodes.begin(), orderedNodes.end(), Node::lessThanSchedOrder);

    std::shared_ptr<Node> zeroSchedNodeCmp = NodeFactory::createNode<MasterUnconnected>(); //Need a node to compare to
    zeroSchedNodeCmp->setSchedOrder(0);

    auto schedIt = std::lower_bound(orderedNodes.begin(), orderedNodes.end(), zeroSchedNodeCmp, Node::lessThanSchedOrder); //Binary search for the first node to be emitted (schedOrder 0)

    //Keep a context stack of the last emitted statement.  This is used to check for context changes.  Also used to check if the 'first' entry should be used.  If first entry is used (ie. previous context at this level in the stack was not in the same famuly, and the subContext emit count is not 0, then contexts are not contiguous -> ie. switch cannot be used)
    std::vector<Context> lastEmittedContext;

    //True if the context that was emitted was opened with the 'first' method
    std::vector<bool> contextFirst;

    //Keep a set of contexts that have been emitted (and exited).  For now, we do not allow re-entry into an already emitted sub context.
    std::set<Context> alreadyEmittedSubContexts;

    //Keep a count of how many subContexts of a contextRoot have been emitted, allows if/else if/else statements to not be contiguous
    std::map<std::shared_ptr<ContextRoot>, int> subContextEmittedCount;

    //Itterate through the schedule and emit
    for(auto it = schedIt; it != orderedNodes.end(); it++){

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

                        contextFirst[i] = true;
                    }else{
                        if(subContextEmittedCount[nodeContext[i].getContextRoot()] >= nodeContext[i].getContextRoot()->getNumSubContexts()-1){ //Check if this is the last in the context family
                            nodeContext[i].getContextRoot()->emitCContextOpenLast(contextStatements, schedType, nodeContext[i].getSubContext());
                        }else{
                            nodeContext[i].getContextRoot()->emitCContextOpenMid(contextStatements, schedType, nodeContext[i].getSubContext());
                        }

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
                cFile << "output[0]." << outputVar.getCVarName(false) << " = " << expr_re << ";" << std::endl;

                //Emit Imag if Datatype is complex
                if (outputDataType.isComplex()) {
                    cFile << std::endl << "//-- Assign Imag Component --" << std::endl;
                    std::vector<std::string> cStatements_im;
                    std::string expr_im = srcNode->emitC(cStatements_im, schedType, srcNodeOutputPortNum, true, true, true);
                    //emit the expressions
                    unsigned long numStatements_im = cStatements_im.size();
                    for (unsigned long j = 0; j < numStatements_im; j++) {
                        cFile << cStatements_im[j] << std::endl;
                    }

                    //emit the assignment
                    cFile << "output[0]." << outputVar.getCVarName(true) << " = " << expr_im << ";" << std::endl;
                }
            }
        }else if(GeneralHelper::isType<Node, StateUpdate>(*it) != nullptr){
            std::shared_ptr<StateUpdate> stateUpdateNode = std::dynamic_pointer_cast<StateUpdate>(*it);
            //Emit state update
            cFile << std::endl << "//---- State Update for " << stateUpdateNode->getPrimaryNode()->getFullyQualifiedName() << " ----" << std::endl;

            std::vector<std::string> stateUpdateExprs;
            (*it)->emitCExprNextState(stateUpdateExprs, schedType);

            for(unsigned long j = 0; j<stateUpdateExprs.size(); j++){
                cFile << stateUpdateExprs[j] << std::endl;
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

void Design::emitSingleThreadedC(std::string path, std::string fileName, std::string designName, SchedParams::SchedType sched) {
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

    //Emit operators
    if(sched == SchedParams::SchedType::BOTTOM_UP){
        emitSingleThreadedOpsBottomUp(cFile, nodesWithState, sched);
    }else if(sched == SchedParams::SchedType::TOPOLOGICAL){
        emitSingleThreadedOpsSched(cFile, sched);
    }else if(sched == SchedParams::SchedType::TOPOLOGICAL_CONTEXT){
        emitSingleThreadedOpsSchedStateUpdateContext(cFile, sched);
    }else{
        throw std::runtime_error("Unknown schedule type");
    }

    //Emit state variable updates
    cFile << std::endl << "//==== Update State Vars ====" << std::endl;
    for(unsigned long i = 0; i<nodesWithState.size(); i++){
        std::vector<std::string> stateUpdateExprs;
        nodesWithState[i]->emitCStateUpdate(stateUpdateExprs, sched);

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

Design Design::copyGraph(std::map<std::shared_ptr<Node>, std::shared_ptr<Node>> &origToCopyNode,
                         std::map<std::shared_ptr<Node>, std::shared_ptr<Node>> &copyToOrigNode,
                         std::map<std::shared_ptr<Arc>, std::shared_ptr<Arc>> &origToCopyArc,
                         std::map<std::shared_ptr<Arc>, std::shared_ptr<Arc>> &copyToOrigArc) {
    Design designCopy;

    //==== Create new master nodes and add them to the node copies list and maps ====
    designCopy.inputMaster = NodeFactory::createNode<MasterInput>();
    designCopy.outputMaster = NodeFactory::createNode<MasterOutput>();
    designCopy.visMaster = NodeFactory::createNode<MasterOutput>();
    designCopy.unconnectedMaster = NodeFactory::createNode<MasterUnconnected>();
    designCopy.terminatorMaster = NodeFactory::createNode<MasterOutput>();

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

        if(GeneralHelper::isType<Node, ContextRoot>(nodeCopies[i]) != nullptr){
            //ContextRoots
            std::shared_ptr<ContextRoot> nodeCopyAsContextRoot = std::dynamic_pointer_cast<ContextRoot>(nodeCopies[i]); //Don't think you can cross cast with a static cast TODO: check

            //TODO: Refactor ContextRoot inheritance
            std::shared_ptr<ContextRoot> origNodeAsContextRoot = GeneralHelper::isType<Node, ContextRoot>(origNode);

            if(origNodeAsContextRoot){
                int numSubContexts = nodeCopyAsContextRoot->getNumSubContexts();
                nodeCopyAsContextRoot->setNumSubContexts(numSubContexts);

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

    designCopy.arcs = arcCopies;

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

    //Find nodes with 0 out-degree when not counting the various master nodes
    std::set<std::shared_ptr<Node>> nodesToIgnore;
    nodesToIgnore.insert(unconnectedMaster);
    nodesToIgnore.insert(terminatorMaster);
    if(includeVisMaster){
        nodesToIgnore.insert(visMaster);
    }

    std::set<std::shared_ptr<Node>> nodesWithZeroOutDeg;
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
    std::set<std::shared_ptr<Node>> candidateNodes;
    for(auto it = nodesWithZeroOutDeg.begin(); it != nodesWithZeroOutDeg.end(); it++){
        std::set<std::shared_ptr<Node>> moreCandidates = (*it)->getConnectedNodes();
        candidateNodes.insert(moreCandidates.begin(), moreCandidates.end());
    }

    //Remove the master nodes from the candidate list as well as any nodes that are about to be removed
    candidateNodes.erase(unconnectedMaster);
    candidateNodes.erase(terminatorMaster);
    candidateNodes.erase(visMaster);
    candidateNodes.erase(inputMaster);
    candidateNodes.erase(outputMaster);

    //Erase
    std::set<std::shared_ptr<Node>> nodesDeleted;
    std::set<std::shared_ptr<Arc>> arcsDeleted;

    while(!nodesWithZeroOutDeg.empty()){
        //Disconnect, erase nodes, remove from candidate set (if it is included)
        for(auto it = nodesWithZeroOutDeg.begin(); it != nodesWithZeroOutDeg.end(); it++){
            std::set<std::shared_ptr<Arc>> arcsToRemove = (*it)->disconnectNode();
            arcsDeleted.insert(arcsToRemove.begin(), arcsToRemove.end());
            nodesDeleted.insert(*it);
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

        if(srcNode != inputMaster && GeneralHelper::isType<Node, Constant>(srcNode) == nullptr && !(srcNode->hasState())){ //Note that the outputs of nodes with state are considered constants
            std::shared_ptr<Node> dstNode = arcs[i]->getDstPort()->getParent();

            //It is allowed for the destination node to have order -1 (ie. not emitted) but the reverse is not OK
            //The srcNode can only be -1 if the destination is also -1
            if(srcNode->getSchedOrder() == -1){
                //Src node is unscheduled
                if(dstNode->getSchedOrder() != -1){
                    //dst node is scheduled
                    throw std::runtime_error("Topological Order Validation: Src Node is Unscheduled but Dst Node is Scheduled");
                }
                //otherwise, there is no error here as both nodes are unscheduled
            }else{
                //Src node is scheduled
                if(dstNode->getSchedOrder() != -1){
                    //Dst node is scheduled
                    if(srcNode->getSchedOrder() >= dstNode->getSchedOrder()){
                        throw std::runtime_error("Topological Order Validation: Src Node (" + srcNode->getFullyQualifiedName() + ") [Sched Order: " + GeneralHelper::to_string(srcNode->getSchedOrder()) + ", ID: " + GeneralHelper::to_string(srcNode->getId()) + "] is not Scheduled before Dst Node (" + dstNode->getFullyQualifiedName() + ") [Sched Order: " + GeneralHelper::to_string(dstNode->getSchedOrder()) +", ID: " + GeneralHelper::to_string(dstNode->getId()) + "]");
                    }
                }
                //Dst node unscheduled is OK
            }
        }
    }
}

std::vector<std::shared_ptr<Node>> Design::topologicalSortDestructive() {
    std::vector<std::shared_ptr<Arc>> arcsToDelete;
    std::vector<std::shared_ptr<Node>> topLevelContextNodes = GraphAlgs::findNodesStopAtContextFamilyContainers(topLevelNodes);

    std::vector<std::shared_ptr<Node>> sortedNodes = GraphAlgs::topologicalSortDestructive(topLevelContextNodes, arcsToDelete, outputMaster, inputMaster, terminatorMaster, unconnectedMaster, visMaster);

    //Delete the arcs
    //TODO: Remove this?  May not be needed for most applications.  We are probably going to distroy the design anyway
    for(auto it = arcsToDelete.begin(); it != arcsToDelete.end(); it++){
        arcs.erase(std::remove(arcs.begin(), arcs.end(), *it), arcs.end());
    }

    return sortedNodes;
}

unsigned long Design::scheduleTopologicalStort(bool prune) {
    std::map<std::shared_ptr<Node>, std::shared_ptr<Node>> origToClonedNodes;
    std::map<std::shared_ptr<Node>, std::shared_ptr<Node>> clonedToOrigNodes;
    std::map<std::shared_ptr<Arc>, std::shared_ptr<Arc>> origToClonedArcs;
    std::map<std::shared_ptr<Arc>, std::shared_ptr<Arc>> clonedToOrigArcs;

    //Make a copy of the design to conduct the destructive topological sort on
    Design designClone = copyGraph(origToClonedNodes, clonedToOrigNodes, origToClonedArcs, clonedToOrigArcs);

    unsigned long numNodesPruned=0;
    if(prune){
        numNodesPruned = designClone.prune(true);
    }

    //==== Remove input master and constants.  Disconnect output arcs from nodes with state ====
    std::set<std::shared_ptr<Arc>> arcsToDelete = designClone.inputMaster->disconnectNode();
    std::set<std::shared_ptr<Node>> nodesToRemove;

    for(unsigned long i = 0; i<designClone.nodes.size(); i++){
        if(GeneralHelper::isType<Node, Constant>(designClone.nodes[i]) != nullptr){
            //This is a constant node, disconnect it and remove it from the graph to be ordered
            std::set<std::shared_ptr<Arc>> newArcsToDelete = designClone.nodes[i]->disconnectNode();
            arcsToDelete.insert(newArcsToDelete.begin(), newArcsToDelete.end());

            nodesToRemove.insert(designClone.nodes[i]);
        }else if(designClone.nodes[i]->hasState() && GeneralHelper::isType<Node, EnableOutput>(designClone.nodes[i]) == nullptr){ //Do not disconnect enabled outputs even though they have state.  They are actually more like transparent latches and do pass signals directly (within the same cycle) when the subsystem is enabled.  Otherwise, the pass the previous output
            //Disconnect output arcs (we still need to calculate the inputs to the delay, however, the outputs are like constants for the cycle)
            std::set<std::shared_ptr<Arc>> newArcsToDelete = designClone.nodes[i]->disconnectOutputs();
            arcsToDelete.insert(newArcsToDelete.begin(), newArcsToDelete.end());
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

    //==== Topological Sort (Destructive) ====
    std::vector<std::shared_ptr<Node>> schedule = designClone.topologicalSortDestructive();

//    std::cout << "Schedule" << std::endl;
//    for(unsigned long i = 0; i<schedule.size(); i++){
//        std::cout << i << ": " << schedule[i]->getFullyQualifiedName() << std::endl;
//    }

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
    std::set<std::shared_ptr<Node>> topLevelNodeSet;
    topLevelNodeSet.insert(topLevelNodes.begin(), topLevelNodes.end());
    GraphAlgs::discoverAndUpdateContexts(topLevelNodeSet, initialStack, discoveredMuxes, discoveredEnabledSubsystems,
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

void Design::encapsulateContexts() {
    //context expansion stops at enabled subsystem boundaries.  Therefore, we cannot have enabled subsystems nested in muxes
    //However, we can have enabled subsystems nested in enabled subsystems, muxes nested in enabled subsystems, and muxes nested in muxes

    //**** Change, do this for all context roots *****

    //<Delete>Start at the top level of the hierarchy and find the nodes with context
    //Create the appropriate ContextContainer of ContextFamilyContainer for each and create a map of node to ContextContainer

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

    std::map<std::shared_ptr<ContextRoot>, std::shared_ptr<ContextFamilyContainer>> contextNodeToFamilyContainer;

    for(unsigned long i = 0; i<contextRootNodes.size(); i++){
        std::shared_ptr<ContextFamilyContainer> familyContainer = NodeFactory::createNode<ContextFamilyContainer>(nullptr); //Temporarily set parent to nullptr
        nodes.push_back(familyContainer);
        contextNodeToFamilyContainer[contextRootNodes[i]] = familyContainer;

        //Set context of FamilyContainer (since it is a node in the graph as well which may be scheduled)
        std::shared_ptr<Node> asNode = std::dynamic_pointer_cast<Node>(contextRootNodes[i]); //TODO: Fix inheritance diamond issue
        std::vector<Context> origContext = asNode->getContext();
        familyContainer->setContext(origContext);

        //Create Context Containers inside of the context Family container;
        int numContexts = contextRootNodes[i]->getNumSubContexts();

        std::vector<std::shared_ptr<ContextContainer>> subContexts;

        for(unsigned long j = 0; j<numContexts; j++){
            std::shared_ptr<ContextContainer> contextContainer = NodeFactory::createNode<ContextContainer>(familyContainer);
            nodes.push_back(contextContainer);
            //Add this to the container order
            subContexts.push_back(contextContainer);

            Context context = Context(contextRootNodes[i], j);
            contextContainer->setContainerContext(context);
            contextContainer->setContext(origContext); //We give the ContextContainer the same Context stack as the ContextFamily container, the containerContext sets the next level of the context for the nodes contained within it
        }
        familyContainer->setSubContextContainers(subContexts);
    }

    //Itterate through them again and set the parent based on the context.  Use the map to find the corresponding context node
    for(unsigned long i = 0; i<contextRootNodes.size(); i++){
        //Get the context stack
        std::shared_ptr<ContextRoot> asContextRoot = contextRootNodes[i];
        std::shared_ptr<Node> asNode = std::dynamic_pointer_cast<Node>(asContextRoot); //TODO: Fix inheritance diamond issue

        std::vector<Context> context = asNode->getContext();
        if(context.size() > 1){
            //This node is in a context, move it's container under the appropriate container.
            Context innerContext = context[context.size()-1];

            std::shared_ptr<ContextContainer> newParent = contextNodeToFamilyContainer[innerContext.getContextRoot()]->getSubContextContainer(innerContext.getSubContext());

            std::shared_ptr<ContextFamilyContainer> toBeMoved = contextNodeToFamilyContainer[asContextRoot];

            toBeMoved->setParent(newParent);
            newParent->addChild(toBeMoved);
        }else{
            //This node is not in a context, its ContextFamily container should exist at the top level
            topLevelNodes.push_back(contextNodeToFamilyContainer[contextRootNodes[i]]);
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
        Context innerContext = contextStack[contextStack.size()-1];

        std::shared_ptr<ContextFamilyContainer> contextFamily = contextNodeToFamilyContainer[innerContext.getContextRoot()];
        std::shared_ptr<ContextContainer> contextContainer = contextFamily->getSubContextContainer(innerContext.getSubContext());

        //Set new parent to be the context container.  Also add it as a child of the context container.
        contextContainer->addChild(nodesInContext[i]);
        nodesInContext[i]->setParent(contextContainer);
    }

    //Move context roots into their ContextFamilyContainers
    for(unsigned long i = 0; i<contextRootNodes.size(); i++){
        std::shared_ptr<Node> asNode = std::dynamic_pointer_cast<Node>(contextRootNodes[i]); //TODO: fix diamond inheritance issue
        std::shared_ptr<SubSystem> origParent = asNode->getParent();

        if(origParent != nullptr){
            origParent->removeChild(asNode);
        }

        std::shared_ptr<ContextFamilyContainer> contextFamilyContainer = contextNodeToFamilyContainer[contextRootNodes[i]];
        contextFamilyContainer->addChild(asNode);
        contextFamilyContainer->setContextRoot(contextRootNodes[i]);
        asNode->setParent(contextFamilyContainer);
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

//Create function to reassign all arcs to nodes within ContextContainers to be to the context itself.  This will be used for scheduling.


//When scheduling nodes within contexts are removed from the scheduler node list to check for zero in degree.  This prevents these nodes from being scheduled outside of their context.  All arcs to these nodes are re-assigned to the ContextContainer for scheduling
//For the scheduler, make a special case for scheduling a context container causing the scheduling algorithm to be called on the graph defined inside the context container.


//For now, the emitter will not track if a temp variable's declaration needs to be moved to allow it to be used outside the local context.  This could occur with contexts not being scheduled together.  It could be aleviated by the emitter checking for context breaks.  However, this will be a future todo for now.