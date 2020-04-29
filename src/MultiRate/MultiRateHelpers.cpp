//
// Created by Christopher Yarp on 4/22/20.
//

#include "MultiRateHelpers.h"
#include "General/GeneralHelper.h"
#include "General/ErrorHelpers.h"
#include "ClockDomain.h"
#include "GraphCore/Node.h"
#include "MasterNodes/MasterInput.h"
#include "MasterNodes/MasterOutput.h"
#include "MasterNodes/MasterUnconnected.h"
#include "RateChange.h"

std::set<std::shared_ptr<Node>>
MultiRateHelpers::getNodesInClockDomainHelper(const std::set<std::shared_ptr<Node>> nodesToSearch) {
    return getNodesInClockDomainHelperFilter<Node>(nodesToSearch);
}

std::shared_ptr<ClockDomain> MultiRateHelpers::findClockDomain(std::shared_ptr<Node> node) {
    std::shared_ptr<ClockDomain> clkDomain = nullptr;
    std::shared_ptr<Node> cursor = node;

    while(cursor != nullptr){
        std::shared_ptr<SubSystem> parent = cursor->getParent();

        if(GeneralHelper::isType<Node, ClockDomain>(parent)){
            clkDomain = std::static_pointer_cast<ClockDomain>(parent);
            cursor = nullptr;
        }else{
            cursor = parent;
        }
    }

    return clkDomain;
}

bool MultiRateHelpers::isOutsideClkDomain(std::shared_ptr<ClockDomain> a, std::shared_ptr<ClockDomain> b) {

    //Check if a is outside of clock domain b

    //Traverse up a's ClockDomain hierarchy and see if b shows up

    std::shared_ptr<ClockDomain> cursor = a;

    bool done = false;

    while(!done){
        if(cursor == b){
            //Found a place in the hierarchy of a where b appears, it is in
            //Need to check this before nullptr since a and b can either or both be nullptrs.
            //If B is a nullptr, a is nested under it as nullptr represents the base clock domain
            return false;
        }else if(cursor == nullptr) {
            //Already convered the equivalency check which handles the case wben b is nullptr
            //At this point, the entire clock domain hierarchy of a has been traversed
            done = true;
        }else{
            //Traverse up the hierarchy
            cursor = findClockDomain(cursor); //When run on a clock domain node, this find the clock domain it is nested in
        }
    }

    //Did not find clock domain b in the clock domain hierarchy of a, so a is outside of b
    return true;
}

bool MultiRateHelpers::isClkDomainOrOneLvlNested(std::shared_ptr<ClockDomain> a, std::shared_ptr<ClockDomain> b) {
    //Check if ClockDomain a is equal to ClockDomain b or is a ClockDomain directly nested within clock domain b (one level below)
    if(a == b){
        return true;
    }

    if(a != nullptr){
        std::shared_ptr<ClockDomain> oneLvlUp = findClockDomain(a); //When run on the clock domain nodes, this finds the clock domain it is nested under
        return oneLvlUp == b;
    }

    return false;
}

bool
MultiRateHelpers::areWithinOneClockDomainOfEachOther(std::shared_ptr<ClockDomain> a, std::shared_ptr<ClockDomain> b) {
    if(a == b){
        return true;
    }

    //We do not actually need to check if a or b are nullptr because findClockDomain will return nullptr
    //This will result in a redundant check
    std::shared_ptr<ClockDomain> outerA = findClockDomain(a);
    std::shared_ptr<ClockDomain> outerB = findClockDomain(b);

    return outerA == outerB || a == outerB || outerA == b;

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
    std::map<std::shared_ptr<Port>, std::shared_ptr<ClockDomain>> inputMasterClockDomains = origMaster->getIoClockDomains();
    for (auto portClkDomain = inputMasterClockDomains.begin();
        portClkDomain != inputMasterClockDomains.end(); portClkDomain++) {

        std::shared_ptr<Port> origPort = portClkDomain->first;
        std::shared_ptr<ClockDomain> origClkDomain = portClkDomain->second;

    //Get clone port based on port number
        std::shared_ptr<OutputPort> clonePort = clonedMaster->getOutputPort(origPort->getPortNum());
        std::shared_ptr<ClockDomain> cloneClkDomain = nullptr;

        if (origClkDomain != nullptr) {
            cloneClkDomain = std::dynamic_pointer_cast<ClockDomain>((origToCopyNode.find(origClkDomain))->second);
        }

        clonedMaster->setPortClkDomain(clonePort, cloneClkDomain);
    }
}