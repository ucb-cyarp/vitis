//
// Created by Christopher Yarp on 6/27/18.
//

#include "Design.h"

#include <algorithm>

#include "NodeFactory.h"
#include "MasterNodes/MasterInput.h"
#include "MasterNodes/MasterOutput.h"
#include "MasterNodes/MasterUnconnected.h"

#include "GraphCore/Node.h"
#include "GraphCore/Arc.h"

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

    for(unsigned long i = 0; i<numNodes; i++){
        nodes[i]->setId(i);  //Start at ID 0
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
