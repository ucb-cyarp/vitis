//
// Created by Christopher Yarp on 10/4/19.
//

#include "MultiThreadPasses.h"

#include "PrimitiveNodes/Delay.h"
#include "PrimitiveNodes/TappedDelay.h"
#include "General/GraphAlgs.h"
#include "GraphCore/ExpandedNode.h"
#include "Blocking/BlockingDomain.h"
#include <iostream>

void MultiThreadPasses::absorbAdjacentDelaysIntoFIFOs(std::map<std::pair<int, int>, std::vector<std::shared_ptr<ThreadCrossingFIFO>>> fifos,
                                                              std::vector<std::shared_ptr<Node>> &new_nodes,
                                                              std::vector<std::shared_ptr<Node>> &deleted_nodes,
                                                              std::vector<std::shared_ptr<Arc>> &new_arcs,
                                                              std::vector<std::shared_ptr<Arc>> &deleted_arcs, bool blockingAlreadyOccurred, bool printActions) {

    for(auto it = fifos.begin(); it != fifos.end(); it++){
        int srcPartition = it->first.first;
        int dstPartition = it->first.second;

        std::vector<std::shared_ptr<ThreadCrossingFIFO>> fifoPtrs = it->second;

        for(int i = 0; i<fifoPtrs.size(); i++) {
            //Check if the FIFO is in a contect.  If so, do not absorb delays.
            //TODO: Remove this when on demand FIFOs are implemented
            //Checks already exist in the input and output absorb methods to check that the delay is in the same context

            //We can allow FIFO delay absorption in different clock domain since they are not conditionally executed
            //Check the context stack to see if they are all
            std::vector<Context> contextStack = fifoPtrs[i]->getContext();
            bool onlyClockDomainAndBlockingContexts = true;
            for(Context &c : contextStack){
                onlyClockDomainAndBlockingContexts &= c.getContextRoot()->allowFIFOAbsorption();
            }

            bool contextEmpty = contextStack.empty();

            if(contextEmpty || (!contextEmpty && onlyClockDomainAndBlockingContexts)) {
                bool done = false;

                //Iterate because there may be a series of delays to ingest
                while (!done) {
                    AbsorptionStatus statusIn = absorbAdjacentInputDelayIfPossible(fifoPtrs[i], srcPartition, new_nodes,
                                                                                   deleted_nodes,
                                                                                   new_arcs, deleted_arcs,
                                                                                   printActions);

                    AbsorptionStatus statusOut = AbsorptionStatus::NO_ABSORPTION;
                    if (statusIn != AbsorptionStatus::PARTIAL_ABSORPTION_FULL_FIFO) {
                        statusOut = absorbAdjacentOutputDelayIfPossible(fifoPtrs[i], dstPartition, new_nodes,
                                                                        deleted_nodes, new_arcs,
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
                MultiThreadPasses::reshapeFIFOInitialConditionsForBlockSize(fifoPtrs[i], new_nodes,
                                                                                      deleted_nodes, new_arcs,
                                                                                      deleted_arcs, blockingAlreadyOccurred, printActions);
            }else{
                if (printActions) {
                    std::cout << "Skipped Delay Absorption for FIFO since it is in a context: "
                              << fifoPtrs[i]->getFullyQualifiedName() << " [ID:" << fifoPtrs[i]->getId()
                              << "]" << std::endl;
                }
            }
        }
    }
}

MultiThreadPasses::AbsorptionStatus
MultiThreadPasses::absorbAdjacentInputDelayIfPossible(std::shared_ptr<ThreadCrossingFIFO> fifo,
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
    //Note: The dimensions of the input must be taken into account
    //Note: elementsPerInput can be for a sub-block of size > 1.  If so, multiplying by the block size will double count the sub-block size
    //We need to take into account the sub-blocking size of the FIFO

    int elementsPerInput = fifo->getInputPort(0)->getDataType().numberOfElements()/fifo->getSubBlockSizeCreateIfNot(0); //This is the size of the primitive element which multiple are transacted at a time if subBlocking>1
    if(fifo->getInitConditions().size() < elementsPerInput*fifo->getBlockSizeCreateIfNot(0)*(fifo->getFifoLength() - 1)) { //The FIFO is sized based on the block size rather than the sub-block size.  The sub-blocking modifies how the block is indexed into
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

            //Should not absorb tapped delays (which are extensions of Delay)
            if (srcDelay != nullptr && GeneralHelper::isType<Node, TappedDelay>(srcNode) == nullptr) {
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
                if (srcDelay->getStateUpdateNodes().size() != 0) {
                    throw std::runtime_error(ErrorHelpers::genErrorStr(
                            "FIFO Delay Absorption should be run before State Update Node Insertion",
                            srcDelay));
                }

                std::set<std::shared_ptr<Arc>> delayFIFOArcs = srcDelay->getOutputArcs();
                if (delayFIFOArcs.size() == 1) {
                    //The delay has only 1 output (which must be this FIFO).  This delay also does not have any output order constraint arcs
                    //Absorb it if possible

                    //NOTE: The number of initial conditions can be expanded beyond the delay amount if circular buffer is used
                    //Use getExportableInitConds to get the intiail conditions without extra elements
                    std::vector<NumericValue> delayInitConds = srcDelay->getInitCondition();
                    std::vector<NumericValue> fifoInitConds = fifo->getInitConditionsCreateIfNot(0);
                    //TODO: Assumes Delay has not been specialized yet and getDelayValue() refers to the number of unblocked items
                    if(fifo->getSubBlockSizeCreateIfNot(0)>1 && !srcDelay->isBlockingSpecializationDeferred() && srcDelay->getDelayValue() != 0){
                        //The one exception is delays of size 0 which we will allow to proceed.  They will be completely absorbed and no initial conditions will be transferred.
                        //Another excpetion is for delays inside of clock domains which do not operate in vector mode.  Delays in these domains are not specialized and will not be in the deferred specialization state.
                        //However, they still can be absorbed.  In this case, the FIFO should be at a sub-block size of 1.
                        throw std::runtime_error(ErrorHelpers::genErrorStr("Expect delay to be in specialization deferred state when absorbing initial conditions into FIFO with sub-block size > 1", srcDelay));
                    }
                    int numberInitCondsInSrc = srcDelay->getDelayValue() * elementsPerInput;

                    if (numberInitCondsInSrc + fifoInitConds.size() <= fifo->getBlockSizeCreateIfNot(0) * elementsPerInput * (fifo->getFifoLength() - 1)) {
                        //Can absorb complete delay
                        fifoInitConds.insert(fifoInitConds.end(), delayInitConds.begin(),
                                             delayInitConds.begin() + numberInitCondsInSrc);  //Because this is at the input to the FIFO, the initial conditions are appended.
                        fifo->setInitConditionsCreateIfNot(0, fifoInitConds);

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
                                      << " into " << fifo->getFullyQualifiedName() << " [ID:" << fifo->getId() << "]"
                                      << " - " << numberInitCondsInSrc / elementsPerInput << " Items" << std::endl;
                        }

                        return AbsorptionStatus::FULL_ABSORPTION;
                    } else {
                        //Partial absorption (due to size)
                        //We already checked that there is room

                        int numToAbsorb = fifo->getBlockSizeCreateIfNot(0)*elementsPerInput*(fifo->getFifoLength() - 1) - fifoInitConds.size();

                        if(numToAbsorb % elementsPerInput != 0){
                            throw std::runtime_error(ErrorHelpers::genErrorStr("Error absorbing delay into FIFO, the number of initial conditions to absorb is not a multiple of the number of elements per input", fifo));
                        }

                        fifoInitConds.insert(fifoInitConds.end(), delayInitConds.begin(), delayInitConds.begin()+numToAbsorb);
                        fifo->setInitConditionsCreateIfNot(0, fifoInitConds);
                        delayInitConds.erase(delayInitConds.begin(), delayInitConds.begin()+numToAbsorb);
                        srcDelay->setInitCondition(delayInitConds);
                        srcDelay->setDelayValue(srcDelay->getDelayValue()-numToAbsorb/elementsPerInput);

                        //No re-wiring required

                        if (printActions) {
                            std::cout << "Delay Partially Absorbed info FIFO: " << srcDelay->getFullyQualifiedName() << " [ID:" << srcDelay->getId() << "]"
                                      << " into " << fifo->getFullyQualifiedName() << " [ID:" << fifo->getId() << "]"
                                      << " - " << numToAbsorb/elementsPerInput << " Items" << std::endl;
                        }

                        return AbsorptionStatus::PARTIAL_ABSORPTION_FULL_FIFO;
                    }

                }//else, delay has more than 1 output (Cannot absorb)
            }//else src was not delay (cannot absorb)
        }//else, did not check because order constraint arcs to this FIFO
    }//else, FIFO full

    return AbsorptionStatus::NO_ABSORPTION;
}

MultiThreadPasses::AbsorptionStatus
MultiThreadPasses::absorbAdjacentOutputDelayIfPossible(std::shared_ptr<ThreadCrossingFIFO> fifo,
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

    //Note: elementsPerInput can be for a sub-block of size > 1.
    int elementsPerInput = fifo->getInputPort(0)->getDataType().numberOfElements()/fifo->getSubBlockSizeCreateIfNot(0);
    //Check if FIFO full
    if (fifo->getInitConditions().size() < fifo->getBlockSizeCreateIfNot(0)*elementsPerInput*(fifo->getFifoLength() - 1)) {
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
                    if(dstDelay == nullptr || GeneralHelper::isType<Node, TappedDelay>(dstDelay) != nullptr){
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

                    //NOTE: The number of initial conditions can be expanded beyond the delay amount if circular buffer is used
                    //Use get getExportableInitConds to avoid added initial conditions for circular buffer or extra array element

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
                //We have a nonzero number of delays that can be merged (otherwise, the longest prefix would have become empty during one of the iterations)

                //Find how many can be absorbed into the FIFO
                std::vector<NumericValue> fifoInitConds = fifo->getInitConditionsCreateIfNot(0);
                int roomInFifo = fifo->getBlockSizeCreateIfNot(0)*elementsPerInput*(fifo->getFifoLength() - 1) - fifoInitConds.size();
                int numToAbsorb = std::min(roomInFifo, (int) longestPostfix.size());
                //Note that the number to absorb must be a multiple of elementsPerInput
                //Remove any remainder
                numToAbsorb = numToAbsorb - numToAbsorb%elementsPerInput;

                //Set FIFO new initial conditions
                int remainingPostfix = longestPostfix.size() - numToAbsorb;
                std::vector<NumericValue> newFIFOInitConds;
                //Pull init conditions from the end of the delay (since the delay is after the FIFO) - from the remaining count to the end
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

                    if(fifo->getSubBlockSizeCreateIfNot(0)>1 && !dstDelay->isBlockingSpecializationDeferred() && dstDelay->getDelayValue() != 0){
                        //The one exception is delays of size 0 which we will allow to proceed.  They will be completely absorbed and no initial conditions will be transferred.
                        //Another excpetion is for delays inside of clock domains which do not operate in vector mode.  Delays in these domains are not specialized and will not be in the deferred specialization state.
                        //However, they still can be absorbed.  In this case, the FIFO should be at a sub-block size of 1.
                        throw std::runtime_error(ErrorHelpers::genErrorStr("Expect delay to be in specialization deferred state when absorbing initial conditions into FIFO with sub-block size > 1", dstDelay));
                    }

                    std::vector<NumericValue> delayInitConditions = dstDelay->getInitCondition();
                    if(delayInitConditions.size() < numToAbsorb){
                        throw std::runtime_error(ErrorHelpers::genErrorStr("Found a delay with an unexpected number of initial conditions during FIFO delay absorption", dstDelay));
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
                                      << " into " << fifo->getFullyQualifiedName() << " [ID:" << fifo->getId() << "]"
                                      << " - " << numToAbsorb/elementsPerInput << " Items" << std::endl;
                        }
                    }else{
                        //Partial absorption
                        foundPartial = true;

                        //Update the delay initial condition and delay value
                        std::vector<NumericValue> delayInitConds = dstDelay->getInitCondition();
                        //Remove from the tail of the initial conditions (fifo is behind it)
                        delayInitConds.erase(delayInitConds.end()-numToAbsorb, delayInitConds.end());
                        dstDelay->setInitCondition(delayInitConds);
                        dstDelay->setDelayValue(dstDelay->getDelayValue() - numToAbsorb/elementsPerInput);

                        if (printActions) {
                            std::cout << "Delay Partially Absorbed info FIFO: " << dstDelay->getFullyQualifiedName() << " [ID:" << dstDelay->getId() << "]"
                                      << " into " << fifo->getFullyQualifiedName() << " [ID:" << fifo->getId() << "]"
                                      << " - " << numToAbsorb/elementsPerInput << " Items" << std::endl;
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

void MultiThreadPasses::reshapeFIFOInitialConditionsForBlockSize(std::shared_ptr<ThreadCrossingFIFO> fifo,
                                                                         std::vector<std::shared_ptr<Node>> &new_nodes,
                                                                         std::vector<std::shared_ptr<Node>> &deleted_nodes,
                                                                         std::vector<std::shared_ptr<Arc>> &new_arcs,
                                                                         std::vector<std::shared_ptr<Arc>> &deleted_arcs,
                                                                         bool blockingAlreadyOccurred,
                                                                         bool printActions){
    //TODO: For now, we will restrict delay absorption to be when there is a single input and output port.
    //FIFO bundling happens after this point
    if(fifo->getInputPorts().size() != 1 || fifo->getOutputPorts().size() != 1){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Currently, initial condition reshaping is only supported with FIFOs that have a single input and output port" , fifo));
    }

    //TODO: will need to be specialized for each port
    std::vector<NumericValue> fifoInitialConditions = fifo->getInitConditionsCreateIfNot(0);
    //Note that the FIFO can have a sub-block size > 1 with the I/O ports transacting multiple elements at once.  The Block size is in terms of these smaller elements and not the number of sub-blocks.  Need to correct for this.
    int numberElementsPerInput = fifo->getInputPort(0)->getDataType().numberOfElements()/fifo->getSubBlockSizeCreateIfNot(0);
    int numPrimitiveElementsToMove = fifoInitialConditions.size() % (fifo->getBlockSizeCreateIfNot(0)*numberElementsPerInput);

    if(numPrimitiveElementsToMove % numberElementsPerInput != 0){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Error when reshaping FIFO size.  The number of initial condition primitive elements to move is not a multiple of the number of primitive elements per input", fifo));
    }
    int itemsToMove = numPrimitiveElementsToMove / numberElementsPerInput;

    if(itemsToMove > 0){
        MultiThreadPasses::reshapeFIFOInitialConditions(fifo, itemsToMove, new_nodes, deleted_nodes, new_arcs, deleted_arcs, blockingAlreadyOccurred);

        if (printActions) {
            std::cout << "FIFO Initial Conditions Reshaped: " << fifo->getFullyQualifiedName() << " [ID:" << fifo->getId() << "]"
                      << " " << itemsToMove << " items moved into delay at input" << std::endl;
        }
    }
}

void MultiThreadPasses::reshapeFIFOInitialConditionsToSizeBlocks(std::shared_ptr<ThreadCrossingFIFO> fifo,
                                                                   int tgtSizeBlocks,
                                                                   std::vector<std::shared_ptr<Node>> &new_nodes,
                                                                   std::vector<std::shared_ptr<Node>> &deleted_nodes,
                                                                   std::vector<std::shared_ptr<Arc>> &new_arcs,
                                                                   std::vector<std::shared_ptr<Arc>> &deleted_arcs,
                                                                   bool blockingAlreadyOccurred,
                                                                   bool printActions){
    //TODO: For now, we will restrict delay absorption to be when there is a single input and output port.
    //FIFO bundling happens after this point
    if(fifo->getInputPorts().size() != 1 || fifo->getOutputPorts().size() != 1){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Currently, initial condition reshaping is only supported with FIFOs that have a single input and output port" , fifo));
    }

    std::vector<NumericValue> fifoInitialConditions = fifo->getInitConditionsCreateIfNot(0);
    //Note that the FIFO can have a sub-block size > 1 with the I/O ports transacting multiple elements at once.  The Block size is in terms of these smaller elements and not the number of sub-blocks.  Need to correct for this.
    int elementsPer = fifo->getInputPort(0)->getDataType().numberOfElements()/fifo->getSubBlockSizeCreateIfNot(0);
    int blkSize = fifo->getBlockSizeCreateIfNot(0);

    if(tgtSizeBlocks > fifoInitialConditions.size()/elementsPer/blkSize){
        throw std::runtime_error(ErrorHelpers::genErrorStr("For Initial Condition reshaping, target initial condition size must be <= the current initial condition size" , fifo));
    }

    if(fifoInitialConditions.size()%elementsPer != 0){
        throw std::runtime_error(ErrorHelpers::genErrorStr("FIFO initial condition size is not a multiple of the number of elements per item", fifo));
    }

    int numItemsToMove = fifoInitialConditions.size() / elementsPer - tgtSizeBlocks * elementsPer * blkSize; //This is the number of items to move

    if(numItemsToMove > 0){
        MultiThreadPasses::reshapeFIFOInitialConditions(fifo, numItemsToMove, new_nodes, deleted_nodes, new_arcs, deleted_arcs, blockingAlreadyOccurred);

        if (printActions) {
            std::cout << "FIFO Initial Conditions Reshaped: " << fifo->getFullyQualifiedName() << " [ID:" << fifo->getId() << "]"
                      << " " << numItemsToMove << " items moved into delay at input" << std::endl;
        }
    }
}

void MultiThreadPasses::reshapeFIFOInitialConditions(std::shared_ptr<ThreadCrossingFIFO> fifo,
                                                             int numItemsToMove, //These are the number of elements in the FIFO (not in initial conditions array) and can be scalars, vectors, or arrays
                                                             std::vector<std::shared_ptr<Node>> &new_nodes,
                                                             std::vector<std::shared_ptr<Node>> &deleted_nodes,
                                                             std::vector<std::shared_ptr<Arc>> &new_arcs,
                                                             std::vector<std::shared_ptr<Arc>> &deleted_arcs,
                                                             bool blockingAlreadyOccurred){
    //TODO: For now, we will restrict delay absorption to be when there is a single input and output port.
    //FIFO bundling happens after this point
    if(fifo->getInputPorts().size() != 1 || fifo->getOutputPorts().size() != 1){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Currently, initial condition reshaping is only supported with FIFOs that have a single input and output port" , fifo));
    }

    std::vector<NumericValue> fifoInitialConditions = fifo->getInitConditionsCreateIfNot(0);

    if(numItemsToMove > 0){
        if(fifo->getInputPorts().size() != 1){
            throw std::runtime_error(ErrorHelpers::genErrorStr("Error when reshaping FIFO initial conditions.  Number of input ports should be 1", fifo));
        }

        std::set<std::shared_ptr<Arc>> inputArcs = fifo->getInputPort(0)->getArcs();

        if(inputArcs.size() != 1){
            throw std::runtime_error(ErrorHelpers::genErrorStr("Error when reshaping FIFO initial conditions.  Number of input arcs should be 1", fifo));
        }

        std::shared_ptr<Arc> origInputArc = *inputArcs.begin();
        //Note, the sub-block size may be >1 and the input may be scaled to transact multiple items per once.  Correcting for that.
        int numPrimitveElementsPerItem = origInputArc->getDataType().numberOfElements() / fifo->getSubBlockSizeCreateIfNot(0);  //This allows us to scale the number of initial conditions transfered to the delay
        int primitiveElementsToMove = numPrimitveElementsPerItem * numItemsToMove;

        //Check if the src is a MasterInput
        if(GeneralHelper::isType<Node, MasterInput>(origInputArc->getSrcPort()->getParent()) == nullptr) {
            //The input is not a MasterInput node.  It is OK to put the delay at the input
            //Need to create a delay to move the remaining elements into
            std::vector<Context> fifoContext = fifo->getContext();

            std::shared_ptr<Delay> delay = NodeFactory::createNode<Delay>(fifo->getParent());
            new_nodes.push_back(delay);
            delay->setPartitionNum(fifo->getPartitionNum());
            delay->setName("OverflowFIFOInitCondFrom" + fifo->getName());
            delay->setContext(fifoContext);
            delay->setBlockingSpecializationDeferred(blockingAlreadyOccurred); //If blocking already arcs were already expanded but delay specialization was deferred
            delay->setDeferredBlockSize(fifo->getSubBlockSizeCreateIfNot(0)); //This is the sub-blocking length of the FIFO
            delay->setDeferredSubBlockSize(1); //The sub-blocking size should be set to 1
            if (fifoContext.size() > 0) {
                int subcontext = fifoContext[fifoContext.size() - 1].getSubContext();
                fifoContext[fifoContext.size() - 1].getContextRoot()->addSubContextNode(subcontext, delay);
            }

            //Set initial conditions
            delay->setDelayValue(numItemsToMove);
            std::vector<NumericValue> delayInitConds;
            delayInitConds.insert(delayInitConds.end(),
                                  fifoInitialConditions.begin() + (fifoInitialConditions.size() - primitiveElementsToMove), //
                                  fifoInitialConditions.end());
            delay->setInitCondition(delayInitConds);

            //Remove init conditions from fifoInitialConditions
            fifoInitialConditions.erase(
                    fifoInitialConditions.begin() + (fifoInitialConditions.size() - primitiveElementsToMove),
                    fifoInitialConditions.end());
            fifo->setInitConditionsCreateIfNot(0, fifoInitialConditions);

            //Rewire
            std::set<std::shared_ptr<Arc>> orderConstraintArcs2Rewire = fifo->getOrderConstraintInputPortCreateIfNot()->getArcs();
            for (auto it = orderConstraintArcs2Rewire.begin(); it != orderConstraintArcs2Rewire.end(); it++) {
                (*it)->setDstPortUpdateNewUpdatePrev(delay->getOrderConstraintInputPortCreateIfNot());
            }

            std::shared_ptr<Arc> newInputArc = Arc::connectNodes(delay->getOutputPortCreateIfNot(0),
                                                                 fifo->getInputPortCreateIfNot(0),
                                                                 origInputArc->getDataType(),
                                                                 origInputArc->getSampleTime());
            new_arcs.push_back(newInputArc);
            origInputArc->setDstPortUpdateNewUpdatePrev(delay->getInputPortCreateIfNot(0));
        }else{
            //The input is a MasterInput, the delay should be placed on the output

            //Check that all of the FIFO outputs are in the same context
            if(!fifo->outputsInSameContext()){
                //TODO: make this a general rule?
                throw std::runtime_error(ErrorHelpers::genErrorStr("When shaping a FIFO connected to a MasterInputs, all destination arcs must be in the same context", fifo));
            }
            std::vector<Context> newDelayContext = fifo->getOutputContext();

            if(!fifo->outputsInSamePartition()){
                throw std::runtime_error(ErrorHelpers::genErrorStr("All destination arcs from a FIFO must go to the same partition", fifo));
            }
            int newDelayPartition = fifo->getOutputPartition();

            //Need to create a delay to move the remaining elements into
            std::shared_ptr<Delay> delay = NodeFactory::createNode<Delay>(fifo->getParent());
            new_nodes.push_back(delay);
            delay->setPartitionNum(newDelayPartition);
            delay->setName("OverflowFIFOInitCondFrom" + fifo->getName());
            delay->setContext(newDelayContext);
            delay->setBlockingSpecializationDeferred(blockingAlreadyOccurred); //If blocking already arcs were already expanded but delay specialization was deferred
            delay->setDeferredBlockSize(fifo->getSubBlockSizeCreateIfNot(0)); //This is the sub-blocking length of the FIFO
            delay->setDeferredSubBlockSize(1); //The sub-blocking size should be set to 1
            if (newDelayContext.size() > 0) {
                int subcontext = newDelayContext[newDelayContext.size() - 1].getSubContext();
                newDelayContext[newDelayContext.size() - 1].getContextRoot()->addSubContextNode(subcontext, delay);
            }

            //Set initial conditions
            delay->setDelayValue(numItemsToMove);
            std::vector<NumericValue> delayInitConds;
            delayInitConds.insert(delayInitConds.end(), fifoInitialConditions.begin(),
                                  fifoInitialConditions.begin() + primitiveElementsToMove);
            delay->setInitCondition(delayInitConds);

            //Remove init conditions from fifoInitialConditions
            fifoInitialConditions.erase(
                    fifoInitialConditions.begin(),
                    fifoInitialConditions.begin() + primitiveElementsToMove);
            fifo->setInitConditionsCreateIfNot(0, fifoInitialConditions);

            //Rewire Outputs
            if(fifo->getOutputPorts().size() != 1){
                throw std::runtime_error(ErrorHelpers::genErrorStr("Error when reshaping FIFO initial conditions.  Number of output ports should be 1", fifo));
            }
            std::set<std::shared_ptr<Arc>> outputArcs = fifo->getOutputPort(0)->getArcs();
            for (auto it = outputArcs.begin(); it != outputArcs.end(); it++) {
                (*it)->setSrcPortUpdateNewUpdatePrev(delay->getOutputPortCreateIfNot(0));
            }

            //Rewire Order Constraints
            std::set<std::shared_ptr<Arc>> orderConstraintArcs2Rewire = fifo->getOrderConstraintOutputPortCreateIfNot()->getArcs();
            for (auto it = orderConstraintArcs2Rewire.begin(); it != orderConstraintArcs2Rewire.end(); it++) {
                (*it)->setSrcPortUpdateNewUpdatePrev(delay->getOrderConstraintOutputPortCreateIfNot());
            }

            //Connect delay to output
            std::shared_ptr<Arc> newInputArc = Arc::connectNodes(fifo->getOutputPortCreateIfNot(0),
                                                                 delay->getInputPortCreateIfNot(0),
                                                                 origInputArc->getDataType(),
                                                                 origInputArc->getSampleTime());
            new_arcs.push_back(newInputArc);
            //Do not actually need to re-wire the input port
        }
    }
}

void MultiThreadPasses::propagatePartitionsFromSubsystemsToChildren(std::set<std::shared_ptr<Node>>& nodes, int partition){
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

void MultiThreadPasses::mergeFIFOs(
        std::map<std::pair<int, int>, std::vector<std::shared_ptr<ThreadCrossingFIFO>>> &fifoMap,
        std::vector<std::shared_ptr<Node>> &nodesToAdd,
        std::vector<std::shared_ptr<Node>> &nodesToRemove,
        std::vector<std::shared_ptr<Arc>> &arcsToAdd,
        std::vector<std::shared_ptr<Arc>> &arcsToRemove,
        std::vector<std::shared_ptr<Node>> &addToTopLevel,
        bool ignoreContexts, bool verbose,
        bool blockingAlreadyOccurred){

    if(verbose){
        std::cout << "*** FIFO Merge ***" << std::endl;
    }

    //Run through the lists of FIFOs between partitions
    //Getting the key set first and using that since we will be modifying the map
    std::set<std::pair<int, int>> partitonCrossings;
    for(auto partCrossingFIFOEntry : fifoMap){
        partitonCrossings.insert(partCrossingFIFOEntry.first);
    }

    for(auto partitionCrossing : partitonCrossings){
        std::vector<std::shared_ptr<ThreadCrossingFIFO>> partitionCrossingFIFOs = fifoMap[partitionCrossing];

        if(partitionCrossingFIFOs.size()>1){
            //Need to at least check if merging is possible

            //Need to get what FIFOs to merge
            std::vector<std::vector<std::shared_ptr<ThreadCrossingFIFO>>> fifoSetToMerge;

            if(ignoreContexts){
                fifoSetToMerge.push_back(partitionCrossingFIFOs); //Merge all of them
            }else{
                //Need to check contexts
                //Should be able to use context vector as a key to the map since it has the comparison operators which check the contents
                std::map<std::vector<Context>, std::vector<std::shared_ptr<ThreadCrossingFIFO>>> contextFIFOs;

                for(std::shared_ptr<ThreadCrossingFIFO> fifo : partitionCrossingFIFOs){
                    std::vector<Context> contextStack = fifo->getContext();

                    //Remove clock domains from the context tree since different clock domains can be handled in a single
                    //FIFO by adjusting block sizing
                    //Also remove blocking domains as different block sizes can be handled by the different ports
                    for(int i = 0; i<contextStack.size(); ){
                        if(GeneralHelper::isType<ContextRoot, ClockDomain>(contextStack[i].getContextRoot()) != nullptr || GeneralHelper::isType<ContextRoot, BlockingDomain>(contextStack[i].getContextRoot()) != nullptr){
                            //Need to remove this entry from the context stack
                            contextStack.erase(contextStack.begin()+i);
                            //Do not increment i since entries will shift down
                        }else{
                            i++;
                        }
                    }

                    contextFIFOs[contextStack].push_back(fifo);
                }

                //Convert to vector of vectors
                for(auto fifoBundle : contextFIFOs){
                    fifoSetToMerge.push_back(fifoBundle.second);
                }
            }

            std::vector<std::shared_ptr<ThreadCrossingFIFO>> newCrossingFIFOs;

            //Merge each set within this partition crossing
            for(auto fifosToMerge : fifoSetToMerge) {

                //Find the minimum number of initial conditions across the FIFOs in the group
                int minInitialConditionsBlocks = -1;
                for(auto fifo : fifosToMerge){
                    for(int portNum = 0; portNum<fifo->getInputPorts().size(); portNum++) {
                        if (fifo->getInputPorts().size() > 1) {
                            throw std::runtime_error(ErrorHelpers::genErrorStr(
                                    "FIFOs with multiple input/output ports are not yet supported by FIFO merge",
                                    fifo));
                        }

                        int initialConditionBlocks = fifo->getInitConditionsCreateIfNot(portNum).size() /
                                                     (fifo->getInputPort(portNum)->getDataType().numberOfElements() /
                                                     fifo->getSubBlockSizeCreateIfNot(portNum)) / //This is because, if the sub-block size is > 1, the number of elements in the type will report the sum of the elements of all items in the sub-block
                                                     fifo->getBlockSizeCreateIfNot(portNum);

                        if (minInitialConditionsBlocks < 0) {
                            //Initialize
                            minInitialConditionsBlocks = initialConditionBlocks;
                        } else {
                            if (initialConditionBlocks < minInitialConditionsBlocks) {
                                minInitialConditionsBlocks = initialConditionBlocks;
                            }
                        }
                    }
                }

                //Reshape the FIFOs
                for(auto fifo : fifosToMerge){
                    MultiThreadPasses::reshapeFIFOInitialConditionsToSizeBlocks(fifo, minInitialConditionsBlocks, nodesToAdd, nodesToRemove, arcsToAdd, arcsToRemove, blockingAlreadyOccurred);
                }

                //Actually Merge the FIFOs in this set
                //The 0th FIFO will remain and the other ports will be added to it
                std::shared_ptr<ThreadCrossingFIFO> fifoToMergeInto = fifosToMerge[0];
                std::shared_ptr<SubSystem> commonAncestor = fifoToMergeInto->getParent();
                std::vector<Context> commonContext = fifoToMergeInto->getContext();
                for(int fifoInd = 1; fifoInd<fifosToMerge.size(); fifoInd++){
                    std::shared_ptr<ThreadCrossingFIFO> fifoToMergeFrom = fifosToMerge[fifoInd];

                    std::vector<std::shared_ptr<InputPort>> oldInputPorts = fifoToMergeFrom->getInputPorts();
                    std::vector<std::shared_ptr<OutputPort>> oldOutputPorts = fifoToMergeFrom->getOutputPorts();

                    //Merge each port pair
                    for(int oldPortNum = 0; oldPortNum<oldInputPorts.size(); oldPortNum++) {
                        int newPortNum = fifoToMergeInto->getInputPorts().size();
                        //Transfer Input Arcs
                        std::set<std::shared_ptr<Arc>> inputArcs = oldInputPorts[oldPortNum]->getArcs();
                        for(auto arc : inputArcs){
                            arc->setDstPortUpdateNewUpdatePrev(fifoToMergeInto->getInputPortCreateIfNot(newPortNum));
                        }

                        //Transfer Output Arcs
                        std::set<std::shared_ptr<Arc>> outputArcs = oldOutputPorts[oldPortNum]->getArcs();
                        for(auto arc : outputArcs){
                            arc->setSrcPortUpdateNewUpdatePrev(fifoToMergeInto->getOutputPortCreateIfNot(newPortNum));
                        }

                        //Transfer Init Condition
                        fifoToMergeInto->setInitConditionsCreateIfNot(newPortNum, fifoToMergeFrom->getInitConditionsCreateIfNot(oldPortNum));

                        //Transfer Block Size
                        fifoToMergeInto->setBlockSize(newPortNum, fifoToMergeFrom->getBlockSizeCreateIfNot(oldPortNum));

                        //Transfer Sub Blocking Domains
                        fifoToMergeInto->setSubBlockSize(newPortNum, fifoToMergeFrom->getSubBlockSizeCreateIfNot(oldPortNum));

                        //Transfer Clock Domain
                        fifoToMergeInto->setClockDomain(newPortNum, fifoToMergeFrom->getClockDomainCreateIfNot(oldPortNum));

                        //Transfer Indexing Exprs
                        fifoToMergeInto->setCBlockIndexExprInput(newPortNum, fifoToMergeFrom->getCBlockIndexExprInputCreateIfNot(oldPortNum));
                        fifoToMergeInto->setCBlockIndexExprOutput(newPortNum, fifoToMergeFrom->getCBlockIndexExprOutputCreateIfNot(oldPortNum));
                    }

                    //Merge the other properties of this FIFO

                    //Transfer Order Constraint Input Arcs
                    std::set<std::shared_ptr<Arc>> orderConstraintInputArcs = fifoToMergeFrom->getOrderConstraintInputArcs();
                    for(auto arc : orderConstraintInputArcs){
                        arc->setDstPortUpdateNewUpdatePrev(fifoToMergeInto->getOrderConstraintInputPort());
                    }

                    //Transfer Order Constraint Output Arcs
                    std::set<std::shared_ptr<Arc>> orderConstraintOutputArcs = fifoToMergeFrom->getOrderConstraintOutputArcs();
                    for(auto arc : orderConstraintOutputArcs){
                        arc->setSrcPortUpdateNewUpdatePrev(fifoToMergeInto->getOrderConstraintOutputPort());
                    }

                    //Get common ancestor
                    commonAncestor = GraphAlgs::findMostSpecificCommonAncestorParent(commonAncestor, fifoToMergeFrom);

                    //Get common context
                    std::vector<Context> fifoToMergeFromContext = fifoToMergeFrom->getContext();
                    int commonContextInd = Context::findMostSpecificCommonContext(commonContext, fifoToMergeFromContext);
                    while(commonContext.size() > commonContextInd+1){ //+1 Since common contextInd is the index at which they are common which starts from 0
                        commonContext.pop_back();
                    }

                    if(verbose){
                        std::cout << "FIFO Merged: " << fifoToMergeFrom->getFullyQualifiedName() << " [ID:" << fifoToMergeFrom->getId() << "]"
                                  << " into " << fifoToMergeInto->getFullyQualifiedName() << " [ID:" << fifoToMergeInto->getId() << "]" << std::endl;
                    }

                    //Delete the old FIFO
                    nodesToRemove.push_back(fifoToMergeFrom);
                }

                //Relocate the 0th FIFO to common ancestor (if different from orig)
                //Add to topLvl if common ancestor is nullptr
                if(commonAncestor != fifoToMergeInto->getParent()){
                    std::string oldLocation = fifoToMergeInto->getFullyQualifiedName();

                    fifoToMergeInto->setParentUpdateNewUpdatePrev(commonAncestor);
                    if(commonAncestor == nullptr){
                        addToTopLevel.push_back(fifoToMergeInto);
                    }

                    if(verbose){
                        std::cout << "Moving " << oldLocation
                                  << " [ID: " << fifoToMergeInto->getId() << "]"
                                  << " to " << fifoToMergeInto->getFullyQualifiedName() << std::endl;
                    }
                }

                //Update context of 0th FIFO (if different from orig)
                if(commonContext != fifoToMergeInto->getContext()){
                    std::cout << "Setting new Context for " << fifoToMergeInto->getFullyQualifiedName()
                              << " [ID: " << fifoToMergeInto->getId() << "]" << std::endl;
                    fifoToMergeInto->setContextUpdateNewUpdatePrev(commonContext);
                }

                //Add the merged to the updated list
                newCrossingFIFOs.push_back(fifoToMergeInto);
            }

            //Update fifo map for this partition crossing
            fifoMap[partitionCrossing] = newCrossingFIFOs;
        }
    }
}

void MultiThreadPasses::propagatePartitionsFromSubsystemsToChildren(Design &design){
    std::vector<std::shared_ptr<Node>> topLevelNodes = design.getTopLevelNodes();
    std::set<std::shared_ptr<Node>> topLevelNodeSet;
    topLevelNodeSet.insert(topLevelNodes.begin(), topLevelNodes.end());

    propagatePartitionsFromSubsystemsToChildren(topLevelNodeSet, -1);
}