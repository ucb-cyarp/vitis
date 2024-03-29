//
// Created by Christopher Yarp on 6/26/18.
//

#include "Arc.h"
#include "Port.h"
#include "InputPort.h"
#include "OutputPort.h"
#include "EnablePort.h"
#include "SelectPort.h"
#include "Node.h"
#include "EnableNode.h"

#include "GraphMLTools/GraphMLHelper.h"
#include "General/GeneralHelper.h"

#include "PrimitiveNodes/Mux.h"

#include "General/ErrorHelpers.h"

Arc::Arc() : id(-1), sampleTime(-1), delay(0), slack(0){
    srcPort = std::shared_ptr<OutputPort>(nullptr);
    dstPort = std::shared_ptr<InputPort>(nullptr);
}

Arc::Arc(std::shared_ptr<OutputPort> srcPort, std::shared_ptr<InputPort> dstPort, DataType dataType, double sampleTime) : id(-1), srcPort(srcPort), dstPort(dstPort), dataType(dataType), sampleTime(sampleTime), delay(0), slack(0){

}

Arc::~Arc() {
    if (srcPort != nullptr) {
        srcPort->removeArc(weakSelf);
    }

    if (dstPort != nullptr) {
        dstPort->removeArc(weakSelf);
    }
}

std::shared_ptr<OutputPort> Arc::getSrcPort() const {
    return srcPort;
}

void Arc::setSrcPort(const std::shared_ptr<OutputPort> &srcPort) {
    Arc::srcPort = srcPort;
}

std::shared_ptr<InputPort> Arc::getDstPort() const {
    return dstPort;
}

void Arc::setDstPort(const std::shared_ptr<InputPort> &dstPort) {
    Arc::dstPort = dstPort;
}

DataType Arc::getDataType() const {
    return dataType;
}

void Arc::setDataType(const DataType &dataType) {
    Arc::dataType = dataType;
}

double Arc::getSampleTime() const {
    return sampleTime;
}

void Arc::setSampleTime(double sampleTime) {
    Arc::sampleTime = sampleTime;
}

int Arc::getDelay() const {
    return delay;
}

void Arc::setDelay(int delay) {
    Arc::delay = delay;
}

int Arc::getSlack() const {
    return slack;
}

void Arc::setSlack(int slack) {
    Arc::slack = slack;
}

void Arc::setSrcPortUpdateNewUpdatePrev(std::shared_ptr<OutputPort> srcPort) {
    //Remove arc from old port if not null
    if (Arc::srcPort != nullptr) {
        Arc::srcPort->removeArc(shared_from_this());
    }

    //Set the src port to the one specified
    Arc::srcPort = srcPort;

    //Add this arc to the new src port
    if (srcPort != nullptr){
        srcPort->addArc(shared_from_this());
    }
}

void Arc::setDstPortUpdateNewUpdatePrev(std::shared_ptr<InputPort> dstPort) {
    //Remove arc from old port if not null
    if(Arc::dstPort != nullptr)
    {
        Arc::dstPort->removeArc(shared_from_this());
    }

    //Set the dst port to the one specified
    Arc::dstPort = dstPort;

    //Add this arc to the new dst port
    if(dstPort != nullptr) {
        dstPort->addArc(shared_from_this());
    }
}

std::shared_ptr<Arc> Arc::createArc() {
    return std::shared_ptr<Arc>(new Arc());
}

std::shared_ptr<Arc>
Arc::connectNodes(std::shared_ptr<Node> src, int srcPortNum, std::shared_ptr<Node> dst, int dstPortNum,
                  DataType dataType, double sampleTime) {

    //Going to leverage setters & getters to take advantage of logic of adding the arc to the ports of the nodes
    //Since shared_from_this is required for these functions, a blank arc is created first.
    std::shared_ptr<Arc> arc = std::shared_ptr<Arc>(new Arc());

    //Set params of arc
    arc->setDataType(dataType);
    arc->setSampleTime(sampleTime);

    //Connect arc
    if(src != nullptr)
    {
        src->addOutArcUpdatePrevUpdateArc(srcPortNum, arc);
    }

    if(dst != nullptr)
    {
        dst->addInArcUpdatePrevUpdateArc(dstPortNum, arc);
    }

    return arc;
}

std::shared_ptr<Arc>
Arc::connectNodes(std::shared_ptr<OutputPort> srcPort, std::shared_ptr<InputPort> dstPort,
                  DataType dataType, double sampleTime){
    //Going to leverage setters & getters to take advantage of logic of adding the arc to the ports of the nodes
    //Since shared_from_this is required for these functions, a blank arc is created first.
    std::shared_ptr<Arc> arc = std::shared_ptr<Arc>(new Arc());

    //Set params of arc
    arc->setDataType(dataType);
    arc->setSampleTime(sampleTime);

    //Connect arc
    arc->setSrcPortUpdateNewUpdatePrev(srcPort);
    arc->setDstPortUpdateNewUpdatePrev(dstPort);

    return arc;
}

std::shared_ptr<Arc>
Arc::connectNodes(std::shared_ptr<Node> src, int srcPortNum, std::shared_ptr<EnableNode> dst, DataType dataType,
                  double sampleTime) {
    //Going to leverage setters & getters to take advantage of logic of adding the arc to the ports of the nodes
    //Since shared_from_this is required for these functions, a blank arc is created first.
    std::shared_ptr<Arc> arc = std::shared_ptr<Arc>(new Arc());

    //Set params of arc
    arc->setDataType(dataType);
    arc->setSampleTime(sampleTime);
    arc->weakSelf = arc; //Store reference to self as weak ptr

    //Connect arc
    if(src != nullptr)
    {
        src->addOutArcUpdatePrevUpdateArc(srcPortNum, arc);
    }

    if(dst != nullptr)
    {
        dst->setEnableArcUpdatePrevUpdateArc(arc);
    }

    return arc;
}

std::shared_ptr<Arc>
Arc::connectNodes(std::shared_ptr<OutputPort> srcPort, std::shared_ptr<EnableNode> dst,
        DataType dataType, double sampleTime){
    //Going to leverage setters & getters to take advantage of logic of adding the arc to the ports of the nodes
    //Since shared_from_this is required for these functions, a blank arc is created first.
    std::shared_ptr<Arc> arc = std::shared_ptr<Arc>(new Arc());

    //Set params of arc
    arc->setDataType(dataType);
    arc->setSampleTime(sampleTime);
    arc->weakSelf = arc; //Store reference to self as weak ptr

    //Connect arc
    arc->setSrcPortUpdateNewUpdatePrev(srcPort);

    if(dst != nullptr)
    {
        dst->setEnableArcUpdatePrevUpdateArc(arc);
    }

    return arc;
}

std::shared_ptr<Arc>
Arc::connectNodes(std::shared_ptr<Node> src, int srcPortNum, std::shared_ptr<Mux> dst, DataType dataType,
                  double sampleTime) {
    //Going to leverage setters & getters to take advantage of logic of adding the arc to the ports of the nodes
    //Since shared_from_this is required for these functions, a blank arc is created first.
    std::shared_ptr<Arc> arc = std::shared_ptr<Arc>(new Arc());

    //Set params of arc
    arc->setDataType(dataType);
    arc->setSampleTime(sampleTime);
    arc->weakSelf = arc; //Store reference to self as weak ptr

    //Connect arc
    if(src != nullptr)
    {
        src->addOutArcUpdatePrevUpdateArc(srcPortNum, arc);
    }

    if(dst != nullptr)
    {
        dst->addSelectArcUpdatePrevUpdateArc(arc);
    }

    return arc;
}

int Arc::getIDFromGraphMLFullPath(std::string fullPath) {
    //Find the location of the last n
    int nIndex = -1;

    int pathLen = (int) fullPath.size(); //Casting to signed to ensure loop termination
    for(int i = pathLen-1; i >= 0; i--){
        if(fullPath[i] == 'e'){
            nIndex = i;
        }
    }

    if(nIndex == -1){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Could not find edge ID in full GraphML path: " + fullPath));
    }

    std::string localIDStr = fullPath.substr(nIndex+1, std::string::npos);

    int localId = std::stoi(localIDStr);

    return localId;
}

int Arc::getId() const {
    return id;
}

void Arc::setId(int id) {
    Arc::id = id;
}

xercesc::DOMElement *Arc::emitGraphML(xercesc::DOMDocument *doc, xercesc::DOMElement *graphNode) {
    //Used CreateDOMDocument.cpp example from Xerces as a guide

    //Will not insert newlines under the assumption that format-pretty-print will be enabled in the serializer

    //---Create the Edge Node---
    xercesc::DOMElement* arcElement = GraphMLHelper::createNode(doc, "edge");
    //Set ID
    GraphMLHelper::setAttribute(arcElement, "id", "e"+GeneralHelper::to_string(id)); //Do not need a full path for the edge
    //Set Src & Dst
    GraphMLHelper::setAttribute(arcElement, "source", srcPort->getParent()->getFullGraphMLPath());
    GraphMLHelper::setAttribute(arcElement, "target", dstPort->getParent()->getFullGraphMLPath());

    //---Add data entries---
    GraphMLHelper::addDataNode(doc, arcElement, "arc_src_port", GeneralHelper::to_string(srcPort->getPortNum()));
    GraphMLHelper::addDataNode(doc, arcElement, "arc_dst_port", GeneralHelper::to_string(dstPort->getPortNum()));
    std::string dstPortType;
    //Check for Specific Input Types
    if(GeneralHelper::isType<InputPort, EnablePort>(dstPort) != nullptr){
        dstPortType = "Enable";
    }else if(GeneralHelper::isType<InputPort, SelectPort>(dstPort) != nullptr){
        dstPortType = "Select";
    }else{
        dstPortType = "Standard";
    }
    GraphMLHelper::addDataNode(doc, arcElement, "arc_dst_port_type", dstPortType);

    //---DataType----
    GraphMLHelper::addDataNode(doc, arcElement, "arc_datatype", dataType.toString());
    std::string complexStr = (dataType.isComplex() ? "true" : "false");
    GraphMLHelper::addDataNode(doc, arcElement, "arc_complex", complexStr);
    GraphMLHelper::addDataNode(doc, arcElement, "arc_dimension", GeneralHelper::vectorToString(dataType.getDimensions()));
    GraphMLHelper::addDataNode(doc, arcElement, "arc_disp_label", labelStr());
    GraphMLHelper::addDataNode(doc, arcElement, "arc_id", GeneralHelper::to_string(getId()));

    //---Add to graph node---
    graphNode->appendChild(arcElement);

    return arcElement;
}

std::string Arc::labelStr() {
    std::string dstPortType;
    if(GeneralHelper::isType<InputPort, EnablePort>(dstPort) != nullptr){
        dstPortType = "Enable";
    }else if(GeneralHelper::isType<InputPort, SelectPort>(dstPort) != nullptr){
        dstPortType = "Select";
    }else{
        dstPortType = "Standard";
    }

    std::string label = "ID: e" + GeneralHelper::to_string(id) +
                        "\nSrc Port: " + GeneralHelper::to_string(srcPort->getPortNum()) +
                        "\nDst Port: " + GeneralHelper::to_string(dstPort->getPortNum()) + " (" + dstPortType + ")"
                        "\nDatatype: " + dataType.toString() + " (" + (dataType.isComplex() ? "Complex" : "Real") + ")" +
                        "\nDimensions: " + GeneralHelper::vectorToString(dataType.getDimensions());

    return label;
}

void Arc::shallowCopyPrameters(Arc *orig) {
    id = orig->id;
    dataType = orig->dataType;
    sampleTime = orig->sampleTime;
    delay = orig->delay;
    slack = orig->slack;

    //do not copy weakSelf as this is set when connecting the nodes
}

std::shared_ptr<Arc>
Arc::connectNodesOrderConstraint(std::shared_ptr<Node> src, int srcPortNum, std::shared_ptr<Node> dst,
                                 DataType dataType, double sampleTime) {
    //Going to leverage setters & getters to take advantage of logic of adding the arc to the ports of the nodes
    //Since shared_from_this is required for these functions, a blank arc is created first.
    std::shared_ptr<Arc> arc = std::shared_ptr<Arc>(new Arc());

    //Set params of arc
    arc->setDataType(dataType);
    arc->setSampleTime(sampleTime);
    arc->weakSelf = arc; //Store reference to self as weak ptr

    //Connect arc
    if(src != nullptr)
    {
        src->addOutArcUpdatePrevUpdateArc(srcPortNum, arc);
    }

    if(dst != nullptr)
    {
        dst->addOrderConstraintInArcUpdatePrevUpdateArc(arc);
    }

    return arc;
}

std::shared_ptr<Arc> Arc::connectNodesOrderConstraint(std::shared_ptr<Node> src, std::shared_ptr<Node> dst,
                                                      DataType dataType, double sampleTime) {
    //Going to leverage setters & getters to take advantage of logic of adding the arc to the ports of the nodes
    //Since shared_from_this is required for these functions, a blank arc is created first.
    std::shared_ptr<Arc> arc = std::shared_ptr<Arc>(new Arc());

    //Set params of arc
    arc->setDataType(dataType);
    arc->setSampleTime(sampleTime);

    //Connect arc
    if(src != nullptr)
    {
        src->addOrderConstraintOutArcUpdatePrevUpdateArc(arc);
    }

    if(dst != nullptr)
    {
        dst->addOrderConstraintInArcUpdatePrevUpdateArc(arc);
    }

    return arc;
}

void Arc::disconnect(){
    setDstPortUpdateNewUpdatePrev(nullptr);
    setSrcPortUpdateNewUpdatePrev(nullptr);
}

bool Arc::PtrID_Compare::operator() (const std::shared_ptr<Arc>& lhs, const std::shared_ptr<Arc>& rhs) const{
    if(lhs->getId() != rhs->getId()){
        return lhs->getId() < rhs->getId();
    }else{
        return lhs<rhs;
    }
}