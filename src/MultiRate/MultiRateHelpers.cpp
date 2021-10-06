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
#include "Blocking/BlockingHelpers.h"

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

void MultiRateHelpers::setFIFOClockDomainsAndBlockingParams(std::vector<std::shared_ptr<ThreadCrossingFIFO>> threadCrossingFIFOs, int baseBlockingLength) {
    //Primarily exists because FIFOs are placed in the context of the source.  FIFOs that are connected to the MasterInput
    //node should have the clock domain of the particular input port.

    //FIFOs connected to the MasterOutput nodes is less problematic as it will be in the context of the source (except
    //if it is connected to an EnableOutput or RateChangeOutput in which case it is in one context above).  It will be
    //the rate will be checked against the rate of the output ports

    for(const std::shared_ptr<ThreadCrossingFIFO> &threadCrossingFIFO : threadCrossingFIFOs) {
        //TODO: For now, we will restrict clock domain assignment to be when there is a single input and output port.
        //      Do FIFO bundling/merging after this
        if(threadCrossingFIFO->getInputPorts().size() != 1 || threadCrossingFIFO->getOutputPorts().size() != 1){
            throw std::runtime_error(ErrorHelpers::genErrorStr("Currently, clock domain assignment is only implemented for FIFOs that have a single input and output port" , threadCrossingFIFO));
        }

        //Check if the FIFOs are connected to IO masters
        std::set<std::shared_ptr<Arc>> inputMasterConnections = EmitterHelpers::getConnectionsToMasterInputNode(
                threadCrossingFIFO, true);
        std::set<std::shared_ptr<Arc>> outputMasterConnections = EmitterHelpers::getConnectionsToMasterOutputNodes(
                threadCrossingFIFO, true);

        //Set clock domain based on input port if connected to input master
        if(inputMasterConnections.size() > 1){
            throw std::runtime_error(ErrorHelpers::genErrorStr("InputFIFO has more than 1 input arc when setting the clock domain", threadCrossingFIFO));
        }

        std::shared_ptr<Node> traceBase = threadCrossingFIFO;
        std::shared_ptr<ClockDomain> clockDomain = findClockDomain(threadCrossingFIFO);

        //Set the clock domain
        if(inputMasterConnections.size() == 1){
            std::shared_ptr<OutputPort> srcPort = (*inputMasterConnections.begin())->getSrcPort();
            std::shared_ptr<MasterInput> masterInput = GeneralHelper::isType<Node, MasterInput>(srcPort->getParent());

            //TODO: Remove Check
            if(masterInput == nullptr){
                throw std::runtime_error(ErrorHelpers::genErrorStr("Error when setting ClockDomain based on Input", threadCrossingFIFO));
            }

            //TODO: Remove Check
            //Check that the FIFO node is not contained in a clock domain subsystem
            std::shared_ptr<ClockDomain> foundClockDomain = findClockDomain(threadCrossingFIFO);
            if(foundClockDomain){
                throw std::runtime_error(ErrorHelpers::genErrorStr("When setting clock domain based on connection to input master, the FIFO was found under another clock domain", threadCrossingFIFO));
            }

            //Set based on input port
            std::shared_ptr<ClockDomain> clkDomainFromInputMaster = nullptr;
            //In some cases, BlockingInput nodes are placed in line with a Master Input line.  In that case,
            //The indexing logic is handled by those BlockingInput nodes.  The arcs should already be scaled
            //accordingly and the FIFO should not scale them again.  Set the clock domain to the base in this case
            if(!masterInput->isPortClockDomainLogicHandledByBlockingBoundary(srcPort)){
                clkDomainFromInputMaster = masterInput->getPortClkDomain(srcPort);
            }
            threadCrossingFIFO->setClockDomain(0, clkDomainFromInputMaster);
            clockDomain = clkDomainFromInputMaster;

            //Check that the destinations of the FIFO are in the discovered clock domain
            //Set one of the destinations as the trace base for determining block sizes and
            std::set<std::shared_ptr<Arc>> outputArcs = threadCrossingFIFO->getDirectOutputArcs();
            for(const std::shared_ptr<Arc> &outArc : outputArcs){
                std::shared_ptr<Node> dstNode = outArc->getDstPort()->getParent();
                std::shared_ptr<ClockDomain> dstClockDomain = findClockDomain(dstNode);
                if(!masterInput->isPortClockDomainLogicHandledByBlockingBoundary(srcPort) && dstClockDomain != clkDomainFromInputMaster){
                    throw std::runtime_error(ErrorHelpers::genErrorStr("Thread Crossing FIFO connected to Input Master Has Destination Node Which Disagrees on Clock Domain", threadCrossingFIFO));
                }

                traceBase = dstNode;
            }
        }else{
            //Set the clock domain based on context
            //std::shared_ptr<ClockDomain> clkDomain = findClockDomain(threadCrossingFIFO); //Done above
            threadCrossingFIFO->setClockDomain(0, clockDomain);
        }

        std::pair<int, int> clockDomainRate = std::pair<int, int>(1, 1);
        if(clockDomain){
            clockDomainRate = clockDomain->getRateRelativeToBase();
        }

        //TODO: Remove check
        //Check the clock domain
        for(auto arc = outputMasterConnections.begin(); arc != outputMasterConnections.end(); arc++){
            std::shared_ptr<InputPort> dstPort = (*arc)->getDstPort();
            std::shared_ptr<MasterOutput> masterOutput = GeneralHelper::isType<Node, MasterOutput>(dstPort->getParent());

            //TODO: Remove check
            if(masterOutput == nullptr){
                throw std::runtime_error(ErrorHelpers::genErrorStr("Error when checking ClockDomain of FIFO against OutputMaster port", threadCrossingFIFO));
            }

            std::shared_ptr<ClockDomain> clockDomainOutput = masterOutput->getPortClkDomain(dstPort);
            std::pair<int, int> clockDomainOutputRate = std::pair<int, int>(1, 1);
            if(clockDomainOutput){
                clockDomainOutputRate = clockDomainOutput->getRateRelativeToBase();
            }

            if(!masterOutput->isPortClockDomainLogicHandledByBlockingBoundary(dstPort) && clockDomainRate != clockDomainOutputRate){
                throw std::runtime_error(ErrorHelpers::genErrorStr("Found a FIFO connected to a MasterOutput where it's discovered clock domain rate is not the same as the MasterOutput port", threadCrossingFIFO));
            }
        }

        //Set the block sizes and index expressions based on the hierarchy
        if(clockDomain != nullptr && !clockDomain->isUsingVectorSamplingMode()){
            //This FIFO serves a clock domain that is not operating in vector mode.
            //For this FIFO, the indexing is performed by the counter variable of the clock domain
            //   - The indexing basically bypasses the blocking domains
            //The FIFO may not be under the global blocking domain (if there is one) - may be connected directly to the Master Input

            std::pair<int, int> rateRelToBase = clockDomain->getRateRelativeToBase();

            if((baseBlockingLength*rateRelToBase.first)%rateRelToBase.second != 0){
                throw std::runtime_error(ErrorHelpers::genErrorStr("Block Size Inside a Clock Domain Must be an Integer", threadCrossingFIFO));
            }

            threadCrossingFIFO->setBlockSize(0, (baseBlockingLength*rateRelToBase.first)/rateRelToBase.second);
            threadCrossingFIFO->setSubBlockSize(0, 1); //If in a clock domain not operating in vector mode, sub-block size must be 1

            Variable executionCountVar = clockDomain->getExecutionCountVariable(baseBlockingLength);
            threadCrossingFIFO->setCBlockIndexExprInput(0, executionCountVar.getCVarName());
            threadCrossingFIFO->setCBlockIndexExprOutput(0, executionCountVar.getCVarName());
        }else{
            std::pair<int, int> rateRelToBase(1, 1);
            if(clockDomain){
                rateRelToBase = clockDomain->getRateRelativeToBase();
            }

            //If the FIFO is in a clock domain, its block size will differ from the base
            //It will also disagree with the block size defined in the global blocking domain
            //However, the number of sub-blocks the larger block is split into should remain consistent
            //For example, let's say we have a base block size of 64 and this FIFO is in a downsample by 2 domain.
            //The FIFO will have a block size of 32.
            //Let's also say that a sub-blocking factor of 8 was used.  In that case, there are 8 sub-blocks of size 8
            //For the base clock rate.  Within the clock domain operating in vector mode, there are 8 sub-blocks of size
            //4 (vectors from the outer domain are sub-sampled from 8 to 4).
            //This means that this FIFO still uses the indexing variable in the global clock domain but scales
            //it by 4 instead of 8.  This FIFO therefore has a block size of 32, a sub-block size of 4, and the index
            //expression of 4*globalBlockingInd

            //Let's take another case where the FIFO is in a blocking domain under the downsample by 2 domain.
            //In this case, the inner blocking domain has its block size divided by 2 compared to the global sub-blocking
            //size because of the re-sampling that occurs at the downsampling domain.
            //The global block domain has a block size of 64, and a sub-blocking size of 8, resulting in 8 sub-blocks.
            //After the downsample domain, the sub-block size is reduced to 4 but there are still 8 of them per global block.
            //The nested blocking domain has a block size of 4 and a sub-blocking size of 1 with 4 sub-blocks per block.
            //For a FIFO existing within the nested blocking domain, it's block size is half of the base block size
            //due to the downsample clock domain.  Its block size is therefore 32.  For the first level of indexing,
            //it has the same number of sub-blocks as the global domain (8), just with a different sub-block size of 4.
            //This results in the first term in the indexing expression being 4*globalBlockingInd.
            //The inner blocking domain handles 4 sub-sub-blocks per sub-block to result in a sub-sub-blocking size of 1 (ie. it handles 4 sub-blocks).
            //** Note, the number of sub-sub-blocks handled * sub-sub-block dize should match the sub-block size of the above domain.
            //Taking the effective sub-blocking size of the FIFO up to this point, splitting it into a further 4 sub-blocks
            //results in a sub-block size of 1 (4/4).  This matches the final sub-blocking size of the domain the FIFO is
            //in.  It also means the indexing expression becomes 4*globalBlockingInd + 1*nestedBlockingInd.

            //It is important to note that, after determining the block size of the FIFO from the clock domains,
            //the clock domains do not enter into the calculation of the indexing expressions.  That occurs by
            //traversing the blocking domain hierarchy and determining the number of sub-blocks handled per blocking
            //domain.  The sub-sampling logic is effectively pushed to the sizing of the FIFO

            //Also note, if the FIFO is at the top of the hierarchy, outside even the global blocking domain (ie. is a
            //I/O FIFO), then the block size and sub-block size will be the same and no indexing will be performed

            if((baseBlockingLength*rateRelToBase.first)%rateRelToBase.second != 0){
                throw std::runtime_error(ErrorHelpers::genErrorStr("Block Size Inside a Clock Domain Must be an Integer", threadCrossingFIFO));
            }
            int blockSize = (baseBlockingLength*rateRelToBase.first)/rateRelToBase.second;
            //I/O input FIFOs are placed in base domain because FIFOs are placed in the domain of the src.
            //However, the indexing actually occurs inside the domain of its destination.  For FIFOs operating at the
            //base rate, they shold be connected to Global Blocking Input nodes and no indexing should be required.
            //However, if the destination node is inside of another clock domain, Blocking Domains Inputs are not
            //used and the FIFO itself handles the indexing based on the domain the destination is in.
            //TODO: Possibly clean up
            std::shared_ptr<Node> blockingDomainIndExprBase = threadCrossingFIFO;
            if(clockDomainRate != std::pair<int, int>(1, 1)){
                //The destination is in another clock domain not operating at the base rate.  If input is from Input master, use the destination node when finding the indexing.  If the FIFO is not connected to the input master, traceBase == threadCrossingFIFO
                blockingDomainIndExprBase = traceBase;
            }
            std::vector<std::shared_ptr<BlockingDomain>> blockingDomainHierarchy = BlockingHelpers::findBlockingDomainStack(blockingDomainIndExprBase);

            //Traverse down the hierarchy
            std::string indexExpr;
            int currentBlockSize = blockSize;
            for(const std::shared_ptr<BlockingDomain> &blockingDomain : blockingDomainHierarchy){
                if(blockingDomain->getBlockingLen()%blockingDomain->getSubBlockingLen() != 0){
                    throw std::runtime_error(ErrorHelpers::genErrorStr("Blocking Domain Found which does not process an integer number of sub-blocks", blockingDomain));
                }
                int subBlockCount = blockingDomain->getBlockingLen()/blockingDomain->getSubBlockingLen();
                if(currentBlockSize%subBlockCount != 0){
                    throw std::runtime_error(ErrorHelpers::genErrorStr("Found a blocking domain whose sub-blocking count is incompatible with the block size of the FIFO", threadCrossingFIFO));
                }

                int localSubBlockLen = currentBlockSize/subBlockCount;
                if(!indexExpr.empty()){
                    indexExpr += "+";
                }
                if(localSubBlockLen != 1){
                    indexExpr += GeneralHelper::to_string(localSubBlockLen) + "*";
                }
                indexExpr += blockingDomain->getBlockIndVar().getCVarName();

                currentBlockSize = localSubBlockLen; //The local sub-block length is the block length of the next nested domain
            }

            int subBlockSize = currentBlockSize; //This is the effective sub-block length at the last blocking domain in the hierarchy that the FIFO is in.
                                                 //It may not be equivalent to the sub-blocking length of the last sub-blocking domain in the hierarchy
                                                 //If clock domain (s) exist below it and the FIFO is not in a nested blocking domain.

            //For FIFOs outside of any blocking domain (ie. I/O FIFOs operating at the base rate), blocking is already
            //being handled by the BlockingDomain inputs and outputs as well as the expansion of the arcs.  Declare
            //the FIFO to have a block size of 1.
            if(blockingDomainHierarchy.empty()){
                blockSize = 1;
                subBlockSize = 1;
            }

            threadCrossingFIFO->setBlockSize(0,blockSize);
            threadCrossingFIFO->setSubBlockSize(0, subBlockSize);
            threadCrossingFIFO->setCBlockIndexExprInput(0, indexExpr);
            threadCrossingFIFO->setCBlockIndexExprOutput(0, indexExpr);
        };
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

bool MultiRateHelpers::arcIsIOAndNotAtBaseRate(std::shared_ptr<Arc> arc){
    std::shared_ptr<Node> srcNode = arc->getSrcPort()->getParent();
    std::shared_ptr<Node> dstNode = arc->getDstPort()->getParent();

    bool inputIsIO = GeneralHelper::isType<Node, MasterInput>(srcNode) != nullptr;
    bool outputIsIO = GeneralHelper::isType<Node, MasterOutput>(dstNode) != nullptr;

    if(inputIsIO && outputIsIO){
        std::cerr << ErrorHelpers::genWarningStr("Arc directly between input and output detected.  Assuming at base rate") << std::endl;
        return false;
    }else if(inputIsIO || outputIsIO){
        std::shared_ptr<Node> nonIONode;
        if(inputIsIO){
            nonIONode = dstNode;
        }else{
            nonIONode = srcNode;
        }

        std::shared_ptr<ClockDomain> clockDomain = findClockDomain(nonIONode);

        return clockDomain != nullptr;
    }

    //Not an I/O arc
    return false;
}
