//
// Created by Christopher Yarp on 7/24/18.
//

#include "VectorFanIn.h"
#include "GraphCore/NodeFactory.h"
#include "General/ErrorHelpers.h"

VectorFanIn::VectorFanIn() {

}

VectorFanIn::VectorFanIn(std::shared_ptr<SubSystem> parent) : VectorFan(parent) {

}

std::shared_ptr<VectorFanIn>
VectorFanIn::createFromGraphML(int id, std::string name, std::map<std::string, std::string> dataKeyValueMap,
                               std::shared_ptr<SubSystem> parent, GraphMLDialect dialect) {
    std::shared_ptr<VectorFanIn> newNode = NodeFactory::createNode<VectorFanIn>(parent);
    newNode->setId(id);
    newNode->setName(name);

    //No Further Parameters
    return newNode;
}

std::set<GraphMLParameter> VectorFanIn::graphMLParameters() {
    //No new parameters
    return std::set<GraphMLParameter>();
}

xercesc::DOMElement *
VectorFanIn::emitGraphML(xercesc::DOMDocument *doc, xercesc::DOMElement *graphNode, bool include_block_node_type) {
    xercesc::DOMElement* thisNode = emitGraphMLBasics(doc, graphNode);
    if(include_block_node_type) {
        GraphMLHelper::addDataNode(doc, thisNode, "block_node_type", "VectorFan");
    }

    GraphMLHelper::addDataNode(doc, thisNode, "block_function", "VectorFanIn");

    return thisNode;
}

std::string VectorFanIn::typeNameStr(){
    return "VectorFanIn";
}

std::string VectorFanIn::labelStr() {
    std::string label = Node::labelStr();

    label += "\nFunction: " + typeNameStr();

    return label;
}

void VectorFanIn::validate() {
    Node::validate();

    if(outputPorts.size() != 1){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - VectorFanIn - Should Have Exactly 1 Output Port", getSharedPointer()));
    }

    //Check that the width of the output arc == number of input ports
    std::shared_ptr<Arc> outputArc = *(outputPorts[0]->getArcs().begin());

    if(outputArc->getDataType().numberOfElements() != inputPorts.size()){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - VectorFanIn - Width of Output Arc Should = Number of Input Ports", getSharedPointer()));
    }
}

VectorFanIn::VectorFanIn(std::shared_ptr<SubSystem> parent, VectorFanIn* orig) : VectorFan(parent, orig) {
    //No additional attributes to copy, just call superclass constructor
}

std::shared_ptr<Node> VectorFanIn::shallowClone(std::shared_ptr<SubSystem> parent) {
    return NodeFactory::shallowCloneNode<VectorFanIn>(parent, this);
}
