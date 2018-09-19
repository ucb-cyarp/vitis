//
// Created by Christopher Yarp on 9/19/18.
//

#include "SelectNode.h"

SelectNode::SelectNode() : Node() {
    selectorPort = std::unique_ptr<SelectPort>(new SelectPort(this, 0)); //Don't need to do this in init as a raw pointer is passed to the port
    //However, any call to get a shared_ptr of the node or port need to be conducted after a shared pointer has returned
}

SelectNode::SelectNode(std::shared_ptr<SubSystem> parent) : Node(parent) {
    selectorPort = std::unique_ptr<SelectPort>(new SelectPort(this, 0)); //Don't need to do this in init as a raw pointer is passed to the port
    //However, any call to get a shared_ptr of the node or port need to be conducted after a shared pointer has returned
}

SelectNode::SelectNode(std::shared_ptr<SubSystem> parent, SelectNode *orig) : Node(parent, orig){

}

void SelectNode::addSelectArcUpdatePrevUpdateArc(std::shared_ptr<Arc> arc) {

    //Set the dst port of the arc, updating the previous port and this one
    arc->setDstPortUpdateNewUpdatePrev(selectorPort->getSharedPointerInputPort());
}

std::shared_ptr<SelectPort> SelectNode::getSelectorPort() const {
    //Selector port should never be null as it is creates by the Mux constructor and accessors to change it to null do not exist.
    return selectorPort->getSharedPointerSelectPort();
}

void SelectNode::cloneInputArcs(std::vector<std::shared_ptr<Arc>> &arcCopies,
                                std::map<std::shared_ptr<Node>, std::shared_ptr<Node>> &origToCopyNode,
                                std::map<std::shared_ptr<Arc>, std::shared_ptr<Arc>> &origToCopyArc,
                                std::map<std::shared_ptr<Arc>, std::shared_ptr<Arc>> &copyToOrigArc) {
    //Copy the input arcs from standard input ports
    Node::cloneInputArcs(arcCopies, origToCopyNode, origToCopyArc, copyToOrigArc);

    //Copy the input arcs from the enable line
    std::set<std::shared_ptr<Arc>> enablePortArcs = selectorPort->getArcs();

    //Adding an arc to a SelectNode node.  The copy of this node should be an SelectNode so we should be able to cast to it
    std::shared_ptr<SelectNode> clonedDstNode = std::dynamic_pointer_cast<SelectNode>(
            origToCopyNode[shared_from_this()]);

    //Itterate through the arcs and duplicate
    for (auto arcIt = enablePortArcs.begin(); arcIt != enablePortArcs.end(); arcIt++) {
        std::shared_ptr<Arc> origArc = (*arcIt);
        //Get Src Output Port Number and Src Node (as of now, all arcs origionate at an output port)
        std::shared_ptr<OutputPort> srcPort = origArc->getSrcPort();
        int srcPortNumber = srcPort->getPortNum();

        std::shared_ptr<Node> origSrcNode = srcPort->getParent();
        std::shared_ptr<Node> clonedSrcNode = origToCopyNode[origSrcNode];

        //Create the Cloned Arc
        std::shared_ptr<Arc> clonedArc = Arc::connectNodes(clonedSrcNode, srcPortNumber, clonedDstNode,
                                                           (*arcIt)->getDataType()); //This creates a new arc and connects them to the referenced node ports.  This method connects to the select port of the clone of this node
        clonedArc->shallowCopyPrameters(origArc.get());

        //Add to arc list and maps
        arcCopies.push_back(clonedArc);
        origToCopyArc[origArc] = clonedArc;
        copyToOrigArc[clonedArc] = origArc;
    }
}