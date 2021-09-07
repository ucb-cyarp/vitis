//
// Created by Christopher Yarp on 9/2/21.
//

#include "DomainPasses.h"

#include "MultiRate/MultiRateHelpers.h"
#include "GraphCore/Node.h"
#include "MasterNodes/MasterInput.h"
#include "MasterNodes/MasterOutput.h"
#include "MasterNodes/MasterUnconnected.h"
#include "MultiThread/ThreadCrossingFIFO.h"

std::vector<std::shared_ptr<ClockDomain>> DomainPasses::specializeClockDomains(Design &design, std::vector<std::shared_ptr<ClockDomain>> clockDomains){
    std::vector<std::shared_ptr<ClockDomain>> specializedDomains;

    for(unsigned long i = 0; i<clockDomains.size(); i++){
        if(!clockDomains[i]->isSpecialized()){
            std::vector<std::shared_ptr<Node>> nodesToAdd, nodesToRemove;
            std::vector<std::shared_ptr<Arc>> arcsToAdd, arcsToRemove;
            std::shared_ptr<ClockDomain> specializedDomain = clockDomains[i]->specializeClockDomain(nodesToAdd, nodesToRemove, arcsToAdd, arcsToRemove);
            specializedDomains.push_back(specializedDomain);
            design.addRemoveNodesAndArcs(nodesToAdd, nodesToRemove, arcsToAdd, arcsToRemove);
        }else{
            specializedDomains.push_back(clockDomains[i]);
        }
    }

    return specializedDomains;
}

std::vector<std::shared_ptr<ClockDomain>> DomainPasses::findClockDomains(Design &design){
    std::vector<std::shared_ptr<ClockDomain>> clockDomains;

    {
        std::vector<std::shared_ptr<Node>> nodes = design.getNodes();
        for (unsigned long i = 0; i < nodes.size(); i++) {
            std::shared_ptr<ClockDomain> asClkDomain = GeneralHelper::isType<Node, ClockDomain>(nodes[i]);

            if (asClkDomain) {
                clockDomains.push_back(asClkDomain);
            }
        }
    }

    return clockDomains;
}

void DomainPasses::createClockDomainSupportNodes(Design &design, std::vector<std::shared_ptr<ClockDomain>> clockDomains, bool includeContext, bool includeOutputBridgingNodes) {
    for(unsigned long i = 0; i<clockDomains.size(); i++){
        std::vector<std::shared_ptr<Node>> nodesToAdd, nodesToRemove;
        std::vector<std::shared_ptr<Arc>> arcsToAdd, arcsToRemove;
        clockDomains[i]->createSupportNodes(nodesToAdd, nodesToRemove, arcsToAdd, arcsToRemove, includeContext, includeOutputBridgingNodes);
        design.addRemoveNodesAndArcs(nodesToAdd, nodesToRemove, arcsToAdd, arcsToRemove);
    }
}

void DomainPasses::resetMasterNodeClockDomainLinks(Design &design) {
    design.getInputMaster()->resetIoClockDomains();
    design.getOutputMaster()->resetIoClockDomains();
    design.getVisMaster()->resetIoClockDomains();
    design.getUnconnectedMaster()->resetIoClockDomains();
    design.getTerminatorMaster()->resetIoClockDomains();
}

//TODO: Update for sub-blocking
std::map<int, std::set<std::pair<int, int>>> DomainPasses::findPartitionClockDomainRates(Design &design) {
    std::map<int, std::set<std::pair<int, int>>> clockDomains;

    //Find clock domains in IO
    //Only looking at Input, Output, and Visualization.  Unconnected and Terminated are unnessasary
    std::set<std::pair<int, int>> ioRates;
    std::vector<std::shared_ptr<MasterNode>> mastersToInclude = {design.getInputMaster(), design.getOutputMaster(), design.getVisMaster()};
    for(int i = 0; i<mastersToInclude.size(); i++) {
        std::shared_ptr<MasterNode> master = mastersToInclude[i];
        std::set<std::shared_ptr<ClockDomain>> inputClockDomains = master->getClockDomains();
        for (auto clkDomain = inputClockDomains.begin(); clkDomain != inputClockDomains.end(); clkDomain++) {
            if (*clkDomain == nullptr) {
                ioRates.emplace(1, 1);
            } else {
                ioRates.insert((*clkDomain)->getRateRelativeToBase());
            }
        }
    }
    clockDomains[IO_PARTITION_NUM] = ioRates;

    {
        std::vector<std::shared_ptr<Node>> nodes = design.getNodes();
        for (unsigned long i = 0; i < nodes.size(); i++) {
            int partition = nodes[i]->getPartitionNum();

            //Do not include clock domains themselves in this
            if (GeneralHelper::isType<Node, ClockDomain>(nodes[i]) == nullptr) {
                //Note that FIFOs report their clock domain differently in order to handle the case when they are connected directly to the InputMaster
                //Each input/output port pair can also have a different clock domain
                if (GeneralHelper::isType<Node, ThreadCrossingFIFO>(nodes[i])) {
                    std::shared_ptr<ThreadCrossingFIFO> fifo = std::dynamic_pointer_cast<ThreadCrossingFIFO>(nodes[i]);
                    for (int portNum = 0; portNum < fifo->getInputPorts().size(); portNum++) {
                        std::shared_ptr<ClockDomain> clkDomain = fifo->getClockDomainCreateIfNot(portNum);

                        if (clkDomain == nullptr) {
                            clockDomains[partition].emplace(1, 1);
                        } else {
                            std::pair<int, int> rate = clkDomain->getRateRelativeToBase();
                            clockDomains[partition].insert(rate);
                        }
                    }
                } else {
                    std::shared_ptr<ClockDomain> clkDomain = MultiRateHelpers::findClockDomain(nodes[i]);

                    if (clkDomain == nullptr) {
                        clockDomains[partition].emplace(1, 1);
                    } else {
                        std::pair<int, int> rate = clkDomain->getRateRelativeToBase();
                        clockDomains[partition].insert(rate);
                    }
                }
            }
        }
    }

    return clockDomains;
}
