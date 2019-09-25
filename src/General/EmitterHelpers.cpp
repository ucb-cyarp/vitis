//
// Created by Christopher Yarp on 9/3/19.
//

#include "EmitterHelpers.h"
#include "PrimitiveNodes/Delay.h"
#include "PrimitiveNodes/BlackBox.h"
#include "GraphCore/ContextRoot.h"
#include <iostream>

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
    //Check if FIFO full
    if(fifo->getInitConditions().size() < (fifo->getFifoLength() - fifo->getBlockSize())) {
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
                    std::vector<NumericValue> fifoInitConds = fifo->getInitConditions();

                    if (fifoInitConds.size() + fifoInitConds.size() <= (fifo->getFifoLength() - fifo->getBlockSize())) {
                        //Can absorb complete delay
                        fifoInitConds.insert(fifoInitConds.end(), delayInitConds.begin(),
                                             delayInitConds.end());  //Because this is at the input to the FIFO, the initial conditions are appended.
                        fifo->setInitConditions(fifoInitConds);

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

                        int numToAbsorb = fifo->getFifoLength() - fifo->getBlockSize() - fifoInitConds.size();

                        fifoInitConds.insert(fifoInitConds.end(), delayInitConds.begin(), delayInitConds.begin()+numToAbsorb);
                        fifo->setInitConditions(fifoInitConds);
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
    //Check if FIFO full
    if (fifo->getInitConditions().size() < (fifo->getFifoLength() - fifo->getBlockSize())) {
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
                std::vector<NumericValue> fifoInitConds = fifo->getInitConditions();
                int roomInFifo = fifo->getFifoLength() - fifo->getBlockSize() - fifoInitConds.size();
                int numToAbsorb = std::min(roomInFifo, (int) longestPostfix.size());

                //Set FIFO new initial conditions
                //Append the existing FIFO init conditions onto the back of the postfix
                int remainingPostfix = longestPostfix.size() - numToAbsorb;
                longestPostfix.insert(longestPostfix.end(), longestPostfix.begin()+remainingPostfix, longestPostfix.end());

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
    statements += structTypeName + " *" + castStructName + " = (*" + structTypeName + ") " + structName + ";\n";

    statements += "\t//Input FIFOs";
    for(unsigned long i = 0; i<inputFIFOs.size(); i++){
        std::vector<Variable> fifoSharedVars = inputFIFOs[i]->getFIFOSharedVariables();

        for(int j = 0; j<fifoSharedVars.size(); j++){
            //All should be pointers
            Variable var = fifoSharedVars[j];
//            DataType varType = var.getDataType();
//            varType.setWidth(varType.getWidth()*blockSize);
//            var.setDataType(varType);
            //Pass as not volatile
            var.setVolatileVar(false);

            statements += var.getCPtrDecl(false) + " = " + castStructName + "->" + var.getCVarName(false) + ";\n";

            //Check if complex
            if(var.getDataType().isComplex()){
                statements += var.getCPtrDecl(true) + " = " + castStructName + "->" + var.getCVarName(true) + ";\n";
            }
        }
    }

    statements += "\t//Output FIFOs";
    for(unsigned long i = 0; i<outputFIFOs.size(); i++){
        std::vector<Variable> fifoSharedVars = outputFIFOs[i]->getFIFOSharedVariables();

        for(int j = 0; j<fifoSharedVars.size(); j++){
            //All should be pointers
            Variable var = fifoSharedVars[j];
//            DataType varType = var.getDataType();
//            varType.setWidth(varType.getWidth()*blockSize);
//            var.setDataType(varType);
            //Pass as not volatile
            var.setVolatileVar(false);

            statements += var.getCPtrDecl(false) + " = " + castStructName + "->" + var.getCVarName(false) + ";\n";

            //Check if complex
            if(var.getDataType().isComplex()){
                statements += var.getCPtrDecl(true) + " = " + castStructName + "->" + var.getCVarName(true) + ";\n";
            }
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
    check += "}\n";

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
        Variable fifoSrcVar = fifos[i]->getCStateVar(); //This is the temp we are creating for.
        std::string tmpName = fifoSrcVar.getName() + "_tmp";
        std::string structType = fifos[i]->getFIFOStructTypeName();
        exprs.push_back(structType + " " + tmpName + ";");
    }

    return exprs;
}

std::vector<std::string> EmitterHelpers::createFIFOWriteTemps(std::vector<std::shared_ptr<ThreadCrossingFIFO>> fifos){
    std::vector<std::string> exprs;
    for(int i = 0; i<fifos.size(); i++) {
        Variable fifoDstVar = fifos[i]->getCStateInputVar(); //This is the temp we are creating for.
        std::string tmpName = fifoDstVar.getName() + "_tmp";
        std::string structType = fifos[i]->getFIFOStructTypeName();
        exprs.push_back(structType + " " + tmpName + ";");
    }

    return exprs;
}

std::vector<std::string> EmitterHelpers::createAndInitializeFIFOWriteTemps(std::vector<std::shared_ptr<ThreadCrossingFIFO>> fifos, std::vector<NumericValue> defaultVal){
    std::vector<std::string> exprs;
    for(int i = 0; i<fifos.size(); i++) {
        Variable fifoDstVar = fifos[i]->getCStateInputVar(); //This is the temp we are creating for.
        std::string tmpName = fifoDstVar.getName() + "_tmp";
        std::string structType = fifos[i]->getFIFOStructTypeName();
        exprs.push_back(structType + " " + tmpName + ";");

        int blockSize = fifos[i]->getBlockSize();
        DataType dt = fifos[i]->getCStateVar().getDataType();
        int width = dt.getWidth();
        if(blockSize == 1 && width == 1){
            exprs.push_back(tmpName + ".real = " + defaultVal[i].toStringComponent(false, dt) + ";");
            if(dt.isComplex()){
                exprs.push_back(tmpName + ".imag = " + defaultVal[i].toStringComponent(true, dt) + ";");
            }
        }else if((blockSize == 1 && width > 1) || (blockSize > 1 && width == 1)){
            int entries = std::max(blockSize, width);
            std::string realInit = tmpName + ".real = {";
            for(int j = 0; j<entries; j++){
                if(j>0){
                    realInit += ", ";
                }
                realInit += defaultVal[i].toStringComponent(false, dt);
            }
            realInit += "};";
            exprs.push_back(realInit);

            if(dt.isComplex()) {
                std::string imagInit = tmpName + ".imag = {";
                for (int j = 0; j < entries; j++) {
                    if (j > 0) {
                        imagInit += ", ";
                    }
                    imagInit += defaultVal[i].toStringComponent(true, dt);
                }
                imagInit += "};";
                exprs.push_back(imagInit);
            }
        }else{
            std::string realInit = tmpName + ".real = {";
            for(int j = 0; j<blockSize; j++){
                if(j>0){
                    realInit += ", ";
                }
                realInit += "{";
                for(int k = 0; k<width; k++) {
                    if(k>0){
                        realInit += ", ";
                    }
                    realInit += defaultVal[i].toStringComponent(false, dt);
                }
                realInit += "}";
            }
            realInit += "};";
            exprs.push_back(realInit);

            if(dt.isComplex()) {
                std::string imagInit = tmpName + ".imag = {";
                for(int j = 0; j<blockSize; j++){
                    if(j>0){
                        imagInit += ", ";
                    }
                    realInit += "{";
                    for(int k = 0; k<width; k++) {
                        if(k>0){
                            imagInit += ", ";
                        }
                        imagInit += defaultVal[i].toStringComponent(false, dt);
                    }
                    imagInit += "}";
                }
                imagInit += "};";
                exprs.push_back(imagInit);
            }
        }
    }

    return exprs;
}

std::vector<std::string> EmitterHelpers::readFIFOsToTemps(std::vector<std::shared_ptr<ThreadCrossingFIFO>> fifos) {
    std::vector<std::string> exprs;

    for(int i = 0; i<fifos.size(); i++){
        Variable fifoSrcVar = fifos[i]->getCStateVar(); //This is the temp we are creating for.
        std::string tmpName = fifoSrcVar.getName() + "_tmp";
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
        Variable fifoDstVar = fifos[i]->getCStateInputVar(); //This is the temp we are creating for.
        std::string tmpName = fifoDstVar.getName() + "_tmp";
        //However, the type will be the FIFO structure rather than the variable type directly.
        //The fifo reads in terms of blocks with the components stored in a structure
        //When calling the function, the relavent component of the structure is passed as an argument

        fifos[i]->emitCWriteToFIFO(exprs, tmpName, 1);
    }

    return exprs;
}

void EmitterHelpers::propagatePartitionsFromSubsystemsToChildren(std::set<std::shared_ptr<Node>>& nodes, int partition, bool firstLevel){
    for(auto it = nodes.begin(); it != nodes.end(); it++){
        //Do this first since expanded nodes are also subsystems
        std::shared_ptr<ExpandedNode> asExpandedNode = GeneralHelper::isType<Node, ExpandedNode>(*it);
        if(asExpandedNode){
            //This is a special case where the subsystem takes the partition of the parent and not itself
            //This is because
            if((!firstLevel) && (partition != -1)){
                asExpandedNode->getOrigNode()->setPartitionNum(partition);

                std::set<std::shared_ptr<Node>> children = asExpandedNode->getChildren();
                propagatePartitionsFromSubsystemsToChildren(children, partition, firstLevel);
            }
        }else{
            std::shared_ptr<SubSystem> asSubsystem = GeneralHelper::isType<Node, SubSystem>(*it);
            if(asSubsystem){
                int nextPartition = asSubsystem->getPartitionNum();
                if(nextPartition == -1){
                    nextPartition = partition;
                }
                std::set<std::shared_ptr<Node>> children = asSubsystem->getChildren();
                propagatePartitionsFromSubsystemsToChildren(children, nextPartition, false);
            }else{
                //Standard node, set partition if !firstLevel
                if(!firstLevel && partition != -1){
                    (*it)->setPartitionNum(partition);
                }
            }
        }
    }
}