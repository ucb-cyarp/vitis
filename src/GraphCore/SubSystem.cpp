//
// Created by Christopher Yarp on 6/25/18.
//

#include "SubSystem.h"
#include "GraphMLTools/GraphMLHelper.h"
#include "GraphCore/NodeFactory.h"
#include "General/GeneralHelper.h"
#include "General/GraphAlgs.h"

SubSystem::SubSystem() {

}

void SubSystem::addChild(std::shared_ptr<Node> child) {
    children.insert(child);
}

void SubSystem::removeChild(std::shared_ptr<Node> child) {
    children.erase(child);
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
    thisNode->appendChild(graphElement);

    for(auto child = children.begin(); child != children.end(); child++){
        (*child)->emitGraphML(doc, graphElement);
    }

    return graphElement;
}

std::string SubSystem::labelStr() {
    std::string label = Node::labelStr();

    label += "\nType: Subsystem";

    return label;
}

bool SubSystem::canExpand() {
    return false;
}

std::set<std::shared_ptr<Node>> SubSystem::getChildren() const {
    return children;
}

void SubSystem::setChildren(const std::set<std::shared_ptr<Node>> &children) {
    SubSystem::children = children;
}

SubSystem::SubSystem(std::shared_ptr<SubSystem> parent, SubSystem* orig) : Node(parent, orig) {
    //Do not copy children
}

std::shared_ptr<Node> SubSystem::shallowClone(std::shared_ptr<SubSystem> parent) {
    return NodeFactory::shallowCloneNode<SubSystem>(parent, this);
}

void
SubSystem::shallowCloneWithChildren(std::shared_ptr<SubSystem> parent, std::vector<std::shared_ptr<Node>> &nodeCopies,
                                    std::map<std::shared_ptr<Node>, std::shared_ptr<Node>> &origToCopyNode,
                                    std::map<std::shared_ptr<Node>, std::shared_ptr<Node>> &copyToOrigNode) {

    //Copy this node
    std::shared_ptr<SubSystem> clonedNode = std::static_pointer_cast<SubSystem>(shallowClone(parent)); //This is a subsystem so we can cast to a subsystem pointer

    //Put into vectors and maps
    nodeCopies.push_back(clonedNode);
    origToCopyNode[shared_from_this()] = clonedNode;
    copyToOrigNode[clonedNode] = shared_from_this();

    //Copy children
    for(auto it = children.begin(); it != children.end(); it++){
        //Recursive call to this function
        (*it)->shallowCloneWithChildren(clonedNode, nodeCopies, origToCopyNode, copyToOrigNode); //Use the copied node as the parent
    }
}

void SubSystem::extendEnabledSubsystemContext(std::vector<std::shared_ptr<Node>> &new_nodes,
                                           std::vector<std::shared_ptr<Node>> &deleted_nodes,
                                           std::vector<std::shared_ptr<Arc>> &new_arcs,
                                           std::vector<std::shared_ptr<Arc>> &deleted_arcs){
    //Run this recursivly on child subsystems

    //Find the subsystems in the list of children and work on them
        //Note, we do this search first because nodes can be moved durring the recursive call
        //However, subsystems are currently not moved, they are replicated

    std::vector<std::shared_ptr<SubSystem>> childSubsystems;
    for(auto child = children.begin(); child != children.end(); child++){
        if(GeneralHelper::isType<Node, SubSystem>(*child) != nullptr){
            childSubsystems.push_back(std::dynamic_pointer_cast<SubSystem>(*child));
        }
    }

    for(unsigned long i = 0; i<childSubsystems.size(); i++){
        //Adding a condition to check if the subsystem still has this node as parent.
        //TODO: remove condition if subsystems are never moved
        if(childSubsystems[i]->getParent() == getSharedPointer()){
            childSubsystems[i]->extendEnabledSubsystemContext(new_nodes, deleted_nodes, new_arcs, deleted_arcs);
        }else{
            throw std::runtime_error("Subsystem moved during enabled subsystem context expansion.  This was unexpected.");
        }
    }
}

void
SubSystem::discoverAndUpdateContexts(std::vector<Context> contextStack,
                                     std::vector<std::shared_ptr<Mux>> &discoveredMux,
                                     std::vector<std::shared_ptr<EnabledSubSystem>> &discoveredEnabledSubSystems,
                                     std::vector<std::shared_ptr<Node>> &discoveredGeneral) {
    std::set<std::shared_ptr<Node>, Node::PtrID_Compare> idOrderedChildren;
    idOrderedChildren.insert(children.begin(), children.end());
    std::vector<std::shared_ptr<Node>> childrenVector;
    childrenVector.insert(childrenVector.end(), idOrderedChildren.begin(), idOrderedChildren.end());
    GraphAlgs::discoverAndUpdateContexts(childrenVector, contextStack, discoveredMux, discoveredEnabledSubSystems,
                                         discoveredGeneral);
}

void SubSystem::orderConstrainZeroInputNodes(std::vector<std::shared_ptr<Node>> predecessorNodes,
                                             std::vector<std::shared_ptr<Node>> &new_nodes,
                                             std::vector<std::shared_ptr<Node>> &deleted_nodes,
                                             std::vector<std::shared_ptr<Arc>> &new_arcs,
                                             std::vector<std::shared_ptr<Arc>> &deleted_arcs) {

    for(auto child = children.begin(); child != children.end(); child++){
        std::shared_ptr<SubSystem> childAsSubsystem = GeneralHelper::isType<Node, SubSystem>(*child);

        if(childAsSubsystem){
            childAsSubsystem->orderConstrainZeroInputNodes(predecessorNodes, new_nodes, deleted_nodes, new_arcs, deleted_arcs);
        }else{
            if((*child)->inDegree() == 0){
                //This child (which is not a subsystem), has 0 indegree
                //Create order constraint arcs
                for(unsigned long i = 0; i<predecessorNodes.size(); i++){
                    if(predecessorNodes[i] != *child){
                        std::shared_ptr<Arc> constraintArc = Arc::connectNodesOrderConstraint(predecessorNodes[i], *child);
                        new_arcs.push_back(constraintArc);
                    }
                }
            }
        }
    }

}
