//
// Created by Christopher Yarp on 10/4/19.
//

#include "EmitterHelpers.h"

#include "GraphCore/Node.h"
#include "GraphCore/ContextRoot.h"
#include "PrimitiveNodes/BlackBox.h"
#include "GeneralHelper.h"
#include "ErrorHelpers.h"
#include "GraphCore/StateUpdate.h"
#include <iostream>
#include <fstream>

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
    for (auto it = orderedNodes.begin(); it != orderedNodes.end(); it++) {

//        std::cout << "Writing " << (*it)->getFullyQualifiedName() << std::endl;

        if (firstNode) {
            firstNode = false;
            partition = (*it)->getPartitionNum();
        } else if (checkForPartitionChange) {
            if (partition != (*it)->getPartitionNum()) {
                //It is unclear what the implication would be for how to open and close contexts if the partition changes in the middle of op emit
                //Therefore, this condition should be checked for
                throw std::runtime_error(
                        ErrorHelpers::genErrorStr("C Emit Error - Partition change occurred when emitting ops", *it));
            }
        }

        //Check if the context has changed
        std::vector<Context> nodeContext = (*it)->getContext();

        std::vector<std::string> contextStatements;

        if (nodeContext != lastEmittedContext) {

            //Check if previous context(s) should be closed (run up the stack until the contexts are the same - we can do this because contexts are in a strict hierarchy [ie. tree])
            for (unsigned long i = 0; i < lastEmittedContext.size(); i++) {
                unsigned long ind = lastEmittedContext.size() - 1 - i;

                if (ind >= nodeContext.size()) {
                    //The new context is less deep than this.  Therefore, we should close this context
                    if (subContextEmittedCount.find(lastEmittedContext[ind].getContextRoot()) ==
                        subContextEmittedCount.end()) {
                        subContextEmittedCount[lastEmittedContext[ind].getContextRoot()] = 0;
                    }

                    if (contextFirst.size() - 1 < ind || contextFirst[ind]) {
                        //This context was created with a call to first
                        lastEmittedContext[ind].getContextRoot()->emitCContextCloseFirst(contextStatements, schedType,
                                                                                         lastEmittedContext[ind].getSubContext(),
                                                                                         (*it)->getPartitionNum());
                    } else if (subContextEmittedCount[lastEmittedContext[ind].getContextRoot()] >=
                               lastEmittedContext[ind].getContextRoot()->getNumSubContexts() - 1) {
                        lastEmittedContext[ind].getContextRoot()->emitCContextCloseLast(contextStatements, schedType,
                                                                                        lastEmittedContext[ind].getSubContext(),
                                                                                        (*it)->getPartitionNum());
                    } else {
                        lastEmittedContext[ind].getContextRoot()->emitCContextCloseMid(contextStatements, schedType,
                                                                                       lastEmittedContext[ind].getSubContext(),
                                                                                       (*it)->getPartitionNum());
                    }

                    //Remove this level of the contextFirst stack because the family was exited
                    contextFirst.pop_back();

                    //Add that context to the exited context set
                    alreadyEmittedSubContexts.insert(lastEmittedContext[ind]);

                    //Increment emitted count by 1
                    subContextEmittedCount[lastEmittedContext[ind].getContextRoot()]++;

                } else if (lastEmittedContext[ind] != nodeContext[ind]) {
                    //TODO: change if split contexts are introduced -> would need to check for first context change, then emit all context closes up to that point
                    //could be either a first, mid, or last
                    if (contextFirst[ind]) { //contextFirst is guarenteed to exist because this level existed in the last emitted context
                        lastEmittedContext[ind].getContextRoot()->emitCContextCloseFirst(contextStatements, schedType,
                                                                                         lastEmittedContext[ind].getSubContext(),
                                                                                         (*it)->getPartitionNum());
                    } else if (subContextEmittedCount[lastEmittedContext[ind].getContextRoot()] >=
                               lastEmittedContext[ind].getContextRoot()->getNumSubContexts() - 1) {
                        lastEmittedContext[ind].getContextRoot()->emitCContextCloseLast(contextStatements, schedType,
                                                                                        lastEmittedContext[ind].getSubContext(),
                                                                                        (*it)->getPartitionNum());
                    } else {
                        lastEmittedContext[ind].getContextRoot()->emitCContextCloseMid(contextStatements, schedType,
                                                                                       lastEmittedContext[ind].getSubContext(),
                                                                                       (*it)->getPartitionNum());
                    }

                    if (lastEmittedContext[ind].getContextRoot() == nodeContext[ind].getContextRoot()) {
                        //In this case, the next context is in the same family, set the contextFirst to false and do not remove it
                        contextFirst[ind] = false;
                    } else {
                        //In this case, the context family is being left, remove the contextFirst entry for the family
                        contextFirst.pop_back();
                    }

                    //Add that context to the exited context set
                    alreadyEmittedSubContexts.insert(lastEmittedContext[ind]);

                    //Increment emitted count by 1
                    subContextEmittedCount[lastEmittedContext[ind].getContextRoot()]++;
                } else {
                    break; //We found the point root of both the old and new contexts, we can stop here
                }
            }

            //If new context(s) have been entered, emit the conditionals (run down the stack to find the root of both, then emit the context entries)
            for (unsigned long i = 0; i < nodeContext.size(); i++) {
                if (i >= lastEmittedContext.size()) {
                    //This context level is new, it must be the first time it has been entered

                    //Declare vars
                    std::vector<Variable> contextVars = nodeContext[i].getContextRoot()->getCContextVars();
                    for (unsigned long j = 0; j < contextVars.size(); j++) {
                        contextStatements.push_back(contextVars[j].getCVarDecl(false, true, false, true) + ";\n");

                        if (contextVars[j].getDataType().isComplex()) {
                            contextStatements.push_back(contextVars[j].getCVarDecl(true, true, false, true) + ";\n");
                        }
                    }

                    //Emit context
                    nodeContext[i].getContextRoot()->emitCContextOpenFirst(contextStatements, schedType,
                                                                           nodeContext[i].getSubContext(),
                                                                           (*it)->getPartitionNum());

                    //push back onto contextFirst
                    contextFirst.push_back(true);
                } else if (lastEmittedContext[i] !=
                           nodeContext[i]) {  //Because contexts must be purely hierarchical, new context is only entered if the context at this level is different from the previously emitted context.
                    //TODO: change if split contexts are introduced -> would need to check for first context change, then emit all context opens after that
                    //This context level already existed, could either be an first, mid, or last

                    //Check if the previous context had the same root.  If so, this is the first
                    if (lastEmittedContext[i].getContextRoot() != nodeContext[i].getContextRoot()) {
                        //Emit context vars
                        std::vector<Variable> contextVars = nodeContext[i].getContextRoot()->getCContextVars();
                        for (unsigned long j = 0; j < contextVars.size(); j++) {
                            contextStatements.push_back(contextVars[j].getCVarDecl(false, true, false, true) + ";\n");

                            if (contextVars[j].getDataType().isComplex()) {
                                contextStatements.push_back(
                                        contextVars[j].getCVarDecl(true, true, false, true) + ";\n");
                            }
                        }

                        //Emit context
                        nodeContext[i].getContextRoot()->emitCContextOpenFirst(contextStatements, schedType,
                                                                               nodeContext[i].getSubContext(),
                                                                               (*it)->getPartitionNum());

                        //This is a new family, push back
                        contextFirst.push_back(true);
                    } else {
                        if (subContextEmittedCount[nodeContext[i].getContextRoot()] >=
                            nodeContext[i].getContextRoot()->getNumSubContexts() -
                            1) { //Check if this is the last in the context family
                            nodeContext[i].getContextRoot()->emitCContextOpenLast(contextStatements, schedType,
                                                                                  nodeContext[i].getSubContext(),
                                                                                  (*it)->getPartitionNum());
                        } else {
                            nodeContext[i].getContextRoot()->emitCContextOpenMid(contextStatements, schedType,
                                                                                 nodeContext[i].getSubContext(),
                                                                                 (*it)->getPartitionNum());
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

        for (unsigned long i = 0; i < contextStatements.size(); i++) {
            cFile << contextStatements[i];
        }

        if (*it == outputMaster) {
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
                std::string expr_re = srcNode->emitC(cStatements_re, schedType, srcNodeOutputPortNum, false, true,
                                                     true);
                //emit the expressions
                unsigned long numStatements_re = cStatements_re.size();
                for (unsigned long j = 0; j < numStatements_re; j++) {
                    cFile << cStatements_re[j] << std::endl;
                }

                //emit the assignment
                Variable outputVar = Variable(outputMaster->getCOutputName(i), outputDataType);

                //Note that with matrix/vector ports an added dimension is prepended to the array when the block size is >1
                //Because of C/C++ array semantics, this results in the elements for each block being stored
                //contiguously.  For multidimensional arrays (ex. array[][5][5]), the index of the block can found
                //by simply employing array[block].  The compiler knows the size of the latter 2 dimensions.
                int varElements = outputVar.getDataType().numberOfElements();
                int varBytes = outputVar.getDataType().getCPUStorageType().getTotalBits() / 8;
                if (!outputVar.getDataType().isScalar()) {
                    if (blockSize > 1) {
                        //Can use the
                        cFile << "memcpy(output[0]." << outputVar.getCVarName(false)
                              << "[" << indVarName << "], " << expr_re << ", " << varElements * varBytes << ");"
                              << std::endl;
                    } else {
                        cFile << "memcpy(output[0]." << outputVar.getCVarName(false) << ", " << expr_re << ", "
                              << varElements * varBytes << ");" << std::endl;
                    }
                } else {
                    if (blockSize > 1) {
                        cFile << "output[0]." << outputVar.getCVarName(false) << "[" << indVarName << "] = " << expr_re
                              << ";" << std::endl;
                    } else {
                        cFile << "output[0]." << outputVar.getCVarName(false) << " = " << expr_re << ";" << std::endl;
                    }
                }

                //Emit Imag if Datatype is complex
                if (outputDataType.isComplex()) {
                    cFile << std::endl << "//-- Assign Imag Component --" << std::endl;
                    std::vector<std::string> cStatements_im;
                    std::string expr_im = srcNode->emitC(cStatements_im, schedType, srcNodeOutputPortNum, true, true,
                                                         true);
                    //emit the expressions
                    unsigned long numStatements_im = cStatements_im.size();
                    for (unsigned long j = 0; j < numStatements_im; j++) {
                        cFile << cStatements_im[j] << std::endl;
                    }

                    //emit the assignment
                    if (!outputVar.getDataType().isScalar()) {
                        if (blockSize > 1) {
                            cFile << "memcpy(output[0]." << outputVar.getCVarName(true)
                                  << "[" << indVarName << "], " << expr_im << ", " << varElements * varBytes << ");"
                                  << std::endl;
                        } else {
                            cFile << "memcpy(output[0]." << outputVar.getCVarName(true) << ", " << expr_im << ", "
                                  << varElements * varBytes << ");" << std::endl;
                        }
                    } else {
                        if (blockSize > 1) {
                            cFile << "output[0]." << outputVar.getCVarName(true) << "[" << indVarName << "] = "
                                  << expr_im << ";" << std::endl;
                        } else {
                            cFile << "output[0]." << outputVar.getCVarName(true) << " = " << expr_im << ";"
                                  << std::endl;
                        }
                    }
                }
            }
        } else if (GeneralHelper::isType<Node, StateUpdate>(*it) != nullptr) {
            std::shared_ptr<StateUpdate> stateUpdateNode = std::dynamic_pointer_cast<StateUpdate>(*it);

            //Emit the StateUpdate node's outputs first so that any downstream dependency (ex. a latch that it is connected to)
            //will have access to the value passed through.
            //Note that this fixes a possible bug when the input of the node is a delay.  The delay could possibly update before
            //a downstream latch updated.

            emitNode(*it, cFile, schedType);

            //Emit state update
            cFile << std::endl << "//---- State Update for "
                  << stateUpdateNode->getPrimaryNode()->getFullyQualifiedName() << " ----" << std::endl;

            std::vector<std::string> stateUpdateExprs;
            (*it)->emitCStateUpdate(stateUpdateExprs, schedType, nullptr); //Do not pass a stateUpdateSrc since this is emitting the StateUpdate for a primary node.  The StateUpdate node will set stateUpdateSrc to itself when calling the function in the primary node

            for (unsigned long j = 0; j < stateUpdateExprs.size(); j++) {
                cFile << stateUpdateExprs[j] << std::endl;
            }
        } else if ((*it)->hasState()) {
            //Call emit for state element input
            //The state element output is treated similarly to a constant and a variable name is always returned
            //The output has no dependencies within the cycle
            //The input can have dependencies

            //Emit comment
            cFile << std::endl << "//---- Calculate " << (*it)->getFullyQualifiedName() << " Inputs ----" << std::endl;

            std::vector<std::string> nextStateExprs;
            (*it)->emitCExprNextState(nextStateExprs, schedType);

            for (unsigned long j = 0; j < nextStateExprs.size(); j++) {
                cFile << nextStateExprs[j] << std::endl;
            }

        } else {
            //Emit standard node
            emitNode(*it, cFile, schedType);

        }

        lastEmittedContext = nodeContext;

    }
}

void EmitterHelpers::emitNode(std::shared_ptr<Node> nodeToEmit, std::ofstream &cFile, SchedParams::SchedType schedType){
    //Emit comment
    cFile << std::endl << "//---- Calculate " << nodeToEmit->getFullyQualifiedName() << " ----" << std::endl;

    unsigned long numOutputPorts = nodeToEmit->getOutputPorts().size();
    //Emit each output port
    //TODO: now checks for unconnected output ports.  Validate
    for (unsigned long i = 0; i < numOutputPorts; i++) {
        std::shared_ptr<OutputPort> outputPortBeingEmitted = nodeToEmit->getOutputPort(i);
        if (!outputPortBeingEmitted->getArcs().empty()) {

            std::vector<std::string> cStatementsRe;
            //Emit real component (force fanout)
            nodeToEmit->emitC(cStatementsRe, schedType, i, false, true,
                         true); //We actually do not use the returned expression.  Dependent nodes will get this by calling the emit function of this block again.

            for (unsigned long j = 0; j < cStatementsRe.size(); j++) {
                cFile << cStatementsRe[j] << std::endl;
            }

            if (nodeToEmit->getOutputPort(i)->getDataType().isComplex()) {
                //Emit imag component (force fanout)
                std::vector<std::string> cStatementsIm;
                //Emit real component (force fanout)
                nodeToEmit->emitC(cStatementsIm, schedType, i, true, true,
                             true); //We actually do not use the returned expression.  Dependent nodes will get this by calling the emit function of this block again.

                for (unsigned long j = 0; j < cStatementsIm.size(); j++) {
                    cFile << cStatementsIm[j] << std::endl;
                }
            }
        } else {
            std::cout << "Output Port " << i << " of " << nodeToEmit->getFullyQualifiedName() << "[ID: "
                      << nodeToEmit->getId() << "] not emitted because it is unconnected" << std::endl;
        }
    }
}

std::pair<std::vector<Variable>, std::vector<std::pair<int, int>>> EmitterHelpers::getCInputVariables(std::shared_ptr<MasterInput> inputMaster) {
    unsigned long numPorts = inputMaster->getOutputPorts().size();

    std::vector<Variable> inputVars;
    std::vector<std::pair<int, int>> inputRates;

    //TODO: Assuming port numbers do not have a discontinuity.  Validate this assumption.
    for(unsigned long i = 0; i<numPorts; i++){
        std::shared_ptr<OutputPort> input = inputMaster->getOutputPort(i); //output of input node
        std::shared_ptr<ClockDomain> inputClkDomain = inputMaster->getPortClkDomain(input);

        //TODO: This is a sanity check for the above todo
        if(input->getPortNum() != i){
            throw std::runtime_error(ErrorHelpers::genErrorStr("Port Number does not match its position in the port array", inputMaster));
        }

        DataType portDataType = input->getDataType();

        Variable var = Variable(inputMaster->getCInputName(i), portDataType);
        inputVars.push_back(var);
        if(inputClkDomain){
            inputRates.push_back(inputClkDomain->getRateRelativeToBase());
        }else{
            inputRates.emplace_back(1, 1);
        }
    }

    return std::pair<std::vector<Variable>, std::vector<std::pair<int, int>>>(inputVars, inputRates);
}

std::pair<std::vector<Variable>, std::vector<std::pair<int, int>>> EmitterHelpers::getCOutputVariables(std::shared_ptr<MasterOutput> outputMaster) {
    std::vector<Variable> outputVars;
    std::vector<std::pair<int, int>> outputRates;

    unsigned long numPorts = outputMaster->getInputPorts().size();

    //TODO: Assuming port numbers do not have a discontinuity.  Validate this assumption.
    for(unsigned long i = 0; i<numPorts; i++){
        std::shared_ptr<InputPort> output = outputMaster->getInputPort(i); //input of output node
        std::shared_ptr<ClockDomain> outputClkDomain = outputMaster->getPortClkDomain(output);

        //TODO: This is a sanity check for the above todo
        if(output->getPortNum() != i){
            throw std::runtime_error(ErrorHelpers::genErrorStr("Port Number does not match its position in the port array", outputMaster));
        }

        DataType portDataType = output->getDataType();

        Variable var = Variable(outputMaster->getCOutputName(i), portDataType);
        outputVars.push_back(var);
        if(outputClkDomain){
            outputRates.push_back(outputClkDomain->getRateRelativeToBase());
        }else{
            outputRates.emplace_back(1, 1);
        }
    }

    return std::pair<std::vector<Variable>, std::vector<std::pair<int, int>>>(outputVars, outputRates);
}

std::string EmitterHelpers::getCIOPortStructDefn(std::vector<Variable> portVars, std::vector<int> portBlockSizes, std::string structTypeName) {
    //TODO: Change this function to use the MasterIO Port ClockDomain
    //Trace where this function is called from (IO Port)
    std::string prototype = "#pragma pack(push, 4)\n";
    prototype += "typedef struct {\n";

    for(unsigned long i = 0; i<portVars.size(); i++){
        Variable var = portVars[i];

        DataType varType = var.getDataType();
        //Expand the width of the type by another dimension.  If scalar, extend the length
        //accordingly.
        varType = varType.expandForBlock(portBlockSizes[i]);
        //Note, these changes to dimensions do not propagate outside of this function

        var.setDataType(varType);

        prototype += "\t" + var.getCVarDecl(false, true, false, true) + ";\n";

        //Check if complex
        if(var.getDataType().isComplex()){
            prototype += "\t" + var.getCVarDecl(true, true, false, true) + ";\n";
        }
    }

    prototype += "} " + structTypeName +  ";\n";
    prototype += "#pragma pack(pop)";
    return prototype;
}

std::string EmitterHelpers::stringEmitTypeHeader(std::string path){
    std::string typeHeaderFileName = VITIS_TYPE_NAME;
    std::cout << "Emitting C File: " << path << "/" << typeHeaderFileName << ".h" << std::endl;
    //#### Emit .h file ####
    std::ofstream headerFile;
    headerFile.open(path+"/"+typeHeaderFileName+".h", std::ofstream::out | std::ofstream::trunc);

    std::string fileNameUpper =  GeneralHelper::toUpper(typeHeaderFileName);
    headerFile << "#ifndef " << fileNameUpper << "_H" << std::endl;
    headerFile << "#define " << fileNameUpper << "_H" << std::endl;
    headerFile << "#include <stdint.h>" << std::endl;
    headerFile << "#include <stdbool.h>" << std::endl;
    headerFile << "typedef uint8_t vitisBool_t;" << std::endl;

    headerFile << "#endif" << std::endl;

    headerFile.close();

    return typeHeaderFileName + ".h";
}

std::string EmitterHelpers::emitParametersHeader(std::string path, std::string fileNamePrefix, int blockSize){
    std::string fileName = fileNamePrefix + "_parameters";
    std::cout << "Emitting C File: " << path << "/" << fileName << ".h" << std::endl;
    //#### Emit .h file ####
    std::ofstream headerFile;
    headerFile.open(path + "/" + fileName + ".h", std::ofstream::out | std::ofstream::trunc);

    std::string fileNameUpper =  GeneralHelper::toUpper(fileName);
    headerFile << "#ifndef " << fileNameUpper << "_H" << std::endl;
    headerFile << "#define " << fileNameUpper << "_H" << std::endl;
    headerFile << std::endl;

    headerFile << "#define " << GeneralHelper::toUpper(fileNamePrefix) << "_BLOCK_SIZE (" << blockSize << ")" << std::endl;

    headerFile << std::endl;
    headerFile << "#endif" << std::endl;
    headerFile.close();

    return fileName+".h";
}

std::string EmitterHelpers::emitTelemetryHelper(std::string path, std::string fileNamePrefix){
    std::string fileName = fileNamePrefix + "_telemetry_helpers";
    std::cout << "Emitting C File: " << path << "/" << fileName << ".h" << std::endl;
    //#### Emit .h file ####
    std::ofstream headerFile;
    headerFile.open(path + "/" + fileName + ".h", std::ofstream::out | std::ofstream::trunc);

    std::string fileNameUpper =  GeneralHelper::toUpper(fileName);
    headerFile << "#ifndef " << fileNameUpper << "_H" << std::endl;
    headerFile << "#define " << fileNameUpper << "_H" << std::endl;
    headerFile << std::endl;

    headerFile << "typedef struct timespec timespec_t;" << std::endl;
    headerFile << "double difftimespec(timespec_t* a, timespec_t* b);" << std::endl;
    headerFile << "double timespecToDouble(timespec_t* a);" << std::endl;

    headerFile << std::endl;
    headerFile << "#endif" << std::endl;
    headerFile.close();

    std::cout << "Emitting C File: " << path << "/" << fileName << ".c" << std::endl;
    //#### Emit .c file ####
    std::ofstream cFile;
    cFile.open(path + "/" + fileName + ".c", std::ofstream::out | std::ofstream::trunc);
    cFile << "#ifndef _GNU_SOURCE" << std::endl;
    cFile << "#define _GNU_SOURCE //For clock_gettime" << std::endl;
    cFile << "#endif" << std::endl;
    cFile << "#include \"" << fileName << ".h" << "\"" << std::endl;
    cFile << "#include <unistd.h>" << std::endl;
    cFile << "#include <time.h>" << std::endl;
    cFile << std::endl;

    cFile << "double difftimespec(timespec_t* a, timespec_t* b){"  << std::endl;
    cFile << "    double a_double = a->tv_sec + (a->tv_nsec)*(0.000000001);" << std::endl;
    cFile << "    double b_double = b->tv_sec + (b->tv_nsec)*(0.000000001);" << std::endl;
    cFile << "    return a_double - b_double;" << std::endl;
    cFile << "}" << std::endl;

    cFile << "double timespecToDouble(timespec_t* a){" << std::endl;
    cFile << "    double a_double = a->tv_sec + (a->tv_nsec)*(0.000000001);" << std::endl;
    cFile << "    return a_double;" << std::endl;
    cFile << "}" << std::endl;
    cFile.close();

    return fileName+".h";
}

void EmitterHelpers::transferArcs(std::shared_ptr<Node> from, std::shared_ptr<Node> to){
    int fromInDeg = from->inDegree();
    int fromOutDeg = from->outDegree();

    int inCount = 0;
    int outCount = 0;

    //Copy standard arcs
    std::vector<std::shared_ptr<InputPort>> origInputPorts = from->getInputPorts();
    for(int i = 0; i<origInputPorts.size(); i++){
        int portNum = origInputPorts[i]->getPortNum();
        std::set<std::shared_ptr<Arc>> inArcs = origInputPorts[i]->getArcs();
        inCount += inArcs.size();

        for(auto inArc = inArcs.begin(); inArc != inArcs.end(); inArc++){
            (*inArc)->setDstPortUpdateNewUpdatePrev(to->getInputPortCreateIfNot(portNum));
        }
    }

    std::vector<std::shared_ptr<OutputPort>> origOutputPorts = from->getOutputPorts();
    for(int i = 0; i<origOutputPorts.size(); i++){
        int portNum = origOutputPorts[i]->getPortNum();
        std::set<std::shared_ptr<Arc>> outArcs = origOutputPorts[i]->getArcs();
        outCount += outArcs.size();

        for(auto outArc = outArcs.begin(); outArc != outArcs.end(); outArc++){
            (*outArc)->setSrcPortUpdateNewUpdatePrev(to->getOutputPortCreateIfNot(portNum));
        }
    }

    //Move order constraint arcs
    std::shared_ptr<InputPort> origOrderConstraintInputPort = from->getOrderConstraintInputPort();
    if(origOrderConstraintInputPort){
        std::set<std::shared_ptr<Arc>> inArcs = origOrderConstraintInputPort->getArcs();

        inCount += inArcs.size();

        for(auto inArc = inArcs.begin(); inArc != inArcs.end(); inArc++){
            (*inArc)->setDstPortUpdateNewUpdatePrev(to->getOrderConstraintInputPortCreateIfNot());
        }
    }

    std::shared_ptr<OutputPort> origOrderConstraintOutputPort = from->getOrderConstraintOutputPort();
    if(origOrderConstraintOutputPort){
        std::set<std::shared_ptr<Arc>> outArcs = origOrderConstraintOutputPort->getArcs();

        outCount += outArcs.size();

        for(auto outArc = outArcs.begin(); outArc != outArcs.end(); outArc++){
            (*outArc)->setSrcPortUpdateNewUpdatePrev(to->getOrderConstraintOutputPortCreateIfNot());
        }
    }

    if(inCount != fromInDeg || outCount != fromOutDeg){
        throw std::runtime_error(ErrorHelpers::genErrorStr("When moving arcs, the number of arcs moved did not match the in+out degree.  This could be because of a special port", from));
    }
}

std::set<std::shared_ptr<Arc>> EmitterHelpers::getConnectionsToMasterInputNode(std::shared_ptr<Node> node, bool directOnly) {
    std::set<std::shared_ptr<Arc>> masterArcs;

    std::set<std::shared_ptr<Arc>> inputArcs = directOnly ? node->getDirectInputArcs() : node->getInputArcs();

    for(auto arc = inputArcs.begin(); arc != inputArcs.end(); arc++){
        std::shared_ptr<OutputPort> srcPort = (*arc)->getSrcPort();
        std::shared_ptr<Node> src = srcPort->getParent();
        std::shared_ptr<MasterInput> srcAsMasterInput = GeneralHelper::isType<Node, MasterInput>(src);
        if(srcAsMasterInput){
            masterArcs.insert(*arc);
        }
    }

    return masterArcs;
}

std::set<std::shared_ptr<Arc>>
EmitterHelpers::getConnectionsToMasterOutputNodes(std::shared_ptr<Node> node, bool directOnly) {
    std::set<std::shared_ptr<Arc>> masterArcs;

    std::set<std::shared_ptr<Arc>> outputArcs = directOnly ? node->getDirectOutputArcs() : node->getOutputArcs();

    for(auto arc = outputArcs.begin(); arc != outputArcs.end(); arc++){
        std::shared_ptr<InputPort> dstPort = (*arc)->getDstPort();
        std::shared_ptr<Node> src = dstPort->getParent();
        std::shared_ptr<MasterOutput> dstAsMasterOutput = GeneralHelper::isType<Node, MasterOutput>(src);
        if(dstAsMasterOutput){
            masterArcs.insert(*arc);
        }
    }

    return masterArcs;
}

std::vector<int>
EmitterHelpers::getBlockSizesFromRates(const std::vector<std::pair<int, int>> &rates, int blockSizeBase) {
    std::vector<int> blockSizes;
    for(unsigned long i = 0; i<rates.size(); i++){
        blockSizes.push_back(blockSizeBase*rates[i].first/rates[i].second);
    }

    return blockSizes;
}

//Val index is a reference so that it can be incremented in the base case
std::string EmitterHelpers::arrayLiteralWorker(std::vector<int> &dimensions, int dimIndex, std::vector<NumericValue> &val, int &valIndex, bool imag, DataType valType, DataType storageType){
    std::string str = "";

    if(dimIndex >= dimensions.size()){
        //Base case, just emit the value, with no {}
        std::string storageTypeStr = storageType.toString(DataType::StringStyle::C);

        str = "(" + storageTypeStr + ")" + val[valIndex].toStringComponent(imag, valType);
        valIndex++;
    }else{
        str += "{";

        for(int i = 0; i<dimensions[dimIndex]; i++){
            if(i > 0){
                str += ", ";
            }

            str += arrayLiteralWorker(dimensions, dimIndex+1, val, valIndex, imag, valType, storageType);
        }

        str += "}";
    }

    return str;
}

std::string EmitterHelpers::arrayLiteral(std::vector<int> &dimensions, std::vector<NumericValue> val, bool imag, DataType valType, DataType storageType){
    int idx = 0;
    return arrayLiteralWorker(dimensions, 0, val, idx, imag, valType, storageType);
}

std::tuple<std::vector<std::string>, std::vector<std::string>, std::vector<std::string>>
EmitterHelpers::generateVectorMatrixForLoops(const std::vector<int>& dimensions) {
    std::vector<std::string> loopDecls;
    std::vector<std::string> loopVars;
    std::vector<std::string> loopClose;

    for(unsigned long i = 0; i<dimensions.size(); i++){
        //The index variable will be indDim# where # is the dimension number
        loopVars.push_back("indDim" + GeneralHelper::to_string(i));

        loopDecls.push_back("for(unsigned long " + loopVars[i] + "; " + loopVars[i] + "<" + GeneralHelper::to_string(dimensions[i]) + "; " + loopVars[i] + "++){");
        loopClose.emplace_back("}");
    }

    return std::tuple<std::vector<std::string>, std::vector<std::string>, std::vector<std::string>>(loopDecls, loopVars, loopClose);
}

std::vector<int> EmitterHelpers::memIdx2ArrayIdx(int idx, std::vector<int> dimensions){
    int idxRemaining = idx;
    int divBy = 1;
    for(int i = 0; i<dimensions.size(); i++){
        divBy *= dimensions[i];
    }

    std::vector<int> arrayIdx;

    //The traversal order is because the largest diemnsions increments the most quickly when itterating through memory in C/C++
    for(int i = 0; i<dimensions.size(); i++){
        divBy /= dimensions[i];
        int localIdx = idxRemaining / divBy;
        idxRemaining -= localIdx*divBy;
        arrayIdx.push_back(localIdx);
    }

    return arrayIdx;
}