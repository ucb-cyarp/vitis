//
// Created by Christopher Yarp on 7/10/18.
//

#include "GraphTestHelper.h"

#include "MasterNodes/MasterInput.h"
#include "MasterNodes/MasterOutput.h"
#include "MasterNodes/MasterUnconnected.h"

void GraphTestHelper::verifyArcLinks(std::shared_ptr<Arc> arc) {
    std::shared_ptr<Port> srcPort = arc->getSrcPort();
    std::shared_ptr<Port> dstPort = arc->getDstPort();

    //Check Link
    std::set<std::shared_ptr<Arc>> srcPortArcs = srcPort->getArcs();
    ASSERT_TRUE(srcPortArcs.find(arc) != srcPortArcs.end());

    std::set<std::shared_ptr<Arc>> dstPortArcs = dstPort->getArcs();
    ASSERT_TRUE(dstPortArcs.find(arc) != dstPortArcs.end());

    //Check only 1 arc connected to dst port (no multi driver nets)
    ASSERT_EQ(dstPortArcs.size(), 1);
}

void GraphTestHelper::verifyArcLinksTerminator(std::shared_ptr<Arc> arc) {
    std::shared_ptr<Port> srcPort = arc->getSrcPort();
    std::shared_ptr<Port> dstPort = arc->getDstPort();

    //Check Link
    std::set<std::shared_ptr<Arc>> srcPortArcs = srcPort->getArcs();
    ASSERT_TRUE(srcPortArcs.find(arc) != srcPortArcs.end());

    std::set<std::shared_ptr<Arc>> dstPortArcs = dstPort->getArcs();
    ASSERT_TRUE(dstPortArcs.find(arc) != dstPortArcs.end());
}

void GraphTestHelper::verifyDataType(DataType dataType, bool floatingPtExpected, bool signedTypeExpected,
                                     bool complexExpected, int totalBitsExpected, int fractionalBitsExpected,
                                     std::vector<int> dimensionsExpected) {
    ASSERT_EQ(dataType.isFloatingPt(), floatingPtExpected);
    ASSERT_EQ(dataType.isSignedType(), signedTypeExpected);
    ASSERT_EQ(dataType.isComplex(), complexExpected);
    ASSERT_EQ(dataType.getTotalBits(), totalBitsExpected);
    ASSERT_EQ(dataType.getFractionalBits(), fractionalBitsExpected);
    ASSERT_EQ(dataType.getDimensions(), dimensionsExpected);
}

void GraphTestHelper::verifyPortNumbers(std::shared_ptr<Node> node) {
    unsigned long numInputPorts = node->getInputPorts().size();

    for(unsigned long i = 0; i<numInputPorts; i++){
        ASSERT_EQ(i, node->getInputPort(i)->getPortNum());
    }

    unsigned long numOutputPorts = node->getOutputPorts().size();

    for(unsigned long i = 0; i<numOutputPorts; i++){
        ASSERT_EQ(i, node->getOutputPort(i)->getPortNum());
    }
}

void GraphTestHelper::verifyNodePortNumbers(Design &design) {
    //Check Master Nodes
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyPortNumbers(design.getInputMaster());
    }
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyPortNumbers(design.getOutputMaster());
    }
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyPortNumbers(design.getTerminatorMaster());
    }
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyPortNumbers(design.getVisMaster());
    }
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyPortNumbers(design.getUnconnectedMaster());
    }

    //Check Nodes in Node Vector
    std::vector<std::shared_ptr<Node>> nodes = design.getNodes();
    for(unsigned long i = 0; i<nodes.size(); i++)
    {
        {
            SCOPED_TRACE("Node: " + GeneralHelper::to_string(i));
            GraphTestHelper::verifyPortNumbers(nodes[i]);
        }
    }
}

void GraphTestHelper::verifyNodesWithoutParentAreInTopLevelList(Design &design) {
    std::vector<std::shared_ptr<Node>> nodes = design.getNodes();
    unsigned long numNodes = nodes.size();

    std::vector<std::shared_ptr<Node>> topLevelNodes = design.getTopLevelNodes();
    unsigned long numTopLevelNodes = topLevelNodes.size();

    for(unsigned long i = 0; i<numNodes; i++){
        if(nodes[i]->getParent() == nullptr) {
            //search top level nodes
            bool found = false;

            for (unsigned long j = 0; j < numTopLevelNodes; j++) {
                if (nodes[i] == topLevelNodes[j]) {
                    found = true;
                    break;
                }
            }

            ASSERT_TRUE(found) << "Node without parent found which is not in top level node list - Name:" << nodes[i]->getName() << ", ID " << nodes[i]->getId();
        }
        else{
            //search top level nodes
            bool found = false;

            for (unsigned long j = 0; j < numTopLevelNodes; j++) {
                if (nodes[i] == topLevelNodes[j]) {
                    found = true;
                    break;
                }
            }

            ASSERT_FALSE(found) << "Node with parent found which is in top level node list";
        }
    }

}

void GraphTestHelper::verifyOutputPortArcCounts(std::shared_ptr<Node> node, std::vector<unsigned long> arcCounts) {
    std::vector<std::shared_ptr<OutputPort>> outputPorts = node->getOutputPorts();

    //Check the number of output ports matches the size of the arcCounts vector
    ASSERT_EQ(outputPorts.size(), arcCounts.size());

    //Check the number of arcs connected to each port
    for(unsigned long i = 0; i<outputPorts.size(); i++){
        ASSERT_EQ(outputPorts[i]->getArcs().size(), arcCounts[i]) << "OutputPort: " << GeneralHelper::to_string(i);
    }
}

void GraphTestHelper::verifyConnection(Design &design, std::shared_ptr<Node> src, int outPort, std::shared_ptr<Node> dst,
                                             GraphTestHelper::InPortType inPortType, int inPort,
                                             std::shared_ptr<Arc> &foundArc, bool dstIsTerminator) {

    //Get the set of arcs for the given output port
    std::set<std::shared_ptr<Arc>> outArcs = src->getOutputPort(outPort)->getArcs();

    //Search through the set of arcs
    bool found = false;
    std::shared_ptr<Arc> foundArcLocal;

    for(auto arcIter = outArcs.begin(); arcIter != outArcs.end(); arcIter++){
        std::shared_ptr<Arc> arc = *arcIter;

        //Check tgt node
        if(arc->getDstPort()->getParent() == dst) {
            //Check tgt port
            if (inPortType == InPortType::STANDARD) {
                //Check dst port number
                if(arc->getDstPort()->getPortNum() == inPort){
                    found = true;
                    foundArcLocal = arc;
                    break;
                }
            } else if (inPortType == InPortType::ENABLE) {
                std::shared_ptr<EnableNode> dstNode = std::dynamic_pointer_cast<EnableNode>(arc->getDstPort()->getParent());
                if(dstNode != nullptr && arc->getDstPort() == dstNode->getEnablePort()){
                    found = true;
                    foundArcLocal = arc;
                    break;
                }

            } else if (inPortType == InPortType::SELECT) {
                std::shared_ptr<Mux> dstNode = std::dynamic_pointer_cast<Mux>(arc->getDstPort()->getParent());
                if(dstNode != nullptr && arc->getDstPort() == dstNode->getSelectorPort()){
                    found = true;
                    foundArcLocal = arc;
                    break;
                }
            }
        }
    }

    ASSERT_TRUE(found) << "Connection not found";

    if(found){
        foundArc = foundArcLocal;

        //Check arc
        if(dstIsTerminator){
            verifyArcLinksTerminator(foundArcLocal);
        }else{
            verifyArcLinks(foundArcLocal);
        }

        //Check if in design
        std::vector<std::shared_ptr<Arc>> arcs = design.getArcs();
        unsigned long numArcs = arcs.size();
        bool found = false;
        for(unsigned long i = 0; i<numArcs; i++){
            if(foundArcLocal == arcs[i]){
                found = true;
                break;
            }
        }

        ASSERT_TRUE(found) << "Arc not found in design arc vector";

    }else{
        foundArc = nullptr;
    }
}

void GraphTestHelper::verifyPortArcCounts(std::shared_ptr<Node> node, unsigned long inputPorts,
                                          std::vector<unsigned long> arcCounts) {
    //Check input ports
    std::vector<std::shared_ptr<InputPort>> inputPortVector = node->getInputPorts();
    unsigned long inputPortLen = inputPortVector.size();

    ASSERT_EQ(inputPortLen, inputPorts);

    for(unsigned long i = 0; i<inputPortLen; i++){
        ASSERT_EQ(inputPortVector[i]->getArcs().size(), 1);
    }

    verifyOutputPortArcCounts(node, arcCounts);
}

void GraphTestHelper::verifyNodesInDesignFromHierarchicalTraversal(Design &design) {
    unsigned long count = 0;

    //Traverse each top level node
    std::vector<std::shared_ptr<Node>> topLevelNodes = design.getTopLevelNodes();
    unsigned long numTopLevelNodes = topLevelNodes.size();

    for(unsigned long i = 0; i<numTopLevelNodes; i++){
        GraphTestHelper::verifyNodesInDesignFromHierarchicalTraversalHelper(design, topLevelNodes[i], count);
    }

    //Check the count
    ASSERT_EQ(count, design.getNodes().size()) << "Counts of nodes found in traversal does not match count of nodes in design vector";
}

void GraphTestHelper::verifyNodesInDesignFromHierarchicalTraversalHelper(Design &design, std::shared_ptr<Node> node, unsigned long &count) {
    //++++Verify the current node++++
    //Search the node array
    std::vector<std::shared_ptr<Node>> designNodes = design.getNodes();
    unsigned long numNodes = designNodes.size();
    bool found = false;
    for(unsigned long i = 0; i<numNodes; i++){
        if(node == designNodes[i]){
            found = true;
            break;
        }
    }

    ASSERT_TRUE(found) << "Node in hierarchy not found in node array";

    //++++Increment count++++
    count++;

    //++++Call on children++++
    //Cast to subsystem if possible

    std::shared_ptr<SubSystem> subsys = std::dynamic_pointer_cast<SubSystem>(node);
    if(subsys != nullptr){
        //Call on children

        std::set<std::shared_ptr<Node>> children = subsys->getChildren();
        for(auto nodeIter = children.begin(); nodeIter != children.end(); nodeIter++){
            verifyNodesInDesignFromHierarchicalTraversalHelper(design, *nodeIter, count);
        }
    }
}

void
GraphTestHelper::verifyConnection(Design &design, std::shared_ptr<Node> src, int outPort, std::shared_ptr<Node> dst,
                                  GraphTestHelper::InPortType inPortType, int inPort,
                                  DataType expectedType,unsigned long &checkCount,  bool dstIsTerminator) {
    std::shared_ptr<Arc> foundArc = nullptr;

    GraphTestHelper::verifyConnection(design, src, outPort, dst, inPortType, inPort, foundArc, dstIsTerminator);

    ASSERT_EQ(foundArc->getDataType(), expectedType) << "Arc had unexpected datatype";

    checkCount++; //Increment the number of arcs which have been checked
}

std::shared_ptr<Node> GraphTestHelper::findNodeByName(Design &design, std::string name){
    std::vector<std::shared_ptr<Node>> nodes = design.getNodes();
    for(const std::shared_ptr<Node> &node : nodes){
        if(node->getName() == name){
            return node;
        }
    }

    return nullptr;
}