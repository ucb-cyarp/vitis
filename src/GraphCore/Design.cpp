//
// Created by Christopher Yarp on 6/27/18.
//

#include "Design.h"

#include "General/GeneralHelper.h"
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

#include "MultiThread/ThreadCrossingFIFO.h"

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

std::set<std::shared_ptr<Node>> Design::getMasterNodes() const {
    std::set<std::shared_ptr<Node>> masterNodes;

    masterNodes.insert(inputMaster);
    masterNodes.insert(outputMaster);
    masterNodes.insert(visMaster);
    masterNodes.insert(unconnectedMaster);
    masterNodes.insert(terminatorMaster);

    return masterNodes;
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

std::vector<std::shared_ptr<ContextRoot>> Design::getTopLevelContextRoots() const {
    return topLevelContextRoots;
}

void Design::setTopLevelContextRoots(const std::vector<std::shared_ptr<ContextRoot>> topLevelContextRoots) {
    Design::topLevelContextRoots = topLevelContextRoots;
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

void Design::addTopLevelContextRoot(std::shared_ptr<ContextRoot> contextRoot) {
    topLevelContextRoots.push_back(contextRoot);
}

void Design::removeTopLevelNode(std::shared_ptr<Node> node) {
    topLevelNodes.erase(std::remove(topLevelNodes.begin(), topLevelNodes.end(), node), topLevelNodes.end());
}

void Design::removeTopLevelContextRoot(std::shared_ptr<ContextRoot> contextRoot) {
    topLevelContextRoots.erase(std::remove(topLevelContextRoots.begin(), topLevelContextRoots.end(), contextRoot), topLevelContextRoots.end());
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

void Design::addRemoveNodesAndArcs(std::set<std::shared_ptr<Node>> &new_nodes,
                                   std::set<std::shared_ptr<Node>> &deleted_nodes,
                                   std::set<std::shared_ptr<Arc>> &new_arcs,
                                   std::set<std::shared_ptr<Arc>> &deleted_arcs){
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
                    if(!origClkDomainDownsample->isUsingVectorSamplingMode()) {
                        throw std::runtime_error(
                                ErrorHelpers::genErrorStr("Error when copying driver of clock domain.  No driver",
                                                          nodes[i]));
                    }//Clock domains using isUsingVectorSamplingMode are not expected to have a driver
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

std::vector<std::shared_ptr<Node>> Design::findNodesWithState() {
    return EmitterHelpers::findNodesWithState(nodes);
}

std::vector<std::shared_ptr<Node>> Design::findNodesWithGlobalDecl() {
    return EmitterHelpers::findNodesWithGlobalDecl(nodes);
}

std::vector<std::shared_ptr<ContextRoot>> Design::findContextRoots() {
    return EmitterHelpers::findContextRoots(nodes);
}

std::vector<std::shared_ptr<BlackBox>> Design::findBlackBoxes(){
    return EmitterHelpers::findBlackBoxes(nodes);
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

//For now, the emitter will not track if a temp variable's declaration needs to be moved to allow it to be used outside the local context.  This could occur with contexts not being scheduled together.  It could be aleviated by the emitter checking for context breaks.  However, this will be a future todo for now.

std::set<int> Design::listPresentPartitions(){
    std::set<int> partitions;

    for(unsigned long i = 0; i<nodes.size(); i++){
        partitions.insert(nodes[i]->getPartitionNum());
    }

    return partitions;
}

void Design::validateNodes() {
    for(const auto & node : nodes){
        node->validate();
    }
}

void Design::clearContextDiscoveryInfo(){
    topLevelContextRoots.clear();

    for(const std::shared_ptr<Node> &node : nodes){
        std::vector<Context> emptyContext;
        node->setContext(emptyContext);

        std::shared_ptr<ContextRoot> asContextRoot = GeneralHelper::isType<Node, ContextRoot>(node);
        if(asContextRoot){
            asContextRoot->clearNodesInSubContexts();
        }
    }
}