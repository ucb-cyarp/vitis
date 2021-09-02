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
#include "General/EmitterHelpers.h"

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
#include "DummyReplica.h"

#include "GraphCore/StateUpdate.h"

#include "GraphMLTools/GraphMLExporter.h"

#include "MultiThread/ThreadCrossingFIFO.h"
#include "MultiThread/LocklessThreadCrossingFIFO.h"
#include "MultiThread/LocklessInPlaceThreadCrossingFIFO.h"
#include "MultiThread/ConstIOThread.h"
#include "MultiThread/StreamIOThread.h"
#include "MultiThread/MultiThreadEmitterHelpers.h"
#include "MultiThread/MultiThreadTransformHelpers.h"

#include "Estimators/EstimatorCommon.h"
#include "Estimators/ComputationEstimator.h"
#include "Estimators/CommunicationEstimator.h"

#include "MultiRate/MultiRateHelpers.h"
#include "MultiRate/DownsampleClockDomain.h"

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
            std::shared_ptr<Arc> arcToSet = arcs[i];
            arcToSet->setId(newID);
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
    parameters.insert(GraphMLParameter("orig_location", "string", true));
    parameters.insert(GraphMLParameter("node_id", "int", true));

    //Add the static entries for arcs
    parameters.insert(GraphMLParameter("arc_src_port", "int", false));
    parameters.insert(GraphMLParameter("arc_dst_port", "int", false));
    parameters.insert(GraphMLParameter("arc_dst_port_type", "string", false));
    parameters.insert(GraphMLParameter("arc_disp_label", "string", false));
    parameters.insert(GraphMLParameter("arc_datatype", "string", false));
    parameters.insert(GraphMLParameter("arc_dimension", "string", false));
    parameters.insert(GraphMLParameter("arc_complex", "string", false));
    parameters.insert(GraphMLParameter("arc_width", "string", false));
    parameters.insert(GraphMLParameter("arc_id", "int", true));

    //Add the static entries for partition crossing arcs in communication graph
    parameters.insert(GraphMLParameter("partition_crossing_init_state_count_blocks", "int", false));
    parameters.insert(GraphMLParameter("partition_crossing_bytes_per_sample", "int", false));
    parameters.insert(GraphMLParameter("partition_crossing_bytes_per_block", "int", false));
    parameters.insert(GraphMLParameter("partition_crossing_bytes_per_base_rate_sample", "double", false));

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

    //Call propagateProperties on the new nodes in case they need to initilize properties from arc connections
    for(const auto & newNode : newNodes){
        newNode->propagateProperties();
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
        //Remove the node from it's parent's children list allong with any other references the node is aware of
        (*node)->removeKnownReferences();

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

std::pair<std::vector<Variable>, std::vector<std::pair<int, int>>> Design::getCInputVariables() {
    return EmitterHelpers::getCInputVariables(inputMaster);
}

std::pair<std::vector<Variable>, std::vector<std::pair<int, int>>> Design::getCOutputVariables() {
    return EmitterHelpers::getCOutputVariables(outputMaster);
}

std::string Design::getCFunctionArgPrototype(bool forceArray) {
    std::string prototype = "";

    std::pair<std::vector<Variable>, std::vector<std::pair<int, int>>> inputVarsWithRates = getCInputVariables();
    std::vector<Variable> inputVars = inputVarsWithRates.first;
    unsigned long numInputVars = inputVars.size();

    //TODO: Assuming port numbers do not have a discontinuity.  Validate this assumption.
    if(numInputVars>0){
        Variable var = inputVars[0];

        //Note, if it is already an array, preserve the dimensions
        if(forceArray && var.getDataType().isScalar()){
            DataType varType = var.getDataType();
            varType.setDimensions({2});
            var.setDataType(varType);
        }

        prototype += "const " + var.getCVarDecl(false, true, false, true);

        //Check if complex
        if(var.getDataType().isComplex()){
            prototype += ", const " + var.getCVarDecl(true, true, false, true);
        }
    }

    for(unsigned long i = 1; i<numInputVars; i++){
        Variable var = inputVars[i];

        //Note, if it is already an array, preserve the dimensions
        if(forceArray && var.getDataType().isScalar()){
            DataType varType = var.getDataType();
            varType.setDimensions({2});
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

std::string Design::getCOutputStructDefn(int blockSize) {
    std::pair<std::vector<Variable>, std::vector<std::pair<int, int>>> outputVarsPairdWithRates = getCOutputVariables();
    std::vector<int> blockSizes = EmitterHelpers::getBlockSizesFromRates(outputVarsPairdWithRates.second, blockSize);
    return EmitterHelpers::getCIOPortStructDefn(outputVarsPairdWithRates.first, blockSizes, "OutputType");
}

void Design::generateSingleThreadedC(std::string outputDir, std::string designName, SchedParams::SchedType schedType, TopologicalSortParameters topoSortParams, unsigned long blockSize, bool emitGraphMLSched, bool printNodeSched){
    if(schedType == SchedParams::SchedType::BOTTOM_UP)
        emitSingleThreadedC(outputDir, designName, designName, schedType, blockSize);
    else if(schedType == SchedParams::SchedType::TOPOLOGICAL) {
        prune(true);
        pruneUnconnectedArcs(true);

        scheduleTopologicalStort(topoSortParams, false, false, designName, outputDir, printNodeSched, false);
        verifyTopologicalOrder(true, schedType);

        if(emitGraphMLSched) {
            //Export GraphML (for debugging)
            std::cout << "Emitting GraphML Schedule File: " << outputDir << "/" << designName
                      << "_scheduleGraph.graphml" << std::endl;
            GraphMLExporter::exportGraphML(outputDir + "/" + designName + "_scheduleGraph.graphml", *this);
        }

        emitSingleThreadedC(outputDir, designName, designName, schedType, blockSize);
    }else if(schedType == SchedParams::SchedType::TOPOLOGICAL_CONTEXT){
        prune(true);
        pruneUnconnectedArcs(true);

        std::vector<std::shared_ptr<ClockDomain>> clockDomains = findClockDomains();
        //TODO Optimize this so a second pass is not needed
        //After pruning, re-discover clock domain parameters since rate change nodes and links to MasterIO ports may have been removed
        //Before doing that, reset the ClockDomain links for master nodes as ports may have become disconnected
        resetMasterNodeClockDomainLinks();
        MultiRateHelpers::rediscoverClockDomainParameters(clockDomains);

        //ClockDomain Rate Change Specialization.  Convert ClockDomains to UpsampleClockDomains or DownsampleClockDomains
        clockDomains = specializeClockDomains(clockDomains);
        assignNodeIDs();
        assignArcIDs();
        //This also converts RateChangeNodes to Input or Output implementations.
        //This is done before other operations since these operations replace the objects because the class is changed to a subclass

        //AfterSpecialization, create support nodes for clockdomains, particularly DownsampleClockDomains
        createClockDomainSupportNodes(clockDomains, false, true); //Have not done context discovery & marking yet so do not request context inclusion
        assignNodeIDs();
        assignArcIDs();

        //Check the ClockDomain rates are appropriate
        MultiRateHelpers::validateClockDomainRates(clockDomains);

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

        //Moved from before discoverAndMarkContexts to afterEncapsulate contexts to better match the multithreaded emitter
        //Since this is occuring after contexts are discovered and marked, the include context options should be set to true
        createStateUpdateNodes(true); //Done after EnabledSubsystem Contexts are expanded to avoid issues with deleting and re-wiring EnableOutputs
        assignNodeIDs(); //Need to assign node IDs since the node ID is used for set ordering and new nodes may have been added
        assignArcIDs();

        //Export GraphML (for debugging)
//        std::cout << "Emitting GraphML pre-Schedule File: " << outputDir << "/" << designName << "_preSortGraph.graphml" << std::endl;
//        GraphMLExporter::exportGraphML(outputDir+"/"+designName+"_preSchedGraph.graphml", *this);

        scheduleTopologicalStort(topoSortParams, false, true, designName, outputDir, printNodeSched, false); //Pruned before inserting state update nodes
        verifyTopologicalOrder(true, schedType);

        //TODO: ValidateIO Block

        //TODO: Create variables for clock domains

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

    EmitterHelpers::emitParametersHeader(outputDir, designName, blockSize);
}

void Design::emitSingleThreadedOpsBottomUp(std::ofstream &cFile, std::vector<std::shared_ptr<Node>> &nodesWithState, SchedParams::SchedType schedType){
    //Emit compute next states
    cFile << std::endl << "//==== Compute Next States ====" << std::endl;
    unsigned long numNodesWithState = nodesWithState.size();
    for(unsigned long i = 0; i<numNodesWithState; i++){
        std::vector<std::string> nextStateExprs;
        nodesWithState[i]->emitCExprNextState(nextStateExprs, schedType);
        cFile << std::endl << "//---- Compute Next States " << nodesWithState[i]->getFullyQualifiedName() <<" ----" << std::endl;
        cFile << "//~~~~ Orig Path: " << nodesWithState[i]->getFullyQualifiedOrigName()  << "~~~~" << std::endl;


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
        CExpr expr_re = srcNode->emitC(cStatements_re, schedType, srcNodeOutputPortNum, false);
        //emit the expressions
        unsigned long numStatements_re = cStatements_re.size();
        for(unsigned long j = 0; j<numStatements_re; j++){
            cFile << cStatements_re[j] << std::endl;
        }

        //emit the assignment
        Variable outputVar = Variable(outputMaster->getCOutputName(i), outputDataType);
        //TODO: Emit Vector Output Support for Single Threaded Emit
        if(!outputDataType.isScalar()){
            throw std::runtime_error(ErrorHelpers::genErrorStr("Vector/Matrix Output Not Currently Supported For Single Threaded Generator"));
        }
        cFile << "output[0]." << outputVar.getCVarName(false) << " = " << expr_re.getExpr() << ";" << std::endl;

        //Emit Imag if Datatype is complex
        if(outputDataType.isComplex()){
            cFile << std::endl << "//-- Compute Imag Component --" << std::endl;
            std::vector<std::string> cStatements_im;
            CExpr expr_im = srcNode->emitC(cStatements_im, schedType, srcNodeOutputPortNum, true);
            //emit the expressions
            unsigned long numStatements_im = cStatements_im.size();
            for(unsigned long j = 0; j<numStatements_im; j++){
                cFile << cStatements_im[j] << std::endl;
            }

            //emit the assignment
            cFile << "output[0]." << outputVar.getCVarName(true) << " = " << expr_im.getExpr() << ";" << std::endl;
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
                CExpr expr_re = srcNode->emitC(cStatements_re, schedType, srcNodeOutputPortNum, false, true, true);
                //emit the expressions
                unsigned long numStatements_re = cStatements_re.size();
                for(unsigned long j = 0; j<numStatements_re; j++){
                    cFile << cStatements_re[j] << std::endl;
                }

                //emit the assignment
                Variable outputVar = Variable(outputMaster->getCOutputName(i), outputDataType);
                //TODO: Emit Vector Output Support for Single Threaded Emit
                if(!outputDataType.isScalar()){
                    throw std::runtime_error(ErrorHelpers::genErrorStr("Vector/Matrix Output Not Currently Supported For Single Threaded Generator"));
                }
                cFile << "output[0]." << outputVar.getCVarName(false) << " = " << expr_re.getExpr() << ";" << std::endl;

                //Emit Imag if Datatype is complex
                if(outputDataType.isComplex()){
                    cFile << std::endl << "//-- Assign Imag Component --" << std::endl;
                    std::vector<std::string> cStatements_im;
                    CExpr expr_im = srcNode->emitC(cStatements_im, schedType, srcNodeOutputPortNum, true, true, true);
                    //emit the expressions
                    unsigned long numStatements_im = cStatements_im.size();
                    for(unsigned long j = 0; j<numStatements_im; j++){
                        cFile << cStatements_im[j] << std::endl;
                    }

                    //emit the assignment
                    cFile << "output[0]." << outputVar.getCVarName(true) << " = " << expr_im.getExpr() << ";" << std::endl;
                }
            }
        }else if((*it)->hasState()){
            //Call emit for state element input
            //The state element output is treateded similarly to a constant and a variable name is always returned
            //The output has no dependencies within the cycle
            //The input can have dependencies

            //Emit comment
            cFile << std::endl << "//---- Calculate " << (*it)->getFullyQualifiedName() << " Inputs ----" << std::endl;
            cFile << "//~~~~ Orig Path: " << (*it)->getFullyQualifiedOrigName()  << "~~~~" << std::endl;

            std::vector<std::string> nextStateExprs;
            (*it)->emitCExprNextState(nextStateExprs, schedType);

            for(unsigned long j = 0; j<nextStateExprs.size(); j++){
                cFile << nextStateExprs[j] << std::endl;
            }

        }else{
            //Emit standard node

            //Emit comment
            cFile << std::endl << "//---- Calculate " << (*it)->getFullyQualifiedName() << " ----" << std::endl;
            cFile << "//~~~~ Orig Path: " << (*it)->getFullyQualifiedOrigName()  << "~~~~" << std::endl;

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

    EmitterHelpers::emitOpsStateUpdateContext(cFile, schedType, toBeEmittedInThisOrder, outputMaster, blockSize, indVarName);
}

void Design::emitSingleThreadedC(std::string path, std::string fileName, std::string designName, SchedParams::SchedType sched, unsigned long blockSize) {
    EmitterHelpers::stringEmitTypeHeader(path);

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

    std::cout << "Emitting C File: " << path << "/" << fileName << ".h" << std::endl;
    //#### Emit .h file ####
    std::ofstream headerFile;
    headerFile.open(path+"/"+fileName+".h", std::ofstream::out | std::ofstream::trunc);

    std::string fileNameUpper =  GeneralHelper::toUpper(fileName);
    headerFile << "#ifndef " << fileNameUpper << "_H" << std::endl;
    headerFile << "#define " << fileNameUpper << "_H" << std::endl;
    headerFile << "#include <stdint.h>" << std::endl;
    headerFile << "#include <stdbool.h>" << std::endl;
    headerFile << "#include <math.h>" << std::endl;
    headerFile << "#include \"" << VITIS_TYPE_NAME << ".h\"" << std::endl;
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

    std::cout << "Emitting C File: " << path << "/" << fileName << ".c" << std::endl;
    //#### Emit .c file ####
    std::ofstream cFile;
    cFile.open(path+"/"+fileName+".c", std::ofstream::out | std::ofstream::trunc);

    cFile << "#include \"" << fileName << ".h" << "\"" << std::endl;

    //Include any external include statements required by nodes in the design
    std::set<std::string> extIncludes;
    for(int i = 0; i<nodes.size(); i++){
        std::set<std::string> nodeIncludes = nodes[i]->getExternalIncludes();
        extIncludes.insert(nodeIncludes.begin(), nodeIncludes.end());
    }

    for(auto it = extIncludes.begin(); it != extIncludes.end(); it++){
        cFile << *it << std::endl;
    }

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

    //Emit the reset constants
    cFile << "//==== State Var Reset Vals ====" << std::endl;
    for(unsigned long i = 0; i<nodesWithStateCount; i++) {
        std::vector<Variable> stateVars = nodesWithState[i]->getCStateVars();

        unsigned long numStateVars = stateVars.size();
        for(unsigned long j = 0; j<numStateVars; j++) {
            std::vector<std::string> rstConsts = stateVars[j].getResetConst(true);
            for(const std::string &constant : rstConsts){
                cFile << constant << std::endl;
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
    DataType blockDT = DataType(false, false, false, (int) std::ceil(std::log2(blockSize)+1), 0, {1});
    if(blockSize > 1) {
        //TODO: Creatr other variables for clock domains
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
            nodesWithState[i]->emitCStateUpdate(stateUpdateExprs, sched, nullptr);

            unsigned long numStateUpdateExprs = stateUpdateExprs.size();
            for(unsigned long j = 0; j<numStateUpdateExprs; j++){
                cFile << stateUpdateExprs[j] << std::endl;
            }
        }
    }

    if(blockSize > 1) {
        //TODO: Increment other variables here
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
            std::vector<std::string> rstStatements = stateVars[j].genReset();
            for(const std::string &rstStatement : rstStatements){
                cFile << rstStatement << std::endl;
            }
        }
    }

    //Call the reset functions of blackboxes if they have state
    if(blackBoxes.size() > 0) {
        cFile << "//==== Reset BlackBoxes ====" << std::endl;

        for(unsigned long i = 0; i<blackBoxes.size(); i++){
            cFile << blackBoxes[i]->getResetFunctionCall() << ";" << std::endl;
        }
    }

    cFile << "}" << std::endl;

    cFile.close();
}

void Design::emitSingleThreadedCBenchmarkingDrivers(std::string path, std::string fileName, std::string designName, int blockSize) {
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
    benchKernel << "#include \"" << VITIS_TYPE_NAME << ".h\"" << std::endl;
    benchKernel << "void bench_"+fileName+"()\n"
                   "{\n";

    //Reset
    benchKernel << "\t" << designName << "_reset();\n";

    benchKernel << "\tOutputType output;\n"
                   "\tunsigned long outputCount;\n";

    //Generate loop
    std::pair<std::vector<Variable>, std::vector<std::pair<int, int>>> inputVarRatePairs = getCInputVariables();
    std::vector<Variable> inputVars = inputVarRatePairs.first;
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

    //TODO: Update for clock domains
    //For a block size greater than 1, constant arrays need to be created
    for (unsigned long i = 0; i < numInputVars; i++) {
        //Expand the size of the variable to account for the block size;
        Variable blockInputVar = inputVars[i];
        DataType blockDT = blockInputVar.getDataType().expandForBlock(blockSize);

        blockInputVar.setDataType(blockDT);

        std::vector<int> blockedDim = blockDT.getDimensions();

        //Insert comma between variables
        if (i > 0) {
            fctnCall += ", ";
        }

        if(!blockDT.isScalar()) {
            //Create a constant array and pass it as the argument
            benchKernel << "\t" << blockInputVar.getCVarDecl(false, true, false, true)
                        << EmitterHelpers::arrayLiteral(blockedDim, defaultArgs[i].toStringComponent(false, blockDT)) << ";";
            fctnCall += blockInputVar.getCVarName(false);
        }else{
            //Just pass the scalar argument
            fctnCall += defaultArgs[i].toStringComponent(false, blockDT);
        }

        if (inputVars[i].getDataType().isComplex()) {
            fctnCall += ", "; //This is guarenteed to be needed as the real component will come first

            if(!blockDT.isScalar()) {
                //Create a constant array and pass it as the argument
                benchKernel << "\t" << blockInputVar.getCVarDecl(true, true, false, true)
                            << EmitterHelpers::arrayLiteral(blockedDim, defaultArgs[i].toStringComponent(true, blockDT)) << ";";
                fctnCall += blockInputVar.getCVarName(true);
            }else{
                //Just pass the scalar argument
                fctnCall += defaultArgs[i].toStringComponent(true, blockDT);
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
                                   "SYSTEM_CFLAGS = -Ofast -c -g -x c++ -std=c++11 -march=native -masm=att\n"
                                   "#Most kernels are allowed to be optomized.  Most assembly kernels use asm 'volitile' to force execution\n"
                                   "KERNEL_CFLAGS = -Ofast -c -g -std=c++11 -march=native -masm=att\n"
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
    benchKernelMem << "#include \"" << VITIS_TYPE_NAME << ".h\"" << std::endl;
    benchKernelMem << "void bench_"+fileName+"_mem(void** in, void** out)" << std::endl;
    benchKernelMem << "{\n";

    //Reset
    benchKernelMem << "\t" << designName << "_reset();\n";

    //Cast the input and output arrays to the correct type
    std::pair<std::vector<Variable>, std::vector<std::pair<int, int>>> inputVarRatePairs = getCInputVariables();
    std::vector<Variable> inputVars = inputVarRatePairs.first;
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

    //TODO: Update for clock domains

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
                                   "SYSTEM_CFLAGS = -Ofast -c -g -x c++ -std=c++11 -march=native -masm=att\n"
                                   "#Most kernels are allowed to be optomized.  Most assembly kernels use asm 'volitile' to force execution\n"
                                   "KERNEL_CFLAGS = -Ofast -c -g -std=c++11 -march=native -masm=att\n"
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

        //Copy StateUpdate(s).  This should preserve the order of the stateUpdateNodes vector in the copy
        std::vector<std::shared_ptr<StateUpdate>> stateUpdateNodes = origNode->getStateUpdateNodes();
        for(unsigned long j = 0; j<stateUpdateNodes.size(); j++){
            std::shared_ptr<Node> stateUpdateNodeCopy = origToCopyNode[stateUpdateNodes[j]];
            std::shared_ptr<StateUpdate> stateUpdateCopyNodeAsStateUpdate = GeneralHelper::isType<Node, StateUpdate>(stateUpdateNodeCopy);

            if(stateUpdateCopyNodeAsStateUpdate) {
                nodeCopies[i]->addStateUpdateNode(stateUpdateCopyNodeAsStateUpdate);
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
                int numSubContexts = origNodeAsContextRoot->getNumSubContexts();
                for(int j = 0; j<numSubContexts; j++){
                    std::vector<std::shared_ptr<Node>> origContextNodes = origNodeAsContextRoot->getSubContextNodes(j);

                    for(unsigned long k = 0; k<origContextNodes.size(); k++){
                        nodeCopyAsContextRoot->addSubContextNode(j, origToCopyNode[origContextNodes[k]]);
                    }
                }

                //Copy Dummy Nodes
                std::map<int, std::shared_ptr<DummyReplica>> origDummyReplicas = origNodeAsContextRoot->getDummyReplicas();
                for(auto origDummyNode = origDummyReplicas.begin(); origDummyNode != origDummyReplicas.end(); origDummyNode++){
                    std::shared_ptr<Node> copyDummyReplicaNode = origToCopyNode[origDummyNode->second];
                    std::shared_ptr<DummyReplica> copyDummyReplica = std::dynamic_pointer_cast<DummyReplica>(copyDummyReplicaNode);
                    if(copyDummyReplica) {
                        nodeCopyAsContextRoot->setDummyReplica(origDummyNode->first, copyDummyReplica);
                    }else{
                        throw std::runtime_error(ErrorHelpers::genErrorStr("Unable to find cloned dummy node", nodeCopies[i]));
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

            //Set the relationships of the context family container
            contextFamilyContainerOrig->cloneContextFamilyContainerRelationships(origToCopyNode);
        }

        if(GeneralHelper::isType<Node, DummyReplica>(nodeCopies[i]) != nullptr){
            //Set the dummyOf field
            std::shared_ptr<DummyReplica> copyNodeAsDummyReplica = std::static_pointer_cast<DummyReplica>(nodeCopies[i]);
            std::shared_ptr<DummyReplica> origNodeAsDummyReplica = std::dynamic_pointer_cast<DummyReplica>(copyToOrigNode[copyNodeAsDummyReplica]);
            std::shared_ptr<ContextRoot> origDummyOf = origNodeAsDummyReplica->getDummyOf();
            std::shared_ptr<Node> copyDummyOf = origToCopyNode[std::dynamic_pointer_cast<Node>(origDummyOf)];
            copyNodeAsDummyReplica->setDummyOf(std::dynamic_pointer_cast<ContextRoot>(copyDummyOf));
        }

        if(GeneralHelper::isType<Node, ThreadCrossingFIFO>(nodeCopies[i]) != nullptr){
            std::shared_ptr<ThreadCrossingFIFO> copyNodeAsThreadCrossingFIFO = std::static_pointer_cast<ThreadCrossingFIFO>(nodeCopies[i]);
            std::shared_ptr<ThreadCrossingFIFO> origNodeAsThreadCrossingFIFO = std::dynamic_pointer_cast<ThreadCrossingFIFO>(copyToOrigNode[copyNodeAsThreadCrossingFIFO]);
            std::vector<std::shared_ptr<ClockDomain>> origClockDomains = origNodeAsThreadCrossingFIFO->getClockDomains();
            for(int j = 0; j<origClockDomains.size(); j++) {
                std::shared_ptr<ClockDomain> cloneClockDomain = std::dynamic_pointer_cast<ClockDomain>(origToCopyNode[origClockDomains[j]]);
                copyNodeAsThreadCrossingFIFO->setClockDomain(j, cloneClockDomain);
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

//    designCopy.arcs = arcCopies;

    //Copy the arc list in the same order
    std::vector<std::shared_ptr<Arc>> arcCopiesOrdered;
    for(unsigned long i = 0; i<arcs.size(); i++){
        if(origToCopyArc.find(arcs[i]) == origToCopyArc.end()){
            throw std::runtime_error("While cloning design, found an arc that has no clone: [" + arcs[i]->getSrcPort()->getParent()->getFullyQualifiedName() + " -> " + arcs[i]->getDstPort()->getParent()->getFullyQualifiedName() + "]");
        }

        arcCopiesOrdered.push_back(origToCopyArc[arcs[i]]);
    }

    designCopy.arcs = arcCopiesOrdered;

    //Update ClockDomain ports (Using arc copies)
    for (unsigned long i = 0; i < numNodes; i++) {
        if(GeneralHelper::isType<Node, ClockDomain>(nodes[i])){
            std::shared_ptr<ClockDomain> origClkDomain = std::static_pointer_cast<ClockDomain>(nodes[i]);
            std::shared_ptr<ClockDomain> cloneClkDomain = std::dynamic_pointer_cast<ClockDomain>(origToCopyNode[origClkDomain]);

            //Update the IO input ports
            std::set<std::shared_ptr<OutputPort>> origInputs = origClkDomain->getIoInput();
            for(auto origPort = origInputs.begin(); origPort != origInputs.end(); origPort++){
                //Get one of the output arcs of this port (dosn't matter which one but there should be at least one since it was connected to a node in the clock domain)
                std::shared_ptr<Arc> origArc = *((*origPort)->getArcs().begin());
                //Get the cloned arc
                std::shared_ptr<Arc> cloneArc = origToCopyArc[origArc];
                if(cloneArc == nullptr){
                    throw std::runtime_error(ErrorHelpers::genErrorStr("Error when copying ports of clock domain", nodes[i]));
                }
                //Get the source port of this cloned arc (for which this arc is an output)
                std::shared_ptr<OutputPort> clonePort = cloneArc->getSrcPort();
                //Add this to the set
                cloneClkDomain->addIOInput(clonePort);
            }

            //Update the IO output ports
            std::set<std::shared_ptr<InputPort>> origOutputs = origClkDomain->getIoOutput();
            for(auto origPort = origInputs.begin(); origPort != origInputs.end(); origPort++){
                //Get the input arc to this port (it should exist as it was conneted to a node in the clock domain
                std::shared_ptr<Arc> origArc = *((*origPort)->getArcs().begin());
                //Get the cloned arc
                std::shared_ptr<Arc> cloneArc = origToCopyArc[origArc];
                if(cloneArc == nullptr){
                    throw std::runtime_error(ErrorHelpers::genErrorStr("Error when copying ports of clock domain", nodes[i]));
                }
                //Get the destination port of this cloned arc (for which this arc is an input)
                std::shared_ptr<InputPort> clonePort = cloneArc->getDstPort();
                //Add this to the set
                cloneClkDomain->addIOOutput(clonePort);
            }

            //Update ContextDrivers for DownsampleSubsystems
            //TODO: Refactor into a function in Node which runs after the initial clone and allows initialization of
            //graph references

            if(GeneralHelper::isType<Node, DownsampleClockDomain>(nodes[i])){
                std::shared_ptr<DownsampleClockDomain> origClkDomainDownsample = std::static_pointer_cast<DownsampleClockDomain>(nodes[i]);
                std::shared_ptr<DownsampleClockDomain> cloneClkDomainDownsample = std::dynamic_pointer_cast<DownsampleClockDomain>(origToCopyNode[origClkDomainDownsample]);

                std::shared_ptr<Arc> origDriver = origClkDomainDownsample->getContextDriver();
                if(origDriver) {
                    std::shared_ptr<Arc> copyArc = origToCopyArc[origDriver];
                    if(copyArc == nullptr){
                        throw std::runtime_error(ErrorHelpers::genErrorStr("Error when copying driver of clock domain", nodes[i]));
                    }
                    cloneClkDomainDownsample->setContextDriver(copyArc);
                }else{
                    throw std::runtime_error(ErrorHelpers::genErrorStr("Error when copying driver of clock domain.  No driver", nodes[i]));
                }
            }

        }
    }

    //Update master port clock domain references
    MultiRateHelpers::cloneMasterNodePortClockDomains(inputMaster, designCopy.inputMaster, origToCopyNode);
    MultiRateHelpers::cloneMasterNodePortClockDomains(outputMaster, designCopy.outputMaster, origToCopyNode);
    MultiRateHelpers::cloneMasterNodePortClockDomains(visMaster, designCopy.visMaster, origToCopyNode);
    MultiRateHelpers::cloneMasterNodePortClockDomains(unconnectedMaster, designCopy.unconnectedMaster, origToCopyNode);
    MultiRateHelpers::cloneMasterNodePortClockDomains(terminatorMaster, designCopy.terminatorMaster, origToCopyNode);

    //Copy topLevelNode entries
    for(unsigned long i = 0; i<numTopLevelNodes; i++){
        designCopy.topLevelNodes.push_back(origToCopyNode[topLevelNodes[i]]);
    }

    return designCopy;
}

void Design::removeNode(std::shared_ptr<Node> node) {
    //Disconnect arcs from the node
    std::set<std::shared_ptr<Arc>> disconnectedArcs = node->disconnectNode();

    //Remove any known references
    node->removeKnownReferences();

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
            //All nodes need to be removed from their parent while some node, such as EnableInput and EnableOutput,
            //need to remove additional references to themselves before being deleted.  These tasks are now handled
            //by removeKnownReferences
            (*it)->removeKnownReferences();

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
        //Removal from parent now occurs as part of the removeKnownReferences function executed above
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

void Design::verifyTopologicalOrder(bool checkOutputMaster, SchedParams::SchedType schedType) {
    //First, check that the output is scheduled (so long as there are output arcs)
    if(checkOutputMaster && outputMaster->inDegree() > 0){
        if(outputMaster->getSchedOrder() == -1){
            throw std::runtime_error("Topological Order Validation: Output was not scheduled even though the system does have outputs.");
        }
    }

    for(unsigned long i = 0; i<arcs.size(); i++){
        //Check if the src node is a constant or the input master node
        //If so, do not check the ordering
        std::shared_ptr<Arc> arc = arcs[i];
        std::shared_ptr<Node> srcNode = arc->getSrcPort()->getParent();

        //The one case that needs to be checked if the src has state is the StateUpdate node for the given state element
        //Otherwise the outputs of nodes with state are considered constants
        //Another exception is EnableOutput nodes which should be checked
        std::shared_ptr<Node> dstNode = arcs[i]->getDstPort()->getParent();
        std::shared_ptr<StateUpdate> dstNodeAsStateUpdate = GeneralHelper::isType<Node, StateUpdate>(dstNode);

        bool shouldCheck;
        if(srcNode != inputMaster && srcNode != unconnectedMaster && srcNode != terminatorMaster && GeneralHelper::isType<Node, Constant>(srcNode) == nullptr && GeneralHelper::isType<Node, ThreadCrossingFIFO>(srcNode) == nullptr) {
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
            if (srcNode->getSchedOrder() == -1) {
                //Src node is unscheduled
                throw std::runtime_error(ErrorHelpers::genErrorStr(
                        "Topological Order Validation: Src Node (" + srcNode->getFullyQualifiedName() +
                        ") [Sched Order: " + GeneralHelper::to_string(srcNode->getSchedOrder()) + ", ID: " +
                        GeneralHelper::to_string(srcNode->getId()) + ", Part: " + GeneralHelper::to_string(srcNode->getPartitionNum()) +
                        "] is Unscheduled"));
            } else {
                //Src node is scheduled
                if (dstNode->getSchedOrder() != -1) {
                    //Dst node is scheduled
                    if (srcNode->getSchedOrder() >= dstNode->getSchedOrder()) {
                        throw std::runtime_error(
                                "Topological Order Validation: Src Node (" + srcNode->getFullyQualifiedName() +
                                ") [Sched Order: " + GeneralHelper::to_string(srcNode->getSchedOrder()) + ", ID: " +
                                GeneralHelper::to_string(srcNode->getId()) + ", Part: " + GeneralHelper::to_string(srcNode->getPartitionNum()) +
                                "] is not Scheduled before Dst Node (" + dstNode->getFullyQualifiedName() +
                                ") [Sched Order: " + GeneralHelper::to_string(dstNode->getSchedOrder()) + ", ID: " +
                                GeneralHelper::to_string(dstNode->getId()) + ", Part: " + GeneralHelper::to_string(dstNode->getPartitionNum()) + "]");
                    }
                }else{
                    throw std::runtime_error(
                            "Topological Order Validation: Src Node (" + srcNode->getFullyQualifiedName() +
                            ") [Sched Order: " + GeneralHelper::to_string(srcNode->getSchedOrder()) + ", ID: " +
                            GeneralHelper::to_string(srcNode->getId()) + ", Part: " + GeneralHelper::to_string(srcNode->getPartitionNum()) +
                            "] is scheduled but Dst Node (" + dstNode->getFullyQualifiedName() +
                            ") [Sched Order: " + GeneralHelper::to_string(dstNode->getSchedOrder()) + ", ID: " +
                            GeneralHelper::to_string(dstNode->getId()) + ", Part: " + GeneralHelper::to_string(dstNode->getPartitionNum()) + "] is not.");
                }
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
                std::vector<std::shared_ptr<Arc>> drivers;

                if(schedType == SchedParams::SchedType::TOPOLOGICAL_CONTEXT) {
                    //Topoligical context creates new context driver arcs durring encapsulation (for each partition the context is present in)
                    //We will check against these as this can handle multiple partition scheduling where
                    //the relative order of the schedule between the different partitons does not matter
                    //but where the context driver arc may produce a false depencency

                    //Check if the context is a clock domain.  For clock domains, only perform the checks if the
                    //partition is not suppressing clock domain logic (ie. multiple clock domains or rate change nodes contained in partition)
                    std::shared_ptr<ClockDomain> contextRootAsClkDomain = GeneralHelper::isType<ContextRoot, ClockDomain>(context[j].getContextRoot());

                    bool check = contextRootAsClkDomain == nullptr; //Check it if contect is not a clock domain
                    if(contextRootAsClkDomain){
                        std::set<int> suppressedPartitions = contextRootAsClkDomain->getSuppressClockDomainLogicForPartitions();
                        check = suppressedPartitions.find(nodes[i]->getPartitionNum()) == suppressedPartitions.end(); //Check if the partition is not included in the suppressed partitions list
                    }

                    if(check) {
                        drivers = context[j].getContextRoot()->getContextDriversForPartition(
                                nodes[i]->getPartitionNum());

                        std::vector<std::shared_ptr<Arc>> origDriver = context[j].getContextRoot()->getContextDecisionDriver(); //This is the context driver before encapsulation

                        //Sanity check if no context partition drivers exist.
                        if (drivers.empty() && !origDriver.empty()) {
                            std::shared_ptr<Node> asNode = std::dynamic_pointer_cast<Node>(context[j].getContextRoot());
                            throw std::runtime_error(ErrorHelpers::genErrorStr(
                                    "ContextRoot Found that has context drivers but no context drivers for a given partition.  This should have been created during encapsulation.",
                                    asNode));
                        }
                    }
                }else{
                    drivers = context[j].getContextRoot()->getContextDecisionDriver();
                }

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
                                    GeneralHelper::to_string(driverSrc->getId()) + ", Part: " + GeneralHelper::to_string(driverSrc->getPartitionNum()) +
                                    "] is not Scheduled before Context Node (" + nodes[i]->getFullyQualifiedName() +
                                    ") [Sched Order: " + GeneralHelper::to_string(nodes[i]->getSchedOrder()) +
                                    ", ID: " +
                                    GeneralHelper::to_string(nodes[i]->getId()) + ", Part: " + GeneralHelper::to_string(nodes[i]->getPartitionNum()) + "]");
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
        sortedNodes = GraphAlgs::topologicalSortDestructive(params, topLevelContextNodes, false, -1, arcsToDelete, outputMaster,
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

std::vector<std::shared_ptr<Node>> Design::topologicalSortDestructive(std::string designName, std::string dir, TopologicalSortParameters params, int partitionNum) {
    std::vector<std::shared_ptr<Arc>> arcsToDelete;
    std::vector<std::shared_ptr<Node>> topLevelContextNodesInPartition = GraphAlgs::findNodesStopAtContextFamilyContainers(topLevelNodes, partitionNum);

    if(partitionNum == IO_PARTITION_NUM) {
        topLevelContextNodesInPartition.push_back(outputMaster);
        topLevelContextNodesInPartition.push_back(visMaster);
    }

    std::vector<std::shared_ptr<Node>> sortedNodes;

    bool failed = false;
    std::exception err;
    try {
        sortedNodes = GraphAlgs::topologicalSortDestructive(params, topLevelContextNodesInPartition, true, partitionNum,
                                                            arcsToDelete, outputMaster, inputMaster, terminatorMaster,
                                                            unconnectedMaster, visMaster);
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
        std::cerr << "Error when scheduling partition " << partitionNum << std::endl;
        std::cerr << "Emitting: " + dir + "/" + designName + "_sort_error_scheduleGraph.graphml" << std::endl;
        GraphMLExporter::exportGraphML(dir + "/" + designName + "_sort_error_scheduleGraph.graphml", *this);

        throw err;
    }

    return sortedNodes;
}

unsigned long Design::scheduleTopologicalStort(TopologicalSortParameters params, bool prune, bool rewireContexts, std::string designName, std::string dir, bool printNodeSched, bool schedulePartitions) {
    //TODO: WARNING: UpsampleClockDomains currently rely on all of their nodes being scheduled together (ie not being split up).
    //This currently is provided by the hierarchical implementation of the scheduler.  However, if this were to be changed
    //later, a method for having vector intermediates would be required.

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

            //Thread crossing FIFOs will have their outputs disconnected by this method as they declare that they have state and no combinational path

            //Note, arcs to state update nodes should not be removed (except for ThreadCrossingFIFOs).

            std::set<std::shared_ptr<Arc>> outputArcs = designClone.nodes[i]->getOutputArcs();
            for(auto it = outputArcs.begin(); it != outputArcs.end(); it++){
                //Check if the dst is a state update.  Keep arcs to state update nodes
                std::shared_ptr<StateUpdate> dstAsStateUpdate = GeneralHelper::isType<Node, StateUpdate>((*it)->getDstPort()->getParent());
                if(dstAsStateUpdate == nullptr){
                    (*it)->disconnect();
                    arcsToDelete.insert(*it);
                }else if(GeneralHelper::isType<Node, ThreadCrossingFIFO>(designClone.nodes[i])){
                    //The destination is a state update but the src is a ThreadCrossingFIFO.
                    //The ThreadCrossingFIFO data should be from another partition and the dependency is effectively handled by
                    //reading rhe ThreadCrossingFIFO at the start of function execution (or on demand during execution if that
                    //model is implemented).  The possible exception to this is when the ThreadCrossingFIFO's input is
                    //a stateful element in which case an order constraint arc is possible and should be honnored.  Note that
                    //ThreadCrossingFIFOs exist in the src partition of the transfer.  It should not be possible for the FIFO's own
                    //state update node to be in a different partition.  This condition will be checked.
                    //Therefore, the arc will be removed only if the ThreadCrossingFIFO and StateUpdate are in different partitions.

                    if(designClone.nodes[i]->getPartitionNum() != dstAsStateUpdate->getPartitionNum()){
                        if(dstAsStateUpdate->getPrimaryNode() == designClone.nodes[i]){
                            throw std::runtime_error(ErrorHelpers::genErrorStr("Arc from ThreadCrossingFIFO to its own state update in another partition", designClone.nodes[i]));
                        }else{
                            (*it)->disconnect();
                            arcsToDelete.insert(*it);
                        }
                    }
                    //Else, this arc to the state update
                }
                //Else, this is a State node connected to its StateUpdate node and should not be removed
            }
        }
    }

    for(auto it = nodesToRemove.begin(); it != nodesToRemove.end(); it++){
        (*it)->removeKnownReferences();

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

//    std::cout << "Emitting: ./rev1BB_receiver_rev1-4_blockLMS_splitEQ/context_scheduleGraph.graphml" << std::endl;
//    GraphMLExporter::exportGraphML("./rev1BB_receiver_rev1-4_blockLMS_splitEQ/context_scheduleGraph.graphml", designClone);

    //==== Topological Sort (Destructive) ====
    if(schedulePartitions) {
        std::set<int> partitions = listPresentPartitions();

        for(auto partitionBeingScheduled = partitions.begin(); partitionBeingScheduled != partitions.end(); partitionBeingScheduled++) {
            std::vector<std::shared_ptr<Node>> schedule = designClone.topologicalSortDestructive(designName, dir, params, *partitionBeingScheduled);

            if (printNodeSched) {
                std::cout << "Schedule [Partition: " << *partitionBeingScheduled << "]" << std::endl;
                for (unsigned long i = 0; i < schedule.size(); i++) {
                    std::cout << i << ": " << schedule[i]->getFullyQualifiedName() << std::endl;
                }
                std::cout << std::endl;
            }

            //==== Back Propagate Schedule ====
            for (unsigned long i = 0; i < schedule.size(); i++) {
                //Index is the schedule number
                std::shared_ptr<Node> origNode = clonedToOrigNodes[schedule[i]];
                origNode->setSchedOrder(i);
            }
        }
    }else{
        std::vector<std::shared_ptr<Node>> schedule = designClone.topologicalSortDestructive(designName, dir, params);

        if (printNodeSched) {
            std::cout << "Schedule" << std::endl;
            for (unsigned long i = 0; i < schedule.size(); i++) {
                std::cout << i << ": " << schedule[i]->getFullyQualifiedName() << std::endl;
            }
            std::cout << std::endl;
        }

        //==== Back Propagate Schedule ====
        for (unsigned long i = 0; i < schedule.size(); i++) {
            //Index is the schedule number
            std::shared_ptr<Node> origNode = clonedToOrigNodes[schedule[i]];
            origNode->setSchedOrder(i);
        }
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
    std::vector<std::shared_ptr<ClockDomain>> discoveredClockDomains;
    std::vector<std::shared_ptr<Node>> discoveredGeneral;

    //The top level context stack has no entries
    std::vector<Context> initialStack;

    //Discover contexts at the top layer (and below non-enabled subsystems).  Also set the contexts of these top level nodes
    GraphAlgs::discoverAndUpdateContexts(topLevelNodes, initialStack, discoveredMuxes, discoveredEnabledSubsystems,
                                         discoveredClockDomains, discoveredGeneral);

    //Add context nodes (muxes, enabled subsystems, and clock domains) to the topLevelContextRoots list
    for(unsigned long i = 0; i<discoveredMuxes.size(); i++){
        topLevelContextRoots.push_back(discoveredMuxes[i]);
    }
    for(unsigned long i = 0; i<discoveredEnabledSubsystems.size(); i++){
        topLevelContextRoots.push_back(discoveredEnabledSubsystems[i]);
    }
    for(unsigned long i = 0; i<discoveredClockDomains.size(); i++){
        std::shared_ptr<ContextRoot> clkDomainAsContextRoot = std::dynamic_pointer_cast<ContextRoot>(discoveredClockDomains[i]);
        if(clkDomainAsContextRoot == nullptr){
            throw std::runtime_error(ErrorHelpers::genErrorStr("Discovered a clock domain that is not a ContextRoot when discovering and marking contexts", discoveredClockDomains[i]));
        }
        topLevelContextRoots.push_back(clkDomainAsContextRoot);
    }

    //Get and mark the Mux contexts
    Mux::discoverAndMarkMuxContextsAtLevel(discoveredMuxes);

    //Recursivly call on the discovered enabled subsystems
    for(unsigned long i = 0; i<discoveredEnabledSubsystems.size(); i++) {
        discoveredEnabledSubsystems[i]->discoverAndMarkContexts(initialStack);
    }

    //Recursivly call on the discovered clock domains
    for(unsigned long i = 0; i<discoveredClockDomains.size(); i++) {
        discoveredClockDomains[i]->discoverAndMarkContexts(initialStack);
    }

}

void Design::createStateUpdateNodes(bool includeContext) {
    //Find nodes with state in design
    std::vector<std::shared_ptr<Node>> nodesWithState = findNodesWithState();

    std::vector<std::shared_ptr<Node>> newNodes;
    std::vector<std::shared_ptr<Node>> deletedNodes;
    std::vector<std::shared_ptr<Arc>> newArcs;
    std::vector<std::shared_ptr<Arc>> deletedArcs;

    for(unsigned long i = 0; i<nodesWithState.size(); i++){
        nodesWithState[i]->createStateUpdateNode(newNodes, deletedNodes, newArcs, deletedArcs, includeContext);
    }

    addRemoveNodesAndArcs(newNodes, deletedNodes, newArcs, deletedArcs);
}

void Design::createContextVariableUpdateNodes(bool includeContext) {
    //Find nodes with state in design
    std::vector<std::shared_ptr<ContextRoot>> contextRoots = findContextRoots();

    for(unsigned long i = 0; i<contextRoots.size(); i++){
        std::vector<std::shared_ptr<Node>> newNodes;
        std::vector<std::shared_ptr<Node>> deletedNodes;
        std::vector<std::shared_ptr<Arc>> newArcs;
        std::vector<std::shared_ptr<Arc>> deletedArcs;

        contextRoots[i]->createContextVariableUpdateNodes(newNodes, deletedNodes, newArcs, deletedArcs, includeContext);

        addRemoveNodesAndArcs(newNodes, deletedNodes, newArcs, deletedArcs);
    }
}

std::vector<std::shared_ptr<Node>> Design::findNodesWithState() {
    return EmitterHelpers::findNodesWithState(nodes);
}

std::vector<std::shared_ptr<Node>> Design::findNodesWithGlobalDecl() {
    return EmitterHelpers::findNodesWithGlobalDecl(nodes);
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
            int partitionNum = nodesInContext[i]->getPartitionNum();
            getContextFamilyContainerCreateIfNotNoParent(contextStack[j].getContextRoot(), partitionNum);
        }

        Context innerContext = contextStack[contextStack.size()-1];
        std::shared_ptr<ContextFamilyContainer> contextFamily = getContextFamilyContainerCreateIfNotNoParent(innerContext.getContextRoot(), nodesInContext[i]->getPartitionNum());
        std::shared_ptr<ContextContainer> contextContainer = contextFamily->getSubContextContainer(innerContext.getSubContext());

        //Set new parent to be the context container.  Also add it as a child of the context container.
        //Note, removed from previous parent above
        contextContainer->addChild(nodesInContext[i]);
        nodesInContext[i]->setParent(contextContainer);
    }

    //Move context roots into their ContextFamilyContainers.  Note, the ContextStack of the ContextRootNode does not include its own context
    for(unsigned long i = 0; i<contextRootNodes.size(); i++){
        std::shared_ptr<Node> asNode = std::dynamic_pointer_cast<Node>(contextRootNodes[i]); //TODO: fix diamond inheritance issue
        std::shared_ptr<SubSystem> origParent = asNode->getParent();

        if(origParent != nullptr){
            origParent->removeChild(asNode);
        }else{
            //If has no parent, remove from topLevelNodes
            topLevelNodes.erase(std::remove(topLevelNodes.begin(), topLevelNodes.end(), asNode), topLevelNodes.end());
        }

        std::shared_ptr<ContextFamilyContainer> contextFamilyContainer = getContextFamilyContainerCreateIfNotNoParent(contextRootNodes[i], asNode->getPartitionNum());
        contextFamilyContainer->addChild(asNode);
        contextFamilyContainer->setContextRoot(contextRootNodes[i]);
        asNode->setParent(contextFamilyContainer);

        //Also move dummy nodes of this ContextRoot into their relevent ContextFamilyContainers
        std::map<int, std::shared_ptr<DummyReplica>> dummyReplicas = contextRootNodes[i]->getDummyReplicas();
        for(auto dummyPair = dummyReplicas.begin(); dummyPair != dummyReplicas.end(); dummyPair++){
            int dummyPartition = dummyPair->first;
            std::shared_ptr<DummyReplica> dummyNode = dummyPair->second;
            std::shared_ptr<SubSystem> dummyOrigParent = dummyNode->getParent();

            //Move the dummy node
            if(dummyOrigParent != nullptr){
                dummyOrigParent->removeChild(dummyNode);
            }

            std::shared_ptr<ContextFamilyContainer> contextFamilyContainer = getContextFamilyContainerCreateIfNotNoParent(contextRootNodes[i], dummyPartition);
            contextFamilyContainer->addChild(dummyNode);
            //Set as the dummy node for ContextFamilyContainer
            contextFamilyContainer->setDummyNode(dummyNode);
            dummyNode->setParent(contextFamilyContainer);
        }
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
                if(newParentFamilyContainers.find(it->second->getPartitionNum()) == newParentFamilyContainers.end()){
                    throw std::runtime_error(ErrorHelpers::genErrorStr("Error while encapsulating contexts.  Unable to find parent ContextFamilyContainer for specified partition", asNode));
                }
                std::shared_ptr<ContextContainer> newParent = newParentFamilyContainers[it->second->getPartitionNum()]->getSubContextContainer(innerContext.getSubContext());

                std::shared_ptr<ContextFamilyContainer> toBeMoved = it->second;

                if(toBeMoved->getParent()){
                    toBeMoved->getParent()->removeChild(toBeMoved);
                }
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

    std::vector<std::shared_ptr<Node>> new_nodes;
    std::vector<std::shared_ptr<Node>> deleted_nodes;
    std::vector<std::shared_ptr<Arc>> new_arcs;
    std::vector<std::shared_ptr<Arc>> deleted_arcs;

    //Call the recursive function on any subsystem at the top level
    for(unsigned long i = 0; i<nodes.size(); i++){
        if(nodes[i]->inDegree() == 0){
            //Needs to be order constrained if in a context

            std::vector<Context> contextStack = nodes[i]->getContext();
            if(contextStack.size() > 0){
                //Create new order constraint arcs for this node.

                //Run through the context stack
                for(unsigned long j = 0; j<contextStack.size(); j++){
                    std::shared_ptr<ContextRoot> contextRoot = contextStack[j].getContextRoot();
                    std::vector<std::shared_ptr<Arc>> driverArcs = contextRoot->getContextDriversForPartition(nodes[i]->getPartitionNum());
                    if(driverArcs.size() == 0 && contextRoot->getContextDecisionDriver().size() > 0){
                        throw std::runtime_error(ErrorHelpers::genErrorStr("When order constraining this node, found no context decision drivers for the partition it is in but found general context driver.  This function was likely called before partitioning and/or encapsulation", nodes[i]));
                    }

                    for(unsigned long k = 0; k<driverArcs.size(); k++){
                        std::shared_ptr<OutputPort> driverSrc = driverArcs[k]->getSrcPort();
                        std::shared_ptr<Arc> orderConstraintArc = Arc::connectNodes(driverSrc, nodes[i]->getOrderConstraintInputPortCreateIfNot(), driverArcs[k]->getDataType(), driverArcs[k]->getSampleTime());
                        new_arcs.push_back(orderConstraintArc);
                    }
                }
            }
        }
    }

    addRemoveNodesAndArcs(new_nodes, deleted_nodes, new_arcs, deleted_arcs);
}

void Design::propagatePartitionsFromSubsystemsToChildren(){
    std::set<std::shared_ptr<Node>> topLevelNodeSet;
    topLevelNodeSet.insert(topLevelNodes.begin(), topLevelNodes.end());

    MultiThreadTransformHelpers::propagatePartitionsFromSubsystemsToChildren(topLevelNodeSet, -1);
}

std::vector<std::shared_ptr<ContextRoot>> Design::findContextRoots() {
    return EmitterHelpers::findContextRoots(nodes);
}

std::vector<std::shared_ptr<BlackBox>> Design::findBlackBoxes(){
    return EmitterHelpers::findBlackBoxes(nodes);
}

//TODO: Check
void Design::rewireArcsToContexts(std::vector<std::shared_ptr<Arc>> &origArcs,
                                  std::vector<std::vector<std::shared_ptr<Arc>>> &contextArcs) {
    //First, let's rewire the ContextRoot driver arcs
    std::vector<std::shared_ptr<ContextRoot>> contextRoots = findContextRoots();
    std::set<std::shared_ptr<Arc>, Arc::PtrID_Compare> contextRootOrigArcs, contextRootRewiredArcs; //Want Arc orders to be consistent across runs

    //For each discovered context root
    //Mark the driver arcs to be removed because order constraint arcs representing the drivers for each partition's
    //ContextFamilyContainer should have already been created durring encapsulation.  This should include an arc to the
    //ContextFamilyContainer in the same partition as the ContextRoot.  In the past, this function would create replicas
    //of the context driver arcs for each partition.  However, that could result in undesierable behavior if a context
    //is split across partitions.  Normally, unless the ContextRoot requests driver node replication, FIFOs are inserted
    //at the order constraint arcs introduced during encapsulation that span partitions.  Arcs after these FIFOs are
    //removed before this function is called as part of scheduling because the FIFOs are stateful nodes with no
    //cobinational path.  This prevents the partition context drivers from becoming false cross partition dependencies

    //NOTE: Clock domain

    //Mark the origional (before encapsulation) ContextDriverArcs for deletion
    for(unsigned long i = 0; i<contextRoots.size(); i++){
        std::shared_ptr<ContextRoot> contextRoot = contextRoots[i];
        std::vector<std::shared_ptr<Arc>> driverArcs = contextRoot->getContextDecisionDriver();

        for(unsigned long j = 0; j<driverArcs.size(); j++){
            //The driver arcs must have the destination re-wired.  At a minimum, they should be rewired to
            //the context container node.  However, if they exist inside of a nested context, the destination
            //may need to be rewired to another context family container higher in the hierarchy

            //TODO: Remove check
            if(driverArcs[j] == nullptr){
                throw std::runtime_error(ErrorHelpers::genErrorStr("Null Arc discovered when rewiring driver arcs"));
            }

            std::shared_ptr<Arc> driverArc = driverArcs[j];

            origArcs.push_back(driverArcs[j]);
            contextRootOrigArcs.insert(driverArcs[j]);
            contextArcs.push_back({}); //Replace with nothing
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
        std::shared_ptr<DummyReplica> srcAsDummyReplica = GeneralHelper::isType<Node, DummyReplica>(candidateArc->getSrcPort()->getParent());
        if(srcAsContextRoot){
            srcContext.push_back(Context(srcAsContextRoot, -1));
        }else if(srcAsDummyReplica){
            srcContext.push_back(Context(srcAsDummyReplica->getDummyOf(), -1));
        }
        std::shared_ptr<ContextRoot> dstAsContextRoot = GeneralHelper::isType<Node, ContextRoot>(candidateArc->getDstPort()->getParent());
        std::shared_ptr<DummyReplica> dstAsDummyReplica = GeneralHelper::isType<Node, DummyReplica>(candidateArc->getDstPort()->getParent());
        if(dstAsContextRoot){
            dstContext.push_back(Context(dstAsContextRoot, -1));
        }else if(dstAsDummyReplica){
            dstContext.push_back(Context(dstAsDummyReplica->getDummyOf(), -1));
        }

        bool rewireSrc = false;
        bool rewireDst = false;

        //Basically, we need to re-wire the arc if the src and destination are not in the same context for the perpose of scheduling
        //If the the destination is in a subcontext of where the src is, the source does not need to be rewired.
        //However, the destination will need to be re-wired up to the ContextFamilyContainer that resides at the same
        //Context level as the source

        //If the src is in a subcontext compared to the destination, the source needs to be re-wired up to the ContextFamilyContainer
        //that resides at the same level as the destination.  The destination does not need to be re-wired.

        //The case where both the source and destination need to be re-wired is when they are in seperate subcontexts
        //and one is not nested inside the other.

        //We may also need to re-wire the the arc for nodes in the same context if they are in different partitions

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
                visOutput->setPartitionNum(contextAsEnabledSubsys->getPartitionNum());
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

    std::vector<std::shared_ptr<Node>> nodesToSearch = nodes;
    //Add the relevent master nodes
    nodesToSearch.push_back(inputMaster);
    nodesToSearch.push_back(outputMaster);
    nodesToSearch.push_back(visMaster);

    for(unsigned long i = 0; i<nodesToSearch.size(); i++){
        int nodePartition = nodesToSearch[i]->getPartitionNum();
        //Ignore standard subsystems in partition -1
        if(nodePartition == -1){
            if(GeneralHelper::isType<Node, SubSystem>(nodesToSearch[i]) == nullptr
                    || GeneralHelper::isType<Node, ContextFamilyContainer>(nodesToSearch[i]) != nullptr
                    || GeneralHelper::isType<Node, ContextContainer>(nodesToSearch[i]) != nullptr
                    || GeneralHelper::isType<Node, EnabledSubSystem>(nodesToSearch[i]) != nullptr){
                partitions[nodePartition].push_back(nodesToSearch[i]); //The map returns a reference to the vector
            }
        }else {
            partitions[nodePartition].push_back(nodesToSearch[i]); //The map returns a reference to the vector
        }
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

    std::vector<std::shared_ptr<Node>> nodesToSearch = nodes;
    nodesToSearch.push_back(inputMaster); //Need to explicitally include the input master

    for(unsigned long i = 0; i<nodesToSearch.size(); i++) {
        int srcPartition = nodesToSearch[i]->getPartitionNum();

        //Note: arcs can only be grouped if they share the same output port
        std::vector<std::shared_ptr<OutputPort>> outputPorts = nodesToSearch[i]->getOutputPortsIncludingOrderConstraint();
        for (int portIdx = 0; portIdx < outputPorts.size(); portIdx++) {
            std::map<int, std::vector<std::shared_ptr<Arc>>> currentGroups;
            //Arcs from a given node output port to the same partition can be grouped
            std::set<std::shared_ptr<Arc>> outArcs = outputPorts[portIdx]->getArcs();

            for (auto outArc = outArcs.begin(); outArc != outArcs.end(); outArc++) {
                //Note that arcs are not necessarily ordered according to partitions
                int dstPartition = (*outArc)->getDstPort()->getParent()->getPartitionNum();
                if (srcPartition != dstPartition) {
                    if (checkForToFromNoPartition && (srcPartition == -1 || dstPartition == -1)) {
                        std::string srcPath = (*outArc)->getSrcPort()->getParent()->getFullyQualifiedName();
                        std::shared_ptr<Node> dst = (*outArc)->getDstPort()->getParent();
                        std::string dstPath = dst->getFullyQualifiedName();
                        throw std::runtime_error(ErrorHelpers::genErrorStr("Found an arc going to/from partition -1 [" + srcPath + "(" + GeneralHelper::to_string(srcPartition) + ") -> " + dstPath + " (" + GeneralHelper::to_string(dstPartition) + ")]"));
                    }

                    currentGroups[dstPartition].push_back(*outArc);
                }
            }

            //Add discovered groups to partitionCrossing map
            for (auto it = currentGroups.begin(); it != currentGroups.end(); it++) {
                int dstPartition = it->first;
                partitionCrossings[std::pair<int, int>(srcPartition, dstPartition)].push_back(it->second);
            }
        }
    }

    return partitionCrossings;
}

void Design::emitMultiThreadedC(std::string path, std::string fileName, std::string designName,
                                SchedParams::SchedType schedType, TopologicalSortParameters schedParams,
                                ThreadCrossingFIFOParameters::ThreadCrossingFIFOType fifoType, bool emitGraphMLSched,
                                bool printSched, int fifoLength, unsigned long blockSize,
                                bool propagatePartitionsFromSubsystems, std::vector<int> partitionMap, bool threadDebugPrint,
                                int ioFifoSize, bool printTelem, std::string telemDumpPrefix,
                                EmitterHelpers::TelemetryLevel telemLevel, int telemCheckBlockFreq, double telemReportPeriodSec,
                                unsigned long memAlignment, bool useSCHEDFIFO,
                                PartitionParams::FIFOIndexCachingBehavior fifoIndexCachingBehavior,
                                MultiThreadEmitterHelpers::ComputeIODoubleBufferType fifoDoubleBuffer,
                                std::string pipeNameSuffix) {

    if(!telemDumpPrefix.empty()){
        telemDumpPrefix = fileName+"_"+telemDumpPrefix;
    }

    //Change the I/O masters to be in partition -2
    //This is a special partition.  Even if they I/O exists on the same node as a process thread, want to handle it seperatly
    inputMaster->setPartitionNum(IO_PARTITION_NUM);
    outputMaster->setPartitionNum(IO_PARTITION_NUM);
    visMaster->setPartitionNum(IO_PARTITION_NUM);

    //==== Propagate Properties from Subsystems ====
    if(propagatePartitionsFromSubsystems){
        propagatePartitionsFromSubsystemsToChildren();
    }

    //==== Prune ====
    prune(true);

    //After pruning, disconnect the terminator master and the unconnected master
    //The remaining nodes should be used for other things since they were not pruned
    //However, they may have ports disconnected which were connected to the unconnected or terminator masters
    //This removal is used to avoid uneeccisary communication to the unconnected and terminator masters (FIFOs
    //would potentially be inserted where they are not needed).
    //Note what nodes and ports are disconnected

    pruneUnconnectedArcs(true);

    //=== Handle Clock Domains ===
    //Should do this after all pruning has taken place (and all node disconnects have taken place)
    //Should do after disconnects take place so that no arcs are going to the terminator master as the clock domain
    //discovery and check logic does not distinguish between the actual output master and the terminator master
    //since they are both MasterOutputs
    //TODO: re-evaluate this
    std::vector<std::shared_ptr<ClockDomain>> clockDomains = findClockDomains();
    //TODO Optimize this so a second pass is not needed
    //After pruning, re-discover clock domain parameters since rate change nodes and links to MasterIO ports may have been removed
    //Before doing that, reset the ClockDomain links for master nodes as ports may have become disconnected
    resetMasterNodeClockDomainLinks();
    MultiRateHelpers::rediscoverClockDomainParameters(clockDomains);

    //ClockDomain Rate Change Specialization.  Convert ClockDomains to UpsampleClockDomains or DownsampleClockDomains
    clockDomains = specializeClockDomains(clockDomains);
    assignNodeIDs();
    assignArcIDs();
    //This also converts RateChangeNodes to Input or Output implementations.
    //This is done before other operations since these operations replace the objects because the class is changed to a subclass

    //AfterSpecialization, create support nodes for clockdomains, particularly DownsampleClockDomains
    createClockDomainSupportNodes(clockDomains, false, false); //Have not done context discovery & marking yet so do not request context inclusion
    assignNodeIDs();
    assignArcIDs();

    //Check the ClockDomain rates are appropriate
    MultiRateHelpers::validateClockDomainRates(clockDomains);

    //==== Proceed with Context Discovery ====
    //This is an optimization pass to widen the enabled subsystem context
    expandEnabledSubsystemContexts();
    //Quick check to make sure vis node has no outgoing arcs (required for createEnabledOutputsForEnabledSubsysVisualization).
    //Is ok for them to exist (specifically order constraint arcs) after state update node and context encapsulation
    if(visMaster->outDegree() > 0){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Visualization Master is Not Expected to Have an Out Degree > 0, has " + GeneralHelper::to_string(visMaster->outDegree())));
    }

    // TODO: Re-enable vis node later?
    //    createEnabledOutputsForEnabledSubsysVisualization();
    assignNodeIDs(); //Need to assign node IDs since the node ID is used for set ordering and new nodes may have been added
    assignArcIDs();

    //Cleanup empty branches of the hierarchy after nodes moved under enabled subsystem contexts
    //cleanupEmptyHierarchy("it was empty or all underlying nodes were moved");

    //==== Partitioning ====
    //TODO: Partition Here
    //      Currently done manually

    //==== Context Encapsulation Preparation (Organizing Contexts and Dealing with Contexts that Cross Partitions) ====
    //Assign EnableNodes to partitions do before encapsulation and Context/StateUpdate node creation, but after EnabledSubsystem expansion so that EnableNodes are moved/created/deleted as appropriate ahead of time
    placeEnableNodesInPartitions();

    //TODO: If FIFO Insertion Moved to Before this Point:
    //      Modify to not stop at FIFOs with no initial state when finding Mux contexts.  FIFOs with initial state are are treated as containing delays.  FIFOs without initial state treated like wires.
    discoverAndMarkContexts();

    //Replicate ContextRoot Drivers (if should be replicated) for each partition
    //Do this after ContextDiscovery and Marking but before encapsulation
    replicateContextRootDriversIfRequested();
    assignNodeIDs(); //Need to assign node IDs since the node ID is used for set ordering and new nodes may have been added
    assignArcIDs();

    //Order constraining zero input nodes in enabled subsystems is not nessisary as rewireArcsToContexts can wire the enable
    //line as a depedency for the enable context to be emitted.  This is currently done in the scheduleTopoloicalSort method called below
    //TODO: re-introduce orderConstrainZeroInputNodes if the entire enable context is not scheduled hierarchically
    //TODO: Modify to check for ContextRootDrivers per partition
    //orderConstrainZeroInputNodes(); //Do this after the contexts being marked since this constraint should not have an impact on contexts

    //==== Encapsulate Contexts ====
    //TODO: fix encapsuleate to duplicate driver arc for each ContextFamilyContainer in other partitions
    //to the given partition and add an out
    encapsulateContexts(); //TODO: Modify to only insert FIFOs for contextDecisionDrivers when replication was not requested.  If replication was requr
    assignNodeIDs(); //Need to assign node IDs since the node ID is used for set ordering and new nodes may have been added
    assignArcIDs();

    //Cleanup empty branches of the hierarchy after nodes moved under enabled subsystem contexts
    //cleanupEmptyHierarchy("all underlying nodes were moved durring context encapsulation");

    //==== Discover Partitions with Single Clock Domain ====

    //Detect partitions with only single clock domain (and no rate change nodes)
    std::map<int, std::pair<int, int>> partitionSingleClockDomainRatesNotBase;
    {
        std::map<int, std::set<std::shared_ptr<ClockDomain>>> partitionsWithSingleClockDomain = MultiRateHelpers::findPartitionsWithSingleClockDomain(nodes);
        std::vector<std::shared_ptr<Node>> nodesToRemove;
        std::vector<std::shared_ptr<Arc>> arcsToRemove;
        for (const auto &partitionClkDomains : partitionsWithSingleClockDomain) {
            int partition = partitionClkDomains.first;
            std::set<std::shared_ptr<ClockDomain>> clkDomainsInPartition = partitionClkDomains.second;
            if (clkDomainsInPartition.empty()) {
                throw std::runtime_error(ErrorHelpers::genErrorStr("Expected ClockDomain set to be non-empty"));
            }
            std::pair<int, int> rate = std::pair<int, int>(1, 1);
            if (*clkDomainsInPartition.begin()) {
                rate = (*clkDomainsInPartition.begin())->getRateRelativeToBase();
            }

            partitionSingleClockDomainRatesNotBase[partition] = rate;

            std::cout << ErrorHelpers::genErrorStr(
                    "Partition " + GeneralHelper::to_string(partition) + " is composed of a single rate (" +
                    GeneralHelper::to_string(rate.first) + "/" + GeneralHelper::to_string(rate.second) + ")",
                    VITIS_STD_INFO_PREAMBLE, "") << std::endl;

            //For each clock domain, set the clock domains to suppress output for this partition
            //Also remove the clock domain drivers for this clock domain in this partition
            for (const auto &clkDomain : clkDomainsInPartition) {
                //It is possible for the clock domain to be the base
                if(clkDomain) {
                    clkDomain->addClockDomainLogicSuppressedPartition(partition);

                    std::shared_ptr<ContextRoot> asContextRoot = GeneralHelper::isType<ClockDomain, ContextRoot>(
                            clkDomain);
                    if (asContextRoot) {
                        std::map<int, std::vector<std::shared_ptr<Arc>>> contextDriversPerPartition = asContextRoot->getContextDriversPerPartition();
                        std::vector<std::shared_ptr<Arc>> contextDrivers = contextDriversPerPartition[partition];
                        if (contextDrivers.size() > 1) {
                            throw std::runtime_error(ErrorHelpers::genErrorStr(
                                    "Expected a single context driver for Clock Domain in partition.", clkDomain));
                        }

                        if (!contextDrivers.empty()) {
                            std::shared_ptr<Arc> driverArc = *contextDrivers.begin();
                            std::shared_ptr<Node> driverNode = driverArc->getSrcPort()->getParent();

                            //Some sanity checks
                            if (driverNode->getOutputPorts().size() != 1) {
                                throw std::runtime_error(ErrorHelpers::genErrorStr(
                                        "Expected Clock Domain Driver Node to Have Single Output Port", driverNode));
                            }

                            if (driverArc->getSrcPort()->getArcs().size() != 1) {
                                throw std::runtime_error(ErrorHelpers::genErrorStr(
                                        "Expected Clock Domain Driver Node to Have Single Output Arc", driverNode));
                            }

                            std::vector<std::shared_ptr<Arc>> baseDrivers = asContextRoot->getContextDecisionDriver();
                            //Sanity check
                            if (baseDrivers.size() != 1) {
                                throw std::runtime_error(ErrorHelpers::genErrorStr(
                                        "Expected a single context driver for Clock Domain in partition.", clkDomain));
                            }

                            contextDriversPerPartition.erase(partition);

                            if ((*baseDrivers.begin())->getSrcPort()->getParent() == driverNode) {
                                //Set the base decision drivers to be from another partition
                                //There should be at least one partition with an active driver for the clock domain because
                                //at least one partition will contain a rate change block
                                clkDomain->setClockDomainDriver(*(*contextDriversPerPartition.begin()).second.begin());
                            }

                            std::set<std::shared_ptr<Arc>> driverArcsToRm = driverNode->disconnectNode();
                            nodesToRemove.push_back(driverNode);
                            arcsToRemove.insert(arcsToRemove.end(), driverArcsToRm.begin(), driverArcsToRm.end());
                        }

                        asContextRoot->setContextDriversPerPartition(contextDriversPerPartition);
                    }
                }
            }
        }

        std::vector<std::shared_ptr<Node>> emptyNodes;
        std::vector<std::shared_ptr<Arc>> emptyArcs;
        addRemoveNodesAndArcs(emptyNodes, nodesToRemove, emptyArcs, arcsToRemove);
    }

    //==== Create Context Variable Update Nodes ====

    //TODO: investigate moving
    //*** Creating context variable update nodes can be done after partitioning since it should inherit the partition
    //of its context master (in the case of Mux).  Note that includeContext should be set to false
    createContextVariableUpdateNodes(true); //Create after expanding the subcontext so that any movement of EnableInput and EnableOutput nodes (is placed in the same partition as the ContextRoot)
    assignNodeIDs(); //Need to assign node IDs since the node ID is used for set ordering and new nodes may have been added
    assignArcIDs();

    //==== Insert and Configure FIFOs (but do not ingest Delays) ====
    //** Done before state variable updates created
    //Also should be before state update nodes created since FIFOs can absorb delay nodes (which should not create state update nodes)
    //Insert FIFOs into the crossings.  Note, the FIFO's partition should reside in the upstream partition of the arc(s) it is being placed in the middle of.
    //They should also reside inside of the source context family container
    //
    //This should also be done after encapsulation as the driver arcs of contexts are duplicated for each ContextFamilyContainer

    //Discover arcs crossing partitions and what partition crossings can be grouped together
    std::map<std::pair<int, int>, std::vector<std::vector<std::shared_ptr<Arc>>>> partitionCrossings = getGroupableCrossings();

    std::map<std::pair<int, int>, std::vector<std::shared_ptr<ThreadCrossingFIFO>>> fifoMap;

    //Insert the partition crossing FIFOs
    {
        std::vector<std::shared_ptr<Node>> new_nodes;
        std::vector<std::shared_ptr<Node>> deleted_nodes;
        std::vector<std::shared_ptr<Arc>> new_arcs;
        std::vector<std::shared_ptr<Arc>> deleted_arcs;

        switch (fifoType) {
            case ThreadCrossingFIFOParameters::ThreadCrossingFIFOType::LOCKLESS_X86:
                //Note that FIFOs are placed in the src partition and context (unless the src is an EnableOutput or RateChangeOutput in which case they are placed one level up).
                fifoMap = MultiThreadTransformHelpers::insertPartitionCrossingFIFOs<LocklessThreadCrossingFIFO>(partitionCrossings, new_nodes, deleted_nodes, new_arcs, deleted_arcs);
                break;
            case ThreadCrossingFIFOParameters::ThreadCrossingFIFOType::LOCKLESS_INPLACE_X86:
                fifoMap = MultiThreadTransformHelpers::insertPartitionCrossingFIFOs<LocklessInPlaceThreadCrossingFIFO>(partitionCrossings, new_nodes, deleted_nodes, new_arcs, deleted_arcs);
                break;
            default:
                throw std::runtime_error(
                        ErrorHelpers::genErrorStr("Unsupported Thread Crossing FIFO Type for Multithreaded Emit"));
        }
        addRemoveNodesAndArcs(new_nodes, deleted_nodes, new_arcs, deleted_arcs);
    }

    assignNodeIDs(); //Need to assign node IDs since the node ID is used for set ordering and new nodes may have been added
    assignArcIDs();

    std::vector<std::shared_ptr<ThreadCrossingFIFO>> fifoVec;
    std::map<int, std::vector<std::shared_ptr<ThreadCrossingFIFO>>> inputFIFOs;
    std::map<int, std::vector<std::shared_ptr<ThreadCrossingFIFO>>> outputFIFOs;
    for(auto it = fifoMap.begin(); it != fifoMap.end(); it++){
        int srcPartition = it->first.first;
        int dstPartition = it->first.second;
        fifoVec.insert(fifoVec.end(), it->second.begin(), it->second.end());

        std::vector<std::shared_ptr<ThreadCrossingFIFO>>& inputFIFOsRef = inputFIFOs[dstPartition];
        inputFIFOsRef.insert(inputFIFOsRef.end(), it->second.begin(), it->second.end());

        std::vector<std::shared_ptr<ThreadCrossingFIFO>>& outputFIFOsRef = outputFIFOs[srcPartition];
        outputFIFOsRef.insert(outputFIFOsRef.end(), it->second.begin(), it->second.end());
    }

    //Set FIFO length and block size here (do before delay ingest)
    for(int i = 0; i<fifoVec.size(); i++){
        fifoVec[i]->setFifoLength(fifoLength);
    }

    //Since FIFOs are placed in the src partition and context (unless the src is an EnableOutput or RateChange output),
    //FIFOs connected to the input master should have their rate set based on the port rate of the input master.
    //FIFOs connected to the output master should have their rate properly set based on the ClockDomain they are in, especially
    //since any FIFO connected to a RateChange output will be placed in the next context up.
    MultiRateHelpers::setFIFOClockDomains(fifoVec);
    MultiRateHelpers::setAndValidateFIFOBlockSizes(fifoVec, blockSize, true);

    //==== Retime ====
    //TODO: Retime Here
    //      Not currently implemented

    //==== FIFO Delay Ingest ====
    //Should be before state update nodes created since FIFOs can absorb delay nodes (which should not create state update nodes)
    //Currently, only nodes that are soley connected to FIFOs are absorbed.  It is possible to absorb other nodes but delay matching
    //and/or multiple FIFOs cascaded (to allow an intermediary tap to be viewed) would potentially be required.
    //TODO: only adjacent delays for now, extend
    {
        std::vector<std::shared_ptr<Node>> new_nodes;
        std::vector<std::shared_ptr<Node>> deleted_nodes;
        std::vector<std::shared_ptr<Arc>> new_arcs;
        std::vector<std::shared_ptr<Arc>> deleted_arcs;

        MultiThreadTransformHelpers::absorbAdjacentDelaysIntoFIFOs(fifoMap, new_nodes, deleted_nodes, new_arcs, deleted_arcs);
        addRemoveNodesAndArcs(new_nodes, deleted_nodes, new_arcs, deleted_arcs);
    }
    assignNodeIDs();
    assignArcIDs();

    //==== Merge FIFOs ====
    //Clock domain was already set for each FIFO above and will be used in the FIFO merge
    //      Add FIFO merge here (so long as on demand / lazy eval FIFOs in conditional execution regions are not considered)
    //      All ports need the same number of initial conditions.  May need to resize initial conditions
    //      Doing this after delay absorption because it currently is only implemented for a single set of input ports
    //      Note that there was a comment I left in the delay absorption code stating that I expected FIFO bundling /
    //      merging to occur after delay absorption
    //      *
    //      Involves moving arcs to ports, moving block size, moving initial conditions, and moving clock domain
    //      Remove other FIFOs.  Place new FIFO outside contexts.  Note: if on-demand / lazy eval fifo implemented later
    //      the merge should not happen between FIFOs in different contexts and new FIFO should be placed inside context.
    //      To make this easier to work with with, insert the FIFO into the context of all the inputs if they are all the same
    //      otherwise, put it outside the contexts


    //TODO: There is currently a problem when ignoring contexts due to scheduling nodes connected to the FIFOs
    {
        std::vector<std::shared_ptr<Node>> new_nodes;
        std::vector<std::shared_ptr<Node>> deleted_nodes;
        std::vector<std::shared_ptr<Arc>> new_arcs;
        std::vector<std::shared_ptr<Arc>> deleted_arcs;
        std::vector<std::shared_ptr<Node>> add_to_top_lvl;

        MultiThreadTransformHelpers::mergeFIFOs(fifoMap, new_nodes, deleted_nodes, new_arcs, deleted_arcs, add_to_top_lvl, false, true);
        addRemoveNodesAndArcs(new_nodes, deleted_nodes, new_arcs, deleted_arcs);
        for(auto topLvlNode : add_to_top_lvl){
            topLevelNodes.push_back(topLvlNode);
        }
    }
    assignNodeIDs();
    assignArcIDs();

    //==== Report FIFOs ====
    std::cout << std::endl;
    std::cout << "========== FIFO Report ==========" << std::endl;
    for(auto it = fifoMap.begin(); it != fifoMap.end(); it++){
        std::vector<std::shared_ptr<ThreadCrossingFIFO>> fifoVec = it->second;
        for(int i = 0; i<fifoVec.size(); i++) {
            std::vector<std::shared_ptr<ClockDomain>> clockDomains = fifoVec[i]->getClockDomains();
            std::string clkDomainDescr = "[";
            for(int i = 0; i<clockDomains.size(); i++){
                if(i>0){
                    clkDomainDescr += ", ";
                }
                std::pair<int, int> rate = std::pair<int, int>(1, 1);
                if(clockDomains[i] != nullptr){ //Clock domain can be null which signifies the base case
                    rate = clockDomains[i]->getRateRelativeToBase();
                }
                clkDomainDescr += GeneralHelper::to_string(rate.first) + "/" + GeneralHelper::to_string(rate.second);
            }
            clkDomainDescr += "]";

            std::cout << "FIFO: " << fifoVec[i]->getName() << ", Length (Blocks): " << fifoVec[i]->getFifoLength() << ", Length (Elements): " << (fifoVec[i]->getFifoLength()*fifoVec[i]->getTotalBlockSizeAllPorts()) << ", Initial Conditions (Blocks): " << fifoVec[i]->getInitConditionsCreateIfNot(0).size()/fifoVec[i]->getOutputPort(0)->getDataType().numberOfElements()/fifoVec[i]->getBlockSizeCreateIfNot(0) << ", Clock Domain(s): " << clkDomainDescr << std::endl;
        }
    }
    std::cout << std::endl;

    //==== Report Communication (and check for communication deadlocks) ====
    //Report Communication before StateUpdate nodes inserted to avoid false positive errors when detecting arcs to state update nodes
    //in the input partition of a ThreadCrossingFIFO
    std::string graphMLCommunicationInitCondFileName = "";
    std::string graphMLCommunicationInitCondSummaryFileName = "";

    if(emitGraphMLSched) {
        //Export GraphML (for debugging)
        graphMLCommunicationInitCondFileName = fileName + "_communicationInitCondGraph.graphml";
        graphMLCommunicationInitCondSummaryFileName = fileName + "_communicationInitCondGraphSummary.graphml";
        std::cout << "Emitting GraphML Communication Initial Conditions File: " << path << "/" << graphMLCommunicationInitCondFileName << std::endl;
        Design communicationGraph = CommunicationEstimator::createCommunicationGraph(*this, false, false);
        GraphMLExporter::exportGraphML(path + "/" + graphMLCommunicationInitCondFileName, communicationGraph);
        std::cout << "Emitting GraphML Communication Initial Conditions Summary File: " << path << "/" << graphMLCommunicationInitCondSummaryFileName << std::endl << std::endl;
        Design communicationGraphSummary = CommunicationEstimator::createCommunicationGraph(*this, true, false);
        GraphMLExporter::exportGraphML(path + "/" + graphMLCommunicationInitCondSummaryFileName, communicationGraphSummary);
    }

    //Check for deadlock among partitions
    CommunicationEstimator::checkForDeadlock(*this, designName, path);

    //==== FIFO Merge Cleanup ====
    //Need to rebuild fifoVec, inputFIFOs, and outputFIFOs, since merging may have occurred
    fifoVec.clear();
    inputFIFOs.clear();
    outputFIFOs.clear();
    for(auto it = fifoMap.begin(); it != fifoMap.end(); it++){
        int srcPartition = it->first.first;
        int dstPartition = it->first.second;
        fifoVec.insert(fifoVec.end(), it->second.begin(), it->second.end());

        std::vector<std::shared_ptr<ThreadCrossingFIFO>>& inputFIFOsRef = inputFIFOs[dstPartition];
        inputFIFOsRef.insert(inputFIFOsRef.end(), it->second.begin(), it->second.end());

        std::vector<std::shared_ptr<ThreadCrossingFIFO>>& outputFIFOsRef = outputFIFOs[srcPartition];
        outputFIFOsRef.insert(outputFIFOsRef.end(), it->second.begin(), it->second.end());
    }

    //==== Create State Update Nodes ====
    //Create state update nodes after delay absorption to avoid creating dependencies that would inhibit delay absorption
    createStateUpdateNodes(true); //Done after EnabledSubsystem Contexts are expanded to avoid issues with deleting and re-wiring EnableOutputs
    assignNodeIDs(); //Need to assign node IDs since the node ID is used for set ordering and new nodes may have been added
    assignArcIDs();

    //==== Check All Nodes have an ID ====
    if(!MultiThreadEmitterHelpers::checkNoNodesInIO(nodes)){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Nodes exist in the I/O partition of the design which are not FIFOs"));
    }

    //==== Schedule Operations within Partitions ===
    scheduleTopologicalStort(schedParams, false, true, designName, path, printSched, true); //Pruned before inserting state update nodes

    //Verify the schedule
    verifyTopologicalOrder(false, schedType);

    //==== Report Schedule and Partition Information ====
    //Emit the schedule (as a graph)
    std::string graphMLSchedFileName = "";
    if(emitGraphMLSched) {
        //Export GraphML (for debugging)
        graphMLSchedFileName = fileName + "_scheduleGraph.graphml";
        std::cout << "Emitting GraphML Schedule File: " << path << "/" << graphMLSchedFileName << std::endl;
        GraphMLExporter::exportGraphML(path + "/" + graphMLSchedFileName, *this);
    }

    //Analyze Partitions
    std::map<int, std::vector<std::shared_ptr<Node>>> partitions = findPartitions();

    std::map<int, std::map<EstimatorCommon::NodeOperation, int>> counts;
    std::map<std::type_index, std::string> names;

    for(auto it = partitions.begin(); it != partitions.end(); it++){
        std::pair<std::map<EstimatorCommon::NodeOperation, int>, std::map<std::type_index, std::string>> partCount = ComputationEstimator::reportComputeInstances(it->second, {visMaster});
        counts[it->first] = partCount.first;

        for(auto nameIt = partCount.second.begin(); nameIt != partCount.second.end(); nameIt++){
            if(names.find(nameIt->first) == names.end()){
                names[nameIt->first] = nameIt->second;
            }
        }
    }

    //Get the ClockDomainRates in each partition (to report)
    // std::map<int, std::set<std::pair<int, int>>> partitionClockDomainRates = findPartitionClockDomainRates();

    std::cout << "Partition Node Count Report:" << std::endl;
    ComputationEstimator::printComputeInstanceTable(counts, names);
    std::cout << std::endl;

    std::map<std::pair<int, int>, EstimatorCommon::InterThreadCommunicationWorkload> commWorkloads = CommunicationEstimator::reportCommunicationWorkload(fifoMap);
    std::cout << "Partition Communication Report:" << std::endl;
    CommunicationEstimator::printComputeInstanceTable(commWorkloads);
    std::cout << std::endl;

    //==== Emit Design ====

    //Emit Types
    EmitterHelpers::stringEmitTypeHeader(path);

    //Emit FIFO header (get struct descriptions from FIFOs)
    std::string fifoHeaderName = MultiThreadEmitterHelpers::emitFIFOStructHeader(path, fileName, fifoVec);

    //Emit FIFO Support File
    std::set<ThreadCrossingFIFOParameters::CopyMode> copyModesUsed = MultiThreadEmitterHelpers::findFIFOCopyModesUsed(fifoVec);
    std::string fifoSupportHeaderName = MultiThreadEmitterHelpers::emitFIFOSupportFile(path, fileName, copyModesUsed);

    //Emit other support files
    MultiThreadEmitterHelpers::writePlatformParameters(path, VITIS_PLATFORM_PARAMS_NAME, memAlignment);
    MultiThreadEmitterHelpers::writeNUMAAllocHelperFiles(path, VITIS_NUMA_ALLOC_HELPERS);

    //====Emit PAPI Helpers====
    std::vector<std::string> otherCFiles;
    std::string papiHelperHFile = "";

    if(EmitterHelpers::usesPAPI(telemLevel) && (printTelem || !telemDumpPrefix.empty())){
        papiHelperHFile = EmitterHelpers::emitPAPIHelper(path, "vitis");
        std::string papiHelperCFile = "vitis_papi_helpers.c";
        otherCFiles.push_back(papiHelperCFile);
    }

    //==== Emit Partitions ====

    //Emit partition functions (computation function and driver function)
    for(auto partitionBeingEmitted = partitions.begin(); partitionBeingEmitted != partitions.end(); partitionBeingEmitted++){
        //Emit each partition (except -2, handle specially)
        if(partitionBeingEmitted->first != IO_PARTITION_NUM) {
            //Note that all I/O into and out of the partitions threads is via thread crossing FIFOs from the IO_PARTITION thread
            bool singleClockDomain = partitionSingleClockDomainRatesNotBase.find(partitionBeingEmitted->first) != partitionSingleClockDomainRatesNotBase.end();
            std::pair<int, int> rate = std::pair<int, int>(1, 1);
            if(singleClockDomain){
                rate = partitionSingleClockDomainRatesNotBase[partitionBeingEmitted->first];
            }

            MultiThreadEmitterHelpers::emitPartitionThreadC(partitionBeingEmitted->first, partitionBeingEmitted->second,
                                                            inputFIFOs[partitionBeingEmitted->first], outputFIFOs[partitionBeingEmitted->first],
                                                            path, fileName, designName, schedType, outputMaster, blockSize, fifoHeaderName,
                                                            fifoSupportHeaderName,
                                                            threadDebugPrint, printTelem,
                                                            telemLevel, telemCheckBlockFreq, telemReportPeriodSec,
                                                            telemDumpPrefix, false, papiHelperHFile,
                                                            fifoIndexCachingBehavior, fifoDoubleBuffer, singleClockDomain, rate);
        }
    }

    EmitterHelpers::emitParametersHeader(path, fileName, blockSize);

    //====Emit Helpers====
    if(printTelem || !telemDumpPrefix.empty()){
        EmitterHelpers::emitTelemetryHelper(path, fileName);
        std::string telemetryCFile = fileName + "_telemetry_helpers.c";
        otherCFiles.push_back(telemetryCFile);
    }

    if(!telemDumpPrefix.empty()){
        std::map<int, int> partitionToCPU;
        for(auto it = partitions.begin(); it!=partitions.end(); it++) {
            if (partitionMap.empty()) {
                //No thread affinities are set, set the CPUs to -1
                partitionToCPU[it->first] = -1;
            } else {
                //Use the partition map
                if (it->first == IO_PARTITION_NUM) {
                    partitionToCPU[it->first] = partitionMap[0]; //Is always the first element and the array is not empty
                } else {
                    if (it->first < 0 || it->first >= partitionMap.size() - 1) {
                        throw std::runtime_error(ErrorHelpers::genErrorStr("The partition map does not contain an entry for partition " + GeneralHelper::to_string(it->first)));
                    }
                    partitionToCPU[it->first] = partitionMap[it->first + 1];
                }

            }
        }

        MultiThreadEmitterHelpers::writeTelemConfigJSONFile(path, telemDumpPrefix, designName, partitionToCPU, IO_PARTITION_NUM, graphMLSchedFileName);
    }

    //====Emit I/O Divers====
    std::set<int> partitionSet;
    for(auto it = partitions.begin(); it != partitions.end(); it++){
        partitionSet.insert(it->first);
    }

    //TODO: Modify Drivers to look at MasterIO
    std::pair<std::vector<Variable>, std::vector<std::pair<int, int>>> inputVarRatePair = getCInputVariables();
    std::vector<Variable> inputVars = inputVarRatePair.first;

    //++++Emit Const I/O Driver++++
    ConstIOThread::emitConstIOThreadC(inputFIFOs[IO_PARTITION_NUM], outputFIFOs[IO_PARTITION_NUM], path, fileName, designName, blockSize, fifoHeaderName, fifoSupportHeaderName, threadDebugPrint, fifoIndexCachingBehavior);
    std::string constIOSuffix = "io_const";

    //Emit the startup function (aka the benchmark kernel)
    MultiThreadEmitterHelpers::emitMultiThreadedBenchmarkKernel(fifoMap, inputFIFOs, outputFIFOs, partitionSet, path, fileName, designName, fifoHeaderName, fifoSupportHeaderName, constIOSuffix, partitionMap, papiHelperHFile, useSCHEDFIFO);

    //Emit the benchmark driver
    MultiThreadEmitterHelpers::emitMultiThreadedMain(path, fileName, designName, constIOSuffix, inputVars);

    //Emit the benchmark makefile
    MultiThreadEmitterHelpers::emitMultiThreadedMakefileMain(path, fileName, designName, partitionSet,
                                                                    constIOSuffix, false, otherCFiles,
                                                                    !papiHelperHFile.empty(), !partitionMap.empty());

    //++++Emit Linux Pipe I/O Driver++++
    StreamIOThread::emitFileStreamHelpers(path, fileName);
    std::vector<std::string> otherCFilesFileStream = otherCFiles;
    std::string filestreamHelpersCFile = fileName + "_filestream_helpers.c";
    otherCFilesFileStream.push_back(filestreamHelpersCFile);

    std::string pipeIOSuffix = "io_linux_pipe";
    StreamIOThread::emitStreamIOThreadC(inputMaster, outputMaster, inputFIFOs[IO_PARTITION_NUM],
                                        outputFIFOs[IO_PARTITION_NUM], path, fileName, designName,
                                        StreamIOThread::StreamType::PIPE, blockSize, fifoHeaderName, fifoSupportHeaderName, 0, threadDebugPrint, printTelem,
                                        telemLevel, telemCheckBlockFreq, telemReportPeriodSec, telemDumpPrefix, false,
                                        fifoIndexCachingBehavior, pipeNameSuffix);

    //Emit the startup function (aka the benchmark kernel)
    MultiThreadEmitterHelpers::emitMultiThreadedBenchmarkKernel(fifoMap, inputFIFOs, outputFIFOs, partitionSet, path, fileName, designName, fifoHeaderName, fifoSupportHeaderName, pipeIOSuffix, partitionMap, papiHelperHFile, useSCHEDFIFO);

    //Emit the benchmark driver
    MultiThreadEmitterHelpers::emitMultiThreadedMain(path, fileName, designName, pipeIOSuffix, inputVars);

    //Emit the client handlers
    StreamIOThread::emitSocketClientLib(inputMaster, outputMaster, path, fileName, fifoHeaderName, designName);

    //Emit the benchmark makefile
    MultiThreadEmitterHelpers::emitMultiThreadedMakefileMain(path, fileName, designName, partitionSet,
                                                                    pipeIOSuffix, false, otherCFilesFileStream,
                                                                    !papiHelperHFile.empty(), !partitionMap.empty());

    //++++Emit Socket Pipe I/O Driver++++
    std::string socketIOSuffix = "io_network_socket";
    StreamIOThread::emitStreamIOThreadC(inputMaster, outputMaster, inputFIFOs[IO_PARTITION_NUM],
                                        outputFIFOs[IO_PARTITION_NUM], path, fileName, designName,
                                        StreamIOThread::StreamType::SOCKET, blockSize,
                                        fifoHeaderName, fifoSupportHeaderName, 0, threadDebugPrint, printTelem,
                                        telemLevel, telemCheckBlockFreq, telemReportPeriodSec, telemDumpPrefix, false,
                                        fifoIndexCachingBehavior, pipeNameSuffix);

    //Emit the startup function (aka the benchmark kernel)
    MultiThreadEmitterHelpers::emitMultiThreadedBenchmarkKernel(fifoMap, inputFIFOs, outputFIFOs, partitionSet, path, fileName, designName, fifoHeaderName, fifoSupportHeaderName, socketIOSuffix, partitionMap, papiHelperHFile, useSCHEDFIFO);

    //Emit the benchmark driver
    MultiThreadEmitterHelpers::emitMultiThreadedMain(path, fileName, designName, socketIOSuffix, inputVars);

    //Emit the benchmark makefile
    MultiThreadEmitterHelpers::emitMultiThreadedMakefileMain(path, fileName, designName, partitionSet,
                                                                    socketIOSuffix, false, otherCFilesFileStream,
                                                                    !papiHelperHFile.empty(), !partitionMap.empty());

    //++++Emit POSIX Shared Memory FIFO Driver++++
    std::string sharedMemoryFIFOSuffix = "io_posix_shared_mem";
    std::string sharedMemoryFIFOCFileName = "BerkeleySharedMemoryFIFO.c";
    StreamIOThread::emitStreamIOThreadC(inputMaster, outputMaster, inputFIFOs[IO_PARTITION_NUM],
                                        outputFIFOs[IO_PARTITION_NUM], path, fileName, designName,
                                        StreamIOThread::StreamType::POSIX_SHARED_MEM, blockSize,
                                        fifoHeaderName, fifoSupportHeaderName, ioFifoSize, threadDebugPrint, printTelem,
                                        telemLevel, telemCheckBlockFreq, telemReportPeriodSec, telemDumpPrefix, false,
                                        fifoIndexCachingBehavior, pipeNameSuffix);

    //Emit the startup function (aka the benchmark kernel)
    MultiThreadEmitterHelpers::emitMultiThreadedBenchmarkKernel(fifoMap, inputFIFOs, outputFIFOs, partitionSet, path, fileName, designName, fifoHeaderName, fifoSupportHeaderName, sharedMemoryFIFOSuffix, partitionMap, papiHelperHFile, useSCHEDFIFO);

    //Emit the benchmark driver
    MultiThreadEmitterHelpers::emitMultiThreadedMain(path, fileName, designName, sharedMemoryFIFOSuffix, inputVars);

    //Emit the benchmark makefile
    std::vector<std::string> otherCFilesSharedMem = otherCFiles;
    otherCFilesSharedMem.push_back(sharedMemoryFIFOCFileName);
    MultiThreadEmitterHelpers::emitMultiThreadedMakefileMain(path, fileName, designName, partitionSet,
                                                                    sharedMemoryFIFOSuffix, true, otherCFilesSharedMem,
                                                                    !papiHelperHFile.empty(), !partitionMap.empty());
}

void Design::pruneUnconnectedArcs(bool removeVisArcs) {
    std::set<std::shared_ptr<OutputPort>> outputPortsWithArcDisconnected;

    //Unconnected master
    std::set<std::shared_ptr<Arc>> arcsToDisconnectFromUnconnectedMaster = unconnectedMaster->getInputArcs();
    for(auto it = arcsToDisconnectFromUnconnectedMaster.begin(); it != arcsToDisconnectFromUnconnectedMaster.end(); it++){
        outputPortsWithArcDisconnected.insert((*it)->getSrcPort());
    }
    std::set<std::shared_ptr<Arc>> unconnectedArcsSet = unconnectedMaster->disconnectNode();
    std::vector<std::shared_ptr<Arc>> unconnectedArcs;
    unconnectedArcs.insert(unconnectedArcs.end(), unconnectedArcsSet.begin(), unconnectedArcsSet.end());
    std::vector<std::shared_ptr<Arc>> emptyArcSet;
    std::vector<std::shared_ptr<Node>> emptyNodeSet;
    addRemoveNodesAndArcs(emptyNodeSet, emptyNodeSet, emptyArcSet, unconnectedArcs);

    //Terminator master
    std::set<std::shared_ptr<Arc>> arcsToDisconnectFromTerminatorMaster = terminatorMaster->getInputArcs();
    for(auto it = arcsToDisconnectFromTerminatorMaster.begin(); it != arcsToDisconnectFromTerminatorMaster.end(); it++){
        outputPortsWithArcDisconnected.insert((*it)->getSrcPort());
    }
    std::set<std::shared_ptr<Arc>> terminatedArcsSet = terminatorMaster->disconnectNode();
    std::vector<std::shared_ptr<Arc>> terminatedArcs;
    terminatedArcs.insert(terminatedArcs.end(), terminatedArcsSet.begin(), terminatedArcsSet.end());
    addRemoveNodesAndArcs(emptyNodeSet, emptyNodeSet, emptyArcSet, terminatedArcs);

    //Vis Master (TODO: Re-enable later?)
    if(removeVisArcs) {
        std::set<std::shared_ptr<Arc>> arcsToDisconnectFromVisMaster = visMaster->getInputArcs();
        for (auto it = arcsToDisconnectFromVisMaster.begin(); it != arcsToDisconnectFromVisMaster.end(); it++) {
            outputPortsWithArcDisconnected.insert((*it)->getSrcPort());
        }
        std::set<std::shared_ptr<Arc>> visArcsSet = visMaster->disconnectNode();
        std::vector<std::shared_ptr<Arc>> visArcs;
        visArcs.insert(visArcs.end(), visArcsSet.begin(), visArcsSet.end());
        addRemoveNodesAndArcs(emptyNodeSet, emptyNodeSet, emptyArcSet, visArcs);
    }

    //Emit a notification of ports that are disconnected (pruned)
//TODO: clean this up so that the disconnect warning durring emit refers back to the origional node path
    for(auto it = outputPortsWithArcDisconnected.begin(); it != outputPortsWithArcDisconnected.end(); it++) {
        if((*it)->getArcs().empty()) {
            std::cout << "Pruned: All Arcs from Output Port " << (*it)->getPortNum() << " of " <<
                      (*it)->getParent()->getFullyQualifiedName(true) << " [ID: " << (*it)->getParent()->getId() << "]"
                      << std::endl;
        }
    }
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
        familyContainer->setPartitionNum(partition);
        familyContainer->setContextRoot(contextRoot);

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

        //Create an order constraint arcs based on the context driver arcs.  This ensures that the context driver will be
        //Available in each partition that the context resides in (so long as FIFOs are inserted after this).
        //Only do this if the context root did not request replication
        if(!contextRoot->shouldReplicateContextDriver()) {
            //For cases like enabled subsystems where multiple enable arcs exist for the context but
            //all derive from the same source, we will avoid creating duplicate arcs
            //by checking to see if an equivalent order constraint arc has already been created durring
            //the encapsulation process

            std::set<std::shared_ptr<OutputPort>> driverSrcPortsSeen;

            std::vector<std::shared_ptr<Arc>> driverArcs = contextRoot->getContextDecisionDriver();
            std::vector<std::shared_ptr<Arc>> partitionDrivers;
            for (int i = 0; i < driverArcs.size(); i++){
                std::shared_ptr<OutputPort> driverSrcPort = driverArcs[i]->getSrcPort();

                //Check if we have already seen this src port.  If we have do not create a duplicate
                //order constraint arc
                if(driverSrcPortsSeen.find(driverSrcPort) == driverSrcPortsSeen.end()) {
                    std::shared_ptr<Arc> partitionDriver = Arc::connectNodes(driverSrcPort,
                                                                             familyContainer->getOrderConstraintInputPortCreateIfNot(),
                                                                             driverArcs[i]->getDataType(),
                                                                             driverArcs[i]->getSampleTime());
                    driverSrcPortsSeen.insert(driverSrcPort);
                    partitionDrivers.push_back(partitionDriver);
                    //Also add arc to design
                    arcs.push_back(partitionDriver);
                }
            }
            contextRoot->addContextDriverArcsForPartition(partitionDrivers, partition);
        }

        //Create Context Containers inside of the context Family container;
        int numContexts = contextRoot->getNumSubContexts();

        std::vector<std::shared_ptr<ContextContainer>> subContexts;

        for(unsigned long j = 0; j<numContexts; j++){
            std::shared_ptr<ContextContainer> contextContainer = NodeFactory::createNode<ContextContainer>(familyContainer);
            contextContainer->setName("ContextContainer_For_"+asNode->getFullyQualifiedName(true, "::")+"_Partition_"+GeneralHelper::to_string(partition)+"_Subcontext_"+GeneralHelper::to_string(j));
            contextContainer->setPartitionNum(partition);
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

std::set<int> Design::listPresentPartitions(){
    std::set<int> partitions;

    for(unsigned long i = 0; i<nodes.size(); i++){
        partitions.insert(nodes[i]->getPartitionNum());
    }

    return partitions;
}

void Design::cleanupEmptyHierarchy(std::string reason){
    std::vector<std::shared_ptr<SubSystem>> emptySubsystems;

    for(int i = 0; i<nodes.size(); i++){
        std::shared_ptr<SubSystem> asSubsys = GeneralHelper::isType<Node, SubSystem>(nodes[i]);
        std::shared_ptr<ContextRoot> asContextRoot = GeneralHelper::isType<Node, ContextRoot>(nodes[i]);
        std::shared_ptr<ContextFamilyContainer> asContextFamilyContainer = GeneralHelper::isType<Node, ContextFamilyContainer>(nodes[i]);
        std::shared_ptr<ContextContainer> asContextContainer = GeneralHelper::isType<Node, ContextContainer>(nodes[i]);
        if(asSubsys != nullptr && asContextRoot == nullptr && asContextFamilyContainer == nullptr && asContextContainer == nullptr){
            if(asSubsys->getChildren().empty()){
                emptySubsystems.push_back(asSubsys);
            }
        }
    }

    std::vector<std::shared_ptr<Node>> emptyNodeVector;
    std::vector<std::shared_ptr<Arc>> emptyArcVector;

    for(int i = 0; i<emptySubsystems.size(); i++){
        std::cout << "Subsystem " << emptySubsystems[i]->getFullyQualifiedName() << " was removed because " << reason << std::endl;
        std::shared_ptr<SubSystem> parent = emptySubsystems[i]->getParent();

        std::vector<std::shared_ptr<Node>> nodesToRemove = {emptySubsystems[i]};
        addRemoveNodesAndArcs(emptyNodeVector, nodesToRemove, emptyArcVector, emptyArcVector);

        while(parent != nullptr && GeneralHelper::isType<Node, ContextRoot>(parent) == nullptr && GeneralHelper::isType<Node, ContextFamilyContainer>(parent) == nullptr && GeneralHelper::isType<Node, ContextContainer>(parent) == nullptr && parent->getChildren().empty()){//Should short circuit before checking for children if parent is null
            //This subsystem has no children, it should be removed
            std::vector<std::shared_ptr<Node>> nodesToRemove = {parent};
            std::cout << "Subsystem " << parent->getFullyQualifiedName() << " was removed because " << reason << std::endl;

            //Get its parent before removing it
            parent = parent->getParent();
            addRemoveNodesAndArcs(emptyNodeVector, nodesToRemove, emptyArcVector, emptyArcVector);
        }
    }
}

void Design::replicateContextRootDriversIfRequested(){
    //Find ContextRoots that request repliction
    std::vector<std::shared_ptr<ContextRoot>> contextRootsRequestingExpansion;
    for(int i = 0; i<nodes.size(); i++){
        std::shared_ptr<ContextRoot> asContextRoot = GeneralHelper::isType<Node, ContextRoot>(nodes[i]);

        if(asContextRoot){
            if(asContextRoot->shouldReplicateContextDriver()){
                contextRootsRequestingExpansion.push_back(asContextRoot);
            }
        }
    }

    for(int i = 0; i<contextRootsRequestingExpansion.size(); i++) {
        std::shared_ptr<ContextRoot> contextRoot = contextRootsRequestingExpansion[i];
        std::set<int> contextPartitions = contextRoot->partitionsInContext();

        std::shared_ptr<Node> contextRootAsNode = std::dynamic_pointer_cast<Node>(contextRoot);
        if(contextRootAsNode == nullptr){
            throw std::runtime_error(ErrorHelpers::genErrorStr("Found a ContextRoot that is not a Node"));
        }
        std::vector<std::shared_ptr<Arc>> driverArcs = contextRoot->getContextDecisionDriver();

        for(int j = 0; j<driverArcs.size(); j++){
            std::shared_ptr<Arc> driverArc = driverArcs[j];
            std::shared_ptr<OutputPort> driverSrcPort = driverArc->getSrcPort();
            std::shared_ptr<Node> driverSrc = driverSrcPort->getParent();

            //Check for conditions of driver src for clone
            //Check that the driver node has no inputs and a single output
            std::set<std::shared_ptr<Arc>> srcInputArcs = driverSrc->getInputArcs();
            if(srcInputArcs.size() != 0){
                throw std::runtime_error(ErrorHelpers::genErrorStr("Context Driver Replication Currently requires the driver source to have no inputs", contextRootAsNode));
            }
            std::set<std::shared_ptr<Arc>> srcOutputArcs = driverSrc->getOutputArcs();
            if(srcOutputArcs.size() != 1){
                throw std::runtime_error(ErrorHelpers::genErrorStr("Context Driver Replication Currently requires the driver source to have only 1 output arc", contextRootAsNode));
            }

            //Replicate for each partition (keep the one that exists and avoid creating a duplicate)
            for(auto partitionIt = contextPartitions.begin(); partitionIt != contextPartitions.end(); partitionIt++){
                int partition = *partitionIt;
                std::shared_ptr<Node> partitionContextDriver;
                std::shared_ptr<Node> partitionContextDst;
                if(driverSrc->getPartitionNum() == partition){
                    //Reuse the existing node
                    //Replicate the arc
                    partitionContextDriver = driverSrc;
                    partitionContextDst = contextRootAsNode;
                }else{
                    //Replicate the node
                    std::shared_ptr<Node> clonedDriver = driverSrc->shallowClone(driverSrc->getParent());
                    //The clone method copies the node ID allong with the other parameters.  When making a copy of the design,
                    //this is desired but, since both nodes will be present in this design, it is not desierable
                    //reset the ID to -1 so that it can be assigned later.
                    //TODO: Make ID copy an option in shallowClone
                    clonedDriver->setId(-1);
                    clonedDriver->setName(driverSrc->getName() + "_Replicated_Partition_" + GeneralHelper::to_string(partition));
                    clonedDriver->setPartitionNum(partition); //Also set the partition number
                    std::vector<Context> origContext = driverSrc->getContext();
                    clonedDriver->setContext(origContext);
                    if(origContext.size()>0){ //Add the replicated node to the ContextRoot if appropriate
                        std::shared_ptr<ContextRoot> contextRoot = origContext[origContext.size()-1].getContextRoot();
                        int subContext = origContext[origContext.size()-1].getSubContext();
                        contextRoot->addSubContextNode(subContext, clonedDriver);
                    }

                    partitionContextDriver = clonedDriver;

                    //Create a dummy node
                    std::shared_ptr<DummyReplica> dummyContextRoot = NodeFactory::createNode<DummyReplica>(contextRootAsNode->getParent());
                    dummyContextRoot->setName(contextRootAsNode->getName() + "_Dummy_Partition_" + GeneralHelper::to_string(partition));
                    dummyContextRoot->setPartitionNum(partition); //Also set the partition number
                    dummyContextRoot->setDummyOf(contextRoot);
                    std::vector<Context> contextRootContext = contextRootAsNode->getContext();
                    dummyContextRoot->setContext(contextRootContext);
                    if(contextRootContext.size()>0){ //Add the replicated node to the ContextRoot if appropriate
                        std::shared_ptr<ContextRoot> contextRoot = contextRootContext[contextRootContext.size()-1].getContextRoot();
                        int subContext = contextRootContext[contextRootContext.size()-1].getSubContext();
                        contextRoot->addSubContextNode(subContext, dummyContextRoot);
                    }
                    contextRoot->setDummyReplica(partition, dummyContextRoot);
                    partitionContextDst = dummyContextRoot;

                    std::vector<std::shared_ptr<Node>> clonedDriverVec = {clonedDriver, dummyContextRoot};
                    std::vector<std::shared_ptr<Node>> emptyNodeVec;
                    std::vector<std::shared_ptr<Arc>> emptyArcVec;

                    //Add replicated node to design
                    addRemoveNodesAndArcs(clonedDriverVec, emptyNodeVec, emptyArcVec, emptyArcVec);
                }

                //Create a new arc for the node
                std::shared_ptr<OutputPort> partitionDriverPort = partitionContextDriver->getOutputPortCreateIfNot(driverSrcPort->getPortNum());
                std::shared_ptr<Arc> partitionDriverArc = Arc::connectNodes(partitionDriverPort, partitionContextDst->getOrderConstraintInputPortCreateIfNot(), driverArc->getDataType(), driverArc->getSampleTime());
                std::vector<std::shared_ptr<Arc>> newArcVec = {partitionDriverArc};
                std::vector<std::shared_ptr<Node>> emptyNodeVec;
                std::vector<std::shared_ptr<Arc>> emptyArcVec;
                addRemoveNodesAndArcs(emptyNodeVec, emptyNodeVec, newArcVec, emptyArcVec);

                //Add it to the list of partition drivers
                contextRoot->addContextDriverArcsForPartition({partitionDriverArc}, partition);
            }
        }
    }
}

std::vector<std::shared_ptr<ClockDomain>> Design::findClockDomains(){
    std::vector<std::shared_ptr<ClockDomain>> clockDomains;

    for(unsigned long i = 0; i<nodes.size(); i++) {
        std::shared_ptr<ClockDomain> asClkDomain = GeneralHelper::isType<Node, ClockDomain>(nodes[i]);

        if (asClkDomain) {
            clockDomains.push_back(asClkDomain);
        }
    }

    return clockDomains;
}

std::vector<std::shared_ptr<ClockDomain>> Design::specializeClockDomains(std::vector<std::shared_ptr<ClockDomain>> clockDomains){
    std::vector<std::shared_ptr<ClockDomain>> specializedDomains;

    for(unsigned long i = 0; i<clockDomains.size(); i++){
        if(!clockDomains[i]->isSpecialized()){
            std::vector<std::shared_ptr<Node>> nodesToAdd, nodesToRemove;
            std::vector<std::shared_ptr<Arc>> arcsToAdd, arcsToRemove;
            std::shared_ptr<ClockDomain> specializedDomain = clockDomains[i]->specializeClockDomain(nodesToAdd, nodesToRemove, arcsToAdd, arcsToRemove);
            specializedDomains.push_back(specializedDomain);
            addRemoveNodesAndArcs(nodesToAdd, nodesToRemove, arcsToAdd, arcsToRemove);
        }else{
            specializedDomains.push_back(clockDomains[i]);
        }
    }

    return specializedDomains;
}

void Design::createClockDomainSupportNodes(std::vector<std::shared_ptr<ClockDomain>> clockDomains, bool includeContext, bool includeOutputBridgingNodes) {
    for(unsigned long i = 0; i<clockDomains.size(); i++){
        std::vector<std::shared_ptr<Node>> nodesToAdd, nodesToRemove;
        std::vector<std::shared_ptr<Arc>> arcsToAdd, arcsToRemove;
        clockDomains[i]->createSupportNodes(nodesToAdd, nodesToRemove, arcsToAdd, arcsToRemove, includeContext, includeOutputBridgingNodes);
        addRemoveNodesAndArcs(nodesToAdd, nodesToRemove, arcsToAdd, arcsToRemove);
    }
}

void Design::resetMasterNodeClockDomainLinks() {
    inputMaster->resetIoClockDomains();
    outputMaster->resetIoClockDomains();
    visMaster->resetIoClockDomains();
    unconnectedMaster->resetIoClockDomains();
    terminatorMaster->resetIoClockDomains();
}

std::map<int, std::set<std::pair<int, int>>> Design::findPartitionClockDomainRates() {
    std::map<int, std::set<std::pair<int, int>>> clockDomains;

    //Find clock domains in IO
    //Only looking at Input, Output, and Visualization.  Unconnected and Terminated are unnessasary
    std::set<std::pair<int, int>> ioRates;
    std::vector<std::shared_ptr<MasterNode>> mastersToInclude = {inputMaster, outputMaster, visMaster};
    for(int i = 0; i<mastersToInclude.size(); i++) {
        std::shared_ptr<MasterNode> master = mastersToInclude[i];
        std::set<std::shared_ptr<ClockDomain>> inputClockDomains = master->getClockDomains();
        for (auto clkDomain = inputClockDomains.begin(); clkDomain != inputClockDomains.end(); clkDomain++) {
            if (*clkDomain == nullptr) {
                ioRates.emplace(1, 1);
            } else {
                ioRates.insert((*clkDomain)->getRateRelativeToBase());
            }
        }
    }
    clockDomains[IO_PARTITION_NUM] = ioRates;

    for(unsigned long i = 0; i<nodes.size(); i++){
        int partition = nodes[i]->getPartitionNum();

        //Do not include clock domains themselves in this
        if(GeneralHelper::isType<Node, ClockDomain>(nodes[i]) == nullptr) {
            //Note that FIFOs report their clock domain differently in order to handle the case when they are connected directly to the InputMaster
            //Each input/output port pair can also have a different clock domain
            if (GeneralHelper::isType<Node, ThreadCrossingFIFO>(nodes[i])) {
                std::shared_ptr<ThreadCrossingFIFO> fifo = std::dynamic_pointer_cast<ThreadCrossingFIFO>(nodes[i]);
                for(int portNum = 0; portNum<fifo->getInputPorts().size(); portNum++) {
                    std::shared_ptr<ClockDomain> clkDomain = fifo->getClockDomainCreateIfNot(portNum);

                    if (clkDomain == nullptr) {
                        clockDomains[partition].emplace(1, 1);
                    } else {
                        std::pair<int, int> rate = clkDomain->getRateRelativeToBase();
                        clockDomains[partition].insert(rate);
                    }
                }
            } else {
                std::shared_ptr<ClockDomain> clkDomain = MultiRateHelpers::findClockDomain(nodes[i]);

                if (clkDomain == nullptr) {
                    clockDomains[partition].emplace(1, 1);
                } else {
                    std::pair<int, int> rate = clkDomain->getRateRelativeToBase();
                    clockDomains[partition].insert(rate);
                }
            }
        }
    }

    return clockDomains;
}

void Design::validateNodes() {
    for(const auto & node : nodes){
        node->validate();
    }
}

void Design::placeEnableNodesInPartitions() {
    std::vector<std::shared_ptr<Node>> nodesToAdd;
    std::vector<std::shared_ptr<Arc>> arcsToAdd;

    for(const std::shared_ptr<Node> &node : nodes){
        if(GeneralHelper::isType<Node, EnableInput>(node)){
            //Should be validated to only have 1 input
            if(node->getPartitionNum() == -1) {
                //Reassign Output arcs to Replicas of this Enable Input as appropriate
                std::shared_ptr<EnableInput> nodeAsEnabledInput = std::dynamic_pointer_cast<EnableInput>(node);
                std::shared_ptr<Arc> inputArc = *node->getInputPortCreateIfNot(0)->getArcs().begin();
                std::set<std::shared_ptr<Arc>> driverArcs = nodeAsEnabledInput->getEnablePort()->getArcs();
                std::set<std::shared_ptr<Arc>> outArcs = node->getOutputPortCreateIfNot(0)->getArcs();
                std::set<std::shared_ptr<Arc>> orderConstraintInputArcs = node->getOrderConstraintInputArcs();
                std::set<std::shared_ptr<Arc>> orderConstraintOutputArcs = node->getOrderConstraintOutputArcs();

                std::map<int, std::shared_ptr<EnableInput>> replicas;
                //Reassign outputs
                for(const std::shared_ptr<Arc> &outArc : outArcs){
                    std::shared_ptr<EnableInput> replicaNode;
                    int dstPartitionNum = outArc->getDstPort()->getParent()->getPartitionNum();

                    if(dstPartitionNum == -1){
                        if(GeneralHelper::isType<Node, EnableOutput>(outArc->getDstPort()->getParent()) != nullptr){
                            throw std::runtime_error(ErrorHelpers::genErrorStr("Encountered an EnableInput directly connected to an EnableOutput", node));
                        }else{
                            throw std::runtime_error(ErrorHelpers::genErrorStr("Encountered an EnableInput that is connected to a node not in a partition", node));
                        }
                    }

                    //Check if this is the first
                    if(node->getPartitionNum() == -1){
                        node->setPartitionNum(dstPartitionNum);
                        replicas[dstPartitionNum] = nodeAsEnabledInput;
                        replicaNode = nodeAsEnabledInput;
                    }else{
                        //Check if there is already a replica
                        if(replicas.find(dstPartitionNum) == replicas.end()){
                            std::shared_ptr<SubSystem> parent = nodeAsEnabledInput->getParent();
                            if(GeneralHelper::isType<SubSystem, EnabledSubSystem>(parent) == nullptr){
                                throw std::runtime_error(ErrorHelpers::genErrorStr("Found an Enable node that was not directly under an EnabledSubsystem", node));
                            }
                            std::shared_ptr<EnabledSubSystem> parentAsEnabledSubsystem = std::dynamic_pointer_cast<EnabledSubSystem>(parent);
                            //TODO: Remove sanity check
                            std::vector<std::shared_ptr<EnableInput>> parentEnabledInputs = parentAsEnabledSubsystem->getEnableInputs();
                            if(std::find(parentEnabledInputs.begin(), parentEnabledInputs.end(), nodeAsEnabledInput) == parentEnabledInputs.end()){
                                throw std::runtime_error(ErrorHelpers::genErrorStr("Found an Enable node that was not directly under its own EnabledSubsystem", node));
                            }

                            //Need to replicate node
                            replicaNode = NodeFactory::shallowCloneNode<EnableInput>(node->getParent(), nodeAsEnabledInput.get());
                            parentAsEnabledSubsystem->addEnableInput(replicaNode);
                            replicaNode->setPartitionNum(dstPartitionNum);
                            nodesToAdd.push_back(replicaNode);
                            replicas[dstPartitionNum] = replicaNode;

                            //Replicate input arc
                            std::shared_ptr<Arc> newInputArc = Arc::connectNodes(inputArc->getSrcPort(),
                                                                                 replicaNode->getInputPortCreateIfNot(inputArc->getDstPort()->getPortNum()),
                                                                                 inputArc->getDataType(), inputArc->getSampleTime());
                            arcsToAdd.push_back(newInputArc);

                            //Replicate order constraint inputs
                            for(const std::shared_ptr<Arc> &orderConstraintInputArc : orderConstraintInputArcs){
                                std::shared_ptr<Arc> newOrderConstraintInputArc = Arc::connectNodes(inputArc->getSrcPort(),
                                                                                                    replicaNode->getInputPortCreateIfNot(inputArc->getDstPort()->getPortNum()),
                                                                                                    inputArc->getDataType(), inputArc->getSampleTime());
                                arcsToAdd.push_back(newOrderConstraintInputArc);
                            }

                        }else{
                            //Get the appropriate replica
                            replicaNode = replicas[dstPartitionNum];
                        }
                    }

                    //Reassign arc
                    if(replicaNode != node){//Only reassign if nessasary
                        outArc->setSrcPortUpdateNewUpdatePrev(replicaNode->getOutputPortCreateIfNot(outArc->getSrcPort()->getPortNum()));
                    }
                }

                //Reassign order constraint outputs error if an output to the same partiton does not exist
                for(const std::shared_ptr<Arc> &orderConstraintOutputArc : orderConstraintOutputArcs){
                    std::shared_ptr<EnableInput> replicaNode;
                    int dstPartitionNum = orderConstraintOutputArc->getDstPort()->getParent()->getPartitionNum();

                    //Check if this is the first
                    if(node->getPartitionNum() == -1){
                        throw std::runtime_error(ErrorHelpers::genErrorStr("Found order constraint to partition where there is no output from EnabledInput" , node));
                    }else{
                        //Check if there is already a replica
                        if(replicas.find(dstPartitionNum) == replicas.end()){
                            throw std::runtime_error(ErrorHelpers::genErrorStr("Found order constraint to partition where there is no output from EnabledInput" , node));
                        }else{
                            //Get the appropriate replica
                            replicaNode = replicas[dstPartitionNum];
                        }
                    }

                    //Reassign arc
                    if(replicaNode != node){//Only reassign if nessasary
                        orderConstraintOutputArc->setSrcPortUpdateNewUpdatePrev(replicaNode->getOrderConstraintOutputPortCreateIfNot());
                    }
                }
            }
        }else if(GeneralHelper::isType<Node, EnableOutput>(node)) {
            if (node->getPartitionNum() == -1) {
                //Place in the partition of the input
                std::shared_ptr<Arc> inputArc = *node->getInputPortCreateIfNot(0)->getArcs().begin();
                int srcPartition = inputArc->getSrcPort()->getParent()->getPartitionNum();

                if(srcPartition == -1){
                    if(GeneralHelper::isType<Node, EnableInput>(inputArc->getSrcPort()->getParent()) != nullptr){
                        throw std::runtime_error(ErrorHelpers::genErrorStr("Encountered an EnableInput directly connected to an EnableOutput", node));
                    }else {
                        throw std::runtime_error(ErrorHelpers::genErrorStr(
                                "Encountered an EnableOutput that is connected to a node not in a partition", node));
                    }
                }

                node->setPartitionNum(srcPartition);
            }
        }
    }

    //Add remove nodes and arcs
    std::vector<std::shared_ptr<Node>> emptyNodes;
    std::vector<std::shared_ptr<Arc>> emptyArcs;
    addRemoveNodesAndArcs(nodesToAdd, emptyNodes, arcsToAdd, emptyArcs);

    //Place EnabledSubsystem nodes into partitons
    for(const std::shared_ptr<Node> &node : nodes) {
        if (GeneralHelper::isType<Node, EnabledSubSystem>(node)) {
            if(node->getPartitionNum() == -1){
                std::shared_ptr<EnabledSubSystem> nodeAsEnabledSubsystem = std::dynamic_pointer_cast<EnabledSubSystem>(node);
                if(!nodeAsEnabledSubsystem->getEnableInputs().empty()){
                    nodeAsEnabledSubsystem->setPartitionNum(nodeAsEnabledSubsystem->getEnableInputs()[0]->getPartitionNum());
                }else if(!nodeAsEnabledSubsystem->getEnableOutputs().empty()){
                    nodeAsEnabledSubsystem->setPartitionNum(nodeAsEnabledSubsystem->getEnableOutputs()[0]->getPartitionNum());
                }else{
                    throw std::runtime_error(ErrorHelpers::genErrorStr("Found enabled subsystem with no inputs or outputs", node));
                }
            }
        }
    }
}
