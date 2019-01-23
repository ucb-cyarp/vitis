//
// Created by Christopher Yarp on 7/11/18.
//

#include "MasterNode.h"
#include "GraphMLTools/GraphMLHelper.h"
#include "General/GeneralHelper.h"

MasterNode::MasterNode() {

}

MasterNode::MasterNode(std::shared_ptr<SubSystem> parent, MasterNode* orig) : Node(parent, orig){

};

xercesc::DOMElement *
MasterNode::emitGraphML(xercesc::DOMDocument *doc, xercesc::DOMElement *graphNode, bool include_block_node_type) {
    xercesc::DOMElement* thisNode = emitGraphMLBasics(doc, graphNode);
    if(include_block_node_type) {
        GraphMLHelper::addDataNode(doc, thisNode, "block_node_type", "Master");
    }

    //Emit port names
    unsigned long numInputPorts = inputPorts.size();
    unsigned long numOutputPorts = outputPorts.size();
    GraphMLHelper::addDataNode(doc, thisNode, "named_input_ports", GeneralHelper::to_string(numInputPorts));
    GraphMLHelper::addDataNode(doc, thisNode, "named_output_ports", GeneralHelper::to_string(numOutputPorts));

    for(unsigned long i = 0; i<numInputPorts; i++){
        GraphMLHelper::addDataNode(doc, thisNode, "input_port_name_" + GeneralHelper::to_string(i), inputPorts[i]->getName());
    }

    for(unsigned long i = 0; i<numOutputPorts; i++){
        GraphMLHelper::addDataNode(doc, thisNode, "output_port_name_" + GeneralHelper::to_string(i), outputPorts[i]->getName());
    }

    return thisNode;
}

std::string MasterNode::labelStr() {
    std::string label = Node::labelStr();

    label += "\nType: Master";

    return label;
}

bool MasterNode::canExpand() {
    return false;
}

std::set<GraphMLParameter> MasterNode::graphMLParameters() {
    std::set<GraphMLParameter> parameters;

    parameters.insert(GraphMLParameter("named_input_ports", "int", true));
    parameters.insert(GraphMLParameter("named_output_ports", "int", true));

    unsigned long numInputPorts = inputPorts.size();
    unsigned long numOutputPorts = outputPorts.size();

    for(unsigned long i = 0; i<numInputPorts; i++){
        parameters.insert(GraphMLParameter("input_port_name_" + GeneralHelper::to_string(i) , "string", true));
    }

    for(unsigned long i = 0; i<numOutputPorts; i++){
        parameters.insert(GraphMLParameter("output_port_name_" + GeneralHelper::to_string(i) , "string", true));
    }

    return parameters;
}
