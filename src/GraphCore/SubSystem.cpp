//
// Created by Christopher Yarp on 6/25/18.
//

#include "SubSystem.h"
#include "GraphMLTools/GraphMLHelper.h"

SubSystem::SubSystem() {

}

void SubSystem::addChild(std::shared_ptr<Node> child) {
    children.insert(child);
}

void SubSystem::removeChild(std::shared_ptr<Node> child) {
    children.insert(child);
}

SubSystem::SubSystem(std::shared_ptr<SubSystem> parent) : Node(parent) {

}

//Collect the parameters from the child nodes
std::set<GraphMLParameter> SubSystem::graphMLParameters() {
    //This node has no parameters itself, however, its children may
    std::set<GraphMLParameter> parameters;

    for(std::set<std::shared_ptr<Node>>::iterator child = children.begin(); child != children.end(); child++){
        //insert the parameters from the child into the set
        std::set<GraphMLParameter> childParameters = (*child)->graphMLParameters();
        parameters.insert(childParameters.begin(), childParameters.end());
    }

    return parameters;
}

xercesc::DOMElement *SubSystem::emitGraphML(xercesc::DOMDocument *doc, xercesc::DOMElement *graphNode, bool include_block_node_type) {

    //Emit the basic info for this node
    xercesc::DOMElement *thisNode = emitGraphMLBasics(doc, graphNode);

    if (include_block_node_type){
        GraphMLHelper::addDataNode(doc, thisNode, "block_node_type", "Subsystem");
    }

    emitGramphMLSubgraphAndChildren(doc, thisNode);

    return thisNode;
}

xercesc::DOMElement *
SubSystem::emitGramphMLSubgraphAndChildren(xercesc::DOMDocument *doc, xercesc::DOMElement *thisNode) {
    xercesc::DOMElement* graphElement = GraphMLHelper::createNode(doc, "graph");
    std::string subgraphID = getFullGraphMLPath() + ":";
    GraphMLHelper::setAttribute(graphElement, "id", subgraphID);
    GraphMLHelper::setAttribute(graphElement, "edgedefault", "directed");

    for(auto child = children.begin(); child != children.end(); child++){
        (*child)->emitGraphML(doc, graphElement);
    }

    return graphElement;
}

