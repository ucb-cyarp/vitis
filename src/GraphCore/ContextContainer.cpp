//
// Created by Christopher Yarp on 10/22/18.
//

#include "ContextContainer.h"
#include "General/ErrorHelpers.h"

Context ContextContainer::getContainerContext() const{
    return containerContext;
}

void ContextContainer::setContainerContext(const Context &context) {
    ContextContainer::containerContext = context;
}

std::vector<std::shared_ptr<Node>> ContextContainer::findStateNodesInContext() {
    std::vector<std::shared_ptr<Node>> nodesWithContext;

    for(auto child = children.begin(); child != children.end(); child++){
        if((*child)->hasState()){
            nodesWithContext.push_back(*child);
        }
    }

    return nodesWithContext;
}

ContextContainer::ContextContainer(std::shared_ptr<SubSystem> parent) : SubSystem(parent) {

}

ContextContainer::ContextContainer() {

}

ContextContainer::ContextContainer(std::shared_ptr<SubSystem> parent, ContextContainer *orig) : SubSystem(parent, orig), containerContext(orig->containerContext) {

}

std::shared_ptr<Node> ContextContainer::shallowClone(std::shared_ptr<SubSystem> parent) {
    return NodeFactory::shallowCloneNode<ContextContainer>(parent, this);
}

std::set<GraphMLParameter> ContextContainer::graphMLParameters() {
    std::set<GraphMLParameter> parameters;
    return parameters;
}

xercesc::DOMElement *
ContextContainer::emitGraphML(xercesc::DOMDocument *doc, xercesc::DOMElement *graphNode, bool include_block_node_type) {
    //Create Node
    xercesc::DOMElement* thisNode = SubSystem::emitGraphML(doc, graphNode, false);

    GraphMLHelper::addDataNode(doc, thisNode, "block_function", "ContextContainer");

    return thisNode;
}

std::string ContextContainer::typeNameStr(){
    return "ContextContainer";
}

std::string ContextContainer::labelStr(){
    std::string label = Node::labelStr();

    label += "\nFunction: " + typeNameStr();

    return label;
}

std::string ContextContainer::getFullyQualifiedOrigName(bool sanitize, std::string delim) {
    std::shared_ptr<Node> asNode = GeneralHelper::isType<ContextRoot, Node>(containerContext.getContextRoot());

    if(asNode){
        std::string origName = asNode->getFullyQualifiedOrigName(sanitize, delim);
        if(!origName.empty()){
            origName += delim;
        }
        origName += typeNameStr();
        return origName;
    }else{
        throw std::runtime_error(ErrorHelpers::genErrorStr("Context Root was not a node", getSharedPointer()));
    }
}
