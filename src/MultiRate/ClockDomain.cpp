//
// Created by Christopher Yarp on 4/20/20.
//

#include "ClockDomain.h"
#include "DownsampleClockDomain.h"
#include "General/GeneralHelper.h"
#include "General/ErrorHelpers.h"
#include "MultiRateHelpers.h"
#include "RateChange.h"
#include "MasterNodes/MasterInput.h"
#include "MasterNodes/MasterOutput.h"
#include "MasterNodes/MasterUnconnected.h"
#include "GraphMLTools/GraphMLHelper.h"
#include "GraphCore/NodeFactory.h"
#include "GraphCore/DummyReplica.h"
#include "General/EmitterHelpers.h"
#include "Blocking/BlockingInput.h"
#include "Blocking/BlockingOutput.h"

#include <iostream>

ClockDomain::ClockDomain() : useVectorSamplingMode(false) {
}

ClockDomain::ClockDomain(std::shared_ptr<SubSystem> parent) : SubSystem(parent), useVectorSamplingMode(false) {
}

ClockDomain::ClockDomain(std::shared_ptr<SubSystem> parent, ClockDomain* orig) : SubSystem(parent, orig), upsampleRatio(orig->upsampleRatio), downsampleRatio(orig->downsampleRatio), useVectorSamplingMode(orig->useVectorSamplingMode) {
    //Do not copy the pointers to the RateChange nodes.  This is handled by the shallowCloneWithChildren
}

void ClockDomain::populateParametersExceptRateChangeNodes(std::shared_ptr<ClockDomain> orig) {
    name = orig->getName();
    partitionNum = orig->getPartitionNum();
    schedOrder = orig->getSchedOrder();
    upsampleRatio = orig->getUpsampleRatio();
    downsampleRatio = orig->getDownsampleRatio();
    useVectorSamplingMode = orig->isUsingVectorSamplingMode();

    ioInput = orig->getIoInput();
    ioOutput = orig->getIoOutput();

    //Do not copy these:
    //    std::set<std::shared_ptr<RateChange>> rateChangeIn;
    //    std::set<std::shared_ptr<RateChange>> rateChangeOut;
}

int ClockDomain::getUpsampleRatio() const {
    return upsampleRatio;
}

void ClockDomain::setUpsampleRatio(int upsampleRatio) {
    ClockDomain::upsampleRatio = upsampleRatio;
}

int ClockDomain::getDownsampleRatio() const {
    return downsampleRatio;
}

void ClockDomain::setDownsampleRatio(int downsampleRatio) {
    ClockDomain::downsampleRatio = downsampleRatio;
}

 std::set<std::shared_ptr<RateChange>> ClockDomain::getRateChangeIn() const {
    return rateChangeIn;
}

void ClockDomain::setRateChangeIn(const std::set<std::shared_ptr<RateChange>> &rateChangeIn) {
    ClockDomain::rateChangeIn = rateChangeIn;
}

std::set<std::shared_ptr<RateChange>> ClockDomain::getRateChangeOut() const {
    return rateChangeOut;
}

void ClockDomain::setRateChangeOut(const std::set<std::shared_ptr<RateChange>> &rateChangeOut) {
    ClockDomain::rateChangeOut = rateChangeOut;
}

std::set<std::shared_ptr<OutputPort>> ClockDomain::getIoInput() const {
    return ioInput;
}

void ClockDomain::setIoInput(const std::set<std::shared_ptr<OutputPort>> &ioInput) {
    ClockDomain::ioInput = ioInput;
}

std::set<std::shared_ptr<InputPort>> ClockDomain::getIoOutput() const {
    return ioOutput;
}

void ClockDomain::setIoOutput(const std::set<std::shared_ptr<InputPort>> &ioOutput) {
    ClockDomain::ioOutput = ioOutput;
}

void ClockDomain::addRateChangeIn(std::shared_ptr<RateChange> rateChange) {
    rateChangeIn.insert(rateChange);
}

void ClockDomain::addRateChangeOut(std::shared_ptr<RateChange> rateChange) {
    rateChangeOut.insert(rateChange);
}

void ClockDomain::addIOInput(std::shared_ptr<OutputPort> input) {
    ioInput.insert(input);
}

void ClockDomain::addIOOutput(std::shared_ptr<InputPort> output) {
    ioOutput.insert(output);
}

void ClockDomain::addIOBlockingInput(std::shared_ptr<BlockingInput> input) {
    ioBlockingInput.insert(input);
}

void ClockDomain::addIOBlockingOutput(std::shared_ptr<BlockingOutput> output) {
    ioBlockingOutput.insert(output);
}

void ClockDomain::removeRateChangeIn(std::shared_ptr<RateChange> rateChange) {
    rateChangeIn.erase(rateChange);
}

void ClockDomain::removeRateChangeOut(std::shared_ptr<RateChange> rateChange) {
    rateChangeOut.erase(rateChange);
}

void ClockDomain::removeRateChange(std::shared_ptr<RateChange> rateChange) {
    removeRateChangeIn(rateChange);
    removeRateChangeOut(rateChange);
}

void ClockDomain::removeIOInput(std::shared_ptr<OutputPort> input) {
    ioInput.erase(input);
}

void ClockDomain::removeIOOutput(std::shared_ptr<InputPort> output) {
    ioOutput.erase(output);
}

void ClockDomain::validate() {
    Node::validate();

    //Check that the ioInputPorts are from an InputMaster
    for(auto it = ioInput.begin(); it != ioInput.end(); it++){
        if(GeneralHelper::isType<Node, MasterInput>((*it)->getParent()) == nullptr){
            throw std::runtime_error(ErrorHelpers::genErrorStr("Input to Clock Domain is not from a MasterInput", getSharedPointer()));
        }
    }

    //Check that the ioInputPorts are from an OutputMaster
    for(auto it = ioOutput.begin(); it != ioOutput.end(); it++){
        if(GeneralHelper::isType<Node, MasterOutput>((*it)->getParent()) == nullptr){
            throw std::runtime_error(ErrorHelpers::genErrorStr("Output from Clock Domain is not to a MasterOutput", getSharedPointer()));
        }
    }

    //Get nodes at this level but not within enabled subsystems or clock domains
    std::set<std::shared_ptr<Node>> nodesInDomain = MultiRateHelpers::getNodesInClockDomainHelper(children);

    //Check that all rateChangeNodes are accounted for and that there are no extra ones.
    std::set<std::shared_ptr<RateChange>> rateChangeNodesInDomain;
    for(auto it = nodesInDomain.begin(); it != nodesInDomain.end(); it++){
        if(GeneralHelper::isType<Node, RateChange>(*it)){
            std::shared_ptr<RateChange> asRateChange = std::static_pointer_cast<RateChange>(*it);
            rateChangeNodesInDomain.insert(asRateChange);
        }
    }

    if(rateChangeNodesInDomain.size() != rateChangeIn.size() + rateChangeOut.size()){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Number of Rate Change Nodes in Clock Domain does not match number of input and output rate change nodes", getSharedPointer()));
    }

    for(auto it = rateChangeNodesInDomain.begin(); it != rateChangeNodesInDomain.end(); it++){
        if(rateChangeIn.find(*it) == rateChangeIn.end() && rateChangeOut.find(*it) == rateChangeOut.end()){
            throw std::runtime_error(ErrorHelpers::genErrorStr("Found a rate change node in a clock domain that is not in the input or output rate change node lists", getSharedPointer()));
        }
    }

    //Check that all arcs into/out of the clock domain have rate change nodes or are connected to I/O
        //Equivalently, check that all nodes in the domain are connected to other nodes in the domain or are
        //connected to inputs or outputs or rate change nodes.

    std::set<std::shared_ptr<Arc>> checkedArcs; //To avoid processing inner arcs twice, keep track of arcs we have already checked.
    std::set<std::shared_ptr<Port>> ioInputPortsFound;
    std::set<std::shared_ptr<Port>> ioOutputPortsFound;
    for(auto node = nodesInDomain.begin(); node != nodesInDomain.end(); node++){
        //Don't check the rate change nodes since they follow other rules.
        if(GeneralHelper::isType<Node, RateChange>(*node) == nullptr) {

            //Check the input arcs of the node
            std::set<std::shared_ptr<Arc>> inputArcs = (*node)->getDirectInputArcs();
            for (auto arc = inputArcs.begin(); arc != inputArcs.end(); arc++) {

                if(checkedArcs.find(*arc) == checkedArcs.end()){ //Don't double check
                    checkedArcs.insert(*arc);

                    //Check if the src node is in the same clock domain
                    std::shared_ptr<Node> src = (*arc)->getSrcPort()->getParent();

                    if(GeneralHelper::isType<Node, MasterInput>(src)){
                        //Check if the input port is in the input port list
                        std::shared_ptr<OutputPort> srcPort = (*arc)->getSrcPort();
                        if(ioInput.find(srcPort) != ioInput.end()){
                            //This is one of the io ports
                            ioInputPortsFound.insert(srcPort);
                        }else{
                            //This is not one of the io ports
                            throw std::runtime_error(ErrorHelpers::genErrorStr(
                                    "Found a node in clock domain " + getFullyQualifiedName() + ", " + (*node)->getFullyQualifiedName()
                                    + ", that is connected to a MasterInput but which is not in the ioInput set", getSharedPointer()));
                        }
                    }else if(GeneralHelper::isType<Node, RateChange>(src)) {
                        std::shared_ptr<ClockDomain> srcDomain = MultiRateHelpers::findClockDomain(src);

                        //It is OK if the source is a rate change node in the same clock domain or one level below (an output)
                        std::shared_ptr<ClockDomain> selfAsClkDomain = std::static_pointer_cast<ClockDomain>(getSharedPointer());
                        if (!MultiRateHelpers::isClkDomainOrOneLvlNested(srcDomain, selfAsClkDomain)) {
                            throw std::runtime_error(ErrorHelpers::genErrorStr(
                                    "Found a node in clock domain " + getFullyQualifiedName() + ", " + (*node)->getFullyQualifiedName() +
                                    ", that is connected to a rate change node that is not in the current domain and is not from one nested domain: "
                                    + src->getFullyQualifiedName(), getSharedPointer()));
                        }
                    }else if(GeneralHelper::isType<Node, MasterUnconnected>(src)) {
                        //Connections to the MasterUnconnected are not checked as they have special semantics
                    }else if(GeneralHelper::isType<Node, BlockingInput>(src) != nullptr && GeneralHelper::contains(std::dynamic_pointer_cast<BlockingInput>(src), ioBlockingInput)){
                        //Not checked Since this is to/from I/O
                    }else{
                        //The src is another type of node (not a MasterInput or a Rate Change Node
                        std::shared_ptr<ClockDomain> srcDomain = MultiRateHelpers::findClockDomain(src);

                        if(srcDomain != getSharedPointer()){
                            //The source should be in the same clock domain as this node
                            throw std::runtime_error(ErrorHelpers::genErrorStr(
                                    "Found a node in clock domain " + getFullyQualifiedName() + ", " + (*node)->getFullyQualifiedName()
                                    + ", that is connected to a node not in the same clock domain: " + src->getFullyQualifiedName(), getSharedPointer()));
                        }
                    }
                }
            }

            //Check the output arcs of the node
            std::set<std::shared_ptr<Arc>> outputArcs = (*node)->getDirectOutputArcs();
            for (auto arc = outputArcs.begin(); arc != outputArcs.end(); arc++) {

                if(checkedArcs.find(*arc) == checkedArcs.end()){ //Don't double check
                    checkedArcs.insert(*arc);

                    //Check if the dst node is in the same clock domain
                    std::shared_ptr<Node> dst = (*arc)->getDstPort()->getParent();

                    if(GeneralHelper::isType<Node, MasterOutput>(dst)){
                        //Check if the output port is in the output port list
                        std::shared_ptr<InputPort> dstPort = (*arc)->getDstPort();
                        if(ioOutput.find(dstPort) != ioOutput.end()){
                            //This is one of the io ports
                            ioOutputPortsFound.insert(dstPort);
                        }else{
                            //This is not one of the io ports
                            throw std::runtime_error(ErrorHelpers::genErrorStr(
                                    "Found a node in clock domain " + getFullyQualifiedName() + ", " + (*node)->getFullyQualifiedName()
                                    + ", that is connected to a MasterOutput but which is not in the ioOuput set", getSharedPointer()));
                        }
                    }else if(GeneralHelper::isType<Node, RateChange>(dst)){
                        std::shared_ptr<ClockDomain> dstDomain = MultiRateHelpers::findClockDomain(dst);

                        //It is OK if the dst node is a rate change node in the current domain or one level below
                        std::shared_ptr<ClockDomain> selfAsClkDomain = std::static_pointer_cast<ClockDomain>(getSharedPointer());
                        if(!MultiRateHelpers::isClkDomainOrOneLvlNested(dstDomain, selfAsClkDomain)){
                            throw std::runtime_error(ErrorHelpers::genErrorStr(
                                    "Found a node in clock domain " + getFullyQualifiedName() + ", " + (*node)->getFullyQualifiedName()
                                    + ", that is connected to a rate change node that is not in the current domain and is not from one nested domain: "
                                    + dst->getFullyQualifiedName(), getSharedPointer()));
                        }
                    }else if(GeneralHelper::isType<Node, MasterUnconnected>(dst)){
                        //Connections to the MasterUnconnected are not checked as they have special semantics
                    }else if(GeneralHelper::isType<Node, BlockingOutput>(dst) != nullptr && GeneralHelper::contains(std::dynamic_pointer_cast<BlockingOutput>(dst), ioBlockingOutput)){
                        //Not checked Since this is to/from I/O
                    }else{
                        std::shared_ptr<ClockDomain> dstDomain = MultiRateHelpers::findClockDomain(dst);

                        //The destination is a standard node.  Should have the same clock domain
                        if(dstDomain != getSharedPointer()){
                            throw std::runtime_error(ErrorHelpers::genErrorStr(
                                    "Found a node in clock domain " + getFullyQualifiedName() + ", " + (*node)->getFullyQualifiedName()
                                    + ", that is connected to a node not in the same clock domain: " + dst->getFullyQualifiedName(), getSharedPointer()));
                        }
                    }
                }
            }
        }
    }

    //Get the IO ports connected to rate change nodes
    for(auto it = rateChangeNodesInDomain.begin(); it != rateChangeNodesInDomain.end(); it++){
        //Check for Input IO
        std::set<std::shared_ptr<Arc>> inputArcs = (*it)->getDirectInputArcs();
        for (auto arc = inputArcs.begin(); arc != inputArcs.end(); arc++) {
            std::shared_ptr<Node> src = (*arc)->getSrcPort()->getParent();

            if (GeneralHelper::isType<Node, MasterInput>(src)) {
                //Check if the input port is in the input port list
                std::shared_ptr<OutputPort> srcPort = (*arc)->getSrcPort();
                if (ioInput.find(srcPort) != ioInput.end()) {
                    //This is one of the io ports
                    ioInputPortsFound.insert(srcPort);
                } else {
                    //This is not one of the io ports
                    throw std::runtime_error(ErrorHelpers::genErrorStr(
                            "Found a node in clock domain " + getFullyQualifiedName() + ", " +
                            (*it)->getFullyQualifiedName()
                            + ", that is connected to a MasterInput but which is not in the ioInput set",
                            getSharedPointer()));
                }
            }
        }

        //Check for Output IO
        std::set<std::shared_ptr<Arc>> outputArcs = (*it)->getDirectOutputArcs();
        for (auto arc = outputArcs.begin(); arc != outputArcs.end(); arc++) {
            std::shared_ptr<Node> dst = (*arc)->getDstPort()->getParent();

            if (GeneralHelper::isType<Node, MasterOutput>(dst)) {
                //Check if the output port is in the output port list
                std::shared_ptr<InputPort> dstPort = (*arc)->getDstPort();
                if (ioOutput.find(dstPort) != ioOutput.end()) {
                    //This is one of the io ports
                    ioOutputPortsFound.insert(dstPort);
                } else {
                    //This is not one of the io ports
                    throw std::runtime_error(ErrorHelpers::genErrorStr(
                            "Found a node in clock domain " + getFullyQualifiedName() + ", " +
                            (*it)->getFullyQualifiedName()
                            + ", that is connected to a MasterOutput but which is not in the ioOutput set",
                            getSharedPointer()));
                }
            }
        }
    }

    //Check input ports found size matches that of the member variable.  Note that things put into the portsFound list were already check against
    if(ioInputPortsFound.size() != ioInput.size()){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Number of MasterInputPorts found to be connected to nodes in the ClockDomain does"
                                                           " not match the number in ioInputPorts", getSharedPointer()));
    }

    //Check output ports found
    if(ioOutputPortsFound.size() != ioOutput.size()){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Number of MasterInputPorts found to be connected to nodes in the ClockDomain does"
                                                           " not match the number in ioInputPorts", getSharedPointer()));
    }

    //Check that all input rate change blocks have the appropriate rate changes
    for(auto it = rateChangeIn.begin(); it != rateChangeIn.end(); it++){
        std::pair<int, int> rateChangeOfNode = (*it)->getRateChangeRatio();
        if(rateChangeOfNode.first != upsampleRatio || rateChangeOfNode.second != downsampleRatio){
            throw std::runtime_error(ErrorHelpers::genErrorStr("Found a rate change node that does not have the expected rate change: " + (*it)->getFullyQualifiedName(), getSharedPointer()));
        }

        if((*it)->isUsingVectorSamplingMode() != useVectorSamplingMode){
            throw std::runtime_error(ErrorHelpers::genErrorStr("Found a rate change node that does not have the expected useVectorSamplingMode", getSharedPointer()));
        }
    }

    //Check that all output rate change blocks have the appropriate rate changes
    for(auto it = rateChangeOut.begin(); it != rateChangeOut.end(); it++){
        std::pair<int, int> rateChangeOfNode = (*it)->getRateChangeRatio();
        //Note, the rate change of the output is the inverse of the rate change of the input (and clock domain)
        if(rateChangeOfNode.second != upsampleRatio || rateChangeOfNode.first != downsampleRatio){
            throw std::runtime_error(ErrorHelpers::genErrorStr("Found a rate change node that does not have the expected rate change: " + (*it)->getFullyQualifiedName(), getSharedPointer()));
        }

        if((*it)->isUsingVectorSamplingMode() != useVectorSamplingMode){
            throw std::runtime_error(ErrorHelpers::genErrorStr("Found a rate change node that does not have the expected useVectorSamplingMode", getSharedPointer()));
        }
    }
}

void ClockDomain::discoverClockDomainParameters() {
    //Reset the parameters to allow re-discovery to occur
    upsampleRatio = 1;
    downsampleRatio = 1;
    rateChangeIn.clear();
    rateChangeOut.clear();
    ioInput.clear();
    ioOutput.clear();

    std::shared_ptr<ClockDomain> thisAsClkDomain = std::static_pointer_cast<ClockDomain>(getSharedPointer());
    //Find all the nodes under this clock domain

    //Get nodes at this level but not within enabled subsystems or clock domains
    std::set<std::shared_ptr<Node>> nodesInDomain = MultiRateHelpers::getNodesInClockDomainHelper(children);

    //Filter into rate change nodes and other nodes
    std::set<std::shared_ptr<RateChange>> rateChangeNodesInDomain;
    std::set<std::shared_ptr<Node>> otherNodesInDomain;
    for(auto it = nodesInDomain.begin(); it != nodesInDomain.end(); it++){
        if(GeneralHelper::isType<Node, RateChange>(*it)){
            std::shared_ptr<RateChange> asRateChange = std::static_pointer_cast<RateChange>(*it);
            rateChangeNodesInDomain.insert(asRateChange);
        }else{
            otherNodesInDomain.insert(*it);
        }
    }

    //Sort the Rate Change Nodes into Inputs and Outputs
    //Also associate Master ports connected to rate change nodes
    for(auto rcNode = rateChangeNodesInDomain.begin(); rcNode != rateChangeNodesInDomain.end(); rcNode++){
        //It is possible for input rate change nodes to have inputs from a clock domain that is not in the outer clock
        //domain.  This can occure when the input to the RateChange comes from a RateChangeOutput of another clock domain.
        //It should be the case that the input to the RateChange input should be from outside the clock domain.  The
        //output of the input rate change node should be to a node in the clock domain or to a rate change node input
        //of a clock domain nested one level down.  Note that master nodes can also be connected to rate change inputs
        //If that is the case, the Master input is at the rate of the outer clock domain.  An Output Master can also be
        //connected to the output of the rate change block, in this case, the

        //The reverse is true for RateChange outputs.  The output should be outside of the current domain.  The input
        //may be from a node in the domain or a rate change node from one level down.  The output can also be an OutputMaster
        //in which case, that OutputMaster port should have its clock domain set to the outer domain

        std::set<std::shared_ptr<Arc>> srcArcs = (*rcNode)->getDirectInputArcs();
        std::set<std::shared_ptr<Arc>> dstArcs = (*rcNode)->getDirectOutputArcs();

        useVectorSamplingMode = (*rcNode)->isUsingVectorSamplingMode();

        if(srcArcs.size() == 0){
            throw std::runtime_error(ErrorHelpers::genErrorStr("Cannot determine if rate change node is input or output due to having no source arc", *rcNode));
        }

        //Will determine if the rate change node is an input or output based on the input arc.  The source of a rate
        //change node is not allowed to be the unconnected master (can possibly change this later)

        std::shared_ptr<OutputPort> srcPort = (*srcArcs.begin())->getSrcPort();
        std::shared_ptr<Node> srcNode = srcPort->getParent();

        if(GeneralHelper::isType<Node, MasterUnconnected>(srcNode)){
            throw std::runtime_error(ErrorHelpers::genErrorStr("Cannot determine if rate change node is input or output due to having the source unconnected", *rcNode));
        }

        std::shared_ptr<ClockDomain> srcDomain = MultiRateHelpers::findClockDomain(srcNode);

        //Will be determined to be an input if the the src is outside the clock domain
        if(MultiRateHelpers::isOutsideClkDomain(srcDomain, thisAsClkDomain)){
            //Validate the rcNode is an input and set the IO port rates of IO directly connected to this node
            //Replicates some of the validation but that is Ok for now
            std::pair<std::set<std::shared_ptr<OutputPort>>, std::set<std::shared_ptr<InputPort>>> masterPorts =
                MultiRateHelpers::validateRateChangeInput_SetMasterRates(*rcNode, true);
            ioInput.insert(masterPorts.first.begin(), masterPorts.first.end());
            ioOutput.insert(masterPorts.second.begin(), masterPorts.second.end());
            rateChangeIn.insert(*rcNode);
            //Set the rate of the clock domain (will possibly be set more than once but will be validated later)
            std::pair<int, int> rateRatio = (*rcNode)->getRateChangeRatio();
            upsampleRatio = rateRatio.first;
            downsampleRatio = rateRatio.second;
        }else{
            //Validate the rcNode is an output and set the IO port rates of IO directly connected to this node
            //Replicates some of the validation but that is Ok for now
            std::pair<std::set<std::shared_ptr<OutputPort>>, std::set<std::shared_ptr<InputPort>>> masterPorts =
                MultiRateHelpers::validateRateChangeOutput_SetMasterRates(*rcNode, true);
            ioInput.insert(masterPorts.first.begin(), masterPorts.first.end());
            ioOutput.insert(masterPorts.second.begin(), masterPorts.second.end());
            rateChangeOut.insert(*rcNode);
            //Set the rate of the clock domain (will possibly be set more than once but will be validated later)
            std::pair<int, int> rateRatio = (*rcNode)->getRateChangeRatio();
            //Note, the rate change of the clock domain is the inverse of the output rate change
            upsampleRatio = rateRatio.second;
            downsampleRatio = rateRatio.first;
        }
    }

    //Locate Master Ports Connected to Nodes in Clock Domain
    for(auto it = otherNodesInDomain.begin(); it != otherNodesInDomain.end(); it++){
        //Check input to node
        std::set<std::shared_ptr<Arc>> inputArcs = (*it)->getDirectInputArcs();
        for(auto arc = inputArcs.begin(); arc != inputArcs.end(); arc++){
            std::shared_ptr<OutputPort> srcPort = (*arc)->getSrcPort();
            std::shared_ptr<Node> srcNode = srcPort->getParent();
            if(GeneralHelper::isType<Node, MasterInput>(srcNode)){
                std::shared_ptr<MasterInput> srcNodeAsMasterInput = std::static_pointer_cast<MasterInput>(srcNode);
                srcNodeAsMasterInput->setPortClkDomain(srcPort, thisAsClkDomain);
                ioInput.insert(srcPort);
            }
        }

        //Check outputs from node
        std::set<std::shared_ptr<Arc>> outputArcs = (*it)->getDirectOutputArcs();
        for(auto arc = outputArcs.begin(); arc != outputArcs.end(); arc++){
            std::shared_ptr<InputPort> dstPort = (*arc)->getDstPort();
            std::shared_ptr<Node> dstNode = dstPort->getParent();
            if(GeneralHelper::isType<Node, MasterOutput>(dstNode)){
                std::shared_ptr<MasterOutput> dstNodeAsMasterOutput = std::static_pointer_cast<MasterOutput>(dstNode);
                dstNodeAsMasterOutput->setPortClkDomain(dstPort, thisAsClkDomain);
                ioOutput.insert(dstPort);
            }
        }
    }
}

std::pair<int, int> ClockDomain::getRateRelativeToBase() {
    int numerator = upsampleRatio;
    int denominator = downsampleRatio;

    std::shared_ptr<ClockDomain> thisAsClockDomain = std::static_pointer_cast<ClockDomain>(getSharedPointer());
    std::shared_ptr<ClockDomain> cursor = MultiRateHelpers::findClockDomain(thisAsClockDomain);

    while(cursor != nullptr){
        numerator *= cursor->getUpsampleRatio();
        denominator *= cursor->getDownsampleRatio();

        cursor = MultiRateHelpers::findClockDomain(cursor);
    }

    int gcd = GeneralHelper::gcd(numerator, denominator); //For simplifying fraction

    return std::pair<int, int>(numerator/gcd, denominator/gcd);
}

std::string ClockDomain::typeNameStr(){
    return "ClockDomain";
}

xercesc::DOMElement *
ClockDomain::emitGraphML(xercesc::DOMDocument *doc, xercesc::DOMElement *graphNode, bool include_block_node_type) {
    //Performs the same function as the Subsystem emit GraphML except that the block type is changed

    //Emit the basic info for this node
    xercesc::DOMElement *thisNode = emitGraphMLBasics(doc, graphNode);

    if (include_block_node_type){
        GraphMLHelper::addDataNode(doc, thisNode, "block_node_type", "ClockDomain");
    }

    emitGramphMLSubgraphAndChildren(doc, thisNode);

    return thisNode;
}

std::shared_ptr<Node> ClockDomain::shallowClone(std::shared_ptr<SubSystem> parent) {
    return NodeFactory::shallowCloneNode<ClockDomain>(parent, this);
}

void ClockDomain::shallowCloneWithChildren(std::shared_ptr<SubSystem> parent,
                                                std::vector<std::shared_ptr<Node>> &nodeCopies,
                                                std::map<std::shared_ptr<Node>, std::shared_ptr<Node>> &origToCopyNode,
                                                std::map<std::shared_ptr<Node>, std::shared_ptr<Node>> &copyToOrigNode) {
    //Copy this node
    std::shared_ptr<ClockDomain> clonedNode = std::dynamic_pointer_cast<ClockDomain>(shallowClone(parent));

    ClockDomain::shallowCloneWithChildrenWork(clonedNode, nodeCopies, origToCopyNode, copyToOrigNode);
}

void ClockDomain::shallowCloneWithChildrenWork(std::shared_ptr<ClockDomain> clonedNode,
                                               std::vector<std::shared_ptr<Node>> &nodeCopies,
                                               std::map<std::shared_ptr<Node>, std::shared_ptr<Node>> &origToCopyNode,
                                               std::map<std::shared_ptr<Node>, std::shared_ptr<Node>> &copyToOrigNode) {
    //Put into vectors and maps
    nodeCopies.push_back(clonedNode);
    origToCopyNode[shared_from_this()] = clonedNode;
    copyToOrigNode[clonedNode] = shared_from_this();

    //Copy children, including the rate change nodes
    for(auto it = children.begin(); it != children.end(); it++){
        //Recursive call to this function
        (*it)->shallowCloneWithChildren(clonedNode, nodeCopies, origToCopyNode, copyToOrigNode); //Use the copied node as the parent
    }

    //Run through the rate change nodes and convert the pointers (should be copied by this point)
    for(auto it = rateChangeIn.begin(); it != rateChangeIn.end(); it++){
        std::shared_ptr<RateChange> rateChangeCopy = std::dynamic_pointer_cast<RateChange>(origToCopyNode[*it]);
        clonedNode->addRateChangeIn(rateChangeCopy);
    }

    for(auto it = rateChangeOut.begin(); it != rateChangeOut.end(); it++){
        std::shared_ptr<RateChange> rateChangeCopy = std::dynamic_pointer_cast<RateChange>(origToCopyNode[*it]);
        clonedNode->addRateChangeOut(rateChangeCopy);
    }
}

bool ClockDomain::isSpecialized() {
    return false;
}

std::vector<std::shared_ptr<Node>> ClockDomain::discoverAndMarkContexts(std::vector<Context> contextStack) {

    throw std::runtime_error(ErrorHelpers::genErrorStr("Cannot perform context discovery with un-specialized ClockDomain", getSharedPointer()));

    return std::vector<std::shared_ptr<Node>>();
}

void ClockDomain::createSupportNodes(std::vector<std::shared_ptr<Node>> &nodesToAdd,
                                     std::vector<std::shared_ptr<Node>> &nodesToRemove,
                                     std::vector<std::shared_ptr<Arc>> &arcsToAdd,
                                     std::vector<std::shared_ptr<Arc>> &arcToRemove,
                                     bool includeContext, bool includeOutputBridgeNodes) {
    //This should be implemented in the subclasses
    throw std::runtime_error(ErrorHelpers::genErrorStr("Cannot create support nodes for un-specialized ClockDomain", getSharedPointer()));
}

std::shared_ptr<ClockDomain> ClockDomain::convertToUpsampleDownsampleDomain(bool convertToUpsampleDomain,
                                                                            std::vector<std::shared_ptr<Node>> &nodesToAdd,
                                                                            std::vector<std::shared_ptr<Node>> &nodesToRemove,
                                                                            std::vector<std::shared_ptr<Arc>> &arcsToAdd,
                                                                            std::vector<std::shared_ptr<Arc>> &arcsToRemove) {
    //Note that addRemoveNodesAndArcs will call removeKnownReferences

    std::shared_ptr<ClockDomain> specificClkDomain;

    if(convertToUpsampleDomain){
        //TODO: Implement
        throw std::runtime_error(ErrorHelpers::genErrorStr("UpsampleClockDomains not yet implemented", getSharedPointer()));
    }else{
        //Create
        std::shared_ptr<DownsampleClockDomain> downsampleClockDomain = NodeFactory::createNode<DownsampleClockDomain>(parent);
        downsampleClockDomain->setBaseSubBlockingLen(baseSubBlockingLen);
        nodesToAdd.push_back(downsampleClockDomain);
        specificClkDomain = downsampleClockDomain;

        std::shared_ptr<ClockDomain> thisAsClockDomain = std::static_pointer_cast<ClockDomain>(getSharedPointer());
        downsampleClockDomain->populateParametersExceptRateChangeNodes(thisAsClockDomain);
        EmitterHelpers::transferArcs(thisAsClockDomain, downsampleClockDomain);

        //Move nodes under the new ClockDomain
        //Do this before specializing the RateChange nodes so they have the correct parent
        std::set<std::shared_ptr<Node>> childrenSetCopy = getChildren();
        for(auto child = childrenSetCopy.begin(); child != childrenSetCopy.end(); child++){
            (*child)->setParent(downsampleClockDomain);
            downsampleClockDomain->addChild(*child);
            //Also remove their references from the old parent.  We got a copy of the set first so this is OK
            removeChild(*child);
        }

        //Convert the RateChange inputs and RateChange outputs
        std::set<std::shared_ptr<RateChange>> rateChangeInCopy = getRateChangeIn();
        for(auto rcIn = rateChangeInCopy.begin(); rcIn != rateChangeInCopy.end(); rcIn++){
            if(!(*rcIn)->isSpecialized()) {
                //This function will add the new node
                //It will also re-wire the node
                std::shared_ptr<RateChange> newRcIn = (*rcIn)->convertToRateChangeInputOutput(true, nodesToAdd,
                                                                                              nodesToRemove, arcsToAdd,
                                                                                              arcsToRemove);
                //Add as a rate change input to the new clockDomain
                downsampleClockDomain->addRateChangeIn(newRcIn);

                //Remove the original RateChange node from the original ClockDomain (since it had been moved out from the origional clock domain, the removeKnownReferences function will miss the ptr)
                removeRateChangeIn(*rcIn);

                //rcIn is added to the remove convertToRateChangeInputOutput
            }
        }

        std::set<std::shared_ptr<RateChange>> rateChangeOutCopy = getRateChangeOut();
        for(auto rcOut = rateChangeOutCopy.begin(); rcOut != rateChangeOutCopy.end(); rcOut++){
            if(!(*rcOut)->isSpecialized()) {
                //This function will add the new node
                //It will also re-wire the node
                std::shared_ptr<RateChange> newRcOut = (*rcOut)->convertToRateChangeInputOutput(false, nodesToAdd,
                                                                                                nodesToRemove,
                                                                                                arcsToAdd,
                                                                                                arcsToRemove);
                //Add as a rate change input to the new clockDomain
                downsampleClockDomain->addRateChangeOut(newRcOut);

                //Remove the original RateChange node from the original ClockDomain (since it had been moved out from the origional clock domain, the removeKnownReferences function will miss the ptr)
                removeRateChangeOut(*rcOut);

                //rcOut is added to the remove convertToRateChangeInputOutput
            }
        }

        //Change any I/O Ports in the master nodes to point to the new ClockDomain object
        for(auto ioInputPort = ioInput.begin(); ioInputPort != ioInput.end(); ioInputPort++){
            std::shared_ptr<OutputPort> input = (*ioInputPort);
            std::shared_ptr<MasterInput> inputMaster = GeneralHelper::isType<Node, MasterInput>(input->getParent());
            if(inputMaster == nullptr){
                throw std::runtime_error(ErrorHelpers::genErrorStr("Error when updating an ioInput port's ClockDomain when specializing this ClockDomain.  The referenced input is not from a MasterInput", getSharedPointer()));
            }
            inputMaster->setPortClkDomain(input, downsampleClockDomain);
        }

        for(auto ioOutputPort = ioOutput.begin(); ioOutputPort != ioOutput.end(); ioOutputPort++){
            std::shared_ptr<InputPort> output = (*ioOutputPort);
            std::shared_ptr<MasterOutput> outputMaster = GeneralHelper::isType<Node, MasterOutput>(output->getParent());
            if(outputMaster == nullptr){
                throw std::runtime_error(ErrorHelpers::genErrorStr("Error when updating an ioOutput port's ClockDomain when specializing this ClockDomain.  The referenced input is not from a MasterOutput", getSharedPointer()));
            }
            outputMaster->setPortClkDomain(output, downsampleClockDomain);
        }

        //Mark this node for delete
        nodesToRemove.push_back(getSharedPointer());
    }

    return specificClkDomain;
}

std::shared_ptr<ClockDomain> ClockDomain::specializeClockDomain(std::vector<std::shared_ptr<Node>> &nodesToAdd,
                                                                std::vector<std::shared_ptr<Node>> &nodesToRemove,
                                                                std::vector<std::shared_ptr<Arc>> &arcsToAdd,
                                                                std::vector<std::shared_ptr<Arc>> &arcToRemove){
    if(upsampleRatio != 1 && downsampleRatio != 1){
        throw std::runtime_error(ErrorHelpers::genErrorStr("ClockDomain has both upsample and downsample ratios, cannot convert to a single Upsample or Downsample clock domain", getSharedPointer()));
    }

    if(upsampleRatio != 1){
        return convertToUpsampleDownsampleDomain(true, nodesToAdd, nodesToRemove, arcsToAdd, arcToRemove);
    }else{
        return convertToUpsampleDownsampleDomain(false, nodesToAdd, nodesToRemove, arcsToAdd, arcToRemove);
    }
}

std::set<int> ClockDomain::getSuppressClockDomainLogicForPartitions() const {
    return suppressClockDomainLogicForPartitions;
}

void ClockDomain::setSuppressClockDomainLogicForPartitions(const std::set<int> &suppressClockDomainLogicForPartitions) {
    ClockDomain::suppressClockDomainLogicForPartitions = suppressClockDomainLogicForPartitions;
}

void ClockDomain::addClockDomainLogicSuppressedPartition(int partitionNum) {
    suppressClockDomainLogicForPartitions.insert(partitionNum);
}

void ClockDomain::setClockDomainDriver(std::shared_ptr<Arc>) {

}

bool ClockDomain::isUsingVectorSamplingMode() const {
    return useVectorSamplingMode;
}

void ClockDomain::setUseVectorSamplingMode(bool useVectorSamplingMode) {
    ClockDomain::useVectorSamplingMode = useVectorSamplingMode;
}

void ClockDomain::setUseVectorSamplingModeAndPropagateToRateChangeNodes(bool useVectorSamplingMode,
                                                                        std::set<std::shared_ptr<Node>> &nodesToRemove,
                                                                        std::set<std::shared_ptr<Arc>> &arcsToRemove) {
    ClockDomain::useVectorSamplingMode = useVectorSamplingMode;

    //Update rate change nodes
    for(const std::shared_ptr<RateChange> &rateChangeNode : rateChangeIn){
        rateChangeNode->setUseVectorSamplingMode(useVectorSamplingMode);
    }

    for(const std::shared_ptr<RateChange> &rateChangeNode : rateChangeOut){
        rateChangeNode->setUseVectorSamplingMode(useVectorSamplingMode);
    }

    std::shared_ptr<Arc> clkDomainDriver = getClockDomainDriver();
    if(clkDomainDriver){
        std::shared_ptr<Node> clkDomainSrc = clkDomainDriver->getSrcPort()->getParent();

        //TODO: Change if clock domain drivers which are not the standard wrapping counter are later introduced.
        std::set<std::shared_ptr<Arc>> clkDomainDriverOutArcs = clkDomainSrc->getOutputArcs();
        for(const std::shared_ptr<Arc> &driverOutArc : clkDomainDriverOutArcs) {
            if (driverOutArc->getDstPort()->getParent() != getSharedPointer()) {
                throw std::runtime_error(ErrorHelpers::genErrorStr(
                        "When setting clock domain to use vector mode, expected clock domain driver to be uniquely allocated to clock domain so it can be removed.",
                        getSharedPointer()));
            }
        }
        if(!clkDomainSrc->getInputArcs().empty()){
            throw std::runtime_error(ErrorHelpers::genErrorStr("When setting clock domain to use vector mode, expected clock domain driver to be a single node", getSharedPointer()));
        }

        std::set<std::shared_ptr<Arc>> disconnectArcs = clkDomainSrc->disconnectNode();

        nodesToRemove.insert(clkDomainSrc);
        arcsToRemove.insert(clkDomainDriverOutArcs.begin(), clkDomainDriverOutArcs.end());
        arcsToRemove.insert(disconnectArcs.begin(), disconnectArcs.end());

        setClockDomainDriver(nullptr);
    }

    //Context Driver Replication May have Occurred Before This, remove the replicas if applicable
    std::shared_ptr<ContextRoot> thisAsContextRoot = GeneralHelper::isType<Node, ContextRoot>(getSharedPointer()); //Only if this clock domain is specialized
    if(thisAsContextRoot) {
        std::map<int, std::vector<std::shared_ptr<Arc>>> contextDriversPerPartition = thisAsContextRoot->getContextDriversPerPartition();
        for (const auto &replicatedDriver: contextDriversPerPartition) {
            std::vector<std::shared_ptr<Arc>> drivers = replicatedDriver.second;

            for(const std::shared_ptr<Arc> &driver : drivers){
                if(!GeneralHelper::contains(driver, arcsToRemove)) { //This protects against duplicate removal of an arc
                    std::shared_ptr<Node> clkDomainSrc = driver->getSrcPort()->getParent();

                    //TODO: Change if clock domain drivers which are not the standard wrapping counter are later introduced.
                    std::set<std::shared_ptr<Arc>> clkDomainDriverOutArcs = clkDomainSrc->getOutputArcs();
                    for (const std::shared_ptr<Arc> &driverOutArc: clkDomainDriverOutArcs) {
                        if (driverOutArc->getDstPort()->getParent() != getSharedPointer() &&
                            driverOutArc->getDstPort()->getParent() !=
                            thisAsContextRoot->getDummyReplica(replicatedDriver.first)) {
                            throw std::runtime_error(ErrorHelpers::genErrorStr(
                                    "When setting clock domain to use vector mode, expected clock domain driver to be uniquely allocated to clock domain so it can be removed.",
                                    getSharedPointer()));
                        }
                    }
                    if (!clkDomainSrc->getInputArcs().empty()) {
                        throw std::runtime_error(ErrorHelpers::genErrorStr(
                                "When setting clock domain to use vector mode, expected clock domain driver to be a single node",
                                getSharedPointer()));
                    }

                    std::set<std::shared_ptr<Arc>> disconnectArcs = clkDomainSrc->disconnectNode();

                    nodesToRemove.insert(clkDomainSrc);
                    arcsToRemove.insert(clkDomainDriverOutArcs.begin(), clkDomainDriverOutArcs.end());
                    arcsToRemove.insert(disconnectArcs.begin(), disconnectArcs.end());
                }
            }
        }

        std::map<int, std::vector<std::shared_ptr<Arc>>> emptyContextDriversPerPartition;
        thisAsContextRoot->setContextDriversPerPartition(emptyContextDriversPerPartition);

        //Remove Dummy Nodes

        std::map<int, std::shared_ptr<DummyReplica>> dummyReplicas = thisAsContextRoot->getDummyReplicas();
        for(const auto &dummyReplica : dummyReplicas){
            if(!dummyReplica.second->getInputArcs().empty() || !dummyReplica.second->getOutputArcs().empty()){
                throw std::runtime_error(ErrorHelpers::genErrorStr(
                        "When setting clock domain to use vector mode, expected dummy nodes to be disconnected",
                        getSharedPointer()));
            }

        }

        std::map<int, std::shared_ptr<DummyReplica>> emptyDummyReplicas;
        thisAsContextRoot->setDummyReplicas(emptyDummyReplicas);
    }


}

void ClockDomain::resetIOPorts(){
    ioInput.clear();
    ioOutput.clear();
}

std::shared_ptr <Arc> ClockDomain::getClockDomainDriver() {
    return nullptr;
}

bool ClockDomain::requiresDeclaringExecutionCount() {
    return !isUsingVectorSamplingMode();
}

Variable ClockDomain::getExecutionCountVariable(int blockSizeBase) {
    //This variable needs to count the number of iterations of this clock domain
    std::pair<int, int> rateRelToBase = getRateRelativeToBase();

    double iterationsPerBlock = std::ceil(blockSizeBase*rateRelToBase.first/((double) rateRelToBase.second));
    int requiredBits = GeneralHelper::numIntegerBits(iterationsPerBlock, false);
    DataType dt(false, false, false, requiredBits, 0, {1});
    dt = dt.getCPUStorageType();

    std::string varName = "ClkDomainCount_n" + GeneralHelper::to_string(id);

    NumericValue initVal((long int) 0);

    return Variable(varName, dt, {initVal});
}

std::string ClockDomain::getExecutionCountVariableName() {
    std::string varName = "ClkDomainCount_n" + GeneralHelper::to_string(id);

    NumericValue initVal((long int) 0);

    return Variable(varName, DataType()).getCVarName();
}

std::set<std::shared_ptr<BlockingInput>> ClockDomain::getIoBlockingInput() const {
    return ioBlockingInput;
}

void ClockDomain::setIoBlockingInput(const std::set<std::shared_ptr<BlockingInput>> &ioBlockingInput) {
    ClockDomain::ioBlockingInput = ioBlockingInput;
}

std::set<std::shared_ptr<BlockingOutput>> ClockDomain::getIoBlockingOutput() const {
    return ioBlockingOutput;
}

void ClockDomain::setIoBlockingOutput(const std::set<std::shared_ptr<BlockingOutput>> &ioBlockingOutput) {
    ClockDomain::ioBlockingOutput = ioBlockingOutput;
}
