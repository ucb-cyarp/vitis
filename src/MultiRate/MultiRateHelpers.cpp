//
// Created by Christopher Yarp on 4/22/20.
//

#include "MultiRateHelpers.h"
#include "General/GeneralHelper.h"
#include "General/EmitterHelpers.h"
#include "General/GraphAlgs.h"
#include "General/ErrorHelpers.h"
#include "ClockDomain.h"
#include "GraphCore/Node.h"
#include "GraphCore/ContextFamilyContainer.h"
#include "MasterNodes/MasterInput.h"
#include "MasterNodes/MasterOutput.h"
#include "MasterNodes/MasterUnconnected.h"
#include "RateChange.h"
#include "MultiThread/ThreadCrossingFIFO.h"
#include <iostream>
#include "Downsample.h"
#include "Upsample.h"
#include "Repeat.h"

std::set<std::shared_ptr<Node>>
MultiRateHelpers::getNodesInClockDomainHelper(const std::set<std::shared_ptr<Node>> nodesToSearch) {
    return GraphAlgs::getNodesInDomainHelperFilter<ClockDomain, Node>(nodesToSearch);
}

std::shared_ptr<ClockDomain> MultiRateHelpers::findClockDomain(std::shared_ptr<Node> node) {
    return GraphAlgs::findDomain<ClockDomain>(node);
}

bool MultiRateHelpers::isOutsideClkDomain(std::shared_ptr<ClockDomain> a, std::shared_ptr<ClockDomain> b) {
    return GraphAlgs::isOutsideDomain(a, b);
}

bool MultiRateHelpers::isClkDomainOrOneLvlNested(std::shared_ptr<ClockDomain> a, std::shared_ptr<ClockDomain> b) {
    return GraphAlgs::isDomainOrOneLvlNested(a, b);
}

bool
MultiRateHelpers::areWithinOneClockDomainOfEachOther(std::shared_ptr<ClockDomain> a, std::shared_ptr<ClockDomain> b) {
    return GraphAlgs::areWithinOneDomainOfEachOther(a, b);
}

std::pair<std::set<std::shared_ptr<OutputPort>>, std::set<std::shared_ptr<InputPort>>> MultiRateHelpers::validateRateChangeInput_SetMasterRates(std::shared_ptr<RateChange> rc, bool setMasterRates) {
    std::set<std::shared_ptr<OutputPort>> masterInputPorts;
    std::set<std::shared_ptr<InputPort>> masterOutputPorts;

    std::set<std::shared_ptr<Arc>> inputArcs = rc->getDirectInputArcs();
    std::set<std::shared_ptr<Arc>> outputArcs = rc->getDirectOutputArcs();

    if(inputArcs.size() != 1){
        throw std::runtime_error(ErrorHelpers::genErrorStr("RateChange node should have exactly 1 input arc", rc));
    }

    if(outputArcs.size() < 1){
        throw std::runtime_error(ErrorHelpers::genErrorStr("RateChange node should have exactly >=1 output arcs", rc));
    }

    std::shared_ptr<ClockDomain> rcDomain = findClockDomain(rc);
    std::shared_ptr<OutputPort> srcPort = (*inputArcs.begin())->getSrcPort();
    std::shared_ptr<Node> srcNode = srcPort->getParent();
    std::shared_ptr<ClockDomain> srcDomain = findClockDomain(srcNode);
    std::shared_ptr<ClockDomain> rcOuterClockDomain = findClockDomain(rcDomain);

    if(rc->isUsingVectorSamplingMode()){
        DataType inputDTScaled = rc->getInputPort(0)->getDataType();
        //Scale the outer dimension
        std::vector<int> inputDim = inputDTScaled.getDimensions();
        std::pair<int, int> rateChange = rc->getRateChangeRatio();
        inputDim[0] *= rateChange.first;
        inputDim[0] /= rateChange.second;
        inputDTScaled.setDimensions(inputDim);

        DataType outputDT = rc->getOutputPort(0)->getDataType();

        if(inputDTScaled != outputDT){
            throw std::runtime_error(ErrorHelpers::genErrorStr("When using VectorSamplingMode, output dimensions should be scaled relative to input dimensions", rc));
        }
    }else{
        DataType inputDT = rc->getInputPort(0)->getDataType();
        DataType outputDT = rc->getOutputPort(0)->getDataType();

        if(inputDT != outputDT){
            throw std::runtime_error(ErrorHelpers::genErrorStr("When not using VectorSamplingMode, output dimensions should be scaled relative to input dimensions", rc));
        }
    }

    //Validate input
    {
        if(GeneralHelper::isType<Node, MasterInput>(srcNode)) {
            if(setMasterRates) {
                //Set the clock domain of the MasterInput port to the outer domain
                std::shared_ptr<MasterInput> srcAsMasterInput = std::static_pointer_cast<MasterInput>(srcNode);
                srcAsMasterInput->setPortClkDomain(srcPort, rcOuterClockDomain);
                masterInputPorts.insert(srcPort);
            }
        }else if(GeneralHelper::isType<Node, RateChange>(srcNode)){
            std::shared_ptr<ClockDomain> srcDomainOuter = findClockDomain(srcDomain);
            bool isOutsideAtSameLvl = srcDomain == nullptr || !(srcDomainOuter == rcOuterClockDomain && srcDomain != rcDomain); //The src domain cannot be null because the base domain does not contain rate change nodes.  Need to also make sure that the src node is from outside the clock domain
            bool isInOuterDomain = srcDomain == rcOuterClockDomain;
            if(!isOutsideAtSameLvl && !isInOuterDomain){
                throw std::runtime_error(ErrorHelpers::genErrorStr("RateChange input should come from outside the clock domain or from a rate change node one level below the outer clock domain", rc));
            }
        }else{
            if(srcDomain != rcOuterClockDomain) {
                //Check that the node is in the outer domain
                throw std::runtime_error(ErrorHelpers::genErrorStr("RateChange input should come from outside the clock domain or from a rate change node one level below the outer clock domain", rc));
            }
        }
    }

    //Validate output
    for(auto arc = outputArcs.begin(); arc != outputArcs.end(); arc++){
        std::shared_ptr<InputPort> dstPort = (*arc)->getDstPort();
        std::shared_ptr<Node> dstNode = dstPort->getParent();

        //If it is a master output, it is OK (do not need to check)
        if(!GeneralHelper::isType<Node, MasterOutput>(dstNode)){
            //Don't check unconnected ports
            if(!GeneralHelper::isType<Node, MasterUnconnected>(dstNode)) {
                //Check the clock domain of the destination node
                std::shared_ptr<ClockDomain> outputDomain = findClockDomain(dstNode);
                if (GeneralHelper::isType<Node, RateChange>(dstNode)) {
                    //Check that the clock domain of the output is one level below the current domain
                    std::shared_ptr<ClockDomain> outerOutputDomain = findClockDomain(outputDomain);
                    if (outerOutputDomain != rcDomain) {
                        throw std::runtime_error(ErrorHelpers::genErrorStr(
                                "MultiRate Input node's output arc should be to a node inside the clock domain or to a rate change node in a clock domain one level down",
                                rc));
                    }
                } else {
                    if (outputDomain != rcDomain) {
                        throw std::runtime_error(ErrorHelpers::genErrorStr(
                                "MultiRate Input node's output arc should be to a node inside the clock domain or to a rate change node in a clock domain one level down",
                                rc));
                    }
                }
            }
        }else{
            //This is an OutputMasterNode
            if(setMasterRates){
                //Set the clock domain of the MasterOutput port to the outer domain
                std::shared_ptr<MasterOutput> dstAsMasterOutput = std::static_pointer_cast<MasterOutput>(dstNode);
                std::shared_ptr<ClockDomain> outerClockDomain = findClockDomain(rcDomain);
                dstAsMasterOutput->setPortClkDomain(dstPort, outerClockDomain);
                masterOutputPorts.insert(dstPort);
            }
        }
    }
    return std::pair<std::set<std::shared_ptr<OutputPort>>, std::set<std::shared_ptr<InputPort>>>(masterInputPorts, masterOutputPorts);
}

std::pair<std::set<std::shared_ptr<OutputPort>>, std::set<std::shared_ptr<InputPort>>> MultiRateHelpers::validateRateChangeOutput_SetMasterRates(std::shared_ptr<RateChange> rc, bool setMasterRates) {
    std::set<std::shared_ptr<OutputPort>> masterInputPorts;
    std::set<std::shared_ptr<InputPort>> masterOutputPorts;

    std::set<std::shared_ptr<Arc>> inputArcs = rc->getDirectInputArcs();
    std::set<std::shared_ptr<Arc>> outputArcs = rc->getDirectOutputArcs();

    if(inputArcs.size() != 1){
        throw std::runtime_error(ErrorHelpers::genErrorStr("MultiRate node should have exactly 1 input arc", rc));
    }

    if(outputArcs.size() < 1){
        throw std::runtime_error(ErrorHelpers::genErrorStr("MultiRate node should have exactly >=1 output arcs", rc));
    }

    std::shared_ptr<ClockDomain> rcDomain = findClockDomain(rc);
    std::shared_ptr<OutputPort> srcPort = (*inputArcs.begin())->getSrcPort();
    std::shared_ptr<Node> srcNode = srcPort->getParent();
    std::shared_ptr<ClockDomain> srcDomain = findClockDomain(srcNode);

    if(rc->isUsingVectorSamplingMode()){
        DataType inputDTScaled = rc->getInputPort(0)->getDataType();
        //Scale the outer dimension
        std::vector<int> inputDim = inputDTScaled.getDimensions();
        std::pair<int, int> rateChange = rc->getRateChangeRatio();
        inputDim[0] *= rateChange.first;
        inputDim[0] /= rateChange.second;
        inputDTScaled.setDimensions(inputDim);

        DataType outputDT = rc->getOutputPort(0)->getDataType();

        if(inputDTScaled != outputDT){
            throw std::runtime_error(ErrorHelpers::genErrorStr("When using VectorSamplingMode, output dimensions should be scaled relative to input dimensions", rc));
        }
    }else{
        DataType inputDT = rc->getInputPort(0)->getDataType();
        DataType outputDT = rc->getOutputPort(0)->getDataType();

        if(inputDT != outputDT){
            throw std::runtime_error(ErrorHelpers::genErrorStr("When not using VectorSamplingMode, output dimensions should be scaled relative to input dimensions", rc));
        }
    }

    //Validate input
    //If it is a master input, it is OK (do not need to check)
    if(!GeneralHelper::isType<Node, MasterInput>(srcNode)){
        //Don't check unconnected ports
        if(!GeneralHelper::isType<Node, MasterUnconnected>(srcNode)) {
            //Check the clock domain of the src node
            if (GeneralHelper::isType<Node, RateChange>(srcNode)) {
                //Check that the clock domain of the src is one level below the current domain
                std::shared_ptr<ClockDomain> outerSrcDomain = findClockDomain(srcDomain);
                if (outerSrcDomain != rcDomain) {
                    throw std::runtime_error(ErrorHelpers::genErrorStr(
                            "MultiRate Output node's input arc should be to a node inside the clock domain or to a rate change node in a clock domain one level down",
                            rc));
                }
            } else {
                if (srcDomain != rcDomain) {
                    throw std::runtime_error(ErrorHelpers::genErrorStr(
                            "MultiRate Output node's input arc should be to a node inside the clock domain or to a rate change node in a clock domain one level down",
                            rc));
                }
            }
        }
    }else{
        //This is an InputMaster Node
        if(setMasterRates){
            //Set the clock domain of the MasterInput to the domain of the rate change node
            std::shared_ptr<MasterInput> srcAsMasterInput = std::static_pointer_cast<MasterInput>(srcNode);
            srcAsMasterInput->setPortClkDomain(srcPort, rcDomain);
            masterInputPorts.insert(srcPort);
        }
    }

    //Validate Output
    std::shared_ptr<ClockDomain> rcOuterDomain = findClockDomain(rcDomain);
    for(auto arc = outputArcs.begin(); arc != outputArcs.end(); arc++){
        std::shared_ptr<InputPort> dstPort = (*arc)->getDstPort();
        std::shared_ptr<Node> dstNode = dstPort->getParent();

        if(GeneralHelper::isType<Node, MasterOutput>(dstNode)){
            //Dst is output master
            if(setMasterRates){
                //Set the clock domain of the MasterOutput to the outer domain
                std::shared_ptr<MasterOutput> dstAsMasterOutput = std::static_pointer_cast<MasterOutput>(dstNode);
                dstAsMasterOutput->setPortClkDomain(dstPort, rcOuterDomain);
                masterOutputPorts.insert(dstPort);
            }
        }else if(GeneralHelper::isType<Node, RateChange>(dstNode)){
            //Check if the rate change node is one level below the outer domain
            std::shared_ptr<ClockDomain> dstDomain = findClockDomain(dstNode);
            std::shared_ptr<ClockDomain> dstOuterDomain = findClockDomain(dstDomain);

            bool isOutsideAtSameLvl = dstDomain == nullptr || !(dstOuterDomain == rcOuterDomain && dstDomain != rcDomain); //The dst domain cannot be null because the base domain does not contain rate change nodes.  Need to also make sure that the dst node is from outside the clock domain
            bool isInOuterDomain = dstDomain == rcOuterDomain;

            if(!isOutsideAtSameLvl && !isInOuterDomain){
                throw std::runtime_error(ErrorHelpers::genErrorStr(
                        "MultiRate Output node's output arc should go to a node in the outer clock domain or to a node one level below the outer domain",
                        rc));
            }
        }else{
            //Don't check disconnected
            if(GeneralHelper::isType<Node, MasterUnconnected>(dstNode) == nullptr) {
                //the destination node should be in the outer domain
                std::shared_ptr<ClockDomain> dstDomain = findClockDomain(dstNode);
                if(dstDomain != rcOuterDomain){
                    throw std::runtime_error(ErrorHelpers::genErrorStr(
                            "MultiRate Output node's output arc should go to a node in the outer clock domain or to a node one level below the outer domain",
                            rc));
                }
            }
        }
    }
    return std::pair<std::set<std::shared_ptr<OutputPort>>, std::set<std::shared_ptr<InputPort>>>(masterInputPorts, masterOutputPorts);
}

void MultiRateHelpers::cloneMasterNodePortClockDomains(std::shared_ptr<MasterNode> origMaster, std::shared_ptr<MasterNode> clonedMaster, const std::map<std::shared_ptr<Node>, std::shared_ptr<Node>> &origToCopyNode) {
    //Update master port clock domain references
    std::map<std::shared_ptr<Port>, std::shared_ptr<ClockDomain>> origMasterClockDomains = origMaster->getIoClockDomains();
    for (auto portClkDomain = origMasterClockDomains.begin();
        portClkDomain != origMasterClockDomains.end(); portClkDomain++) {

        std::shared_ptr<Port> origPort = portClkDomain->first;
        std::shared_ptr<ClockDomain> origClkDomain = portClkDomain->second;

        std::shared_ptr<ClockDomain> cloneClkDomain = nullptr;
        if (origClkDomain != nullptr) {
            auto cloneClkDomainNodeFind = origToCopyNode.find(origClkDomain);
            if(cloneClkDomainNodeFind == origToCopyNode.end()){
                throw std::runtime_error(ErrorHelpers::genErrorStr("Clock domain was not cloned", origClkDomain));
            }
            cloneClkDomain = std::dynamic_pointer_cast<ClockDomain>(cloneClkDomainNodeFind->second);
        }

        //Get clone port based on port number
        std::shared_ptr<Port> clonePort;
        if(GeneralHelper::isType<Port, OutputPort>(origPort)) {
            std::shared_ptr<OutputPort> clonePort = clonedMaster->getOutputPort(origPort->getPortNum());
            clonedMaster->setPortClkDomain(clonePort, cloneClkDomain);
        }else{
            std::shared_ptr<InputPort> clonePort = clonedMaster->getInputPort(origPort->getPortNum());
            clonedMaster->setPortClkDomain(clonePort, cloneClkDomain);
        }
    }
}

void MultiRateHelpers::validateSpecialiedClockDomain(std::shared_ptr<ClockDomain> clkDomain) {
    //Check that the input nodes are expanded and inputs
    std::set<std::shared_ptr<RateChange>> rateChangeIn = clkDomain->getRateChangeIn();
    for(auto it = rateChangeIn.begin(); it != rateChangeIn.end(); it++){
        if(!(*it)->isSpecialized()){
            throw std::runtime_error(ErrorHelpers::genErrorStr("Error when validating DownsampleClockDomain - A RateChange input was not specialized", clkDomain));
        }

        if(!(*it)->isInput()){
            throw std::runtime_error(ErrorHelpers::genErrorStr("Error when validating DownsampleClockDomain - A RateChange input was not a specialized input", clkDomain));
        }
    }

    //Check that the output nodes are expanded and inputs
    std::set<std::shared_ptr<RateChange>> rateChangeOut = clkDomain->getRateChangeOut();
    for(auto it = rateChangeOut.begin(); it != rateChangeOut.end(); it++){
        if(!(*it)->isSpecialized()){
            throw std::runtime_error(ErrorHelpers::genErrorStr("Error when validating DownsampleClockDomain - A RateChange output was not specialized", clkDomain));
        }

        if((*it)->isInput()){
            throw std::runtime_error(ErrorHelpers::genErrorStr("Error when validating DownsampleClockDomain - A RateChange output was not a specialized output", clkDomain));
        }
    }
}

void MultiRateHelpers::validateClockDomainRates(std::vector<std::shared_ptr<ClockDomain>> clockDomainsInDesign) {
    for(unsigned long i = 0; i<clockDomainsInDesign.size(); i++){
        std::pair<int, int> clockDomainRate = clockDomainsInDesign[i]->getRateRelativeToBase();

        //Check that neither the upsample or downsample coefficient are 0.
        if(clockDomainRate.first <= 0 || clockDomainRate.second <= 0){
            throw std::runtime_error(ErrorHelpers::genErrorStr("Rate Relative to Base cannot have a 0 in either the numerator or denominator", clockDomainsInDesign[i]));
        }

        //Check that at most one of the coefficients is not 1
        if(clockDomainRate.first != 1 && clockDomainRate.second != 1){
            //TODO: Support arbitrary resampling
            throw std::runtime_error(ErrorHelpers::genErrorStr("Rate Relative to Base must be an integer upsample or downsample factor ", clockDomainsInDesign[i]));
        }
    }
}

void MultiRateHelpers::rediscoverClockDomainParameters(std::vector<std::shared_ptr<ClockDomain>> clockDomainsInDesign) {
    for(unsigned long i = 0; i<clockDomainsInDesign.size(); i++){
        clockDomainsInDesign[i]->discoverClockDomainParameters();
    }
}

void MultiRateHelpers::checkIOBlockSizes(std::set<std::shared_ptr<MasterNode>> masterNodes, int blockSize) {
    for(auto master = masterNodes.begin(); master != masterNodes.end(); master++){
        std::shared_ptr<MasterNode> masterNode = *master;

        std::map<std::shared_ptr<Port>, std::shared_ptr<ClockDomain>> clockDomainMap = masterNode->getIoClockDomains();

        for(auto clkDomainPair = clockDomainMap.begin(); clkDomainPair != clockDomainMap.end(); clkDomainPair++){
            std::shared_ptr<ClockDomain> clkDomain = clkDomainPair->second;

            std::pair<int, int> clkDomainRate = clkDomain->getRateRelativeToBase();

            if((blockSize*clkDomainRate.first)%clkDomainRate.second != 0){
                throw std::runtime_error(ErrorHelpers::genErrorStr("Found port " + GeneralHelper::to_string(clkDomainPair->first->getPortNum()) + " whose block size is non-integer: " + GeneralHelper::to_string(double(blockSize)*clkDomainRate.first/clkDomainRate.second), masterNode));
            }
        }
    }
}

std::map<int, std::set<std::shared_ptr<ClockDomain>>> MultiRateHelpers::findPartitionsWithSingleClockDomain(const std::vector<std::shared_ptr<Node>> &nodes_orig){
    //Remove clock domain driver nodes from the list of nodes to check

    std::vector<std::shared_ptr<Node>> nodes = nodes_orig;

    //Find clock domains in design
    std::set<std::shared_ptr<ClockDomain>> clockDomains;
    for(std::shared_ptr<Node> &node : nodes){
        std::shared_ptr<ClockDomain> asClkDomain = GeneralHelper::isType<Node, ClockDomain>(node);
        if(asClkDomain){
            clockDomains.insert(asClkDomain);
        }
    }

    //Find clock domain drivers and remove them from the nodes to be checked
    std::set<std::shared_ptr<Node>> driverNodes;
    for(const std::shared_ptr<ClockDomain> &clockDomain : clockDomains){
        //Only look at clock domains that have been specialized
        std::shared_ptr<ContextRoot> asContextRoot = GeneralHelper::isType<ClockDomain, ContextRoot>(clockDomain);
        if(asContextRoot){
            std::vector<std::shared_ptr<Arc>> driverArcs = asContextRoot->getContextDecisionDriver(); //This does not include the replicas
            for(const std::shared_ptr<Arc> &driverArc : driverArcs){
                driverNodes.insert(driverArc->getSrcPort()->getParent());
            }

            //This may contains the origional context drivers.  It should also return the replicas
            std::map<int, std::vector<std::shared_ptr<Arc>>> driverArcsPerPartition = asContextRoot->getContextDriversPerPartition();
            for(const auto &partitionDrivers : driverArcsPerPartition){
                for(const std::shared_ptr<Arc> &driverArc : partitionDrivers.second){
                    driverNodes.insert(driverArc->getSrcPort()->getParent());
                }
            }
        }else{
            throw std::runtime_error(ErrorHelpers::genErrorStr("Expecting all clock domains to be specialized by this point", clockDomain));
        }
    }

    for(const std::shared_ptr<Node> &driverNode : driverNodes){
        nodes.erase(std::remove(nodes.begin(), nodes.end(), driverNode), nodes.end());
    }

    std::map<int, std::pair<int, int>> partitionDiscoveredRate;
    std::set<int> partitionsWithConflictingRatesOrRateChangeNodes;
    std::map<int, std::set<std::shared_ptr<ClockDomain>>> partitionClockDomains;

    for(const std::shared_ptr<Node> &node : nodes){
        //Don't check subsystems
        if(GeneralHelper::isType<Node, SubSystem>(node) == nullptr) {
            int partition = node->getPartitionNum();

            //Only bother checking if a conflict has not already been found for this partition
            if (partitionsWithConflictingRatesOrRateChangeNodes.find(partition) ==
                partitionsWithConflictingRatesOrRateChangeNodes.end()) {
                std::shared_ptr<ClockDomain> clkDomain = findClockDomain(node);
                std::pair<int, int> clkRate = std::pair<int, int>(1, 1);
                if (clkDomain) {
                    clkRate = clkDomain->getRateRelativeToBase();
                }

                if (GeneralHelper::isType<Node, RateChange>(node)) {
                    //We found a rate change, node, invalidate the rate for this partition
                    partitionsWithConflictingRatesOrRateChangeNodes.insert(partition);
                    partitionDiscoveredRate.erase(partition); //erase if exists
                    partitionClockDomains.erase(partition); //erase if exists
                } else {
                    if (partitionDiscoveredRate.find(partition) == partitionDiscoveredRate.end()) {
                        //This is the first time this partition was encountered
                        partitionDiscoveredRate[partition] = clkRate;
                        partitionClockDomains[partition].insert(clkDomain);
                    } else {
                        //This is not the first time we have seen this partition, we need to check
                        if (partitionDiscoveredRate[partition] == clkRate) {
                            //This is the same rate!  Insert the clock domain of this node into the set.  If it has already been discovered, no duplicate will be created
                            partitionClockDomains[partition].insert(clkDomain);
                        } else {
                            //We found a conflict
                            partitionsWithConflictingRatesOrRateChangeNodes.insert(partition);
                            partitionDiscoveredRate.erase(partition); //erase if exists
                            partitionClockDomains.erase(partition); //erase if exists
                        }
                    }
                }
            }
        }
    }

    return partitionClockDomains;
}
