//
// Created by Christopher Yarp on 6/26/18.
//

#include "ExpandedNode.h"
#include "GraphMLTools/GraphMLHelper.h"

ExpandedNode::ExpandedNode() {
    origNode = std::shared_ptr<ExpandedNode>(nullptr);
}

ExpandedNode::ExpandedNode(std::shared_ptr<SubSystem> parent) : SubSystem(parent) {
    origNode = std::shared_ptr<ExpandedNode>(nullptr);
}

const std::shared_ptr<Node> ExpandedNode::getOrigNode() const {
    return origNode;
}

void ExpandedNode::setOrigNode(std::shared_ptr<Node> origNode) {
    ExpandedNode::origNode = origNode;
}

ExpandedNode::ExpandedNode(std::shared_ptr<SubSystem> parent, std::shared_ptr<Node> orig) : SubSystem(parent), origNode(orig) {
    //Set ID
    id = orig->getId();
    name = "Expanded(" + orig->getName() + ")";

    //Copy input ports of orig node
    std::vector<std::shared_ptr<InputPort>> origInputPorts = orig->getInputPorts();
    unsigned long numInputPorts = origInputPorts.size();

    for(unsigned long i = 0; i<numInputPorts; i++){
        //Create new port in expand node
        inputPorts.push_back(std::unique_ptr<InputPort>(new InputPort(this, i)));

        //Add arcs to expanded node (without changing dst ports).  This is just to get a pointer to the arc and not to change the dst
        for(auto arc = origInputPorts[i]->arcsBegin(); arc != origInputPorts[i]->arcsEnd(); arc++){
            inputPorts[i]->addArc((*arc).lock()); //This call does not update the arc object's src or dst and does not change any other port
        }
    }

    //Copy output ports of orig node
    std::vector<std::shared_ptr<OutputPort>> origOutputPorts = orig->getOutputPorts();
    unsigned long numOutputPorts = origOutputPorts.size();

    for(unsigned long i = 0; i<numOutputPorts; i++){
        //Create new port in expand node
        outputPorts.push_back(std::unique_ptr<OutputPort>(new OutputPort(this, i)));

        //Add arcs to expanded node (without changing src ports).  This is just to get a pointer to the arc and not to change the src
        for(auto arc = origOutputPorts[i]->arcsBegin(); arc != origOutputPorts[i]->arcsEnd(); arc++){
            outputPorts[i]->addArc((*arc).lock()); //This call does not update the arc object's src or dst and does not change any other port
        }
    }
}


xercesc::DOMElement *
ExpandedNode::emitGraphML(xercesc::DOMDocument *doc, xercesc::DOMElement *graphNode, bool include_block_node_type) {

    //Get the parameters from the orig node.  Have the original emit everything but its block type
    xercesc::DOMElement* thisNode = origNode->emitGraphML(doc, graphNode, false);

    if(include_block_node_type) {
        GraphMLHelper::addDataNode(doc, thisNode, "block_node_type", "Expanded");
    }

    emitGramphMLSubgraphAndChildren(doc, thisNode);

    return thisNode;
}

std::string ExpandedNode::labelStr() {
    std::string label = Node::labelStr();

    label += "\nType: Expanded";

    return label;
}
