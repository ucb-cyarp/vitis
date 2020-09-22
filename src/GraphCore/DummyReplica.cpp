//
// Created by Christopher Yarp on 5/7/20.
//

#include "DummyReplica.h"
#include "General/ErrorHelpers.h"

DummyReplica::DummyReplica() {

}

DummyReplica::DummyReplica(std::shared_ptr<SubSystem> parent) : Node(parent) {

}

DummyReplica::DummyReplica(std::shared_ptr<SubSystem> parent, Node *orig) : Node(parent, orig) {

}

bool DummyReplica::canExpand() {
    return false;
}

std::shared_ptr<DummyReplica>
DummyReplica::createFromGraphML(int id, std::string name, std::map<std::string, std::string> dataKeyValueMap,
                                std::shared_ptr<SubSystem> parent, GraphMLDialect dialect) {
    //==== Create Node and set common properties ====
    std::shared_ptr<DummyReplica> newNode = NodeFactory::createNode<DummyReplica>(parent);
    newNode->setId(id);
    newNode->setName(name);

    if (dialect != GraphMLDialect::VITIS) {
        //This is a vitis only node
        throw std::runtime_error(ErrorHelpers::genErrorStr("Unsupported Dialect when parsing XML - DummyReplica", newNode));
    }

    return newNode;
}

std::set<GraphMLParameter> DummyReplica::graphMLParameters() {
    return Node::graphMLParameters();
}

xercesc::DOMElement *
DummyReplica::emitGraphML(xercesc::DOMDocument *doc, xercesc::DOMElement *graphNode, bool include_block_node_type) {
    //Create Node
    xercesc::DOMElement* thisNode = emitGraphMLBasics(doc, graphNode);

    //Add Parameters / Attributes to Node
    if(include_block_node_type) {
        GraphMLHelper::addDataNode(doc, thisNode, "block_node_type", "Standard");
    }

    GraphMLHelper::addDataNode(doc, thisNode, "block_function", "DummyReplica");

    return thisNode;
}

std::string DummyReplica::typeNameStr() {
    return "DummyReplica";
}

std::string DummyReplica::labelStr() {
    std::string label = Node::labelStr();

    label += "\nFunction: " + typeNameStr();

    return label;
}

void DummyReplica::validate() {
    Node::validate();
}

std::shared_ptr<Node> DummyReplica::shallowClone(std::shared_ptr<SubSystem> parent) {
    return NodeFactory::shallowCloneNode<DummyReplica>(parent, this);
}

CExpr
DummyReplica::emitCExpr(std::vector<std::string> &cStatementQueue, SchedParams::SchedType schedType, int outputPortNum,
                        bool imag) {
    return CExpr("", false); //Don't return anything
}

std::string
DummyReplica::emitC(std::vector<std::string> &cStatementQueue, SchedParams::SchedType schedType, int outputPortNum,
                    bool imag, bool checkFanout, bool forceFanout) {
    //Like constant, do not check for fanout
    return Node::emitC(cStatementQueue, schedType, outputPortNum, imag, false, false);
}

std::shared_ptr<ContextRoot> DummyReplica::getDummyOf() const {
    return dummyOf;
}

void DummyReplica::setDummyOf(const std::shared_ptr<ContextRoot> &dummyOf) {
    DummyReplica::dummyOf = dummyOf;
}
