//
// Created by Christopher Yarp on 9/3/19.
//

#include "EmitterHelpers.h"
#include "PrimitiveNodes/Delay.h"
#include "PrimitiveNodes/BlackBox.h"
#include "GraphCore/ContextRoot.h"
#include "GraphCore/NodeFactory.h"
#include "MasterNodes/MasterUnconnected.h"
#include <iostream>
#include <fstream>

void EmitterHelpers::absorbAdjacentDelaysIntoFIFOs(std::map<std::pair<int, int>, std::vector<std::shared_ptr<ThreadCrossingFIFO>>> fifos,
                                                   std::vector<std::shared_ptr<Node>> &new_nodes,
                                                   std::vector<std::shared_ptr<Node>> &deleted_nodes,
                                                   std::vector<std::shared_ptr<Arc>> &new_arcs,
                                                   std::vector<std::shared_ptr<Arc>> &deleted_arcs, bool printActions) {

    for(auto it = fifos.begin(); it != fifos.end(); it++){
        int srcPartition = it->first.first;
        int dstPartition = it->first.second;

        std::vector<std::shared_ptr<ThreadCrossingFIFO>> fifoPtrs = it->second;

        for(int i = 0; i<fifoPtrs.size(); i++) {
            bool done = false;

            //Iterate because there may be a series of delays to ingest
            while (!done) {
                AbsorptionStatus statusIn = absorbAdjacentInputDelayIfPossible(fifoPtrs[i], srcPartition, new_nodes, deleted_nodes,
                                                                               new_arcs, deleted_arcs, printActions);

                AbsorptionStatus statusOut = AbsorptionStatus::NO_ABSORPTION;
                if (statusIn != AbsorptionStatus::PARTIAL_ABSORPTION_FULL_FIFO) {
                    statusOut = absorbAdjacentOutputDelayIfPossible(fifoPtrs[i], dstPartition, new_nodes, deleted_nodes, new_arcs,
                                                                    deleted_arcs, printActions);
                }

                if (statusIn == AbsorptionStatus::PARTIAL_ABSORPTION_FULL_FIFO ||
                    statusOut == AbsorptionStatus::PARTIAL_ABSORPTION_FULL_FIFO) {
                    //FIFO filled up, not worth continuing
                    done = true;
                } else if (statusIn == AbsorptionStatus::NO_ABSORPTION &&
                           (statusOut == AbsorptionStatus::NO_ABSORPTION ||
                            statusOut == AbsorptionStatus::PARTIAL_ABSORPTION_MERGE_INIT_COND)) {
                    //Could not absorb due to conflicts on input or output, done
                    done = true;
                }
            }

            //TODO: Refactor into absorption?  Tricky because there may be a chain of delays.  Possibly when re-timer is implemented
            //The number of initial conditions in the FIFO must be a multiple of the block size
            //remove the remainder from the FIFO and insert into a delay at the input
            EmitterHelpers::reshapeFIFOInitialConditionsForBlockSize(fifoPtrs[i], new_nodes, deleted_nodes, new_arcs, deleted_arcs, printActions);
        }
    }
}

EmitterHelpers::AbsorptionStatus
EmitterHelpers::absorbAdjacentInputDelayIfPossible(std::shared_ptr<ThreadCrossingFIFO> fifo,
                                                   int inputPartition,
                                                   std::vector<std::shared_ptr<Node>> &new_nodes,
                                                   std::vector<std::shared_ptr<Node>> &deleted_nodes,
                                                   std::vector<std::shared_ptr<Arc>> &new_arcs,
                                                   std::vector<std::shared_ptr<Arc>> &deleted_arcs, bool printActions) {
    //TODO: For now, we will restrict delay absorption to be when there is a single input and output port.
    //FIFO bundling happens after this point
    if(fifo->getInputPorts().size() != 1 || fifo->getOutputPorts().size() != 1){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Currently, delay absorption is only supported with FIFOs that have a single input and output port" , fifo));
    }

    //Check if FIFO full
    if(fifo->getInitConditions().size() < (fifo->getFifoLength()*fifo->getBlockSize() - fifo->getBlockSize())) {
        //There is still room

        std::set<std::shared_ptr<Arc>> inputArcs = fifo->getInputPortCreateIfNot(0)->getArcs();
        if (inputArcs.size() != 1) {
            throw std::runtime_error(
                    ErrorHelpers::genErrorStr("Unexpected number of input arcs when absorbing delays into FIFO", fifo));
        }

        if (fifo->getInputArcs().size() == 1) {
            //No order constraint arcs into this FIFO, proceed with checking if src absorption is possible

            //=== Check if this FIFO has a delay at its input
            std::shared_ptr<Node> srcNode = (*inputArcs.begin())->getSrcPort()->getParent();
            std::shared_ptr<Delay> srcDelay = GeneralHelper::isType<Node, Delay>(srcNode);

            if (srcDelay) {
                //Found a delay at the src

                //Check that the delay is in the correct partition and context
                if(srcDelay->getPartitionNum() != inputPartition){
                    return AbsorptionStatus::NO_ABSORPTION;
                }

                if(!Context::isEqContext(fifo->getContext(), srcDelay->getContext())){
                    return AbsorptionStatus::NO_ABSORPTION;
                }

                //Check if it is connected to any other node

                //TODO: For now, delay absorption should occur before stateUpdateNode are inserted.  Consider loosing this requirement.  With stateUpdateNodes, no delay connected to a FIFO will have only 1 dependant node (since the state update will be one od
                if (srcDelay->getStateUpdateNode() != nullptr) {
                    throw std::runtime_error(ErrorHelpers::genErrorStr(
                            "FIFO Delay Absorption should be run before State Update Node Insertion",
                            srcDelay));
                }

                std::set<std::shared_ptr<Arc>> delayFIFOArcs = srcDelay->getOutputArcs();
                if (delayFIFOArcs.size() == 1) {
                    //The delay has only 1 output (which must be this FIFO).  This delay also does not have any output order constraint arcs
                    //Absorb it if possible

                    std::vector<NumericValue> delayInitConds = srcDelay->getInitCondition();
                    std::vector<NumericValue> fifoInitConds = fifo->getInitConditionsCreateIfNot(0);

                    if (delayInitConds.size() + fifoInitConds.size() <= (fifo->getFifoLength()*fifo->getBlockSize() - fifo->getBlockSize())) {
                        //Can absorb complete delay
                        fifoInitConds.insert(fifoInitConds.end(), delayInitConds.begin(),
                                             delayInitConds.end());  //Because this is at the input to the FIFO, the initial conditions are appended.
                        fifo->setInitConditionsCreateIfNot(0, fifoInitConds);

                        //TODO:
                        //Remove the delay node and re-wire arcs to the FIFO (including order constraint arcs)
                        //Disconnect the existing arc between the delay and the FIFO
                        std::shared_ptr<Arc> delayFIFOArc = *delayFIFOArcs.begin(); //We already checked that there is only one
                        delayFIFOArc->disconnect();
                        deleted_arcs.push_back(delayFIFOArc);

                        //re-wire
                        std::set<std::shared_ptr<Arc>> directArcs = srcDelay->getDirectInputArcs();
                        std::set<std::shared_ptr<Arc>> orderConstraintArcs = srcDelay->getOrderConstraintInputArcs();

                        for (auto inputArc = directArcs.begin(); inputArc != directArcs.end(); inputArc++) {
                            (*inputArc)->setDstPortUpdateNewUpdatePrev(fifo->getInputPortCreateIfNot(0));
                        }

                        for (auto inputArc = orderConstraintArcs.begin();
                             inputArc != orderConstraintArcs.end(); inputArc++) {
                            (*inputArc)->setDstPortUpdateNewUpdatePrev(fifo->getOrderConstraintInputPortCreateIfNot());
                        }

                        deleted_nodes.push_back(srcDelay);

                        if (printActions) {
                            std::cout << "Delay Absorbed info FIFO: " << srcDelay->getFullyQualifiedName() << " [ID:" << srcDelay->getId() << "]"
                                      << " into " << fifo->getFullyQualifiedName() << " [ID:" << fifo->getId() << "]" << std::endl;
                        }

                        return AbsorptionStatus::FULL_ABSORPTION;
                    } else {
                        //Partial absorption (due to size)
                        //We already checked that there is room

                        int numToAbsorb = fifo->getFifoLength()*fifo->getBlockSize() - fifo->getBlockSize() - fifoInitConds.size();

                        fifoInitConds.insert(fifoInitConds.end(), delayInitConds.begin(), delayInitConds.begin()+numToAbsorb);
                        fifo->setInitConditionsCreateIfNot(0, fifoInitConds);
                        delayInitConds.erase(delayInitConds.begin(), delayInitConds.begin()+numToAbsorb);
                        srcDelay->setInitCondition(delayInitConds);
                        srcDelay->setDelayValue(srcDelay->getDelayValue()-numToAbsorb);

                        //No re-wiring required

                        if (printActions) {
                            std::cout << "Delay Partially Absorbed info FIFO: " << srcDelay->getFullyQualifiedName() << " [ID:" << srcDelay->getId() << "]"
                                      << " into " << fifo->getFullyQualifiedName() << " [ID:" << fifo->getId() << "]" << std::endl;
                        }

                        return AbsorptionStatus::PARTIAL_ABSORPTION_FULL_FIFO;
                    }

                }//else, delay has more than 1 output (Cannot absorb)
            }//else src was not delay (cannot absorb)
        }//else, did not check because order constraint arcs to this FIFO
    }//else, FIFO full

    return AbsorptionStatus::NO_ABSORPTION;
}

EmitterHelpers::AbsorptionStatus
EmitterHelpers::absorbAdjacentOutputDelayIfPossible(std::shared_ptr<ThreadCrossingFIFO> fifo,
                                                    int outputPartition,
                                                    std::vector<std::shared_ptr<Node>> &new_nodes,
                                                    std::vector<std::shared_ptr<Node>> &deleted_nodes,
                                                    std::vector<std::shared_ptr<Arc>> &new_arcs,
                                                    std::vector<std::shared_ptr<Arc>> &deleted_arcs,
                                                    bool printActions) {
    //TODO: For now, we will restrict delay absorption to be when there is a single input and output port.
    //FIFO bundling happens after this point
    if(fifo->getInputPorts().size() != 1 || fifo->getOutputPorts().size() != 1){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Currently, delay absorption is only supported with FIFOs that have a single input and output port" , fifo));
    }

    //Check if FIFO full
    if (fifo->getInitConditions().size() < (fifo->getFifoLength()*fifo->getBlockSize() - fifo->getBlockSize())) {
        //There is still room in the FIFO

        //Check if the FIFO has order constraint outputs
        if (fifo->getOrderConstraintOutputArcs().size() == 0) {
            //Check dsts
            std::set<std::shared_ptr<Arc>> fifoOutputArcs = fifo->getDirectOutputArcs();
            if(fifoOutputArcs.size() > 0) {

                //Check if all outputs are delays
                for (auto it = fifoOutputArcs.begin(); it != fifoOutputArcs.end(); it++) {
                    std::shared_ptr<Node> dstNode = (*it)->getDstPort()->getParent();

                    std::shared_ptr<Delay> dstDelay = GeneralHelper::isType<Node, Delay>(dstNode);
                    if(dstDelay == nullptr){
                        //One of the outputs is not a delay
                        return AbsorptionStatus::NO_ABSORPTION;
                    }

                    //Check that the delay is in the correct partition and context
                    if(dstDelay->getPartitionNum() != outputPartition){
                        return AbsorptionStatus::NO_ABSORPTION;
                    }

                    if(!Context::isEqContext(fifo->getContext(), dstDelay->getContext())){
                        return AbsorptionStatus::NO_ABSORPTION;
                    }
                }

                //All of the outputs are delays, find the common set of initial conditions
                //Special case first.  We checked that there was at least 1 output
                std::vector<NumericValue> longestPostfix;
                {
                    auto it = fifoOutputArcs.begin();
                    std::shared_ptr<Node> dstNode = (*it)->getDstPort()->getParent();
                    std::shared_ptr<Delay> dstDelay = std::static_pointer_cast<Delay>(dstNode);
                    longestPostfix = dstDelay->getInitCondition();

                    if(longestPostfix.empty()){
                        //The first delay is degenerate, we can't merge
                        return AbsorptionStatus::NO_ABSORPTION;
                    }

                    for (++it; it != fifoOutputArcs.end(); it++) {
                        std::shared_ptr<Node> dstNode = (*it)->getDstPort()->getParent();
                        std::shared_ptr<Delay> dstDelay = std::static_pointer_cast<Delay>(dstNode); //We already checked that this cast could be made in the loop above

                        longestPostfix = GeneralHelper::longestPostfix(longestPostfix, dstDelay->getInitCondition());

                        if(longestPostfix.empty()){
                            //The longest postfix is empty, no need to keep searching, we can't merge
                            return AbsorptionStatus::NO_ABSORPTION;
                        }
                    }
                }
                //We have a nonzero number of delays that can be merged (otherwise, the longest prefix would have become empty durring one of the itterations)

                //Find how many can be absorbed into the FIFO
                std::vector<NumericValue> fifoInitConds = fifo->getInitConditionsCreateIfNot(0);
                int roomInFifo = fifo->getFifoLength()*fifo->getBlockSize() - fifo->getBlockSize() - fifoInitConds.size();
                int numToAbsorb = std::min(roomInFifo, (int) longestPostfix.size());

                //Set FIFO new initial conditions
                int remainingPostfix = longestPostfix.size() - numToAbsorb;
                std::vector<NumericValue> newFIFOInitConds;
                newFIFOInitConds.insert(newFIFOInitConds.end(), longestPostfix.begin()+remainingPostfix, longestPostfix.end());
                //Append the existing FIFO init conditions onto the back of the postfix
                newFIFOInitConds.insert(newFIFOInitConds.end(), fifoInitConds.begin(), fifoInitConds.end());
                fifo->setInitConditionsCreateIfNot(0, newFIFOInitConds);

                //For each delay: Adjust Delay initial conditions or remove delay entirely
                //Remember to place initial conditions in correct order (append FIFO init conditions to delay initial conditions)
                bool foundPartial = false;
                for (auto it = fifoOutputArcs.begin(); it != fifoOutputArcs.end(); it++) {
                    std::shared_ptr<Node> dstNode = (*it)->getDstPort()->getParent();
                    std::shared_ptr<Delay> dstDelay = std::static_pointer_cast<Delay>(dstNode); //We already checked that this cast could be made in the loop above

                    std::vector<NumericValue> delayInitConditions = dstDelay->getInitCondition();
                    if(delayInitConditions.size() > numToAbsorb){
                        throw std::runtime_error(ErrorHelpers::genErrorStr("Found a delay with an unexpected number of initial conditions durring FIFO delay absorption", dstDelay));
                    }else if(delayInitConditions.size() == numToAbsorb){
                        //Complete absorption

                        //Disconnect the arc between the FIFO and delay
                        (*it)->disconnect();
                        deleted_arcs.push_back(*it);

                        std::set<std::shared_ptr<Arc>> delayDirectOutputArcs = dstDelay->getDirectOutputArcs();
                        std::set<std::shared_ptr<Arc>> delayOrderConstraintOutputs = dstDelay->getOrderConstraintOutputArcs();

                        //Rewire arcs
                        for(auto directArc = delayDirectOutputArcs.begin(); directArc != delayDirectOutputArcs.end(); directArc++){
                            (*directArc)->setSrcPortUpdateNewUpdatePrev(fifo->getOutputPortCreateIfNot(0));
                        }

                        for(auto orderConstraintArc = delayOrderConstraintOutputs.begin(); orderConstraintArc != delayOrderConstraintOutputs.end(); orderConstraintArc++){
                            (*orderConstraintArc)->setSrcPortUpdateNewUpdatePrev(fifo->getOrderConstraintOutputPortCreateIfNot());
                        }

                        //Now disconnected, delete delay
                        deleted_nodes.push_back(dstDelay);

                        if (printActions) {
                            std::cout << "Delay Absorbed info FIFO: " << dstDelay->getFullyQualifiedName() << " [ID:" << dstDelay->getId() << "]"
                                      << " into " << fifo->getFullyQualifiedName() << " [ID:" << fifo->getId() << "]" << std::endl;
                        }
                    }else{
                        //Partial absorption
                        foundPartial = true;

                        //Update the delay initial condition and delay value
                        std::vector<NumericValue> delayInitConds = dstDelay->getInitCondition();
                        //Remove from the tail of the initial conditions (fifo is behind it)
                        delayInitConds.erase(delayInitConds.end()-numToAbsorb, delayInitConds.end());
                        dstDelay->setInitCondition(delayInitConds);
                        dstDelay->setDelayValue(dstDelay->getDelayValue() - numToAbsorb);

                        if (printActions) {
                            std::cout << "Delay Partially Absorbed info FIFO: " << dstDelay->getFullyQualifiedName() << " [ID:" << dstDelay->getId() << "]"
                                      << " into " << fifo->getFullyQualifiedName() << " [ID:" << fifo->getId() << "]" << std::endl;
                        }
                    }
                }

                if((numToAbsorb == roomInFifo) && foundPartial){
                    return AbsorptionStatus::PARTIAL_ABSORPTION_FULL_FIFO;
                }else if(foundPartial){
                    return AbsorptionStatus::PARTIAL_ABSORPTION_MERGE_INIT_COND;
                }else{
                    return AbsorptionStatus::FULL_ABSORPTION;
                }
            }//else, no output arcs
        }//else, has order constraint outputs, will not absorb. TODO: consider relaxing this
    }//else, fifo full

    return AbsorptionStatus::NO_ABSORPTION;
}

 void EmitterHelpers::reshapeFIFOInitialConditionsForBlockSize(std::shared_ptr<ThreadCrossingFIFO> fifo,
                                                     std::vector<std::shared_ptr<Node>> &new_nodes,
                                                     std::vector<std::shared_ptr<Node>> &deleted_nodes,
                                                     std::vector<std::shared_ptr<Arc>> &new_arcs,
                                                     std::vector<std::shared_ptr<Arc>> &deleted_arcs,
                                                     bool printActions){
     //TODO: For now, we will restrict delay absorption to be when there is a single input and output port.
     //FIFO bundling happens after this point
     if(fifo->getInputPorts().size() != 1 || fifo->getOutputPorts().size() != 1){
         throw std::runtime_error(ErrorHelpers::genErrorStr("Currently, initial condition reshaping is only supported with FIFOs that have a single input and output port" , fifo));
     }

    std::vector<NumericValue> fifoInitialConditions = fifo->getInitConditionsCreateIfNot(0);

    int numElementsToMove = fifoInitialConditions.size() % fifo->getBlockSize();

    if(numElementsToMove > 0){
        EmitterHelpers::reshapeFIFOInitialConditions(fifo, numElementsToMove, new_nodes, deleted_nodes, new_arcs, deleted_arcs);

        if (printActions) {
            std::cout << "FIFO Initial Conditions Reshaped: " << fifo->getFullyQualifiedName() << " [ID:" << fifo->getId() << "]"
                      << " " << numElementsToMove << " initial conditions moved into delay at input" << std::endl;
        }
    }
}

void EmitterHelpers::reshapeFIFOInitialConditionsToSize(std::shared_ptr<ThreadCrossingFIFO> fifo,
                                                              int tgtSize,
                                                              std::vector<std::shared_ptr<Node>> &new_nodes,
                                                              std::vector<std::shared_ptr<Node>> &deleted_nodes,
                                                              std::vector<std::shared_ptr<Arc>> &new_arcs,
                                                              std::vector<std::shared_ptr<Arc>> &deleted_arcs,
                                                              bool printActions){
    //TODO: For now, we will restrict delay absorption to be when there is a single input and output port.
    //FIFO bundling happens after this point
    if(fifo->getInputPorts().size() != 1 || fifo->getOutputPorts().size() != 1){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Currently, initial condition reshaping is only supported with FIFOs that have a single input and output port" , fifo));
    }

    std::vector<NumericValue> fifoInitialConditions = fifo->getInitConditionsCreateIfNot(0);

    if(tgtSize > fifoInitialConditions.size()){
        throw std::runtime_error(ErrorHelpers::genErrorStr("For Initial Condition reshaping, target initial condition size must be <= the current initial condition size" , fifo));
    }

    int numElementsToMove = fifoInitialConditions.size() - tgtSize;

    if(numElementsToMove > 0){
        EmitterHelpers::reshapeFIFOInitialConditions(fifo, numElementsToMove, new_nodes, deleted_nodes, new_arcs, deleted_arcs);

        if (printActions) {
            std::cout << "FIFO Initial Conditions Reshaped: " << fifo->getFullyQualifiedName() << " [ID:" << fifo->getId() << "]"
                      << " " << numElementsToMove << " initial conditions moved into delay at input" << std::endl;
        }
    }
}

void EmitterHelpers::reshapeFIFOInitialConditions(std::shared_ptr<ThreadCrossingFIFO> fifo,
                                                              int numElementsToMove,
                                                              std::vector<std::shared_ptr<Node>> &new_nodes,
                                                              std::vector<std::shared_ptr<Node>> &deleted_nodes,
                                                              std::vector<std::shared_ptr<Arc>> &new_arcs,
                                                              std::vector<std::shared_ptr<Arc>> &deleted_arcs){
    //TODO: For now, we will restrict delay absorption to be when there is a single input and output port.
    //FIFO bundling happens after this point
    if(fifo->getInputPorts().size() != 1 || fifo->getOutputPorts().size() != 1){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Currently, initial condition reshaping is only supported with FIFOs that have a single input and output port" , fifo));
    }

    std::vector<NumericValue> fifoInitialConditions = fifo->getInitConditionsCreateIfNot(0);

    if(numElementsToMove > 0){
        //Need to create a delay to move the remaining elements into
        std::vector<Context> fifoContext = fifo->getContext();

        std::shared_ptr<Delay> delay = NodeFactory::createNode<Delay>(fifo->getParent());
        new_nodes.push_back(delay);
        delay->setPartitionNum(fifo->getPartitionNum());
        delay->setName("OverflowFIFOInitCondFrom"+fifo->getName());
        delay->setContext(fifoContext);
        if(fifoContext.size() > 0){
            int subcontext = fifoContext[fifoContext.size()-1].getSubContext();
            fifoContext[fifoContext.size()-1].getContextRoot()->addSubContextNode(subcontext, delay);
        }

        //Set initial conditions
        delay->setDelayValue(numElementsToMove);
        std::vector<NumericValue> delayInitConds;
        delayInitConds.insert(delayInitConds.end(), fifoInitialConditions.begin()+(fifoInitialConditions.size() - numElementsToMove), fifoInitialConditions.end());
        delay->setInitCondition(delayInitConds);

        //Remove init conditions from fifoInitialConditions
        fifoInitialConditions.erase(fifoInitialConditions.begin()+(fifoInitialConditions.size() - numElementsToMove), fifoInitialConditions.end());
        fifo->setInitConditionsCreateIfNot(0, fifoInitialConditions);

        //Rewire
        std::set<std::shared_ptr<Arc>> orderConstraintArcs2Rewire = fifo->getOrderConstraintInputPortCreateIfNot()->getArcs();
        for(auto it = orderConstraintArcs2Rewire.begin(); it != orderConstraintArcs2Rewire.end(); it++){
            (*it)->setDstPortUpdateNewUpdatePrev(delay->getOrderConstraintInputPortCreateIfNot());
        }

        if(fifo->getInputPorts().size() != 1){
            throw std::runtime_error(ErrorHelpers::genErrorStr("Error when reshaping FIFO initial conditions.  Number of input ports should be 1", fifo));
        }

        std::set<std::shared_ptr<Arc>> inputArcs = fifo->getInputPort(0)->getArcs();

        if(inputArcs.size() != 1){
            throw std::runtime_error(ErrorHelpers::genErrorStr("Error when reshaping FIFO initial conditions.  Number of input arcs should be 1", fifo));
        }

        std::shared_ptr<Arc> origInputArc = *inputArcs.begin();

        std::shared_ptr<Arc> newInputArc =  Arc::connectNodes(delay->getOutputPortCreateIfNot(0), fifo->getInputPortCreateIfNot(0), origInputArc->getDataType(), origInputArc->getSampleTime());
        new_arcs.push_back(newInputArc);
        origInputArc->setDstPortUpdateNewUpdatePrev(delay->getInputPortCreateIfNot(0));
    }
}

std::map<std::pair<int, int>, std::vector<std::shared_ptr<ThreadCrossingFIFO>>>
        EmitterHelpers::mergeFIFOs(std::map<std::pair<int, int>, std::vector<std::shared_ptr<ThreadCrossingFIFO>>> fifos,
                       std::vector<std::shared_ptr<Node>> &new_nodes,
                       std::vector<std::shared_ptr<Node>> &deleted_nodes,
                       std::vector<std::shared_ptr<Arc>> &new_arcs,
                       std::vector<std::shared_ptr<Arc>> &deleted_arcs,
                       bool printActions){

    std::map<std::pair<int, int>, std::vector<std::shared_ptr<ThreadCrossingFIFO>>> newMap;

    for(auto it = fifos.begin(); it != fifos.end(); it++){
        std::pair<int, int> crossing = it->first;
        std::vector<std::shared_ptr<ThreadCrossingFIFO>> partitionCrossingFIFOs = it->second;

        if(partitionCrossingFIFOs.size()>1){
            //Only merge if > 1 FIFO crossing a given partition crossing

            //Find the minimum number of initial conditions present for each FIFO in this set
            int minInitConds = partitionCrossingFIFOs[0]->getInitConditionsCreateIfNot(0).size(); //Guarenteed to have at least port 0
            for(int i = 0; i<partitionCrossingFIFOs.size(); i++){
                for(int portNum = 0; portNum<partitionCrossingFIFOs[i]->getInputPorts().size(); portNum++){
                    minInitConds = std::min(minInitConds, (int) partitionCrossingFIFOs[i]->getInitConditionsCreateIfNot(portNum).size());
                }
            }

            //Reshape FIFO0 to the correct number of initial conditions
            reshapeFIFOInitialConditionsToSize(partitionCrossingFIFOs[0], minInitConds, new_nodes, deleted_nodes, new_arcs, deleted_arcs, printActions);

            //std::cout << partitionCrossingFIFOs[0]->getFullyQualifiedName() << " [ID:" << partitionCrossingFIFOs[0]->getId() << "] " << " Port 0 Output Arcs: " << partitionCrossingFIFOs[0]->getOutputPortCreateIfNot(0)->getArcs().size() << " Dst: " << (*partitionCrossingFIFOs[0]->getOutputPortCreateIfNot(0)->getArcs().begin())->getDstPort()->getParent()->getName() << std::endl;

            //Absorb FIFOs into FIFO0
            for(int i = 1; i<partitionCrossingFIFOs.size(); i++){
                //Trim FIFO initial conditions to minimum of set
                reshapeFIFOInitialConditionsToSize(partitionCrossingFIFOs[i], minInitConds, new_nodes, deleted_nodes, new_arcs, deleted_arcs, printActions);

                //Rewire the input and output ports (which come in pairs)
                for(int portNum = 0; portNum<partitionCrossingFIFOs[i]->getInputPorts().size(); portNum++){
                    int newPortNum = partitionCrossingFIFOs[0]->getInputPorts().size(); //Since goes from 0 to size-1, size is the next port number

                    //Transfer initial conditions
                    partitionCrossingFIFOs[0]->setInitConditionsCreateIfNot(newPortNum, partitionCrossingFIFOs[i]->getInitConditionsCreateIfNot(portNum));

                    //Rewire inputs
                    std::shared_ptr<InputPort> newInputPort = partitionCrossingFIFOs[0]->getInputPortCreateIfNot(newPortNum);
                    std::set<std::shared_ptr<Arc>> inputArcs = partitionCrossingFIFOs[i]->getInputPort(portNum)->getArcs();
                    for(auto arcIt = inputArcs.begin(); arcIt != inputArcs.end(); arcIt++){
                        (*arcIt)->setDstPortUpdateNewUpdatePrev(newInputPort);
                    }

                    //Rewire outputs
                    std::shared_ptr<OutputPort> newOutputPort = partitionCrossingFIFOs[0]->getOutputPortCreateIfNot(newPortNum);
                    std::set<std::shared_ptr<Arc>> outputArcs = partitionCrossingFIFOs[i]->getOutputPort(portNum)->getArcs();
                    for(auto arcIt = outputArcs.begin(); arcIt != outputArcs.end(); arcIt++){
                        (*arcIt)->setSrcPortUpdateNewUpdatePrev(newOutputPort);
                    }
                }

                //Rewire Order Constraint Inputs
                std::set<std::shared_ptr<Arc>> orderConstraintInputArcs = partitionCrossingFIFOs[i]->getOrderConstraintInputArcs();
                for(auto arcIt = orderConstraintInputArcs.begin(); arcIt != orderConstraintInputArcs.end(); arcIt++){
                    (*arcIt)->setDstPortUpdateNewUpdatePrev(partitionCrossingFIFOs[0]->getOrderConstraintInputPortCreateIfNot());
                }

                //Rewire Order Constraint Outputs
                std::set<std::shared_ptr<Arc>> orderConstraintOutputArcs = partitionCrossingFIFOs[i]->getOrderConstraintOutputArcs();
                for(auto arcIt = orderConstraintOutputArcs.begin(); arcIt != orderConstraintOutputArcs.end(); arcIt++){
                    (*arcIt)->setSrcPortUpdateNewUpdatePrev(partitionCrossingFIFOs[0]->getOrderConstraintOutputPortCreateIfNot());
                }

                if(printActions){
                    std::cout << "FIFO Merged: " << partitionCrossingFIFOs[i]->getFullyQualifiedName() << " [ID:" << partitionCrossingFIFOs[i]->getId() << "]"
                              << " into " << partitionCrossingFIFOs[0]->getFullyQualifiedName() << " [ID:" << partitionCrossingFIFOs[0]->getId() << "]" << std::endl;
                }

                //Mark FIFO for deletion
                deleted_nodes.push_back(partitionCrossingFIFOs[i]);
            }

            //std::cout << partitionCrossingFIFOs[0]->getFullyQualifiedName() << " [ID:" << partitionCrossingFIFOs[0]->getId() << "] " << " Port 0 Output Arcs: " << partitionCrossingFIFOs[0]->getOutputPortCreateIfNot(0)->getArcs().size() << std::endl;

            //Only FIFO0 is left
            newMap[crossing] = {partitionCrossingFIFOs[0]};
        }else{
            //Nothing changed, return the unchanged result
            newMap[crossing] = partitionCrossingFIFOs;
        }
    }
    return newMap;
}

void EmitterHelpers::findPartitionInputAndOutputFIFOs(
        std::map<std::pair<int, int>, std::vector<std::shared_ptr<ThreadCrossingFIFO>>> fifoMap, int partitionNum,
        std::vector<std::shared_ptr<ThreadCrossingFIFO>> &inputFIFOs, std::vector<std::shared_ptr<ThreadCrossingFIFO>> &outputFIFOs) {

    //TODO: Optimize

    for(auto it = fifoMap.begin(); it != fifoMap.end(); it++){
        if(it->first.first == partitionNum && it->first.second == partitionNum){
            throw std::runtime_error(ErrorHelpers::genErrorStr("Found a partition crossing map entry from a partition to itself"));
        }

        if(it->first.first == partitionNum){
            //This is an outgoing FIFO
            outputFIFOs.insert(outputFIFOs.end(), it->second.begin(), it->second.end());
        }else if(it->first.second == partitionNum){
            //This is an incoming FIFO
            inputFIFOs.insert(inputFIFOs.end(), it->second.begin(), it->second.end());
        }
    }
}

std::vector<std::shared_ptr<Node>> EmitterHelpers::findNodesWithState(std::vector<std::shared_ptr<Node>> &nodesToSearch) {
    std::vector<std::shared_ptr<Node>> nodesWithState;

    for(unsigned long i = 0; i<nodesToSearch.size(); i++) {
        if (nodesToSearch[i]->hasState()) {
            nodesWithState.push_back(nodesToSearch[i]);
        }
    }

    return nodesWithState;
}

std::vector<std::shared_ptr<Node>> EmitterHelpers::findNodesWithGlobalDecl(std::vector<std::shared_ptr<Node>> &nodesToSearch) {
    std::vector<std::shared_ptr<Node>> nodesWithGlobalDecl;

    for(unsigned long i = 0; i<nodesToSearch.size(); i++){
        if(nodesToSearch[i]->hasGlobalDecl()){
            nodesWithGlobalDecl.push_back(nodesToSearch[i]);
        }
    }

    return nodesWithGlobalDecl;
}

std::vector<std::shared_ptr<ContextRoot>> EmitterHelpers::findContextRoots(std::vector<std::shared_ptr<Node>> &nodesToSearch) {
    std::vector<std::shared_ptr<ContextRoot>> contextRoots;

    for(unsigned long i = 0; i<nodesToSearch.size(); i++){
        std::shared_ptr<ContextRoot> nodeAsContextRoot = GeneralHelper::isType<Node, ContextRoot>(nodesToSearch[i]);

        if(nodeAsContextRoot){
            contextRoots.push_back(nodeAsContextRoot);
        }
    }

    return contextRoots;
}

std::vector<std::shared_ptr<BlackBox>> EmitterHelpers::findBlackBoxes(std::vector<std::shared_ptr<Node>> &nodesToSearch){
    std::vector<std::shared_ptr<BlackBox>> blackBoxes;

    for(unsigned long i = 0; i<nodesToSearch.size(); i++){
        std::shared_ptr<BlackBox> nodeAsBlackBox = GeneralHelper::isType<Node, BlackBox>(nodesToSearch[i]);

        if(nodeAsBlackBox){
            blackBoxes.push_back(nodeAsBlackBox);
        }
    }

    return blackBoxes;
}

std::string EmitterHelpers::emitCopyCThreadArgs(std::vector<std::shared_ptr<ThreadCrossingFIFO>> inputFIFOs, std::vector<std::shared_ptr<ThreadCrossingFIFO>> outputFIFOs, std::string structName, std::string structTypeName){
    std::string statements;
    //Cast the input structure
    std::string castStructName = structName + "_cast";
    statements += structTypeName + " *" + castStructName + " = (" + structTypeName + "*) " + structName + ";\n";

    statements += "//Input FIFOs\n";
    for(unsigned long i = 0; i<inputFIFOs.size(); i++){
        std::vector<std::pair<Variable, std::string>> fifoSharedVars = inputFIFOs[i]->getFIFOSharedVariables();

        for(int j = 0; j<fifoSharedVars.size(); j++){
            //All should be pointers
            Variable var = fifoSharedVars[j].first;
            std::string structName = fifoSharedVars[j].second;
//            DataType varType = var.getDataType();
//            varType.setWidth(varType.getWidth()*blockSize);
//            var.setDataType(varType);

            //We handle volatile seperatly here (because of struct name), set to false for getCPtrDecl
            bool isVolatile = var.isVolatileVar();
            var.setVolatileVar(false);

            //Check if complex
            statements += (isVolatile ? "volatile " : "") + (structName.empty() ? var.getCPtrDecl(false) : inputFIFOs[i]->getFIFOStructTypeName() + "* " + var.getCVarName(false)) + " = " + castStructName + "->" + var.getCVarName(false) + ";\n";
        }
    }

    statements += "//Output FIFOs\n";
    for(unsigned long i = 0; i<outputFIFOs.size(); i++){
        std::vector<std::pair<Variable, std::string>> fifoSharedVars = outputFIFOs[i]->getFIFOSharedVariables();

        for(int j = 0; j<fifoSharedVars.size(); j++){
            //All should be pointers
            Variable var = fifoSharedVars[j].first;
            std::string structName = fifoSharedVars[j].second;
//            DataType varType = var.getDataType();
//            varType.setWidth(varType.getWidth()*blockSize);
//            var.setDataType(varType);
            //We handle volatile seperatly here (because of struct name), set to false for getCPtrDecl
            bool isVolatile = var.isVolatileVar();
            var.setVolatileVar(false);

            std::string outputFIFOStatement = (isVolatile ? "volatile " : "") + (structName.empty() ? var.getCPtrDecl(false) : outputFIFOs[i]->getFIFOStructTypeName() + "* " + var.getCVarName(false)) + " = " + castStructName + "->" + var.getCVarName(false) + ";\n";
            statements += outputFIFOStatement;
            //statements += outputFIFOs[i]->getFIFOStructTypeName() + "* " +  var.getCVarName(false) + " = " + castStructName + "->" + var.getCVarName(false) + ";\n";
        }
    }

    return statements;
}

std::string EmitterHelpers::emitFIFOChecks(std::vector<std::shared_ptr<ThreadCrossingFIFO>> fifos, bool checkFull, std::string checkVarName, bool shortCircuit, bool blocking, bool includeThreadCancelCheck){
    //Began work on version which replicates context check below.  Requires context check to be replicated
    //This version simply checks

    //TODO: Introduce a mode where each FIFO has an indevidual while loop.  Another Blocking mode

    //Now, emit the beginning of the check
    std::string check = "";
    if(blocking) {
        check += "bool " + checkVarName +" = false;\n";
        check += "while(!" + checkVarName + "){\n";
        check += checkVarName + " =  true;\n";
    }else{
        check += "bool " + checkVarName +" = true;\n";
    }

    if(includeThreadCancelCheck){
        check += "pthread_testcancel();\n";
    }

    //Emit the actual FIFO checks
    for(int i = 0; i<fifos.size(); i++) {
        //Note: do not need to check if complex since complex values come via the same FIFO as a struct
        check += checkVarName + " &= " + (checkFull ? fifos[i]->emitCIsNotFull() : fifos[i]->emitCIsNotEmpty()) + ";\n";
        if(shortCircuit && blocking){
            check += "if(!" + checkVarName + "){\n";
            check += "continue;\n";
            check += "}\n";
        }else if(shortCircuit && i < (fifos.size()-1)){ //Short circuit and not blocking (and not the last FIFO)
            //If still true (ie. all fifos up to this point have been ready) check the next one.  Otherwise, no need to check the rest.
            check += "if(" + checkVarName + "){\n";
        }
    }

    if(shortCircuit && !blocking){
        //Close conditions
        for(int i = 0; i<(fifos.size()-1); i++) {
            check += "}\n";
        }
    }

    //Close the check
    if(blocking) {
        check += "}\n";
    }

    return check;
}

//Version with scheduling
//std::string EmitterHelpers::emitFIFOChecks(std::vector<std::shared_ptr<ThreadCrossingFIFO>> fifos, int partition, bool checkFull, std::string checkVarName, bool shortCircuit){
//    //create a map of nodes to parents
//
//    std::set<std::shared_ptr<Node>> fifoSet;
//    fifoSet.insert(fifos.begin(), fifos.end());
//
//    std::map<std::shared_ptr<ThreadCrossingFIFO>, std::shared_ptr<ThreadCrossingFIFO>> fifoParentMap;
//    std::map<std::shared_ptr<ThreadCrossingFIFO>, std::vector<std::shared_ptr<ThreadCrossingFIFO>>> fifoChildrenMap;
//
//    for(int i = 0; i<fifos.size(); i++){
//        std::vector<Context> fifoContext = fifos[i]->getContext(); //FIFO should be in the context of the source
//        if(fifoContext.empty()){
//            fifoParentMap[fifos[i]] = nullptr;
//        }else{
//            Context lowestLevelContext = fifoContext[fifoContext.size()-1];
//
//            //find the driver of the lowest level context
//            std::map<int, std::vector<std::shared_ptr<Arc>>> driversPerPartition = lowestLevelContext.getContextRoot()->getContextDriversPerPartition();
//
//            if(driversPerPartition[partition].empty()){
//                throw std::runtime_error(ErrorHelpers::genErrorStr("Found a FIFO which did not have a driver for partition" + GeneralHelper::to_string(partition), fifos[i]));
//            }
//
//            //Get one of the drivers, they should all have the same source
//            std::shared_ptr<Node> driverNode =  driversPerPartition[partition][0]->getSrcPort()->getParent();
//
//            if(fifoSet.find(driverNode) == fifoSet.end()){
//                throw std::runtime_error(ErrorHelpers::genErrorStr("Found a FIFO whose driver is not another FIFO into the partition" + GeneralHelper::to_string(partition), fifos[i]));
//            }
//
//            std::shared_ptr<ThreadCrossingFIFO> driverFIFO = std::dynamic_pointer_cast<ThreadCrossingFIFO>(driverNode);
//            fifoParentMap[fifos[i]] = driverFIFO;
//            fifoChildrenMap[driverFIFO].push_back(fifos[i]);
//        }
//    }
//
//    std::set<std::shared_ptr<ThreadCrossingFIFO>> traversed;
//    //This should form a tree
//
//    //Now, emit the beginning of the check
//    std::string check = "bool " + checkVarName +" = false;\n";
//    check += "while(!" + checkVarName + "){\n";
//    check += "\t" + checkVarName + " =  true;\n";
//
//    //Emit the actual FIFO checks
//    DFS traversal replicating context checks
//
//    //Close the check
//    check += "}\n";
//}

std::vector<std::string> EmitterHelpers::createFIFOReadTemps(std::vector<std::shared_ptr<ThreadCrossingFIFO>> fifos){
    std::vector<std::string> exprs;
    for(int i = 0; i<fifos.size(); i++) {
        std::string tmpName = fifos[i]->getName() + "_readTmp";
        std::string structType = fifos[i]->getFIFOStructTypeName();
        exprs.push_back(structType + " " + tmpName + ";");
    }

    return exprs;
}

std::vector<std::string> EmitterHelpers::createFIFOWriteTemps(std::vector<std::shared_ptr<ThreadCrossingFIFO>> fifos){
    std::vector<std::string> exprs;
    for(int i = 0; i<fifos.size(); i++) {
        std::string tmpName = fifos[i]->getName() + "_writeTmp";
        std::string structType = fifos[i]->getFIFOStructTypeName();
        exprs.push_back(structType + " " + tmpName + ";");
    }

    return exprs;
}

std::vector<std::string> EmitterHelpers::createAndInitializeFIFOWriteTemps(std::vector<std::shared_ptr<ThreadCrossingFIFO>> fifos, std::vector<std::vector<NumericValue>> defaultVal){
    std::vector<std::string> exprs;
    for(int i = 0; i<fifos.size(); i++) {
        std::string tmpName = fifos[i]->getName() + "_writeTmp";
        std::string structType = fifos[i]->getFIFOStructTypeName();
        exprs.push_back(structType + " " + tmpName + ";");

        int blockSize = fifos[i]->getBlockSize();

        for(int portNum = 0; portNum<fifos[i]->getInputPorts().size(); portNum++) {

            DataType dt = fifos[i]->getCStateVar(portNum).getDataType();
            int width = dt.getWidth();
            if (blockSize == 1 && width == 1) {
                exprs.push_back(tmpName + ".port" + GeneralHelper::to_string(portNum) + "_real = " + defaultVal[i][portNum].toStringComponent(false, dt) + ";");
                if (dt.isComplex()) {
                    exprs.push_back(tmpName + ".port" + GeneralHelper::to_string(portNum) + "_imag = " + defaultVal[i][portNum].toStringComponent(true, dt) + ";");
                }
            } else if ((blockSize == 1 && width > 1) || (blockSize > 1 && width == 1)) {
                //Cannot initialize with = {} because inside a structure
                int entries = std::max(blockSize, width);
                for (int j = 0; j < entries; j++) {
                    exprs.push_back(tmpName + ".port" + GeneralHelper::to_string(portNum) + "_real[" + GeneralHelper::to_string(j) + "] = " +
                                    defaultVal[i][portNum].toStringComponent(false, dt) + ";");
                }
                if (dt.isComplex()) {
                    for (int j = 0; j < entries; j++) {
                        exprs.push_back(tmpName + ".port" + GeneralHelper::to_string(portNum) + "_imag[" + GeneralHelper::to_string(j) + "] = " +
                                        defaultVal[i][portNum].toStringComponent(true, dt) + ";");
                    }
                }
            } else {
                for (int j = 0; j < blockSize; j++) {
                    for (int k = 0; k < width; k++) {
                        exprs.push_back(
                                tmpName + ".port" + GeneralHelper::to_string(portNum) + "_real[" + GeneralHelper::to_string(j) + "][" + GeneralHelper::to_string(k) +
                                "] = " + defaultVal[i][portNum].toStringComponent(false, dt) + ";");
                    }
                }

                if (dt.isComplex()) {
                    for (int j = 0; j < blockSize; j++) {
                        for (int k = 0; k < width; k++) {
                            exprs.push_back(tmpName + ".port" + GeneralHelper::to_string(portNum) + "_imag[" + GeneralHelper::to_string(j) + "][" +
                                            GeneralHelper::to_string(k) + "] = " +
                                            defaultVal[i][portNum].toStringComponent(true, dt) + ";");
                        }
                    }
                }
            }
        }
    }

    return exprs;
}

std::vector<std::string> EmitterHelpers::readFIFOsToTemps(std::vector<std::shared_ptr<ThreadCrossingFIFO>> fifos) {
    std::vector<std::string> exprs;

    for(int i = 0; i<fifos.size(); i++){
        std::string tmpName = fifos[i]->getName() + "_readTmp";
        //However, the type will be the FIFO structure rather than the variable type directly.
        //The fifo reads in terms of blocks with the components stored in a structure
        //When calling the function, the relavent component of the structure is passed as an argument

        fifos[i]->emitCReadFromFIFO(exprs, tmpName, 1);
    }

    return exprs;
}

std::vector<std::string> EmitterHelpers::writeFIFOsFromTemps(std::vector<std::shared_ptr<ThreadCrossingFIFO>> fifos) {
    std::vector<std::string> exprs;

    for(int i = 0; i<fifos.size(); i++){
        std::string tmpName = fifos[i]->getName() + "_writeTmp";
        //However, the type will be the FIFO structure rather than the variable type directly.
        //The fifo reads in terms of blocks with the components stored in a structure
        //When calling the function, the relavent component of the structure is passed as an argument

        fifos[i]->emitCWriteToFIFO(exprs, tmpName, 1);
    }

    return exprs;
}

void EmitterHelpers::propagatePartitionsFromSubsystemsToChildren(std::set<std::shared_ptr<Node>>& nodes, int partition){
    for(auto it = nodes.begin(); it != nodes.end(); it++){
        //Do this first since expanded nodes are also subsystems
        std::shared_ptr<ExpandedNode> asExpandedNode = GeneralHelper::isType<Node, ExpandedNode>(*it);
        if(asExpandedNode){
            //This is a special case where the subsystem takes the partition of the parent and not itself
            //This is because
            if(partition != -1) {
                asExpandedNode->getOrigNode()->setPartitionNum(partition);
                asExpandedNode->setPartitionNum(partition);
            }

            std::set<std::shared_ptr<Node>> children = asExpandedNode->getChildren();
            propagatePartitionsFromSubsystemsToChildren(children, partition); //still at same level as before with same partition
        }else{
            std::shared_ptr<SubSystem> asSubsystem = GeneralHelper::isType<Node, SubSystem>(*it);
            if(asSubsystem){
                int nextPartition = asSubsystem->getPartitionNum();
                if(nextPartition == -1){
                    nextPartition = partition;
                    asSubsystem->setPartitionNum(partition);
                }
                std::set<std::shared_ptr<Node>> children = asSubsystem->getChildren();
                propagatePartitionsFromSubsystemsToChildren(children, nextPartition);
            }else{
                //Standard node, set partition if !firstLevel
                if(partition != -1){
                    (*it)->setPartitionNum(partition);
                }
            }
        }
    }
}

std::pair<std::string, std::string> EmitterHelpers::getCThreadArgStructDefn(std::vector<std::shared_ptr<ThreadCrossingFIFO>> inputFIFOs, std::vector<std::shared_ptr<ThreadCrossingFIFO>> outputFIFOs, std::string designName, int partitionNum){
    std::string structureType = designName + "_partition" + (partitionNum >= 0?GeneralHelper::to_string(partitionNum):"N"+GeneralHelper::to_string(-partitionNum)) + "_threadArgs_t";
    std::string prototype = "typedef struct {\n";

    prototype += "//Input FIFOs\n";
    for(unsigned long i = 0; i<inputFIFOs.size(); i++){
        std::vector<std::pair<Variable, std::string>> fifoSharedVars = inputFIFOs[i]->getFIFOSharedVariables();

        for(int j = 0; j<fifoSharedVars.size(); j++){
            //All should be pointers
            Variable var = fifoSharedVars[j].first;
            std::string structName = fifoSharedVars[j].second;
//            DataType varType = var.getDataType();
//            varType.setWidth(varType.getWidth()*blockSize);
//            var.setDataType(varType);
            //Pass as not volatile
            var.setVolatileVar(false);

            //prototype += "\t" + inputFIFOs[i]->getFIFOStructTypeName() + "* " + var.getCVarName(false) +  + ";\n";
            prototype += (structName.empty() ? var.getCPtrDecl(false) : inputFIFOs[i]->getFIFOStructTypeName() + "* " + var.getCVarName(false)) + ";\n";

        }
    }

    prototype += "//Output FIFOs\n";
    for(unsigned long i = 0; i<outputFIFOs.size(); i++){
        std::vector<std::pair<Variable, std::string>> fifoSharedVars = outputFIFOs[i]->getFIFOSharedVariables();

        for(int j = 0; j<fifoSharedVars.size(); j++){
            //All should be pointers
            Variable var = fifoSharedVars[j].first;
            std::string structName = fifoSharedVars[j].second;
//            DataType varType = var.getDataType();
//            varType.setWidth(varType.getWidth()*blockSize);
//            var.setDataType(varType);
            //Pass as not volatile
            var.setVolatileVar(false);

            //prototype += "\t" + outputFIFOs[i]->getFIFOStructTypeName() + "* " + var.getCVarName(false) +  + ";\n";
            prototype += (structName.empty() ? var.getCPtrDecl(false) : outputFIFOs[i]->getFIFOStructTypeName() + "* " + var.getCVarName(false)) + ";\n";
        }
    }

    prototype += "} " + structureType + ";";

    return std::pair<std::string, std::string> (prototype, structureType);
}

std::string EmitterHelpers::emitFIFOStructHeader(std::string path, std::string fileNamePrefix, std::vector<std::shared_ptr<ThreadCrossingFIFO>> fifos) {
    std::string fileName = fileNamePrefix + "_fifoTypes";
    std::cout << "Emitting C File: " << path << "/" << fileName << ".h" << std::endl;
    //#### Emit .h file ####
    std::ofstream headerFile;
    headerFile.open(path + "/" + fileName + ".h", std::ofstream::out | std::ofstream::trunc);

    std::string fileNameUpper =  GeneralHelper::toUpper(fileName);
    headerFile << "#ifndef " << fileNameUpper << "_H" << std::endl;
    headerFile << "#define " << fileNameUpper << "_H" << std::endl;
    headerFile << "#include <stdint.h>" << std::endl;
    headerFile << "#include <stdbool.h>" << std::endl;

    for(int i = 0; i<fifos.size(); i++){
        std::string fifoStruct = fifos[i]->createFIFOStruct();
        headerFile << fifoStruct << std::endl;
    }

    headerFile << "#endif" << std::endl;
    headerFile.close();

    return fileName+".h";
}

void EmitterHelpers::emitMultiThreadedBenchmarkKernel(std::map<std::pair<int, int>, std::vector<std::shared_ptr<ThreadCrossingFIFO>>> fifoMap,
                                              std::map<int, std::vector<std::shared_ptr<ThreadCrossingFIFO>>> inputFIFOMap,
                                              std::map<int, std::vector<std::shared_ptr<ThreadCrossingFIFO>>> outputFIFOMap, std::set<int> partitions,
                                              std::string path, std::string fileNamePrefix, std::string designName, std::string fifoHeaderFile,
                                              std::string ioBenchmarkSuffix, std::vector<int> partitionMap){
    std::string fileName = fileNamePrefix+"_"+ioBenchmarkSuffix+"_kernel";
    std::cout << "Emitting C File: " << path << "/" << fileName << ".h" << std::endl;
    //#### Emit .h file ####
    std::ofstream headerFile;
    headerFile.open(path+"/"+fileName+".h", std::ofstream::out | std::ofstream::trunc);

    std::string fileNameUpper =  GeneralHelper::toUpper(fileName);
    headerFile << "#ifndef " << fileNameUpper << "_H" << std::endl;
    headerFile << "#define " << fileNameUpper << "_H" << std::endl;
    headerFile << "#include <stdint.h>" << std::endl;
    headerFile << "#include <stdbool.h>" << std::endl;
    headerFile << "#include <stdlib.h>" << std::endl;
    headerFile << "#include <math.h>" << std::endl;
    headerFile << "#include <pthread.h>" << std::endl;
    headerFile << "#include <errno.h>" << std::endl;
    headerFile << "#include \"" << fifoHeaderFile << "\"" << std::endl;
    headerFile << std::endl;

    //Output the function prototype for the I/O thread function
    std::string fctnDecl = "void " + designName + "_" + ioBenchmarkSuffix + "_kernel()";
    headerFile << std::endl;
    headerFile << fctnDecl << ";" << std::endl;
    headerFile << "#endif" << std::endl;

    //#### Emit .c File ####
    std::cout << "Emitting C File: " << path << "/" << fileName << ".c" << std::endl;
    std::ofstream cFile;
    cFile.open(path+"/"+fileName+".c", std::ofstream::out | std::ofstream::trunc);

    //Include other generated headers
    cFile << "//Need _GNU_SOURCE, sched.h, and unistd.h for setting thread affinity in Linux" << std::endl; //Linux scheduler source for setting thread affinity (C++ did not complain not having this but C does)
    cFile << "#define _GNU_SOURCE" << std::endl;
    cFile << "#include <unistd.h>" << std::endl;
    cFile << "#include <sched.h>" << std::endl;
    cFile << "#include <stdio.h>" << std::endl;
    cFile << "#include <errno.h>" << std::endl;
    cFile << "#include \"" << fileName << ".h" << "\"" << std::endl;
    //Include other thread headers
    for(auto it = partitions.begin(); it!=partitions.end(); it++){
        if(*it != IO_PARTITION_NUM){
            std::string partitionFileName = fileNamePrefix+"_partition"+GeneralHelper::to_string(*it);
            cFile << "#include \"" << partitionFileName << ".h" << "\"" << std::endl;
        }
    }
    //Include I/O thread header
    std::string ioThreadFile = fileNamePrefix+"_" + ioBenchmarkSuffix;
    cFile << "#include \"" << ioThreadFile << ".h" << "\"" << std::endl;

    cFile << std::endl;
    cFile << fctnDecl << "{" << std::endl;

    cFile << "//Reset" << std::endl;
    for(auto it = partitions.begin(); it != partitions.end(); it++){
        if(*it != IO_PARTITION_NUM){
            cFile << designName + "_partition" << *it << "_reset();" << std::endl;
        }
    }

    cFile << "//Allocate and Initialize FIFO Shared Variables" << std::endl;

    //For each FIFO, allocate the shared arrays
    for(auto it = fifoMap.begin(); it != fifoMap.end(); it++){
        std::vector<std::shared_ptr<ThreadCrossingFIFO>> fifos = it->second;

        std::vector<std::string> statements;
        for(int i = 0; i<fifos.size(); i++){
            fifos[i]->createSharedVariables(statements);
            fifos[i]->initializeSharedVariables(statements);
        }

        for(int i = 0; i<statements.size(); i++){
            cFile << statements[i] << std::endl;
        }
    }

    cFile << std::endl;

    //Create Argument structures for threads
    cFile << "//Create Thread Arguments" << std::endl;
    for(auto it = partitions.begin(); it != partitions.end(); it++){
        //Including the I/O thread
        int currentPartition = *it;
        cFile << "//Partition " << currentPartition << " Arguments" << std::endl;
        std::string threadArgStructName = designName + "_partition" + (currentPartition < 0 ? "N" + GeneralHelper::to_string(-currentPartition) : GeneralHelper::to_string(currentPartition)) + "_threadArgs";
        std::string threadArgStructType = threadArgStructName + "_t";
        cFile << threadArgStructType << " " << threadArgStructName << ";" << std::endl;
        //Set pointers
        std::vector<std::shared_ptr<ThreadCrossingFIFO>> fifos = inputFIFOMap[*it];
        std::vector<std::shared_ptr<ThreadCrossingFIFO>> outputFIFOs = outputFIFOMap[*it];
        fifos.insert(fifos.end(), outputFIFOs.begin(), outputFIFOs.end());

        for(int i = 0; i<fifos.size(); i++){
            std::vector<std::pair<Variable, std::string>> fifoVars = fifos[i]->getFIFOSharedVariables();
            for(int j = 0; j<fifoVars.size(); j++){
                //All should be pointers
                Variable var = fifoVars[j].first;
                std::string structName = fifoVars[j].second;

                //No need to actually check for the struct type name here since this is simply assigning the pointer

                cFile << threadArgStructName << "." << var.getCVarName(false) << " = " << var.getCVarName(false)
                      << ";" << std::endl;
            }
        }
    }

    //Create Thread Parameters
    cFile << std::endl;
    cFile << "//Create Thread Parameters" << std::endl;
    cFile << "int status;" << std::endl;
    for(auto it = partitions.begin(); it != partitions.end(); it++){
        std::string partitionSuffix = (*it < 0 ? "N" + GeneralHelper::to_string(-*it) : GeneralHelper::to_string(*it));
        cFile << "cpu_set_t cpuset_" << partitionSuffix << ";" << std::endl;
        cFile << "pthread_t thread_" << partitionSuffix << ";" << std::endl;
        cFile << "pthread_attr_t attr_" << partitionSuffix << ";" << std::endl;
        //cFile << "void *res_" << *it << ";" << std::endl;
        cFile << std::endl;
        cFile << "status = pthread_attr_init(&attr_" << partitionSuffix << ");" << std::endl;
        cFile << "if(status != 0)" << std::endl;
        cFile << "{" << std::endl;
        cFile << "printf(\"Could not create pthread attributes ... exiting\");" << std::endl;
        cFile << "exit(1);" << std::endl;
        cFile << "}" << std::endl;
        cFile << std::endl;
        cFile << "CPU_ZERO(&cpuset_" << partitionSuffix << "); //Clear cpuset" << std::endl;
        int core = *it;
        if(partitionMap.empty()) {
            //Default case.  Assign I/O thread to CPU0 and each thread on the
            if (*it == IO_PARTITION_NUM) {
                core = 0;
                std::cout << "Setting I/O thread to run on CPU" << core << std::endl;
            } else {
                if (core < 0) {
                    throw std::runtime_error(ErrorHelpers::genErrorStr(
                            "Partition Requested Core " + GeneralHelper::to_string(core) + " which is not valid"));
//                    std::cerr << "Warning! Partition Requested Core " << core << " which is not valid.  Replacing with CPU0" << std::endl;
//                    core = 0;
                }

                std::cout << "Setting Partition " << *it <<  " thread to run on CPU" << core << std::endl;
            }
        }else{
            //Use the partition map
            if (*it == IO_PARTITION_NUM) {
                core = partitionMap[0]; //Is always the first element and the array is not empty
                std::cout << "Setting I/O thread to run on CPU" << core << std::endl;
            }else{
                if(*it < 0 || *it >= partitionMap.size()-1){
                    throw std::runtime_error(ErrorHelpers::genErrorStr("The partition map does not contain an entry for partition " + GeneralHelper::to_string(*it)));
                }

                core = partitionMap[*it+1];
                std::cout << "Setting Partition " << *it << " thread to run on CPU" << core << std::endl;
            }

        }
        cFile << "CPU_SET(" << core << ", &cpuset_" << partitionSuffix << "); //Add CPU to cpuset" << std::endl;
        cFile << "status = pthread_attr_setaffinity_np(&attr_" << partitionSuffix << ", sizeof(cpu_set_t), &cpuset_" << partitionSuffix
              << ");//Set thread CPU affinity" << std::endl;
        cFile << "if(status != 0)" << std::endl;
        cFile << "{" << std::endl;
        cFile << "printf(\"Could not set thread core affinity ... exiting\");" << std::endl;
        cFile << "exit(1);" << std::endl;
        cFile << "}" << std::endl;
        cFile << std::endl;
    }

    //Start all threads except the I/O thread
    cFile << std::endl;
    cFile << "//Start Threads" << std::endl;
    for(auto it = partitions.begin(); it != partitions.end(); it++){
        if(*it != IO_PARTITION_NUM) {
            std::string threadFun = designName + "_partition" + GeneralHelper::to_string(*it) + "_thread";
            std::string threadArgStructName = designName + "_partition" + GeneralHelper::to_string(*it) + "_threadArgs";
            cFile << "status = pthread_create(&thread_" << *it << ", &attr_" << *it << ", " << threadFun << ", &" << threadArgStructName << ");" << std::endl;
            cFile << "if(status != 0)" << std::endl;
            cFile << "{" << std::endl;
            cFile << "printf(\"Could not create a thread ... exiting\");" << std::endl;
            cFile << "errno = status;" << std::endl;
            cFile << "perror(NULL);" << std::endl;
            cFile << "exit(1);" << std::endl;
            cFile << "}" << std::endl;
        }
    }

    //Start I/O thread
    std::string partitionSuffix = (IO_PARTITION_NUM < 0 ? "N" + GeneralHelper::to_string(-IO_PARTITION_NUM) : GeneralHelper::to_string(IO_PARTITION_NUM));
    std::string threadFun = designName + "_" + ioBenchmarkSuffix + "_thread";
    std::string threadArgStructName = designName + "_partition" + partitionSuffix + "_threadArgs";
    cFile << "status = pthread_create(&thread_" << partitionSuffix << ", &attr_" << partitionSuffix << ", " << threadFun << ", &" << threadArgStructName << ");" << std::endl;
    cFile << "if(status != 0)" << std::endl;
    cFile << "{" << std::endl;
    cFile << "printf(\"Could not create a thread ... exiting\");" << std::endl;
    cFile << "errno = status;" << std::endl;
    cFile << "perror(NULL);" << std::endl;
    cFile << "exit(1);" << std::endl;
    cFile << "}" << std::endl;

    //Join on the I/O thread
    cFile << std::endl;
    cFile << "//Wait for I/O Thread to Finish" << std::endl;
    cFile << "void *res_" << partitionSuffix << ";" << std::endl;
    cFile << "status = pthread_join(thread_" << partitionSuffix << ", &res_" << partitionSuffix << ");" << std::endl;
    cFile << "if(status != 0)" << std::endl;
    cFile << "{" << std::endl;
    cFile << "printf(\"Could not join a thread ... exiting\");" << std::endl;
    cFile << "errno = status;" << std::endl;
    cFile << "perror(NULL);" << std::endl;
    cFile << "exit(1);" << std::endl;
    cFile << "}" << std::endl;

    //Cancel the other threads
    cFile << std::endl;
    cFile << "//Cancel Other Threads" << std::endl;
    for(auto it = partitions.begin(); it != partitions.end(); it++){
        if(*it != IO_PARTITION_NUM) {
            cFile << "status = pthread_cancel(thread_" << *it << ");" << std::endl;
            cFile << "if(status != 0)" << std::endl;
            cFile << "{" << std::endl;
            cFile << "printf(\"Could not cancel a thread ... exiting\");" << std::endl;
            cFile << "errno = status;" << std::endl;
            cFile << "perror(NULL);" << std::endl;
            cFile << "exit(1);" << std::endl;
            cFile << "}" << std::endl;
        }
    }

    cFile << "}" << std::endl;
}

void EmitterHelpers::emitMultiThreadedDriver(std::string path, std::string fileNamePrefix, std::string designName, int blockSize, std::string ioBenchmarkSuffix, std::vector<Variable> inputVars){
    //#### Emit Driver File ####
    std::string kernelFileName = fileNamePrefix+"_"+ioBenchmarkSuffix+"_kernel";
    std::string fileName = fileNamePrefix+"_"+ioBenchmarkSuffix+"_driver";
    std::cout << "Emitting C++ File: " << path << "/" << fileName << ".h" << std::endl;
    std::ofstream benchDriver;
    benchDriver.open(path+"/"+fileName+".cpp", std::ofstream::out | std::ofstream::trunc);

    benchDriver << "#include <map>" << std::endl;
    benchDriver << "#include <string>" << std::endl;
    benchDriver << "#include \"intrin_bench_default_defines.h\"" << std::endl;
    benchDriver << "#include \"benchmark_throughput_test.h\"" << std::endl;
    benchDriver << "#include \"kernel_runner.h\"" << std::endl;
    benchDriver << "extern \"C\"{" << std::endl;
    benchDriver << "#include \"" + kernelFileName + ".h\"" << std::endl;
    benchDriver << "}" << std::endl;


    //Driver will define a zero arg kernel that sets reasonable inputs and repeatedly runs the function.
    //The function will be compiled in a separate object file and should not be in-lined (potentially resulting in erroneous
    //optimizations for the purpose of benchmarking since we are feeding dummy data in).  This should be the case so long as
    //the compiler flag -flto is not used during compile and linking (https://stackoverflow.com/questions/35922966/lto-with-llvm-and-cmake)
    //(https://llvm.org/docs/LinkTimeOptimization.html), (https://clang.llvm.org/docs/CommandGuide/clang.html),
    //(https://gcc.gnu.org/wiki/LinkTimeOptimization), (https://gcc.gnu.org/onlinedocs/gccint/LTO-Overview.html).

    //Emit name, file, and units string
    benchDriver << "std::string getBenchSuiteName(){\n\treturn \"Generated System: " + designName + "\";\n}" << std::endl;
    benchDriver << "std::string getReportFileName(){\n\treturn \"" + fileName + "_" + ioBenchmarkSuffix + "_benchmarking_report\";\n}" << std::endl;
    benchDriver << "std::string getReportUnitsName(){\n\treturn \"STIM_LEN: \" + std::to_string(STIM_LEN) + \" (Samples/Vector/Trial), TRIALS: \" + std::to_string(TRIALS);\n}" << std::endl;

    //Emit Benchmark Report Selection
    benchDriver << "void getBenchmarksToReport(std::vector<std::string> &kernels, std::vector<std::string> &vec_ext){\n"
                   "\tkernels.push_back(\"" + designName + "\");\tvec_ext.push_back(\"Multi-Threaded (" + ioBenchmarkSuffix + ")\");\n}" << std::endl;

    //Emit Benchmark Type Report Selection
    std::string typeStr = "";
    int numInputVars = inputVars.size();
    if(numInputVars > 0){
        typeStr = inputVars[0].getDataType().toString(DataType::StringStyle::C);
        typeStr += inputVars[0].getDataType().isComplex() ? " (c)" : " (r)";
    }

    for(unsigned long i = 1; i<numInputVars; i++){
        typeStr += ", " + inputVars[i].getDataType().toString(DataType::StringStyle::C);
        typeStr += inputVars[i].getDataType().isComplex() ? " (c)" : " (r)";
    }

    benchDriver << "std::vector<std::string> getVarientsToReport(){\n"
                   "\tstd::vector<std::string> types;\n"
                   "\ttypes.push_back(\"" + typeStr + "\");\n"
                                                      "\treturn types;\n}" << std::endl;

    //Generate call to loop
    std::string kernelFctnName = designName + "_" + ioBenchmarkSuffix + "_kernel";
    benchDriver << "std::map<std::string, std::map<std::string, Results*>*> runBenchSuite(Profiler* profiler, int* cpu_num_int){\n"
                   "\tstd::map<std::string, std::map<std::string, Results*>*> kernel_results;\n"
                   "\n"
                   "\tstd::map<std::string, Results*>* type_result = new std::map<std::string, Results*>;\n"
                   "\tResults* result = zero_arg_kernel(profiler, &" + kernelFctnName + ", *cpu_num_int, \"===== Generated System: " + designName + " =====\");\n"
                   "\t(*type_result)[\"" + typeStr + "\"] = result;\n"
                   "\tkernel_results[\"" + designName + "\"] = type_result;\n"
                   "\tprintf(\"\\n\");\n"
                   "\treturn kernel_results;\n}" << std::endl;

    benchDriver << "void initInput(void* ptr, unsigned long index){}" << std::endl;

    benchDriver.close();
}

void EmitterHelpers::emitMultiThreadedMakefile(std::string path, std::string fileNamePrefix, std::string designName, int blockSize, std::set<int> partitions, std::string ioBenchmarkSuffix){
    //#### Emit Makefiles ####

    std::string systemSrcs = "";

    for(auto it = partitions.begin(); it != partitions.end(); it++){
        if(*it != IO_PARTITION_NUM) {
            std::string threadFileName = fileNamePrefix + "_partition" + GeneralHelper::to_string(*it) + ".c";
            systemSrcs += threadFileName + " ";
        }
    }
    std::string ioFileName = fileNamePrefix+"_"+ioBenchmarkSuffix;
    systemSrcs += ioFileName;

    std::string kernelFileName = fileNamePrefix+"_"+ioBenchmarkSuffix+"_kernel.c";
    std::string driverFileName = fileNamePrefix+"_"+ioBenchmarkSuffix+"_driver.cpp";

    std::string makefileContent =   "BUILD_DIR=build\n"
                                    "DEPENDS_DIR=./depends\n"
                                    "COMMON_DIR=./common\n"
                                    "SRC_DIR=./intrin\n"
                                    "LIB_DIR=.\n"
                                    "\n"
                                    "#Parameters that can be supplied to this makefile\n"
                                    "USE_PCM=1\n"
                                    "USE_AMDuPROF=1\n"
                                    "\n"
                                    "UNAME:=$(shell uname)\n"
                                    "\n"
                                    "#Compiler Parameters\n"
                                    "#CXX=g++\n"
                                    "#Main Benchmark file is not optomized to avoid timing code being re-organized\n"
                                    "CFLAGS = -O0 -c -g -std=c++11 -march=native -masm=att\n"
                                    "#Generated system should be allowed to optomize - reintepret as c++ file\n"
                                    "SYSTEM_CFLAGS = -O3 -c -g -std=c11 -march=native -masm=att\n"
                                    "#Most kernels are allowed to be optomized.  Most assembly kernels use asm 'volitile' to force execution\n"
                                    "KERNEL_CFLAGS = -O3 -c -g -std=c11 -march=native -masm=att\n"
                                    "#For kernels that should not be optimized, the following is used\n"
                                    "KERNEL_NO_OPT_CFLAGS = -O0 -c -g -std=c11 -march=native -masm=att\n"
                                    "INC=-I $(COMMON_DIR) -I $(SRC_DIR)\n"
                                    "LIB_DIRS=-L $(COMMON_DIR)\n"
                                    "LIB=-pthread -lProfilerCommon\n"
                                    "\n"
                                    "DEFINES=\n"
                                    "\n"
                                    "DEPENDS=\n"
                                    "DEPENDS_LIB=$(COMMON_DIR)/libProfilerCommon.a\n"
                                    "\n"
                                    "ifeq ($(USE_PCM), 1)\n"
                                    "DEFINES+= -DUSE_PCM=1\n"
                                    "DEPENDS+= $(DEPENDS_DIR)/pcm/Makefile\n"
                                    "DEPENDS_LIB+= $(DEPENDS_DIR)/pcm/libPCM.a\n"
                                    "INC+= -I $(DEPENDS_DIR)/pcm\n"
                                    "LIB_DIRS+= -L $(DEPENDS_DIR)/pcm\n"
                                    "#Need an additional include directory if on MacOS.\n"
                                    "#Using the technique in pcm makefile to detect MacOS\n"
                                    "UNAME:=$(shell uname)\n"
                                    "ifeq ($(UNAME), Darwin)\n"
                                    "INC+= -I $(DEPENDS_DIR)/pcm/MacMSRDriver\n"
                                    "LIB+= -lPCM -lPcmMsr\n"
                                    "APPLE_DRIVER = $(DEPENDS_DIR)/pcm/MacMSRDriver/build/Release/libPcmMsr.dylib\n"
                                    "LIB_DIRS+= -L $(DEPENDS_DIR)/pcm/MacMSRDriver/build/Release\n"
                                    "else\n"
                                    "LIB+= -lrt -lPCM\n"
                                    "APPLE_DRIVER = \n"
                                    "endif\n"
                                    "else\n"
                                    "DEFINES+= -DUSE_PCM=0\n"
                                    "endif\n"
                                    "\n"
                                    "ifeq ($(USE_AMDuPROF), 1)\n"
                                    "DEFINES+= -DUSE_AMDuPROF=1\n"
                                    "LIB+= -lAMDPowerProfileAPI\n"
                                    "else\n"
                                    "DEFINES+= -DUSE_AMDuPROF=0\n"
                                    "endif\n"
                                    "\n"
                                    "MAIN_FILE = benchmark_throughput_test.cpp\n"
                                    "LIB_SRCS = " + driverFileName + " #These files are not optimized. micro_bench calls the kernel runner (which starts the timers by calling functions in the profilers).  Re-ordering code is not desired\n"
                                    "SYSTEM_SRC = " + systemSrcs + ".c\n"
                                    "KERNEL_SRCS = " + kernelFileName + "\n"
                                    "KERNEL_NO_OPT_SRCS = \n"
                                    "\n"
                                    "SRCS=$(MAIN_FILE)\n"
                                    "OBJS=$(patsubst %.cpp,$(BUILD_DIR)/%.o,$(SRCS))\n"
                                    "LIB_OBJS=$(patsubst %.cpp,$(BUILD_DIR)/%.o,$(LIB_SRCS))\n"
                                    "SYSTEM_OBJS=$(patsubst %.c,$(BUILD_DIR)/%.o,$(SYSTEM_SRC))\n"
                                    "KERNEL_OBJS=$(patsubst %.c,$(BUILD_DIR)/%.o,$(KERNEL_SRCS))\n"
                                    "KERNEL_NO_OPT_OBJS=$(patsubst %.c,$(BUILD_DIR)/%.o,$(KERNEL_NO_OPT_SRCS))\n"
                                    "\n"
                                    "#Production\n"
                                    "all: benchmark_" + fileNamePrefix + "_" + ioBenchmarkSuffix + "\n"
                                    "\n"
                                    "benchmark_" + fileNamePrefix + "_" + ioBenchmarkSuffix + ": $(OBJS) $(SYSTEM_OBJS) $(LIB_OBJS) $(KERNEL_OBJS) $(KERNEL_NO_OPT_OBJS) $(DEPENDS_LIB) \n"
                                    "\t$(CXX) $(INC) $(LIB_DIRS) -o benchmark_" + fileNamePrefix + "_" + ioBenchmarkSuffix + " $(OBJS) $(SYSTEM_OBJS) $(LIB_OBJS) $(KERNEL_OBJS) $(KERNEL_NO_OPT_OBJS) $(LIB)\n"
                                    "\n"
                                    "$(KERNEL_NO_OPT_OBJS): $(BUILD_DIR)/%.o : $(SRC_DIR)/%.c | $(BUILD_DIR)/ $(DEPENDS)\n"
                                    "\t$(CC) $(KERNEL_NO_OPT_CFLAGS) $(INC) $(DEFINES) -o $@ $<\n"
                                    "\n"
                                    "$(SYSTEM_OBJS): $(BUILD_DIR)/%.o : $(LIB_DIR)/%.c | $(BUILD_DIR)/ $(DEPENDS)\n"
                                    "\t$(CC) $(SYSTEM_CFLAGS) $(INC) $(DEFINES) -o $@ $<\n"
                                    "\n"
                                    "$(KERNEL_OBJS): $(BUILD_DIR)/%.o : $(LIB_DIR)/%.c | $(BUILD_DIR)/ $(DEPENDS)\n"
                                    "\t$(CC) $(KERNEL_CFLAGS) $(INC) $(DEFINES) -o $@ $<\n"
                                    "\n"
                                    "$(LIB_OBJS): $(BUILD_DIR)/%.o : $(LIB_DIR)/%.cpp | $(BUILD_DIR)/ $(DEPENDS)\n"
                                    "\t$(CXX) $(CFLAGS) $(INC) $(DEFINES) -o $@ $<\n"
                                    "\n"
                                    "$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp | $(BUILD_DIR)/ $(DEPENDS)\n"
                                    "\t$(CXX) $(CFLAGS) $(INC) $(DEFINES) -o $@ $<\n"
                                    "\n"
                                    "$(BUILD_DIR)/:\n"
                                    "\tmkdir -p $@\n"
                                    "\n"
                                    "$(DEPENDS_DIR)/pcm/Makefile:\n"
                                    "\tgit submodule update --init --recursive $(DEPENDS_DIR)/pcm\n"
                                    "\n"
                                    "$(DEPENDS_DIR)/pcm/libPCM.a: $(DEPENDS_DIR)/pcm/Makefile\n"
                                    "\tcd $(DEPENDS_DIR)/pcm; make libPCM.a\n"
                                    "\n"
                                    "$(COMMON_DIR)/libProfilerCommon.a:\n"
                                    "\tcd $(COMMON_DIR); make USE_PCM=$(USE_PCM) USE_AMDuPROF=$(USE_AMDuPROF)\n"
                                    "\t\n"
                                    "clean:\n"
                                    "\trm -f benchmark_" + fileNamePrefix + "_" + ioBenchmarkSuffix + "\n"
                                    "\trm -rf build/*\n"
                                    "\n"
                                    ".PHONY: clean\n";

    std::cout << "Emitting Makefile: " << path << "/Makefile_" << fileNamePrefix << "_" << ioBenchmarkSuffix << ".mk" << std::endl;
    std::ofstream makefile;
    makefile.open(path+"/Makefile_" + fileNamePrefix + "_" + ioBenchmarkSuffix + ".mk", std::ofstream::out | std::ofstream::trunc);
    makefile << makefileContent;
    makefile.close();
}

void EmitterHelpers::emitPartitionThreadC(int partitionNum, std::vector<std::shared_ptr<Node>> nodesToEmit, std::vector<std::shared_ptr<ThreadCrossingFIFO>> inputFIFOs, std::vector<std::shared_ptr<ThreadCrossingFIFO>> outputFIFOs, std::string path, std::string fileNamePrefix, std::string designName, SchedParams::SchedType schedType, std::shared_ptr<MasterOutput> outputMaster, unsigned long blockSize, std::string fifoHeaderFile, bool threadDebugPrint){
    std::string blockIndVar = "";

    if(blockSize > 1) {
        blockIndVar = "blkInd";
    }

    //Set the index variable in the input FIFOs
    for(int i = 0; i<inputFIFOs.size(); i++){
        inputFIFOs[i]->setCBlockIndexVarInputName(blockIndVar);
    }

    //Also need to set the index variable of the output FIFOs
    for(int i = 0; i<outputFIFOs.size(); i++){
        outputFIFOs[i]->setCBlockIndexVarOutputName(blockIndVar);
    }

    //Note: If the blockSize == 1, the function prototype can include scalar arguments.  If blockSize > 1, only pointer
    //types are allowed since multiple values are being passed

    //For thread functions, there is no output.  All values are passed as references (for scalars) or pointers (for arrays)

    std::string computeFctnProtoArgs = getPartitionComputeCFunctionArgPrototype(inputFIFOs, outputFIFOs, blockSize);
    std::string computeFctnName = designName + "_partition"+(partitionNum >= 0?GeneralHelper::to_string(partitionNum):"N"+GeneralHelper::to_string(-partitionNum)) + "_compute";
    std::string computeFctnProto = "void " + computeFctnName + "(" + computeFctnProtoArgs + ")";

    std::string fileName = fileNamePrefix+"_partition"+(partitionNum >= 0?GeneralHelper::to_string(partitionNum):"N"+GeneralHelper::to_string(-partitionNum));
    std::cout << "Emitting C File: " << path << "/" << fileName << ".h" << std::endl;
    //#### Emit .h file ####
    std::ofstream headerFile;
    headerFile.open(path+"/"+fileName+".h", std::ofstream::out | std::ofstream::trunc);

    std::string fileNameUpper =  GeneralHelper::toUpper(fileName);
    headerFile << "#ifndef " << fileNameUpper << "_H" << std::endl;
    headerFile << "#define " << fileNameUpper << "_H" << std::endl;
    headerFile << "#include <stdint.h>" << std::endl;
    headerFile << "#include <stdbool.h>" << std::endl;
    headerFile << "#include <math.h>" << std::endl;
    headerFile << "#include <pthread.h>" << std::endl;
    headerFile << "#include \"" << fifoHeaderFile << "\"" << std::endl;
    //headerFile << "#include <thread.h>" << std::endl;
    headerFile << std::endl;

    //Output the Function Definition
    headerFile << computeFctnProto << ";" << std::endl;
    headerFile << std::endl;

    //Create the threadFunction argument structure
    std::pair<std::string, std::string> threadArgStructAndTypeName = EmitterHelpers::getCThreadArgStructDefn(inputFIFOs, outputFIFOs, designName, partitionNum);
    std::string threadArgStruct = threadArgStructAndTypeName.first;
    std::string threadArgTypeName = threadArgStructAndTypeName.second;
    headerFile << threadArgStruct << std::endl;
    headerFile << std::endl;

    //Output the thread function definition
    std::string threadFctnDecl = "void* " + designName + "_partition" + (partitionNum >= 0?GeneralHelper::to_string(partitionNum):"N"+GeneralHelper::to_string(-partitionNum)) + "_thread(void *args)";
    headerFile << threadFctnDecl << ";" << std::endl;

    //Output the reset function definition
    headerFile << "void " << designName + "_partition" << partitionNum << "_reset();" << std::endl;
    headerFile << std::endl;

    //Find nodes with state & global decls
    std::vector<std::shared_ptr<Node>> nodesWithState = EmitterHelpers::findNodesWithState(nodesToEmit);
    std::vector<std::shared_ptr<Node>> nodesWithGlobalDecl = EmitterHelpers::findNodesWithGlobalDecl(nodesToEmit);
    unsigned long numNodes = nodesToEmit.size();

    headerFile << "//==== State Variable Definitions ====" << std::endl;
    //We also need to declare the state variables here as extern;

    //Emit Definition
    unsigned long nodesWithStateCount = nodesWithState.size();
    for(unsigned long i = 0; i<nodesWithStateCount; i++){
        std::vector<Variable> stateVars = nodesWithState[i]->getCStateVars();
        //Emit State Vars
        unsigned long numStateVars = stateVars.size();
        for(unsigned long j = 0; j<numStateVars; j++){
            //cFile << "_Thread_local static " << stateVars[j].getCVarDecl(false, true, true) << ";" << std::endl;
            headerFile << "extern " << stateVars[j].getCVarDecl(false, true, false, true) << ";" << std::endl;

            if(stateVars[j].getDataType().isComplex()){
                headerFile << "extern " << stateVars[j].getCVarDecl(true, true, false, true) << ";" << std::endl;
            }
        }
    }

    headerFile << std::endl;

    //Insert BlackBox Headers
    std::vector<std::shared_ptr<BlackBox>> blackBoxes = EmitterHelpers::findBlackBoxes(nodesToEmit);
    if(blackBoxes.size() > 0) {
        headerFile << "//==== BlackBox Headers ====" << std::endl;

        for(unsigned long i = 0; i<blackBoxes.size(); i++){
            headerFile << "//**** BEGIN BlackBox " << blackBoxes[i]->getFullyQualifiedName() << " Header ****" << std::endl;
            headerFile << blackBoxes[i]->getCppHeaderContent() << std::endl;
            headerFile << "//**** END BlackBox " << blackBoxes[i]->getFullyQualifiedName() << " Header ****" << std::endl;
        }
    }

    headerFile << "#endif" << std::endl;

    headerFile.close();

    std::cout << "Emitting C File: " << path << "/" << fileName << ".c" << std::endl;
    //#### Emit .c file ####
    std::ofstream cFile;
    cFile.open(path+"/"+fileName+".c", std::ofstream::out | std::ofstream::trunc);
    if(threadDebugPrint) {
        cFile << "#include <stdio.h>" << std::endl;
    }
    cFile << "#include \"" << fileName << ".h" << "\"" << std::endl;
    cFile << std::endl;

    //Find nodes with state & Emit state variable declarations
    cFile << "//==== Init State Vars ====" << std::endl;
    for(unsigned long i = 0; i<nodesWithStateCount; i++){
        std::vector<Variable> stateVars = nodesWithState[i]->getCStateVars();
        //Emit State Vars
        unsigned long numStateVars = stateVars.size();
        for(unsigned long j = 0; j<numStateVars; j++){
            //cFile << "_Thread_local static " << stateVars[j].getCVarDecl(false, true, true) << ";" << std::endl;
            cFile << stateVars[j].getCVarDecl(false, true, true, true) << ";" << std::endl;

            if(stateVars[j].getDataType().isComplex()){
                cFile << stateVars[j].getCVarDecl(true, true, true, true) << ";" << std::endl;
            }
        }
    }

    cFile << std::endl;

    cFile << "//==== Global Declarations ====" << std::endl;
    unsigned long nodesWithGlobalDeclCount = nodesWithGlobalDecl.size();
    for(unsigned long i = 0; i<nodesWithGlobalDeclCount; i++){
        cFile << nodesWithGlobalDecl[i]->getGlobalDecl() << std::endl;
    }

    cFile << std::endl;

    //Emit BlackBox C++ functions
    if(blackBoxes.size() > 0) {
        cFile << "//==== BlackBox Functions ====" << std::endl;

        for(unsigned long i = 0; i<blackBoxes.size(); i++){
            cFile << "//**** BEGIN BlackBox " << blackBoxes[i]->getFullyQualifiedName() << " Functions ****" << std::endl;
            cFile << blackBoxes[i]->getCppBodyContent() << std::endl;
            cFile << "//**** END BlackBox " << blackBoxes[i]->getFullyQualifiedName() << " Functions ****" << std::endl;
        }
    }

    cFile << std::endl;

    cFile << "//==== Functions ====" << std::endl;

    //Emit the compute function

    cFile << computeFctnProto << "{" << std::endl;

    //emit inner loop
    DataType blockDT = DataType(false, false, false, (int) std::ceil(std::log2(blockSize)+1), 0, 1);
    if(blockSize > 1) {
        cFile << "for(" + blockDT.getCPUStorageType().toString(DataType::StringStyle::C, false, false) + " " + blockIndVar + " = 0; " + blockIndVar + "<" + GeneralHelper::to_string(blockSize) + "; " + blockIndVar + "++){" << std::endl;
    }

    //Emit operators
    if(schedType == SchedParams::SchedType::TOPOLOGICAL_CONTEXT){
        emitSelectOpsSchedStateUpdateContext(cFile, nodesToEmit, schedType, outputMaster, blockSize, blockIndVar);
    }else{
        throw std::runtime_error("Only TOPOLOGICAL_CONTEXT scheduler varient is supported for multi-threaded emit");
    }

    if(blockSize > 1) {
        cFile << "}" << std::endl;
    }

    cFile << "}" << std::endl;

    cFile << std::endl;

    cFile << "void " << designName + "_partition" << partitionNum << "_reset(){" << std::endl;
    cFile << "//==== Reset State Vars ====" << std::endl;
    for(unsigned long i = 0; i<nodesWithStateCount; i++){
        std::vector<Variable> stateVars = nodesWithState[i]->getCStateVars();
        //Emit State Vars
        unsigned long numStateVars = stateVars.size();
        for(unsigned long j = 0; j<numStateVars; j++){
            //cFile << "_Thread_local static " << stateVars[j].getCVarDecl(false, true, true) << ";" << std::endl;
            DataType initDataType = stateVars[j].getDataType();
            std::vector<NumericValue> initVals = stateVars[j].getInitValue();
            unsigned long initValsLen = initVals.size();
            if(initValsLen == 1){
                cFile << stateVars[j].getCVarName(false) << " = " << initVals[0].toStringComponent(false, initDataType) << ";" << std::endl;
            }else{
                for(unsigned long k = 0; k < initValsLen; k++){
                    cFile << stateVars[j].getCVarName(false) << "[" << k << "] = " << initVals[k].toStringComponent(false, initDataType) << ";" << std::endl;
                }
            }

            if(stateVars[j].getDataType().isComplex()){
                if(initValsLen == 1){
                    cFile << stateVars[j].getCVarName(true) << " = " << initVals[0].toStringComponent(true, initDataType) << ";" << std::endl;
                }else{
                    for(unsigned long k = 0; k < initValsLen; k++){
                        cFile << stateVars[j].getCVarName(true) << "[" << k << "] = " << initVals[k].toStringComponent(true, initDataType) << ";" << std::endl;
                    }
                }
            }
        }
    }

    //Call the reset functions of blackboxes if they have state
    if(blackBoxes.size() > 0) {
        cFile << "//==== Reset BlackBoxes ====" << std::endl;

        for(unsigned long i = 0; i<blackBoxes.size(); i++){
            cFile << blackBoxes[i]->getResetName() << "();" << std::endl;
        }
    }

    cFile << "}" << std::endl;
    cFile << std::endl;

    //Emit thread function
    cFile << threadFctnDecl << "{" << std::endl;
    //Copy ptrs from struct argument
    cFile << EmitterHelpers::emitCopyCThreadArgs(inputFIFOs, outputFIFOs, "args", threadArgTypeName);

    //Create Loop
    cFile << "while(1){" << std::endl;

    if(threadDebugPrint) {
        cFile << "printf(\"Partition " + GeneralHelper::to_string(partitionNum) + " waiting for inputs ...\\n\");"
              << std::endl;
    }

    //Check FIFO input FIFOs (will spin until ready)
    cFile << EmitterHelpers::emitFIFOChecks(inputFIFOs, false, "inputFIFOsReady", false, true, true); //Include pthread_testcancel check

    //Create temp entries for FIFO inputs
    std::vector<std::string> tmpReadDecls = EmitterHelpers::createFIFOReadTemps(inputFIFOs);
    for(int i = 0; i<tmpReadDecls.size(); i++){
        cFile << tmpReadDecls[i] << std::endl;
    }

    //Read input FIFOs
    if(threadDebugPrint) {
        cFile << "printf(\"Partition " + GeneralHelper::to_string(partitionNum) + " reading inputs ...\\n\");"
              << std::endl;
    }
    std::vector<std::string> readFIFOExprs = EmitterHelpers::readFIFOsToTemps(inputFIFOs);
    for(int i = 0; i<readFIFOExprs.size(); i++){
        cFile << readFIFOExprs[i] << std::endl;
    }

    //Create temp entries for outputs
    std::vector<std::string> tmpWriteDecls = EmitterHelpers::createFIFOWriteTemps(outputFIFOs);
    for(int i = 0; i<tmpWriteDecls.size(); i++){
        cFile << tmpWriteDecls[i] << std::endl;
    }

    if(threadDebugPrint) {
        cFile << "printf(\"Partition " + GeneralHelper::to_string(partitionNum) + " computing ...\\n\");" << std::endl;
    }
    //Call compute function (recall that the compute function is declared with outputs as references)
    std::string call = getCallPartitionComputeCFunction(computeFctnName, inputFIFOs, outputFIFOs, blockSize);
    cFile << call << std::endl;

    //Check output FIFOs (will spin until ready)
    if(threadDebugPrint) {
        cFile << "printf(\"Partition " + GeneralHelper::to_string(partitionNum) +
                 " waiting for room in output FIFOs ...\\n\");" << std::endl;
    }
    cFile << EmitterHelpers::emitFIFOChecks(outputFIFOs, true, "outputFIFOsReady", false, true, true); //Include pthread_testcancel check

    //Write result to FIFOs
    if(threadDebugPrint) {
        cFile << "printf(\"Partition " + GeneralHelper::to_string(partitionNum) + " writing outputs ...\\n\");"
              << std::endl;
    }
    std::vector<std::string> writeFIFOExprs = EmitterHelpers::writeFIFOsFromTemps(outputFIFOs);
    for(int i = 0; i<writeFIFOExprs.size(); i++){
        cFile << writeFIFOExprs[i] << std::endl;
    }
    if(threadDebugPrint) {
        cFile << "printf(\"Partition " + GeneralHelper::to_string(partitionNum) + " done writing outputs ...\\n\");"
              << std::endl;
    }

    //Close loop
    cFile << "}" << std::endl;

    cFile << "return NULL;" << std::endl;

    //Close function
    cFile << "}" << std::endl;

    cFile.close();
}

std::string EmitterHelpers::getPartitionComputeCFunctionArgPrototype(std::vector<std::shared_ptr<ThreadCrossingFIFO>> inputFIFOs, std::vector<std::shared_ptr<ThreadCrossingFIFO>> outputFIFOs, int blockSize){
    std::string prototype = "";

    std::vector<Variable> inputVars;
    for(int i = 0; i<inputFIFOs.size(); i++){
        for(int j = 0; j<inputFIFOs[i]->getInputPorts().size(); j++){
            inputVars.push_back(inputFIFOs[i]->getCStateVar(j));
        }
    }

    //TODO: Assuming port numbers do not have a discontinuity.  Validate this assumption.
    for(unsigned long i = 0; i<inputVars.size(); i++){
        Variable var = inputVars[i];

        if(blockSize>1){
            DataType varType = var.getDataType();
            varType.setWidth(varType.getWidth()*blockSize);
            var.setDataType(varType);
        }

        //Pass as not volatile
        var.setVolatileVar(false);

        if(i > 0){
            prototype += ", ";
        }
        prototype += "const " + var.getCVarDecl(false, false, false, true);

        //Check if complex
        if(var.getDataType().isComplex()){
            prototype += ", const " + var.getCVarDecl(true, false, false, true);
        }
    }

    //Add outputs
    std::vector<Variable> outputVars;
    for(int i = 0; i<outputFIFOs.size(); i++){
        for(int j = 0; j<outputFIFOs[i]->getInputPorts().size(); j++){
            outputVars.push_back(outputFIFOs[i]->getCStateInputVar(j));
        }
    }

    if(inputVars.size()>0){
        prototype += ", ";
    }

    //TODO: Assuming port numbers do not have a discontinuity.  Validate this assumption.

    for(unsigned long i = 0; i<outputVars.size(); i++){
        Variable var = outputVars[i];

        if(blockSize>1){
            DataType varType = var.getDataType();
            varType.setWidth(varType.getWidth()*blockSize);
            var.setDataType(varType);
        }

        //Pass as not volatile
        var.setVolatileVar(false);

        if(i > 0) {
            prototype += ", ";
        }
        //The output variables are always pointers
        prototype += var.getCPtrDecl(false);

        //Check if complex
        if(var.getDataType().isComplex()){
            prototype += ", ";
            prototype += var.getCPtrDecl(true);
        }
    }

    return prototype;
}

std::string EmitterHelpers::getCallPartitionComputeCFunction(std::string computeFctnName, std::vector<std::shared_ptr<ThreadCrossingFIFO>> inputFIFOs, std::vector<std::shared_ptr<ThreadCrossingFIFO>> outputFIFOs, int blockSize){
    std::string call = computeFctnName + "(";

    int foundInputVar = false;
    //TODO: Assuming port numbers do not have a discontinuity.  Validate this assumption.
    for(unsigned long i = 0; i<inputFIFOs.size(); i++) {
        for (unsigned long portNum = 0; portNum < inputFIFOs[i]->getInputPorts().size(); portNum++) {
            Variable var = inputFIFOs[i]->getCStateVar(portNum);
            std::string tmpName = inputFIFOs[i]->getName() + "_readTmp";

            if (foundInputVar) {
                call += ", ";
            }
            call += tmpName + ".port" + GeneralHelper::to_string(portNum) + "_real";

            foundInputVar = true;

            //Check if complex
            if (var.getDataType().isComplex()) {
                call += ", " + tmpName + ".port" + GeneralHelper::to_string(portNum) + "_imag";
            }
        }
    }

    //Add output
    if(foundInputVar>0){
        call += ", ";
    }

    bool foundOutputVar = false;

    //TODO: Assuming port numbers do not have a discontinuity.  Validate this assumption.

    for(unsigned long i = 0; i<outputFIFOs.size(); i++) {
        for (unsigned long portNum = 0; portNum < outputFIFOs[i]->getInputPorts().size(); portNum++) {
            Variable var = outputFIFOs[i]->getCStateInputVar(portNum);
            std::string tmpName = "";
            if (var.getDataType().getWidth() * blockSize == 1) {
                tmpName += "&";
            }
            tmpName += outputFIFOs[i]->getName() + "_writeTmp";

            if (foundOutputVar) {
                call += ", ";
            }

            foundOutputVar = true;

            call += tmpName + ".port" + GeneralHelper::to_string(portNum) + "_real";

            //Check if complex
            if (var.getDataType().isComplex()) {
                call += ", " + tmpName + ".port" + GeneralHelper::to_string(portNum) + "_imag";
            }
        }
    }

    call += ");\n";

    return call;
}

//NOTE: if scheduling the output master is desired, it must be included in the nodes to emit
void EmitterHelpers::emitSelectOpsSchedStateUpdateContext(std::ofstream &cFile, std::vector<std::shared_ptr<Node>> &nodesToEmit, SchedParams::SchedType schedType, std::shared_ptr<MasterOutput> outputMaster, int blockSize, std::string indVarName){

    cFile << std::endl << "//==== Compute Operators ====" << std::endl;

    //Sort nodes by schedOrder.
    std::vector<std::shared_ptr<Node>> orderedNodes = nodesToEmit;
    std::sort(orderedNodes.begin(), orderedNodes.end(), Node::lessThanSchedOrder);

    std::shared_ptr<Node> zeroSchedNodeCmp = NodeFactory::createNode<MasterUnconnected>(); //Need a node to compare to
    zeroSchedNodeCmp->setSchedOrder(0);

    auto schedIt = std::lower_bound(orderedNodes.begin(), orderedNodes.end(), zeroSchedNodeCmp, Node::lessThanSchedOrder); //Binary search for the first node to be emitted (schedOrder 0 or greater)

    std::vector<std::shared_ptr<Node>> toBeEmittedInThisOrder;
    std::copy(schedIt, orderedNodes.end(), std::back_inserter(toBeEmittedInThisOrder));

    emitOpsStateUpdateContext(cFile, schedType, toBeEmittedInThisOrder, outputMaster, blockSize, indVarName);
}

void EmitterHelpers::emitOpsStateUpdateContext(std::ofstream &cFile, SchedParams::SchedType schedType, std::vector<std::shared_ptr<Node>> orderedNodes, std::shared_ptr<MasterOutput> outputMaster, int blockSize,
                                       std::string indVarName, bool checkForPartitionChange) {
    //Keep a context stack of the last emitted statement.  This is used to check for context changes.  Also used to check if the 'first' entry should be used.  If first entry is used (ie. previous context at this level in the stack was not in the same famuly, and the subContext emit count is not 0, then contexts are not contiguous -> ie. switch cannot be used)
    std::vector<Context> lastEmittedContext;

    //True if the context that was emitted was opened with the 'first' method
    std::vector<bool> contextFirst;

    //Keep a set of contexts that have been emitted (and exited).  For now, we do not allow re-entry into an already emitted sub context.
    std::set<Context> alreadyEmittedSubContexts;

    //Keep a count of how many subContexts of a contextRoot have been emitted, allows if/else if/else statements to not be contiguous
    std::map<std::shared_ptr<ContextRoot>, int> subContextEmittedCount;

    //Check if a new partition has been entered
    int partition = -1;
    bool firstNode = true;

    //Itterate through the schedule and emit
    for(auto it = orderedNodes.begin(); it != orderedNodes.end(); it++){

//        std::cout << "Writing " << (*it)->getFullyQualifiedName() << std::endl;

        if(firstNode){
            firstNode = false;
            partition = (*it)->getPartitionNum();
        }else if(checkForPartitionChange){
            if(partition != (*it)->getPartitionNum()){
                //It is unclear what the implication would be for how to open and close contexts if the partition changes in the middle of op emit
                //Therefore, this condition should be checked for
                throw std::runtime_error(ErrorHelpers::genErrorStr("C Emit Error - Partition change occurred when emitting ops", *it));
            }
        }

        //Check if the context has changed
        std::vector<Context> nodeContext = (*it)->getContext();

        std::vector<std::string> contextStatements;

        if(nodeContext != lastEmittedContext) {

            //Check if previous context(s) should be closed (run up the stack until the contexts are the same - we can do this because contexts are in a strict hierarchy [ie. tree])
            for(unsigned long i = 0; i<lastEmittedContext.size(); i++){
                unsigned long ind = lastEmittedContext.size()-1-i;

                if(ind >= nodeContext.size()){
                    //The new context is less deep than this.  Therefore, we should close this context
                    if(subContextEmittedCount.find(lastEmittedContext[ind].getContextRoot()) == subContextEmittedCount.end()){
                        subContextEmittedCount[lastEmittedContext[ind].getContextRoot()] = 0;
                    }

                    if(contextFirst.size() - 1 < ind || contextFirst[ind]){
                        //This context was created with a call to first
                        lastEmittedContext[ind].getContextRoot()->emitCContextCloseFirst(contextStatements, schedType, lastEmittedContext[ind].getSubContext(), (*it)->getPartitionNum());
                    }else if(subContextEmittedCount[lastEmittedContext[ind].getContextRoot()] >= lastEmittedContext[ind].getContextRoot()->getNumSubContexts()-1 ){
                        lastEmittedContext[ind].getContextRoot()->emitCContextCloseLast(contextStatements, schedType, lastEmittedContext[ind].getSubContext(), (*it)->getPartitionNum());
                    }else{
                        lastEmittedContext[ind].getContextRoot()->emitCContextCloseMid(contextStatements, schedType, lastEmittedContext[ind].getSubContext(), (*it)->getPartitionNum());
                    }

                    //Remove this level of the contextFirst stack because the family was exited
                    contextFirst.pop_back();

                    //Add that context to the exited context set
                    alreadyEmittedSubContexts.insert(lastEmittedContext[ind]);

                    //Increment emitted count by 1
                    subContextEmittedCount[lastEmittedContext[ind].getContextRoot()]++;

                }else if(lastEmittedContext[ind] != nodeContext[ind]){
                    //TODO: change if split contexts are introduced -> would need to check for first context change, then emit all context closes up to that point
                    //could be either a first, mid, or last
                    if(contextFirst[ind]) { //contextFirst is guarenteed to exist because this level existed in the last emitted context
                        lastEmittedContext[ind].getContextRoot()->emitCContextCloseFirst(contextStatements, schedType, lastEmittedContext[ind].getSubContext(), (*it)->getPartitionNum());
                    }else if(subContextEmittedCount[lastEmittedContext[ind].getContextRoot()] >= lastEmittedContext[ind].getContextRoot()->getNumSubContexts()-1 ){
                        lastEmittedContext[ind].getContextRoot()->emitCContextCloseLast(contextStatements, schedType, lastEmittedContext[ind].getSubContext(), (*it)->getPartitionNum());
                    }else{
                        lastEmittedContext[ind].getContextRoot()->emitCContextCloseMid(contextStatements, schedType, lastEmittedContext[ind].getSubContext(), (*it)->getPartitionNum());
                    }

                    if(lastEmittedContext[ind].getContextRoot() == nodeContext[ind].getContextRoot()){
                        //In this case, the next context is in the same family, set the contextFirst to false and do not remove it
                        contextFirst[ind] = false;
                    }else{
                        //In this case, the context family is being left, remove the contextFirst entry for the family
                        contextFirst.pop_back();
                    }

                    //Add that context to the exited context set
                    alreadyEmittedSubContexts.insert(lastEmittedContext[ind]);

                    //Increment emitted count by 1
                    subContextEmittedCount[lastEmittedContext[ind].getContextRoot()]++;
                }else{
                    break; //We found the point root of both the old and new contexts, we can stop here
                }
            }

            //If new context(s) have been entered, emit the conditionals (run down the stack to find the root of both, then emit the context entries)
            for(unsigned long i = 0; i<nodeContext.size(); i++){
                if(i>=lastEmittedContext.size()){
                    //This context level is new, it must be the first time it has been entered

                    //Declare vars
                    std::vector<Variable> contextVars = nodeContext[i].getContextRoot()->getCContextVars();
                    for(unsigned long j = 0; j<contextVars.size(); j++) {
                        contextStatements.push_back(contextVars[j].getCVarDecl(false, true, false, true) + ";\n");

                        if (contextVars[j].getDataType().isComplex()) {
                            contextStatements.push_back(contextVars[j].getCVarDecl(true, true, false, true) + ";\n");
                        }
                    }

                    //Emit context
                    nodeContext[i].getContextRoot()->emitCContextOpenFirst(contextStatements, schedType, nodeContext[i].getSubContext(), (*it)->getPartitionNum());

                    //push back onto contextFirst
                    contextFirst.push_back(true);
                }else if(lastEmittedContext[i] != nodeContext[i]){  //Because contexts must be purely hierarchical, new context is only entered if the context at this level is different from the previously emitted context.
                    //TODO: change if split contexts are introduced -> would need to check for first context change, then emit all context opens after that
                    //This context level already existed, could either be an first, mid, or last

                    //Check if the previous context had the same root.  If so, this is the first
                    if(lastEmittedContext[i].getContextRoot() != nodeContext[i].getContextRoot()){
                        //Emit context vars
                        std::vector<Variable> contextVars = nodeContext[i].getContextRoot()->getCContextVars();
                        for(unsigned long j = 0; j<contextVars.size(); j++) {
                            contextStatements.push_back(contextVars[j].getCVarDecl(false, true, false, true) + ";\n");

                            if (contextVars[j].getDataType().isComplex()) {
                                contextStatements.push_back(contextVars[j].getCVarDecl(true, true, false, true) + ";\n");
                            }
                        }

                        //Emit context
                        nodeContext[i].getContextRoot()->emitCContextOpenFirst(contextStatements, schedType, nodeContext[i].getSubContext(), (*it)->getPartitionNum());

                        //This is a new family, push back
                        contextFirst.push_back(true);
                    }else{
                        if(subContextEmittedCount[nodeContext[i].getContextRoot()] >= nodeContext[i].getContextRoot()->getNumSubContexts()-1){ //Check if this is the last in the context family
                            nodeContext[i].getContextRoot()->emitCContextOpenLast(contextStatements, schedType, nodeContext[i].getSubContext(), (*it)->getPartitionNum());
                        }else{
                            nodeContext[i].getContextRoot()->emitCContextOpenMid(contextStatements, schedType, nodeContext[i].getSubContext(), (*it)->getPartitionNum());
                        }

                        //Contexts are in the same family
                        contextFirst[i] = false;
                    }

                }
                //Else, we have not yet found where the context stacks diverge (if they in fact do) --> continue
            }

            //Check to see if this is the first, last, or other conditional being emitted (if there is only 1 context, default to first call)
            //Check to see if the previous context at this level (if one existed) was in the same family: if so, this is either the middle or end, if not, this is a first
            //Check the count
            //Check to see if the count of emitted subContests for this context root is max# contexts -1.  If so, it is last.  Else, it is a middle

            //If the first time, call the context preparation function (ie. for declaring outputs outside of context)

            //Set contextFirst to true (add it to the stack) if this was the first context, false otherwise

            //Emit proper conditional
        }

        for(unsigned long i = 0; i<contextStatements.size(); i++){
            cFile << contextStatements[i];
        }

        if(*it == outputMaster) {
            //Emit output (using same basic code as bottom up except forcing fanout - all results will be availible as temp vars)
            unsigned long numOutputs = outputMaster->getInputPorts().size();
            for (unsigned long i = 0; i < numOutputs; i++) {
                std::shared_ptr<InputPort> output = outputMaster->getInputPort(i);
                cFile << std::endl << "//---- Assign Output " << i << ": " << output->getName() << " ----" << std::endl;

                //Get the arc connected to the output
                std::shared_ptr<Arc> outputArc = *(output->getArcs().begin());

                DataType outputDataType = outputArc->getDataType();

                std::shared_ptr<OutputPort> srcOutputPort = outputArc->getSrcPort();
                int srcNodeOutputPortNum = srcOutputPort->getPortNum();

                std::shared_ptr<Node> srcNode = srcOutputPort->getParent();

                cFile << "//-- Assign Real Component --" << std::endl;
                std::vector<std::string> cStatements_re;
                std::string expr_re = srcNode->emitC(cStatements_re, schedType, srcNodeOutputPortNum, false, true, true);
                //emit the expressions
                unsigned long numStatements_re = cStatements_re.size();
                for (unsigned long j = 0; j < numStatements_re; j++) {
                    cFile << cStatements_re[j] << std::endl;
                }

                //emit the assignment
                Variable outputVar = Variable(outputMaster->getCOutputName(i), outputDataType);
                int varWidth = outputVar.getDataType().getWidth();
                int varBytes = outputVar.getDataType().getCPUStorageType().getTotalBits()/8;
                if(varWidth > 1){
                    if(blockSize > 1) {
                        cFile << "memcpy(output[0]." << outputVar.getCVarName(false) << "+" << blockSize
                              << "*" << indVarName << ", " << expr_re << ", " << varWidth * varBytes << ");"
                              << std::endl;
                    }else{
                        cFile << "memcpy(output[0]." << outputVar.getCVarName(false) << ", " << expr_re << ", "
                              << varWidth * varBytes << ");" << std::endl;
                    }
                }else{
                    if(blockSize > 1) {
                        cFile << "output[0]." << outputVar.getCVarName(false) << "[" << indVarName << "] = " << expr_re << ";" << std::endl;
                    }else{
                        cFile << "output[0]." << outputVar.getCVarName(false) << " = " << expr_re << ";" << std::endl;
                    }
                }

                //Emit Imag if Datatype is complex
                if (outputDataType.isComplex()) {
                    cFile << std::endl <<  "//-- Assign Imag Component --" << std::endl;
                    std::vector<std::string> cStatements_im;
                    std::string expr_im = srcNode->emitC(cStatements_im, schedType, srcNodeOutputPortNum, true, true, true);
                    //emit the expressions
                    unsigned long numStatements_im = cStatements_im.size();
                    for (unsigned long j = 0; j < numStatements_im; j++) {
                        cFile << cStatements_im[j] << std::endl;
                    }

                    //emit the assignment
                    if(varWidth > 1){
                        if(blockSize > 1) {
                            cFile << "memcpy(output[0]." << outputVar.getCVarName(true) << "+" << blockSize
                                  << "*" << indVarName << ", " << expr_im << ", " << varWidth * varBytes << ");"
                                  << std::endl;
                        }else{
                            cFile << "memcpy(output[0]." << outputVar.getCVarName(true) << ", " << expr_im << ", "
                                  << varWidth * varBytes << ");" << std::endl;
                        }
                    }else{
                        if(blockSize > 1) {
                            cFile << "output[0]." << outputVar.getCVarName(true) << "[" << indVarName << "] = " << expr_im << ";" << std::endl;
                        }else{
                            cFile << "output[0]." << outputVar.getCVarName(true) << " = " << expr_im << ";" << std::endl;
                        }
                    }
                }
            }
        }else if(GeneralHelper::isType<Node, StateUpdate>(*it) != nullptr){
            std::shared_ptr<StateUpdate> stateUpdateNode = std::dynamic_pointer_cast<StateUpdate>(*it);
            //Emit state update
            cFile << std::endl << "//---- State Update for " << stateUpdateNode->getPrimaryNode()->getFullyQualifiedName() << " ----" << std::endl;

            std::vector<std::string> stateUpdateExprs;
            (*it)->emitCStateUpdate(stateUpdateExprs, schedType);

            for(unsigned long j = 0; j<stateUpdateExprs.size(); j++){
                cFile << stateUpdateExprs[j] << std::endl;
            }

        }else if((*it)->hasState()){
            //Call emit for state element input
            //The state element output is treated similarly to a constant and a variable name is always returned
            //The output has no dependencies within the cycle
            //The input can have dependencies

            //Emit comment
            cFile << std::endl << "//---- Calculate " << (*it)->getFullyQualifiedName() << " Inputs ----" << std::endl;

            std::vector<std::string> nextStateExprs;
            (*it)->emitCExprNextState(nextStateExprs, schedType);

            for(unsigned long j = 0; j<nextStateExprs.size(); j++){
                cFile << nextStateExprs[j] << std::endl;
            }

        }else{
            //Emit standard node

            //Emit comment
            cFile << std::endl << "//---- Calculate " << (*it)->getFullyQualifiedName() << " ----" << std::endl;

            unsigned long numOutputPorts = (*it)->getOutputPorts().size();
            //Emit each output port
            //TODO: now checks for unconnected output ports.  Validate
            for(unsigned long i = 0; i<numOutputPorts; i++){
                std::shared_ptr<OutputPort> outputPortBeingEmitted = (*it)->getOutputPort(i);
                if(!outputPortBeingEmitted->getArcs().empty()) {

                    std::vector<std::string> cStatementsRe;
                    //Emit real component (force fanout)
                    (*it)->emitC(cStatementsRe, schedType, i, false, true,
                                 true); //We actually do not use the returned expression.  Dependent nodes will get this by calling the emit function of this block again.

                    for (unsigned long j = 0; j < cStatementsRe.size(); j++) {
                        cFile << cStatementsRe[j] << std::endl;
                    }

                    if ((*it)->getOutputPort(i)->getDataType().isComplex()) {
                        //Emit imag component (force fanout)
                        std::vector<std::string> cStatementsIm;
                        //Emit real component (force fanout)
                        (*it)->emitC(cStatementsIm, schedType, i, true, true,
                                     true); //We actually do not use the returned expression.  Dependent nodes will get this by calling the emit function of this block again.

                        for (unsigned long j = 0; j < cStatementsIm.size(); j++) {
                            cFile << cStatementsIm[j] << std::endl;
                        }
                    }
                }else{
                    std::cout << "Output Port " << i << " of " << (*it)->getFullyQualifiedName() << "[ID: " << (*it)->getId() << "] not emitted because it is unconnected" << std::endl;
                }
            }
        }

        lastEmittedContext = nodeContext;

    }
}