//
// Created by Christopher Yarp on 2019-03-15.
//

#include "Concatenate.h"

Concatenate::Concatenate() {

}

Concatenate::Concatenate(std::shared_ptr<SubSystem> parent) : Node(parent) {

}

Concatenate::Concatenate(std::shared_ptr<SubSystem> parent, Concatenate *orig) : Node(parent, orig) {

}

std::shared_ptr<Concatenate>
Concatenate::createFromGraphML(int id, std::string name, std::map<std::string, std::string> dataKeyValueMap,
                               std::shared_ptr<SubSystem> parent, GraphMLDialect dialect) {
    std::shared_ptr<Concatenate> newNode = NodeFactory::createNode<Concatenate>(parent);
    newNode->setId(id);
    newNode->setName(name);

    //No Further Parameters
    return newNode;
}

bool Concatenate::canExpand() {
    return false;
}

std::set<GraphMLParameter> Concatenate::graphMLParameters() {
    return std::set<GraphMLParameter>();
}

xercesc::DOMElement *
Concatenate::emitGraphML(xercesc::DOMDocument *doc, xercesc::DOMElement *graphNode, bool include_block_node_type) {
    xercesc::DOMElement* thisNode = emitGraphMLBasics(doc, graphNode);
    if(include_block_node_type) {
        GraphMLHelper::addDataNode(doc, thisNode, "block_node_type", "Standard");
    }

    GraphMLHelper::addDataNode(doc, thisNode, "block_function", "Concatenate");

    return thisNode;
}

std::string Concatenate::typeNameStr(){
    return "Concatenate";
};

std::string Concatenate::labelStr() {
    std::string label = Node::labelStr();

    label += "\nFunction: " + typeNameStr();

    return label;
}

void Concatenate::validate() {
    Node::validate();

    //Do not validate number of ports because simulink

//    if(outputPorts.size() != 1){
//        throw std::runtime_error("Validation Failed - TappedDelay - Should Have Exactly 1 Output Port");
//    }
}

std::shared_ptr<Node> Concatenate::shallowClone(std::shared_ptr<SubSystem> parent) {
    return NodeFactory::shallowCloneNode<Concatenate>(parent, this);
}
