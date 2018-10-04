//
// Created by Christopher Yarp on 7/24/18.
//

#include "VectorFanOut.h"
#include "GraphCore/NodeFactory.h"

VectorFanOut::VectorFanOut() {

}

VectorFanOut::VectorFanOut(std::shared_ptr<SubSystem> parent) : VectorFan(parent) {

}

std::shared_ptr<VectorFanOut>
VectorFanOut::createFromGraphML(int id, std::string name, std::map<std::string, std::string> dataKeyValueMap,
                                std::shared_ptr<SubSystem> parent, GraphMLDialect dialect) {
    std::shared_ptr<VectorFanOut> newNode = NodeFactory::createNode<VectorFanOut>(parent);
    newNode->setId(id);
    newNode->setName(name);

    //No Further Parameters
    return newNode;
}

std::set<GraphMLParameter> VectorFanOut::graphMLParameters() {
    //No new parameters
    return std::set<GraphMLParameter>();
}

xercesc::DOMElement *
VectorFanOut::emitGraphML(xercesc::DOMDocument *doc, xercesc::DOMElement *graphNode, bool include_block_node_type) {
    xercesc::DOMElement* thisNode = emitGraphMLBasics(doc, graphNode);
    if(include_block_node_type) {
        GraphMLHelper::addDataNode(doc, thisNode, "block_node_type", "VectorFan");
    }

    GraphMLHelper::addDataNode(doc, thisNode, "block_function", "VectorFanOut");

    return thisNode;
}

std::string VectorFanOut::labelStr() {
    std::string label = Node::labelStr();

    label += "\nFunction: VectorFanOut";

    return label;
}

void VectorFanOut::validate() {
    Node::validate();

    if(inputPorts.size() != 1){
        throw std::runtime_error("Validation Failed - VectorFanOut - Should Have Exactly 1 Output Port");
    }

    //Check that the width of the input arc == number of output ports
    std::shared_ptr<Arc> inputArc = *(inputPorts[0]->getArcs().begin());

    if(inputArc->getDataType().getWidth() != outputPorts.size()){
        throw std::runtime_error("Validation Failed - VectorFanIn - Width of Input Arc Should = Number of Output Ports");
    }
}

VectorFanOut::VectorFanOut(std::shared_ptr<SubSystem> parent, VectorFanOut* orig) : VectorFan(parent, orig){
    //No additional attributes to copy, just call superclass constructor
}

std::shared_ptr<Node> VectorFanOut::shallowClone(std::shared_ptr<SubSystem> parent) {
    return NodeFactory::shallowCloneNode<VectorFanOut>(parent, this);
}
