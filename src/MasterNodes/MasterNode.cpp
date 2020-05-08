//
// Created by Christopher Yarp on 7/11/18.
//

#include "MasterNode.h"
#include "GraphMLTools/GraphMLHelper.h"
#include "GraphMLTools/GraphMLExporter.h"
#include "General/GeneralHelper.h"
#include "General/ErrorHelpers.h"

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

std::string MasterNode::typeNameStr(){
    return "Master";
}

std::string MasterNode::labelStr() {
    std::string label = Node::labelStr();

    label += "\nType: " + typeNameStr();

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

int MasterNode::getBlockSize() const {
    return blockSize;
}

void MasterNode::setBlockSize(int blockSize) {
    MasterNode::blockSize = blockSize;
}

const std::string &MasterNode::getIndVarName() const {
    return indVarName;
}

void MasterNode::setIndVarName(const std::string &indVarName) {
    MasterNode::indVarName = indVarName;
}

std::map<std::shared_ptr<Port>, std::shared_ptr<ClockDomain>> MasterNode::getIoClockDomains() const {
    return ioClockDomains;
}

void
MasterNode::setIoClockDomains(const std::map<std::shared_ptr<Port>, std::shared_ptr<ClockDomain>> &ioClockDomains) {
    MasterNode::ioClockDomains = ioClockDomains;
}

void MasterNode::setPortClkDomain(std::shared_ptr<OutputPort> port, std::shared_ptr<ClockDomain> clkDomain) {
    //Since this is in the base class, we do not know if this is a Master Input or Master Output
    //so, we will check both.  One should be empty so the check should be fast
    std::vector<std::shared_ptr<OutputPort>> outPorts = getOutputPorts();
    bool isOutputNode = std::find(outPorts.begin(), outPorts.end(), port) != outPorts.end();

    if(!isOutputNode){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Requested setting the Clock Domain of a Port that is not associated with this Master node", getSharedPointer()));
    }

    ioClockDomains[port] = clkDomain;
}

void MasterNode::setPortClkDomain(std::shared_ptr<InputPort> port, std::shared_ptr<ClockDomain> clkDomain) {
    //Since this is in the base class, we do not know if this is a Master Input or Master Output
    //so, we will check both.  One should be empty so the check should be fast
    std::vector<std::shared_ptr<InputPort>> inPorts = getInputPorts();
    bool isInputNode = std::find(inPorts.begin(), inPorts.end(), port) != inPorts.end();

    if(!isInputNode){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Requested setting the Clock Domain of a Port that is not associated with this Master node", getSharedPointer()));
    }

    ioClockDomains[port] = clkDomain;
}

std::shared_ptr<ClockDomain> MasterNode::getPortClkDomain(std::shared_ptr<OutputPort> port){
    //Since this is in the base class, we do not know if this is a Master Input or Master Output
    //so, we will check both.  One should be empty so the check should be fast
    std::vector<std::shared_ptr<OutputPort>> outPorts = getOutputPorts();
    bool isOutputNode = std::find(outPorts.begin(), outPorts.end(), port) != outPorts.end();

    if(!isOutputNode){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Requested setting the Clock Domain of a Port that is not associated with this Master node", getSharedPointer()));
    }

    auto clkDomainIter = ioClockDomains.find(port);
    if(clkDomainIter != ioClockDomains.end()){
        return (*clkDomainIter).second;
    }else{
        return nullptr;
    }
}

std::shared_ptr<ClockDomain> MasterNode::getPortClkDomain(std::shared_ptr<InputPort> port){
    //Since this is in the base class, we do not know if this is a Master Input or Master Output
    //so, we will check both.  One should be empty so the check should be fast
    std::vector<std::shared_ptr<InputPort>> inPorts = getInputPorts();
    bool isInputNode = std::find(inPorts.begin(), inPorts.end(), port) != inPorts.end();

    if(!isInputNode){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Requested setting the Clock Domain of a Port that is not associated with this Master node", getSharedPointer()));
    }

    auto clkDomainIter = ioClockDomains.find(port);
    if(clkDomainIter != ioClockDomains.end()){
        return (*clkDomainIter).second;
    }else{
        return nullptr;
    }
}

void MasterNode::resetIoClockDomains() {
    ioClockDomains.clear();
}