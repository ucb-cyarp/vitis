//
// Created by Christopher Yarp on 8/31/20.
//

#include "PartitionCrossing.h"
#include "GraphMLTools/GraphMLHelper.h"
#include "General/GeneralHelper.h"

int PartitionCrossing::getInitStateCountBlocks() const {
    return initStateCountBlocks;
}

void PartitionCrossing::setInitStateCountBlocks(int initStateCountBlocks) {
    PartitionCrossing::initStateCountBlocks = initStateCountBlocks;
}

PartitionCrossing::PartitionCrossing() : Arc() {

}

PartitionCrossing::PartitionCrossing(std::shared_ptr<OutputPort> srcPort, std::shared_ptr<InputPort> dstPort,
                                     DataType dataType, double sampleTime) : Arc(srcPort, dstPort, dataType,
                                                                                 sampleTime) {

}

std::shared_ptr<PartitionCrossing> PartitionCrossing::createArc() {
    return std::shared_ptr<PartitionCrossing>(new PartitionCrossing());
}

std::shared_ptr<PartitionCrossing>
PartitionCrossing::connectNodes(std::shared_ptr<OutputPort> srcPort, std::shared_ptr<InputPort> dstPort,
                                DataType dataType, double sampleTime) {
    //Going to leverage setters & getters to take advantage of logic of adding the arc to the ports of the nodes
    //Since shared_from_this is required for these functions, a blank arc is created first.
    std::shared_ptr<PartitionCrossing> arc = std::shared_ptr<PartitionCrossing>(new PartitionCrossing());

    //Set params of arc
    arc->setDataType(dataType);
    arc->setSampleTime(sampleTime);

    //Connect arc
    arc->setSrcPortUpdateNewUpdatePrev(srcPort);
    arc->setDstPortUpdateNewUpdatePrev(dstPort);

    return arc;
}

xercesc::DOMElement *PartitionCrossing::emitGraphML(xercesc::DOMDocument *doc, xercesc::DOMElement *graphNode) {
    xercesc::DOMElement *arcXMLNode = Arc::emitGraphML(doc, graphNode);

    //Add the Block Size and Initial State Parameters
    GraphMLHelper::addDataNode(doc, arcXMLNode, "partition_crossing_init_state_count_blocks", GeneralHelper::to_string(initStateCountBlocks));

    return arcXMLNode;
}

std::string PartitionCrossing::labelStr() {
    std::string label = "Init State Count Blocks: " + GeneralHelper::to_string(initStateCountBlocks);

    return label;
}
