//
// Created by Christopher Yarp on 9/13/18.
//

#include "UnsupportedSink.h"

#include <iostream>

UnsupportedSink::UnsupportedSink() {

}

UnsupportedSink::UnsupportedSink(std::shared_ptr<SubSystem> parent) : PrimitiveNode(parent) {

}

std::string UnsupportedSink::getNodeType() const {
    return nodeType;
}

void UnsupportedSink::setNodeType(const std::string &nodeType) {
    UnsupportedSink::nodeType = nodeType;
}

std::map<std::string, std::string> UnsupportedSink::getDataKeyValueMap() const {
    return dataKeyValueMap;
}

void UnsupportedSink::setDataKeyValueMap(const std::map<std::string, std::string> &dataKeyValueMap) {
    UnsupportedSink::dataKeyValueMap = dataKeyValueMap;
}

std::shared_ptr<UnsupportedSink> UnsupportedSink::createFromGraphML(int id, std::string name, std::string type,
                                                                    std::map<std::string, std::string> dataKeyValueMap,
                                                                    std::shared_ptr<SubSystem> parent) {
    std::shared_ptr<UnsupportedSink> newNode = NodeFactory::createNode<UnsupportedSink>(parent);
    newNode->setId(id);
    newNode->setName(name);
    newNode->setNodeType(type);
    newNode->setDataKeyValueMap(dataKeyValueMap);
    return newNode;
}

std::set<GraphMLParameter> UnsupportedSink::graphMLParameters() {
    std::set<GraphMLParameter> params;

    for(auto it = dataKeyValueMap.begin(); it != dataKeyValueMap.end(); it++){
        params.insert(GraphMLParameter(it->first, "string", true));
    }

    return params;
}

xercesc::DOMElement *
UnsupportedSink::emitGraphML(xercesc::DOMDocument *doc, xercesc::DOMElement *graphNode, bool include_block_node_type) {
    xercesc::DOMElement* thisNode = emitGraphMLBasics(doc, graphNode);
    if(include_block_node_type) {
        GraphMLHelper::addDataNode(doc, thisNode, "block_node_type", "Standard");
    }

    //Set the node type to be the origional type
    GraphMLHelper::addDataNode(doc, thisNode, "block_function", nodeType);

    //Add Data Nodes for the other parameters
    for(auto it = dataKeyValueMap.begin(); it != dataKeyValueMap.end(); it++){
        GraphMLHelper::addDataNode(doc, thisNode, it->first, it->second);
    }

    return thisNode;
}

std::string UnsupportedSink::labelStr() {
    std::string label = Node::labelStr();

    label += "\nUnsupported Sink\nType: " + nodeType;

    return label;
}

void UnsupportedSink::validate() {
    Node::validate();

    std::cerr << "Warning: Unsupported Sink Node Exists in Design: " << getFullyQualifiedName() << " (" << nodeType << ")" << std::endl;
}

CExpr UnsupportedSink::emitCExpr(std::vector<std::string> &cStatementQueue, int outputPortNum, bool imag) {
    //Do nothing
    return CExpr("", false);
}

UnsupportedSink::UnsupportedSink(std::shared_ptr<SubSystem> parent, UnsupportedSink* orig) : PrimitiveNode(parent, orig), nodeType(orig->nodeType), dataKeyValueMap(orig->dataKeyValueMap) {

}

std::shared_ptr<Node> UnsupportedSink::shallowClone(std::shared_ptr<SubSystem> parent) {
    return NodeFactory::shallowCloneNode<UnsupportedSink>(parent, this);
}



