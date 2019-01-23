//
// Created by Christopher Yarp on 6/26/18.
//

#include "EnableNode.h"
#include "Port.h"
#include "EnabledSubSystem.h"
#include "Arc.h"

EnableNode::EnableNode() {
    enablePort = std::unique_ptr<EnablePort>(new EnablePort(this, 0)); //Don't need to do this in init as a raw pointer is passed to the port
    //However, any call to get a shared_ptr of the node or port need to be conducted after a shared pointer has returned
}

EnableNode::EnableNode(std::shared_ptr<SubSystem> parent) : Node(parent) {
    enablePort = std::unique_ptr<EnablePort>(new EnablePort(this, 0)); //Don't need to do this in init as a raw pointer is passed to the port
    //However, any call to get a shared_ptr of the node or port need to be conducted after a shared pointer has returned
}

std::shared_ptr<Port> EnableNode::getEnablePort(){
    return enablePort->getSharedPointer();
}

void EnableNode::setEnableArcUpdatePrevUpdateArc(std::shared_ptr<Arc> arc) {
    //Set the dst port of the arc, updating the previous port and this one
    arc->setDstPortUpdateNewUpdatePrev(enablePort->getSharedPointerInputPort());
}

void EnableNode::validate() {
    Node::validate();

    enablePort->validate();

    //Check that Parent is an Enabled Subsystem
    std::shared_ptr<EnabledSubSystem> parentEnabled = std::dynamic_pointer_cast<EnabledSubSystem>(parent);
    if(!parentEnabled){
        throw std::runtime_error("Enable Node found whose parent is not an EnabledSubSystem");
    }
}

bool EnableNode::canExpand() {
    return false;
}

EnableNode::EnableNode(std::shared_ptr<SubSystem> parent, EnableNode* orig) : Node(parent, orig) {
    //Do not copy enable port but create a new one
    enablePort = std::unique_ptr<EnablePort>(new EnablePort(this, 0)); //Don't need to do this in init as a raw pointer is passed to the port
}

void EnableNode::cloneInputArcs(std::vector<std::shared_ptr<Arc>> &arcCopies,
                                std::map<std::shared_ptr<Node>, std::shared_ptr<Node>> &origToCopyNode,
                                std::map<std::shared_ptr<Arc>, std::shared_ptr<Arc>> &origToCopyArc,
                                std::map<std::shared_ptr<Arc>, std::shared_ptr<Arc>> &copyToOrigArc) {
    //Copy the input arcs from standard input ports
    Node::cloneInputArcs(arcCopies, origToCopyNode, origToCopyArc, copyToOrigArc);

    //Copy the input arcs from the enable line
    std::set<std::shared_ptr<Arc>> enablePortArcs = enablePort->getArcs();

    //Adding an arc to an enable node.  The copy of this node should be an EnableNode so we should be able to cast to it
    std::shared_ptr<EnableNode> clonedDstNode = std::dynamic_pointer_cast<EnableNode>(
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
                                                           (*arcIt)->getDataType()); //This creates a new arc and connects them to the referenced node ports.  This method connects to the enable port of the clone of this node
        clonedArc->shallowCopyPrameters(origArc.get());

        //Add to arc list and maps
        arcCopies.push_back(clonedArc);
        origToCopyArc[origArc] = clonedArc;
        copyToOrigArc[clonedArc] = origArc;

    }
}

std::set<std::shared_ptr<Arc>> EnableNode::disconnectInputs() {
    std::set<std::shared_ptr<Arc>> disconnectedArcs = Node::disconnectInputs(); //Remove the arcs connected to the standard ports

    //Remove arcs connected to the enable port
    std::set<std::shared_ptr<Arc>> enableArcs = enablePort->getArcs();

    //Disconnect each of the arcs from both ends
    //We can do this without disturbing the port the set since the set here is a copy of the port set
    for(auto it = enableArcs.begin(); it != enableArcs.end(); it++){
        //These functions update the previous endpoints of the arc (ie. removes the arc from them)
        (*it)->setDstPortUpdateNewUpdatePrev(nullptr);
        (*it)->setSrcPortUpdateNewUpdatePrev(nullptr);

        //Add this arc to the list of arcs removed
        disconnectedArcs.insert(*it);
    }

    return disconnectedArcs;
}

std::set<std::shared_ptr<Node>> EnableNode::getConnectedInputNodes() {
    std::set<std::shared_ptr<Node>> connectedNodes = Node::getConnectedInputNodes(); //Get the nodes connected to the standard input ports

    //Get nodes connected to the enable port
    std::set<std::shared_ptr<Arc>> enableArcs = enablePort->getArcs();

    for(auto it = enableArcs.begin(); it != enableArcs.end(); it++){
        connectedNodes.insert((*it)->getSrcPort()->getParent()); //The enable port is an input port
    }

    return connectedNodes;
}

unsigned long EnableNode::inDegree() {
    unsigned long count = Node::inDegree(); //Get the number of arcs connected to the standard input ports

    //Get the arcs connected to the enable port
    count += enablePort->getArcs().size();

    return count;
}

std::vector<std::shared_ptr<Arc>>
EnableNode::connectUnconnectedPortsToNode(std::shared_ptr<Node> connectToSrc, std::shared_ptr<Node> connectToSink,
                                          int srcPortNum, int sinkPortNum) {
    std::vector<std::shared_ptr<Arc>> newArcs = Node::connectUnconnectedPortsToNode(connectToSrc, connectToSink, srcPortNum, sinkPortNum); //Connect standard ports

    unsigned long numEnableArcs = enablePort->getArcs().size();

    if(numEnableArcs == 0){
        //This port is unconnected, connect it to the given node.

        //Connect from given node to this node's input port
        //TODO: Use default datatype for now.  Perhaps change later?
        std::shared_ptr<Arc> arc = Arc::connectNodes(connectToSrc, srcPortNum, std::dynamic_pointer_cast<EnableNode>(shared_from_this()), DataType());
        newArcs.push_back(arc);
    }

    return newArcs;
}

std::vector<std::shared_ptr<InputPort>> EnableNode::getInputPortsIncludingSpecial() {
    //Get standard input ports
    std::vector<std::shared_ptr<InputPort>> portList = Node::getInputPortsIncludingSpecial();

    //Add enable port
    portList.push_back(enablePort->getSharedPointerEnablePort());

    return portList;
}
