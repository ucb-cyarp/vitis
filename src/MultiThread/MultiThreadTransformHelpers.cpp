//
// Created by Christopher Yarp on 10/4/19.
//

#include "MultiThreadTransformHelpers.h"

#include "PrimitiveNodes/Delay.h"
#include <iostream>

void MultiThreadTransformHelpers::absorbAdjacentDelaysIntoFIFOs(std::map<std::pair<int, int>, std::vector<std::shared_ptr<ThreadCrossingFIFO>>> fifos,
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
            MultiThreadTransformHelpers::reshapeFIFOInitialConditionsForBlockSize(fifoPtrs[i], new_nodes, deleted_nodes, new_arcs, deleted_arcs, printActions);
        }
    }
}

MultiThreadTransformHelpers::AbsorptionStatus
MultiThreadTransformHelpers::absorbAdjacentInputDelayIfPossible(std::shared_ptr<ThreadCrossingFIFO> fifo,
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
                if (srcDelay->getStateUpdateNodes().size() != 0) {
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

MultiThreadTransformHelpers::AbsorptionStatus
MultiThreadTransformHelpers::absorbAdjacentOutputDelayIfPossible(std::shared_ptr<ThreadCrossingFIFO> fifo,
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
                    if(delayInitConditions.size() < numToAbsorb){
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

void MultiThreadTransformHelpers::reshapeFIFOInitialConditionsForBlockSize(std::shared_ptr<ThreadCrossingFIFO> fifo,
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
        MultiThreadTransformHelpers::reshapeFIFOInitialConditions(fifo, numElementsToMove, new_nodes, deleted_nodes, new_arcs, deleted_arcs);

        if (printActions) {
            std::cout << "FIFO Initial Conditions Reshaped: " << fifo->getFullyQualifiedName() << " [ID:" << fifo->getId() << "]"
                      << " " << numElementsToMove << " initial conditions moved into delay at input" << std::endl;
        }
    }
}

void MultiThreadTransformHelpers::reshapeFIFOInitialConditionsToSize(std::shared_ptr<ThreadCrossingFIFO> fifo,
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
        MultiThreadTransformHelpers::reshapeFIFOInitialConditions(fifo, numElementsToMove, new_nodes, deleted_nodes, new_arcs, deleted_arcs);

        if (printActions) {
            std::cout << "FIFO Initial Conditions Reshaped: " << fifo->getFullyQualifiedName() << " [ID:" << fifo->getId() << "]"
                      << " " << numElementsToMove << " initial conditions moved into delay at input" << std::endl;
        }
    }
}

void MultiThreadTransformHelpers::reshapeFIFOInitialConditions(std::shared_ptr<ThreadCrossingFIFO> fifo,
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
        if(fifo->getInputPorts().size() != 1){
            throw std::runtime_error(ErrorHelpers::genErrorStr("Error when reshaping FIFO initial conditions.  Number of input ports should be 1", fifo));
        }

        std::set<std::shared_ptr<Arc>> inputArcs = fifo->getInputPort(0)->getArcs();

        if(inputArcs.size() != 1){
            throw std::runtime_error(ErrorHelpers::genErrorStr("Error when reshaping FIFO initial conditions.  Number of input arcs should be 1", fifo));
        }

        std::shared_ptr<Arc> origInputArc = *inputArcs.begin();

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
            if (fifoContext.size() > 0) {
                int subcontext = fifoContext[fifoContext.size() - 1].getSubContext();
                fifoContext[fifoContext.size() - 1].getContextRoot()->addSubContextNode(subcontext, delay);
            }

            //Set initial conditions
            delay->setDelayValue(numElementsToMove);
            std::vector<NumericValue> delayInitConds;
            delayInitConds.insert(delayInitConds.end(),
                                  fifoInitialConditions.begin() + (fifoInitialConditions.size() - numElementsToMove),
                                  fifoInitialConditions.end());
            delay->setInitCondition(delayInitConds);

            //Remove init conditions from fifoInitialConditions
            fifoInitialConditions.erase(
                    fifoInitialConditions.begin() + (fifoInitialConditions.size() - numElementsToMove),
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
                throw std::runtime_error(ErrorHelpers::genErrorStr("All destination arcs from a FIFO must go to the same partiton", fifo));
            }
            int newDelayPartition = fifo->getOutputPartition();

            //Need to create a delay to move the remaining elements into
            std::shared_ptr<Delay> delay = NodeFactory::createNode<Delay>(fifo->getParent());
            new_nodes.push_back(delay);
            delay->setPartitionNum(newDelayPartition);
            delay->setName("OverflowFIFOInitCondFrom" + fifo->getName());
            delay->setContext(newDelayContext);
            if (newDelayContext.size() > 0) {
                int subcontext = newDelayContext[newDelayContext.size() - 1].getSubContext();
                newDelayContext[newDelayContext.size() - 1].getContextRoot()->addSubContextNode(subcontext, delay);
            }

            //Set initial conditions
            delay->setDelayValue(numElementsToMove);
            std::vector<NumericValue> delayInitConds;
            delayInitConds.insert(delayInitConds.end(), fifoInitialConditions.begin(),
                                  fifoInitialConditions.begin() + numElementsToMove);
            delay->setInitCondition(delayInitConds);

            //Remove init conditions from fifoInitialConditions
            fifoInitialConditions.erase(
                    fifoInitialConditions.begin(),
                    fifoInitialConditions.begin() + numElementsToMove);
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

std::map<std::pair<int, int>, std::vector<std::shared_ptr<ThreadCrossingFIFO>>>
MultiThreadTransformHelpers::mergeFIFOs(std::map<std::pair<int, int>, std::vector<std::shared_ptr<ThreadCrossingFIFO>>> fifos,
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

void MultiThreadTransformHelpers::propagatePartitionsFromSubsystemsToChildren(std::set<std::shared_ptr<Node>>& nodes, int partition){
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