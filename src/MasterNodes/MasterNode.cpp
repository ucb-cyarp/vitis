//
// Created by Christopher Yarp on 7/11/18.
//

#include "MasterNode.h"
#include "GraphMLTools/GraphMLHelper.h"
#include "GraphMLTools/GraphMLExporter.h"
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

    GraphMLExporter::exportPortNames(getSharedPointer(), doc, thisNode);

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

    GraphMLExporter::addPortNameProperties(getSharedPointer(), parameters);

    return parameters;
}
