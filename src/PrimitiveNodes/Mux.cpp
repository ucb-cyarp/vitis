//
// Created by Christopher Yarp on 7/16/18.
//

#include "Mux.h"

Mux::Mux() {
    selectorPort = std::unique_ptr<SelectPort>(new SelectPort(this, 0)); //Don't need to do this in init as a raw pointer is passed to the port
    //However, any call to get a shared_ptr of the node or port need to be conducted after a shared pointer has returned
}

Mux::Mux(std::shared_ptr<SubSystem> parent) : PrimitiveNode(parent) {
    selectorPort = std::unique_ptr<SelectPort>(new SelectPort(this, 0)); //Don't need to do this in init as a raw pointer is passed to the port
    //However, any call to get a shared_ptr of the node or port need to be conducted after a shared pointer has returned
}

std::shared_ptr<SelectPort> Mux::getSelectorPort() const {
    //Selector port should never be null as it is creates by the Mux constructor and accessors to change it to null do not exist.
    return selectorPort->getSharedPointerSelectPort();
}

std::shared_ptr<Mux>
Mux::createFromGraphML(int id, std::string name, std::map<std::string, std::string> dataKeyValueMap,
                       std::shared_ptr<SubSystem> parent, GraphMLDialect dialect) {
    //==== Check Valid Import ====
    if (dialect == GraphMLDialect::SIMULINK_EXPORT) {
        //Not supported for simulink import
        throw std::runtime_error("Mux import is not supported for the SIMULINK_EXPORT dialect");
    } else if (dialect == GraphMLDialect::VITIS) {
        //==== Create Node and set common properties ====
        std::shared_ptr<Mux> newNode = NodeFactory::createNode<Mux>(parent);
        newNode->setId(id);
        newNode->setName(name);

        return newNode;
    } else {
        throw std::runtime_error("Unsupported Dialect when parsing XML - Mux");
    }
}

std::set<GraphMLParameter> Mux::graphMLParameters() {
    return std::set<GraphMLParameter>(); //No parameters to return
}

xercesc::DOMElement *
Mux::emitGraphML(xercesc::DOMDocument *doc, xercesc::DOMElement *graphNode, bool include_block_node_type) {
    //Create Node
    xercesc::DOMElement* thisNode = emitGraphMLBasics(doc, graphNode);

    //Add Parameters / Attributes to Node
    if(include_block_node_type) {
        GraphMLHelper::addDataNode(doc, thisNode, "block_node_type", "Standard");
    }

    GraphMLHelper::addDataNode(doc, thisNode, "block_function", "Mux");

    return thisNode;
}

std::string Mux::labelStr() {
    std::string label = Node::labelStr();

    label += "\nFunction: Mux";

    return label;
}

void Mux::validate() {
    Node::validate();

    selectorPort->validate();
}

