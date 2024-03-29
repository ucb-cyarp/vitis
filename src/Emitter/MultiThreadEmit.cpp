//
// Created by Christopher Yarp on 9/3/19.
//

#include "MultiThreadEmit.h"
#include "PrimitiveNodes/Delay.h"
#include "PrimitiveNodes/BlackBox.h"
#include "GraphCore/ContextRoot.h"
#include "GraphCore/NodeFactory.h"
#include "MasterNodes/MasterUnconnected.h"
#include "General/EmitterHelpers.h"
#include <GraphCore/Node.h>
#include <GraphCore/Arc.h>
#include <MultiThread/ThreadCrossingFIFO.h>
#include <General/ErrorHelpers.h>
#include "General/GeneralHelper.h"
#include <GraphCore/EnableOutput.h>
#include <PrimitiveNodes/BlackBox.h>
#include "GraphCore/ContextContainer.h"
#include "GraphCore/ContextFamilyContainer.h"
#include "General/EmitterHelpers.h"

#include <iostream>
#include <fstream>
#include <stdexcept>

void MultiThreadEmit::findPartitionInputAndOutputFIFOs(
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

std::string MultiThreadEmit::emitCopyCThreadArgs(std::vector<std::shared_ptr<ThreadCrossingFIFO>> inputFIFOs, std::vector<std::shared_ptr<ThreadCrossingFIFO>> outputFIFOs, std::string structName, std::string structTypeName){
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
            bool isAtomic = var.isAtomicVar();
            var.setAtomicVar(false);

            //Check if complex
            statements += (isAtomic ? "_Atomic " : "") + (structName.empty() ? var.getCPtrDecl(false) : inputFIFOs[i]->getFIFOStructTypeName() + "* " + var.getCVarName(false)) + " = " + castStructName + "->" + var.getCVarName(false) + ";\n";
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
            bool isAtomic = var.isAtomicVar();
            var.setAtomicVar(false);

            std::string outputFIFOStatement = (isAtomic ? "_Atomic " : "") + (structName.empty() ? var.getCPtrDecl(false) : outputFIFOs[i]->getFIFOStructTypeName() + "* " + var.getCVarName(false)) + " = " + castStructName + "->" + var.getCVarName(false) + ";\n";
            statements += outputFIFOStatement;
            //statements += outputFIFOs[i]->getFIFOStructTypeName() + "* " +  var.getCVarName(false) + " = " + castStructName + "->" + var.getCVarName(false) + ";\n";
        }
    }

    return statements;
}

std::string MultiThreadEmit::emitFIFOChecks(std::vector<std::shared_ptr<ThreadCrossingFIFO>> fifos, bool producer, std::string checkVarName, bool shortCircuit, bool blocking, bool includeThreadCancelCheck, PartitionParams::FIFOIndexCachingBehavior fifoIndexCachingBehavior){
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

    ThreadCrossingFIFO::Role producerRoll = ThreadCrossingFIFO::Role::PRODUCER;
    ThreadCrossingFIFO::Role consumerRoll = ThreadCrossingFIFO::Role::CONSUMER;

    if(fifoIndexCachingBehavior == PartitionParams::FIFOIndexCachingBehavior::PRODUCER_CONSUMER_CACHE ||
    fifoIndexCachingBehavior == PartitionParams::FIFOIndexCachingBehavior::PRODUCER_CACHE){
        producerRoll = ThreadCrossingFIFO::Role::PRODUCER_FULLCACHE;
    }
    if(fifoIndexCachingBehavior == PartitionParams::FIFOIndexCachingBehavior::PRODUCER_CONSUMER_CACHE ||
       fifoIndexCachingBehavior == PartitionParams::FIFOIndexCachingBehavior::CONSUMER_CACHE){
        consumerRoll = ThreadCrossingFIFO::Role::CONSUMER_FULLCACHE;
    }

    //Emit the actual FIFO checks
    for(int i = 0; i<fifos.size(); i++) {
        //Note: do not need to check if complex since complex values come via the same FIFO as a struct
        std::vector<std::string> statementQueue;
        std::string checkStmt = checkVarName + " &= " + (producer ? fifos[i]->emitCIsNotFull(statementQueue, producerRoll) : fifos[i]->emitCIsNotEmpty(statementQueue, consumerRoll)) + ";";
        check += "{\n";
        for(unsigned int i = 0; i<statementQueue.size(); i++){
            check += statementQueue[i] + "\n";
        }
        check += checkStmt + "\n";
        check += "}\n";

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
//std::string MultiThreadEmit::emitFIFOChecks(std::vector<std::shared_ptr<ThreadCrossingFIFO>> fifos, int partition, bool checkFull, std::string checkVarName, bool shortCircuit){
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

std::vector<std::string> MultiThreadEmit::createAndInitFIFOLocalVars(std::vector<std::shared_ptr<ThreadCrossingFIFO>> fifos){
    std::vector<std::string> exprs;
    for(int i = 0; i<fifos.size(); i++) {
        fifos[i]->createLocalVars(exprs);
        fifos[i]->initLocalVars(exprs, ThreadCrossingFIFO::Role::NONE);
    }

    return exprs;
}

std::vector<std::string> MultiThreadEmit::createFIFOReadTemps(std::vector<std::shared_ptr<ThreadCrossingFIFO>> fifos){
    std::vector<std::string> exprs;
    for(int i = 0; i<fifos.size(); i++) {
        std::string tmpName = fifos[i]->getName() + "_readTmp";
        std::string structType = fifos[i]->getFIFOStructTypeName();
        exprs.push_back(structType + " " + tmpName + ";");
    }

    return exprs;
}

std::vector<std::string> MultiThreadEmit::createFIFOWriteTemps(std::vector<std::shared_ptr<ThreadCrossingFIFO>> fifos){
    std::vector<std::string> exprs;
    for(int i = 0; i<fifos.size(); i++) {
        std::string tmpName = fifos[i]->getName() + "_writeTmp";
        std::string structType = fifos[i]->getFIFOStructTypeName();
        exprs.push_back(structType + " " + tmpName + ";");
    }

    return exprs;
}

std::vector<std::string> MultiThreadEmit::createFIFODoubleBufferReadVars(
        std::vector<std::shared_ptr<ThreadCrossingFIFO>> fifos, ComputeIODoubleBufferType doubleBuffer){
    std::vector<std::string> exprs;

    if(doubleBuffer == ComputeIODoubleBufferType::INPUT_AND_OUTPUT || doubleBuffer == ComputeIODoubleBufferType::INPUT) {
        for (int i = 0; i < fifos.size(); i++) {
            std::string structType = fifos[i]->getFIFOStructTypeName();

            std::string tmpNameA = fifos[i]->getName() + "_bufferA";
            std::string tmpNameB = fifos[i]->getName() + "_bufferB";
            exprs.push_back(structType + " " + tmpNameA + ";");
            exprs.push_back(structType + " " + tmpNameB + ";");

            std::string currentName = fifos[i]->getName() + "_current";
            std::string nextName = fifos[i]->getName() + "_next";

            exprs.push_back(structType + " *" + currentName + " = &" + tmpNameA + ";");
            exprs.push_back(structType + " *" + nextName + " = &" + tmpNameB + ";");
        }
    }

    return exprs;
}

std::vector<std::string> MultiThreadEmit::createFIFODoubleBufferWriteVars(
        std::vector<std::shared_ptr<ThreadCrossingFIFO>> fifos, ComputeIODoubleBufferType doubleBuffer){
    std::vector<std::string> exprs;

    if(doubleBuffer == ComputeIODoubleBufferType::INPUT_AND_OUTPUT || doubleBuffer == ComputeIODoubleBufferType::OUTPUT) {
        for (int i = 0; i < fifos.size(); i++) {
            std::string structType = fifos[i]->getFIFOStructTypeName();

            std::string tmpNameA = fifos[i]->getName() + "_bufferA";
            std::string tmpNameB = fifos[i]->getName() + "_bufferB";
            exprs.push_back(structType + " " + tmpNameA + ";");
            exprs.push_back(structType + " " + tmpNameB + ";");

            std::string currentName = fifos[i]->getName() + "_current";
            std::string prevName = fifos[i]->getName() + "_prev";

            exprs.push_back(structType + " *" + currentName + " = &" + tmpNameA + ";");
            exprs.push_back(structType + " *" + prevName + " = &" + tmpNameB + ";");
        }
    }

    return exprs;
}

std::vector<std::string> MultiThreadEmit::swapReadDoubleBufferPtrs(
        std::vector<std::shared_ptr<ThreadCrossingFIFO>> fifos, ComputeIODoubleBufferType doubleBuffer){
    std::vector<std::string> exprs;

    if(doubleBuffer == ComputeIODoubleBufferType::INPUT_AND_OUTPUT || doubleBuffer == ComputeIODoubleBufferType::INPUT) {
        for (int i = 0; i < fifos.size(); i++) {
            std::string structType = fifos[i]->getFIFOStructTypeName();

            std::string currentName = fifos[i]->getName() + "_current";
            std::string nextName = fifos[i]->getName() + "_next";
            std::string tmpName = fifos[i]->getName() + "_tmp";

            //Will open a block scope for the temporary var
            exprs.push_back("{");
            exprs.push_back(structType + " *" + tmpName + " = " + currentName + ";");
            exprs.push_back(currentName + " = " + nextName + ";");
            exprs.push_back(nextName + " = " + tmpName + ";");
            exprs.push_back("}");
        }
    }

    return exprs;
}

std::vector<std::string> MultiThreadEmit::swapWriteDoubleBufferPtrs(
        std::vector<std::shared_ptr<ThreadCrossingFIFO>> fifos, ComputeIODoubleBufferType doubleBuffer){
    std::vector<std::string> exprs;

    if(doubleBuffer == ComputeIODoubleBufferType::INPUT_AND_OUTPUT || doubleBuffer == ComputeIODoubleBufferType::OUTPUT) {
        for (int i = 0; i < fifos.size(); i++) {
            std::string structType = fifos[i]->getFIFOStructTypeName();

            std::string currentName = fifos[i]->getName() + "_current";
            std::string prevName = fifos[i]->getName() + "_prev";
            std::string tmpName = fifos[i]->getName() + "_tmp";

            //Will open a block scope for the temporary var
            exprs.push_back("{");
            exprs.push_back(structType + " *" + tmpName + " = " + prevName + ";");
            exprs.push_back(prevName + " = " + currentName + ";");
            exprs.push_back(currentName + " = " + tmpName + ";");
            exprs.push_back("}");
        }
    }

    return exprs;
}

std::vector<std::string> MultiThreadEmit::createAndInitializeFIFOWriteTemps(std::vector<std::shared_ptr<ThreadCrossingFIFO>> fifos, std::vector<std::vector<NumericValue>> defaultVal){
    std::vector<std::string> exprs;
    for(int i = 0; i<fifos.size(); i++) {
        std::string tmpName = fifos[i]->getName() + "_writeTmp";
        std::string structType = fifos[i]->getFIFOStructTypeName();
        exprs.push_back(structType + " " + tmpName + ";");

        for(int portNum = 0; portNum<fifos[i]->getInputPorts().size(); portNum++) {
            //Note that the datatype when we use getCStateVar does not include
            //the block size. However, the structure that is generated for the FIFO
            //is expanded for the block size.
            //Expand it here to match the structure definition
            DataType dt = fifos[i]->getCStateVarExpandedForBlockSize(portNum).getDataType();

            //Open for loops if datatype (after expansion for blocks) is a vector/matrix
            std::vector<std::string> forLoopIndexVars;
            std::vector<std::string> forLoopClose;
            //If the output is a vector, construct a for loop which puts the results in a temporary array
            if(!dt.isScalar()){
                //Create nested loops for a given array
                std::tuple<std::vector<std::string>, std::vector<std::string>, std::vector<std::string>> forLoopStrs =
                        EmitterHelpers::generateVectorMatrixForLoops(dt.getDimensions());
                std::vector<std::string> forLoopOpen = std::get<0>(forLoopStrs);
                forLoopIndexVars = std::get<1>(forLoopStrs);
                forLoopClose = std::get<2>(forLoopStrs);
                exprs.insert(exprs.end(), forLoopOpen.begin(), forLoopOpen.end());
            }

            //Deref if necessary
            exprs.push_back(tmpName + ".port" + GeneralHelper::to_string(portNum) + "_real" +
                (dt.isScalar() ? "" : EmitterHelpers::generateIndexOperation(forLoopIndexVars)) +
                " = " + defaultVal[i][portNum].toStringComponent(false, dt) + ";");
            if (dt.isComplex()) {
                exprs.push_back(tmpName + ".port" + GeneralHelper::to_string(portNum) + "_imag" +
                    (dt.isScalar() ? "" : EmitterHelpers::generateIndexOperation(forLoopIndexVars)) +
                    " = " + defaultVal[i][portNum].toStringComponent(true, dt) + ";");
            }

            //Close for loop
            if(!dt.isScalar()){
                exprs.insert(exprs.end(), forLoopClose.begin(), forLoopClose.end());
            }
        }
    }

    return exprs;
}

std::vector<std::string> MultiThreadEmit::readFIFOsToTemps(std::vector<std::shared_ptr<ThreadCrossingFIFO>> fifos, bool forcePull, bool pushAfter, bool forceNotInPlace) {
    std::vector<std::string> exprs;

    for(int i = 0; i<fifos.size(); i++){
        std::string tmpName = fifos[i]->getName() + "_readTmp";
        //However, the type will be the FIFO structure rather than the variable type directly.
        //The fifo reads in terms of blocks with the components stored in a structure
        //When calling the function, the relavent component of the structure is passed as an argument

        fifos[i]->emitCReadFromFIFO(exprs, tmpName, 1, forcePull ? ThreadCrossingFIFO::Role::NONE : ThreadCrossingFIFO::Role::CONSUMER, pushAfter, forceNotInPlace);
    }

    return exprs;
}

std::vector<std::string> MultiThreadEmit::pushReadFIFOsStatus(std::vector<std::shared_ptr<ThreadCrossingFIFO>> fifos){
    std::vector<std::string> exprs;

    exprs.push_back("//Pushing status for input FIFOs");
    for(int i = 0; i<fifos.size(); i++){
        fifos[i]->pushLocalVars(exprs, ThreadCrossingFIFO::Role::CONSUMER);
    }

    return exprs;
}

std::vector<std::string> MultiThreadEmit::writeFIFOsFromTemps(std::vector<std::shared_ptr<ThreadCrossingFIFO>> fifos, bool forcePull, bool pushAfter, bool forceNotInPlace) {
    std::vector<std::string> exprs;

    for(int i = 0; i<fifos.size(); i++){
        std::string tmpName = fifos[i]->getName() + "_writeTmp";
        //However, the type will be the FIFO structure rather than the variable type directly.
        //The fifo reads in terms of blocks with the components stored in a structure
        //When calling the function, the relavent component of the structure is passed as an argument

        fifos[i]->emitCWriteToFIFO(exprs, tmpName, 1, forcePull ? ThreadCrossingFIFO::Role::NONE : ThreadCrossingFIFO::Role::PRODUCER, pushAfter, forceNotInPlace);
    }

    return exprs;
}

std::vector<std::string> MultiThreadEmit::pushWriteFIFOsStatus(std::vector<std::shared_ptr<ThreadCrossingFIFO>> fifos){
    std::vector<std::string> exprs;

    exprs.push_back("//Pushing status for output FIFOs");
    for(int i = 0; i<fifos.size(); i++){
        fifos[i]->pushLocalVars(exprs, ThreadCrossingFIFO::Role::PRODUCER);
    }

    return exprs;
}

std::pair<std::string, std::string> MultiThreadEmit::getCThreadArgStructDefn(std::vector<std::shared_ptr<ThreadCrossingFIFO>> inputFIFOs, std::vector<std::shared_ptr<ThreadCrossingFIFO>> outputFIFOs, std::string designName, int partitionNum){
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

            //prototype += "\t" + outputFIFOs[i]->getFIFOStructTypeName() + "* " + var.getCVarName(false) +  + ";\n";
            prototype += (structName.empty() ? var.getCPtrDecl(false) : outputFIFOs[i]->getFIFOStructTypeName() + "* " + var.getCVarName(false)) + ";\n";
        }
    }

    prototype += "} " + structureType + ";";

    return std::pair<std::string, std::string> (prototype, structureType);
}

std::string MultiThreadEmit::emitFIFOStructHeader(std::string path, std::string fileNamePrefix, std::vector<std::shared_ptr<ThreadCrossingFIFO>> fifos) {
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
    headerFile << "#include \"" << VITIS_TYPE_NAME << ".h\"" << std::endl;

    for(int i = 0; i<fifos.size(); i++){
        std::string fifoStruct = fifos[i]->createFIFOStruct();
        headerFile << fifoStruct << std::endl;
    }

    headerFile << "#endif" << std::endl;
    headerFile.close();

    return fileName+".h";
}

std::string MultiThreadEmit::emitFIFOSupportFile(std::string path, std::string fileNamePrefix, std::set<ThreadCrossingFIFOParameters::CopyMode> copyModes) {
    bool shouldEmit = false;

    //Get the required header and body support from the FIFOs
    std::string headerContents;

    if(copyModes.find(ThreadCrossingFIFOParameters::CopyMode::FAST_COPY_UNALIGNED) != copyModes.end()) {
        shouldEmit = true;

        headerContents +=
                "static inline void* fast_copy_unaligned_ramp_in(void* restrict dst, void* restrict src, size_t elementSize, size_t numElements){\n"
                "    size_t totalBytesToCopy = elementSize*numElements;\n"
                "    \n"
                "    #ifdef __AVX__\n"
                "        size_t bytesPerBlock = 32; //256-bit Wide Vector, 32 Bytes\n"
                "    #elif defined (__SSE2__)\n"
                "        size_t bytesPerBlock = 16; //128-bit Wide Vector, 16 Bytes\n"
                "    #else\n"
                "        size_t bytesPerBlock = 8;  //64-bit Wide Vector, 8 Bytes\n"
                "    #endif\n"
                "\n"
                "    size_t blocksToCopy = totalBytesToCopy/bytesPerBlock;\n"
                "    size_t bytesToCopy = totalBytesToCopy%bytesPerBlock;\n"
                "\n"
                "    char* restrict srcByte = ((char* restrict) src);\n"
                "    char* restrict dstByte = ((char* restrict) dst);\n"
                "\n"
                "    //Copy RemainingBytes\n"
                "    for(size_t i = 0; i<bytesToCopy; i++){\n"
                "        dstByte[i] = srcByte[i];\n"
                "    }\n"
                "\n"
                "    char* restrict srcBlock = src+bytesToCopy;\n"
                "    char* restrict dstBlock = dst+bytesToCopy;\n"
                "\n"
                "    //Copy Large Blocks\n"
                "    for(size_t i = 0; i<blocksToCopy; i++){\n"
                "        char* restrict srcCursor = srcBlock+i*bytesPerBlock;\n"
                "        char* restrict dstCursor = dstBlock+i*bytesPerBlock;\n"
                "        #ifdef __AVX__\n"
                "            #ifdef FAST_COPY_USE_DOUBLE_LOAD_STORE\n"
                "                __m256d tmp = _mm256_loadu_pd((double*) srcCursor);\n"
                "                _mm256_storeu_pd((double*) dstCursor, tmp);\n"
                "            #else\n"
                "                __m256i tmp = _mm256_loadu_si256((__m256i*) srcCursor);\n"
                "                _mm256_storeu_si256((__m256i*) dstCursor, tmp);\n"
                "            #endif\n"
                "            \n"
                "        #elif defined (__SSE2__)\n"
                "            #ifdef FAST_COPY_USE_DOUBLE_LOAD_STORE\n"
                "                __m128d tmp = _mm_loadu_pd((double*) srcCursor);\n"
                "                _mm_storeu_pd((double*) dstCursor, tmp);\n"
                "            #else\n"
                "                __m128i tmp = _mm_loadu_si128((__m128i*) srcCursor);\n"
                "                _mm_storeu_si128((__m128i*) dstCursor, tmp);\n"
                "            #endif\n"
                "        #else\n"
                "            *((int64_t*) dstCursor) = *((int64_t*) srcCursor);\n"
                "        #endif\n"
                "    }\n"
                "\n"
                "    return dst;\n"
                "}\n";
    }

    if(shouldEmit) {
        std::string fileName = fileNamePrefix + "_fifoSupport";
        std::cout << "Emitting C File: " << path << "/" << fileName << ".h" << std::endl;
        //#### Emit .h file ####
        std::ofstream headerFile;
        headerFile.open(path + "/" + fileName + ".h", std::ofstream::out | std::ofstream::trunc);

        std::string fileNameUpper = GeneralHelper::toUpper(fileName);
        headerFile << "#ifndef " << fileNameUpper << "_H" << std::endl;
        headerFile << "#define " << fileNameUpper << "_H" << std::endl;
        headerFile << "#include <stdint.h>" << std::endl;
        headerFile << "#include <stdbool.h>" << std::endl;
        headerFile << "#include <immintrin.h>" << std::endl;
        headerFile << "#include \"" << VITIS_TYPE_NAME << ".h\"" << std::endl;

        headerFile << headerContents << std::endl;

        headerFile << "#endif" << std::endl;
        headerFile.close();

        return fileName + ".h";
    }else{
        //Return nothing if no header file was emitted
        return "";
    }
}

std::set<ThreadCrossingFIFOParameters::CopyMode> MultiThreadEmit::findFIFOCopyModesUsed(std::vector<std::shared_ptr<ThreadCrossingFIFO>> &fifos){
    std::set<ThreadCrossingFIFOParameters::CopyMode> copyModes;
    for(const auto &fifo : fifos){
        copyModes.insert(fifo->getCopyMode());
    }

    return copyModes;
}

int MultiThreadEmit::getCore(int parititon, const std::vector<int> &partitionMap, bool print){
    int core = 0;
    if(partitionMap.empty()) {
        //In this case, no thread pinning occurs
        return -1;
    }else{
        //Use the partition map
        if (parititon == IO_PARTITION_NUM) {
            core = partitionMap[0]; //Is always the first element and the array is not empty
            if(print) {
                std::cout << "Setting I/O thread to run on CPU" << core << std::endl;
            }
        }else{
            if(parititon < 0 || parititon >= partitionMap.size()-1){
                throw std::runtime_error(ErrorHelpers::genErrorStr("The partition map does not contain an entry for partition " + GeneralHelper::to_string(parititon)));
            }

            core = partitionMap[parititon+1];
            if(print) {
                std::cout << "Setting Partition " << parititon << " thread to run on CPU" << core << std::endl;
            }
        }

    }

    return core;
}

void MultiThreadEmit::emitMultiThreadedBenchmarkKernel(std::map<std::pair<int, int>, std::vector<std::shared_ptr<ThreadCrossingFIFO>>> fifoMap,
                                                                 std::map<int, std::vector<std::shared_ptr<ThreadCrossingFIFO>>> inputFIFOMap,
                                                                 std::map<int, std::vector<std::shared_ptr<ThreadCrossingFIFO>>> outputFIFOMap, std::set<int> partitions,
                                                                 std::string path, std::string fileNamePrefix, std::string designName, std::string fifoHeaderFile,
                                                                 std::string fifoSupportFile, std::string ioBenchmarkSuffix, std::vector<int> partitionMap,
                                                                 std::string papiHelperHeader, bool useSCHEDFIFO){
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
    headerFile << "#include \"" << VITIS_TYPE_NAME << ".h\"" << std::endl;
    headerFile << "#include \"" << fifoHeaderFile << "\"" << std::endl;
    if(!fifoSupportFile.empty()){
        headerFile << "#include \"" << fifoSupportFile << "\"" << std::endl;
    }
    headerFile << std::endl;

    //Output the function prototype for the I/O thread function
    std::string fctnDecl = "void " + designName + "_" + ioBenchmarkSuffix + "_kernel()";
    headerFile << std::endl;
    headerFile << fctnDecl << ";" << std::endl;
    headerFile << "#endif" << std::endl;
    headerFile.close();

    //#### Emit .c File ####
    std::cout << "Emitting C File: " << path << "/" << fileName << ".c" << std::endl;
    std::ofstream cFile;
    cFile.open(path+"/"+fileName+".c", std::ofstream::out | std::ofstream::trunc);

    //Include other generated headers
    cFile << "#ifndef _GNU_SOURCE" << std::endl;
    cFile << "//Need _GNU_SOURCE, sched.h, and unistd.h for setting thread affinity in Linux" << std::endl; //Linux scheduler source for setting thread affinity (C++ did not complain not having this but C does)
    cFile << "#define _GNU_SOURCE" << std::endl;
    cFile << "#endif" << std::endl;
    cFile << "#include <unistd.h>" << std::endl;
    cFile << "#include <sched.h>" << std::endl;
    cFile << "#include <stdio.h>" << std::endl;
    cFile << "#include <errno.h>" << std::endl;
    cFile << "#include \"" << fileName << ".h" << "\"" << std::endl;
    cFile << "#include \"" << VITIS_PLATFORM_PARAMS_NAME << ".h\"" << std::endl;
    cFile << "#include \"" << VITIS_NUMA_ALLOC_HELPERS << ".h\"" << std::endl;
    if(!papiHelperHeader.empty()){
        cFile << "#include \"" << papiHelperHeader << "\"" << std::endl;
    }
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

    cFile << "//Note: Each partition's thread function inits/resets that partition's design state before processing samples" << std::endl;
    cFile << std::endl;

    cFile << "//Allocate and Initialize FIFO Shared Variables" << std::endl;

    //For each FIFO, allocate the shared arrays
    for(auto it = fifoMap.begin(); it != fifoMap.end(); it++){
        std::vector<std::shared_ptr<ThreadCrossingFIFO>> fifos = it->second;
        int srcPartition  = it->first.first;
        int core = MultiThreadEmit::getCore(srcPartition, partitionMap);

        std::vector<std::string> statements;
        for(int i = 0; i<fifos.size(); i++){

            fifos[i]->createSharedVariables(statements, core); //Note, if the CPU is -1, which occurs if no CPU map is provided (no thresd pinning is performed), the array is allocated on the CPU running the setup code
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

    //Initialize PAPI before starting threads
    if(!papiHelperHeader.empty()){
        cFile << std::endl << "//Setup PAPI" << std::endl;
        cFile << "setupPapi();" << std::endl;
    }

    //Create Thread Parameters
    cFile << std::endl;
    cFile << "//Create Thread Parameters" << std::endl;
    cFile << "int status;" << std::endl;
    for(auto it = partitions.begin(); it != partitions.end(); it++){
        std::string partitionSuffix = (*it < 0 ? "N" + GeneralHelper::to_string(-*it) : GeneralHelper::to_string(*it));
        std::string attrName = "attr_" + partitionSuffix;
        cFile << "pthread_t thread_" << partitionSuffix << ";" << std::endl;
        cFile << "pthread_attr_t " << attrName << ";" << std::endl;
        //cFile << "void *res_" << *it << ";" << std::endl;
        cFile << std::endl;
        cFile << "status = pthread_attr_init(&" << attrName << ");" << std::endl;
        cFile << "if(status != 0)" << std::endl;
        cFile << "{" << std::endl;
        cFile << "printf(\"Could not create pthread attributes ... exiting\");" << std::endl;
        cFile << "exit(1);" << std::endl;
        cFile << "}" << std::endl;
        cFile << std::endl;
        //Only set the thread affinity if a partition map was provided.  If none was provided, core will be -1
        int core = MultiThreadEmit::getCore(*it, partitionMap, true);
        if(core >= 0) {
            if(useSCHEDFIFO){
                //Set SCHED_FIFO with max scheduling priority for this thread
                cFile << "//Set partition " << partitionSuffix << " to run with SCHED_FIFO RT Scheduler with Max Priority" << std::endl;
                cFile << "//NOTE! This can lock up the computer if this thread is run on a CPU where system tasks are running." << std::endl;
                cFile << "status = pthread_attr_setinheritsched(&" << attrName << ", PTHREAD_EXPLICIT_SCHED);" << std::endl;
                cFile << "if(status != 0)" << std::endl;
                cFile << "{" << std::endl;
                cFile << "printf(\"Could not set pthread explicit schedule attribute ... exiting\\n\");" << std::endl;
                cFile << "exit(1);" << std::endl;
                cFile << "}" << std::endl;
                cFile << std::endl;

                cFile << "status = pthread_attr_setschedpolicy(&" << attrName << ", SCHED_FIFO);" << std::endl;
                cFile << "if(status != 0)" << std::endl;
                cFile << "{" << std::endl;
                cFile << "printf(\"Could not set pthread schedule policy to SCHED_FIFO ... exiting\\n\");" << std::endl;
                cFile << "exit(1);" << std::endl;
                cFile << "}" << std::endl;
                cFile << std::endl;

                std::string threadParamsName = "threadParams_" + partitionSuffix;
                cFile << "struct sched_param " << threadParamsName << ";" << std::endl;
                cFile << threadParamsName << ".sched_priority = sched_get_priority_max(SCHED_FIFO);" << std::endl;

                cFile << "status = pthread_attr_setschedparam(&" << attrName << ", &" << threadParamsName << ");" << std::endl;
                cFile << "if(status != 0)" << std::endl;
                cFile << "{" << std::endl;
                cFile << "printf(\"Could not set pthread schedule parameter ... exiting\\n\");" << std::endl;
                cFile << "exit(1);" << std::endl;
                cFile << "}" << std::endl;
            }

            cFile << "//Set partition " << partitionSuffix << " to run on CPU " << core << std::endl;
            cFile << "cpu_set_t cpuset_" << partitionSuffix << ";" << std::endl;
            cFile << "CPU_ZERO(&cpuset_" << partitionSuffix << "); //Clear cpuset" << std::endl;
            cFile << "CPU_SET(" << core << ", &cpuset_" << partitionSuffix << "); //Add CPU to cpuset" << std::endl;
            cFile << "status = pthread_attr_setaffinity_np(&attr_" << partitionSuffix << ", sizeof(cpu_set_t), &cpuset_"
                  << partitionSuffix
                  << ");//Set thread CPU affinity" << std::endl;
            cFile << "if(status != 0)" << std::endl;
            cFile << "{" << std::endl;
            cFile << "printf(\"Could not set thread core affinity ... exiting\");" << std::endl;
            cFile << "exit(1);" << std::endl;
            cFile << "}" << std::endl;
            cFile << std::endl;
        }
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

    //Get the stack information
    cFile << std::endl;
    cFile << "//Get/report stack information" << std::endl;
    cFile << "FILE* stackInfo = fopen(\"" << designName << "_stack_info.txt\", \"w\");" << std::endl;

    for(auto it = partitions.begin(); it != partitions.end(); it++) {
        std::string partitionSuffix = (*it < 0 ? "N" + GeneralHelper::to_string(-*it) : GeneralHelper::to_string(*it));
        std::string threadName = "thread_" + partitionSuffix;
        std::string createdThreadAttr = "attr_created_" + partitionSuffix;
        std::string stackGuard = "stack_guard_" + partitionSuffix;
        std::string stackBase = "stack_base_" + partitionSuffix;
        std::string stackSize = "stack_size_" + partitionSuffix;

        cFile << "pthread_attr_t " << createdThreadAttr << ";" << std::endl;
        cFile << "status = pthread_getattr_np(" << threadName << ", &" << createdThreadAttr << ");" << std::endl;
        cFile << "if(status != 0)" << std::endl;
        cFile << "{" << std::endl;
        cFile << "printf(\"Could not get thread attributes from created thread ... exiting\");" << std::endl;
        cFile << "errno = status;" << std::endl;
        cFile << "perror(NULL);" << std::endl;
        cFile << "exit(1);" << std::endl;
        cFile << "}" << std::endl;

        cFile << "size_t " << stackGuard << ";" << std::endl;
        cFile << "status = pthread_attr_getguardsize(&" << createdThreadAttr << ", &" << stackGuard << ");" << std::endl;
        cFile << "if(status != 0)" << std::endl;
        cFile << "{" << std::endl;
        cFile << "printf(\"Could not get thread stack guard ... exiting\");" << std::endl;
        cFile << "errno = status;" << std::endl;
        cFile << "perror(NULL);" << std::endl;
        cFile << "exit(1);" << std::endl;
        cFile << "}" << std::endl;

        cFile << "void* " << stackBase << ";" << std::endl;
        cFile << "size_t " << stackSize << ";" << std::endl;
        cFile << "status = pthread_attr_getstack(&" << createdThreadAttr << ", &" << stackBase << ", &" << stackSize << ");" << std::endl;
        cFile << "if(status != 0)" << std::endl;
        cFile << "{" << std::endl;
        cFile << "printf(\"Could not get thread stack addr ... exiting\");" << std::endl;
        cFile << "errno = status;" << std::endl;
        cFile << "perror(NULL);" << std::endl;
        cFile << "exit(1);" << std::endl;
        cFile << "}" << std::endl;

        cFile << "printf(\"Thread %3s Guard: %lu\\n\", \"" << partitionSuffix << "\", " << stackGuard << ");" << std::endl;
        cFile << "fprintf(stackInfo, \"Thread %3s Guard: %lu\\n\", \"" << partitionSuffix << "\", " << stackGuard << ");" << std::endl;

        cFile << "if(" << stackGuard << " < VITIS_MEM_ALIGNMENT){" << std::endl;
        cFile << "fprintf(stderr, \"Warning, Thread %s Stack Guard (%zu) Is Less than Cache Line Size (%d), Cache Polution / Unexpected Communication Between Cores May Occur!\\n\", \"" << partitionSuffix << "\", " << stackGuard << ", VITIS_MEM_ALIGNMENT);" << std::endl;
        cFile << "fprintf(stackInfo, \"Warning, Thread %s Stack Guard (%zu) Is Less than Cache Line Size (%d), Cache Polution / Unexpected Communication Between Cores May Occur!\\n\", \"" << partitionSuffix << "\", " << stackGuard << ", VITIS_MEM_ALIGNMENT);" << std::endl;
        cFile << "}" << std::endl;

        cFile << "printf(\"Thread %3s Stack Base (Lowest Addr): %p, Size: %zu, Stack End Addr (Inclusive): %p\\n\", \"" << partitionSuffix << "\", " << stackBase << ", " << stackSize << ", " << stackBase<< "+" << stackSize << "-1);" << std::endl;
        cFile << "fprintf(stackInfo, \"Thread %3s Stack Base (Lowest Addr): %p, Size: %zu, Stack End Addr (Inclusive): %p\\n\", \"" << partitionSuffix << "\", " << stackBase << ", " << stackSize << ", " << stackBase<< "+" << stackSize << "-1);" << std::endl;
    }

    cFile << "fclose(stackInfo);" << std::endl;

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
    cFile.close();
}

void MultiThreadEmit::emitMultiThreadedDriver(std::string path, std::string fileNamePrefix, std::string designName, std::string ioBenchmarkSuffix, std::vector<Variable> inputVars){
    //#### Emit Driver File ####
    std::string kernelFileName = fileNamePrefix+"_"+ioBenchmarkSuffix+"_kernel";
    std::string fileName = fileNamePrefix+"_"+ioBenchmarkSuffix+"_driver";
    std::cout << "Emitting C++ File: " << path << "/" << fileName << ".h" << std::endl;
    std::ofstream benchDriver;
    benchDriver.open(path+"/"+fileName+".cpp", std::ofstream::out | std::ofstream::trunc);

    benchDriver << "#include <map>" << std::endl;
    benchDriver << "#include <string>" << std::endl;
    benchDriver << "#include \"benchmark_throughput_test.h\"" << std::endl;
    benchDriver << "#include \"kernel_runner.h\"" << std::endl;
    benchDriver << "extern \"C\"{" << std::endl;
    benchDriver << "#include \"" << VITIS_TYPE_NAME << ".h\"" << std::endl;
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

void MultiThreadEmit::emitMultiThreadedMain(std::string path, std::string fileNamePrefix, std::string designName, std::string ioBenchmarkSuffix, std::vector<Variable> inputVars){
    //#### Emit Driver File ####
    std::string kernelFileName = fileNamePrefix+"_"+ioBenchmarkSuffix+"_kernel";
    std::string fileName = fileNamePrefix+"_"+ioBenchmarkSuffix+"_driver";
    std::cout << "Emitting C File: " << path << "/" << fileName << ".c" << std::endl;
    std::ofstream benchDriver;
    benchDriver.open(path+"/"+fileName+".c", std::ofstream::out | std::ofstream::trunc);

    benchDriver << "#include <stdio.h>" << std::endl;
    benchDriver << "#include \"" + kernelFileName + ".h\"" << std::endl;

    benchDriver << "int main(int argc, char* argv[]){" << std::endl;

    //Emit name, file, and units string
    benchDriver << "printf(\"===== Generated System: " + designName + " =====\\n\");" << std::endl;

    //Generate call to loop
    benchDriver << "//Call the generated function" << std::endl;
    std::string kernelFctnName = designName + "_" + ioBenchmarkSuffix + "_kernel";

    benchDriver << kernelFctnName << "();" << std::endl;
    benchDriver << "}" << std::endl;

    benchDriver.close();
}

void MultiThreadEmit::emitMultiThreadedMakefileBenchDriver(std::string path, std::string fileNamePrefix,
                                                                     std::string designName, std::set<int> partitions,
                                                                     std::string ioBenchmarkSuffix, bool includeLrt,
                                                                     std::vector<std::string> additionalSystemSrc,
                                                                     bool includePAPI,
                                                                     bool enableBenchmarkSetAffinity) {
    //#### Emit Makefiles ####

    std::string systemSrcs = "";

    for(auto it = partitions.begin(); it != partitions.end(); it++){
        if(*it != IO_PARTITION_NUM) {
            std::string threadFileName = fileNamePrefix + "_partition" + GeneralHelper::to_string(*it) + ".c";
            systemSrcs += threadFileName + " ";
        }
    }
    std::string ioFileName = fileNamePrefix+"_"+ioBenchmarkSuffix;
    systemSrcs += ioFileName + ".c";
    systemSrcs += " " + std::string(VITIS_NUMA_ALLOC_HELPERS) + ".c";

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
                                    "SYSTEM_CFLAGS = -Ofast -c -g -std=gnu11 -march=native -masm=att\n"
                                    "#Most kernels are allowed to be optomized.  Most assembly kernels use asm 'volitile' to force execution\n"
                                    "KERNEL_CFLAGS = -Ofast -c -g -std=gnu11 -march=native -masm=att\n"
                                    "#For kernels that should not be optimized, the following is used\n"
                                    "KERNEL_NO_OPT_CFLAGS = -O0 -c -g -std=gnu11 -march=native -masm=att\n"
                                    "INC=-I $(COMMON_DIR) -I $(SRC_DIR)\n"
                                    "LIB_DIRS=-L $(COMMON_DIR)\n";
                 makefileContent += "LIB=-pthread\n";
                                    if(includeLrt){
                                        makefileContent += "LIB+= -lrt\n";
                                    }
                 makefileContent += "LIB+= -lProfilerCommon\n";
                 makefileContent += "#Using the technique in pcm makefile to detect MacOS\n"
                                    "UNAME:=$(shell uname)\n"
                                    "ifneq ($(UNAME), Darwin)\n"
                                    "LIB+= -latomic\n"
                                    "endif\n";
                                    if(includePAPI){
                                        makefileContent += "LIB+= -lpapi\n";
                                    }
                                    makefileContent += "\n"
                                    "DEFINES=\n"
                                    "\n"
                                    "DEPENDS=\n"
                                    "DEPENDS_LIB=$(COMMON_DIR)/libProfilerCommon.a\n"
                                    "\n";
                 if(!enableBenchmarkSetAffinity){
                     //This controls if the benchmark code which is used as support for the radio,
                     //sets its initial thread affinity to CPU0, this is inherited by child threads
                     // unless overwritten which is an issue if the partition threads are not set
                     //to particular partitions (partitionMap is empty).  Including this will allow
                     //threads to run on any available core
                     makefileContent += "DEFINES+= -DBENCH_NSET_AFFINITY=1 -DUSE_SCHED_FIFO=0\n\n";
                 }
                 makefileContent += "ifeq ($(USE_PCM), 1)\n"
                                    "DEFINES+= -DUSE_PCM=1\n"
                                    "DEPENDS+= $(DEPENDS_DIR)/pcm/Makefile\n"
                                    "DEPENDS_LIB+= $(DEPENDS_DIR)/pcm/libPCM.a\n"
                                    "INC+= -I $(DEPENDS_DIR)/pcm\n"
                                    "LIB_DIRS+= -L $(DEPENDS_DIR)/pcm\n"
                                    "#Need an additional include directory if on MacOS.\n"
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
                                    "SYSTEM_SRC = " + systemSrcs;
                                    for(int i = 0; i<additionalSystemSrc.size(); i++){
                                        makefileContent += " " + additionalSystemSrc[i];
                                    }
                                    makefileContent += "\n"
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

void MultiThreadEmit::emitMultiThreadedMakefileMain(std::string path, std::string fileNamePrefix,
                                                              std::string designName, std::set<int> partitions,
                                                              std::string ioBenchmarkSuffix, bool includeLrt,
                                                              std::vector<std::string> additionalSystemSrc,
                                                              bool includePAPI,
                                                              bool enableBenchmarkSetAffinity) {
    //#### Emit Makefiles ####

    std::string systemSrcs = "";

    for(auto it = partitions.begin(); it != partitions.end(); it++){
        if(*it != IO_PARTITION_NUM) {
            std::string threadFileName = fileNamePrefix + "_partition" + GeneralHelper::to_string(*it) + ".c";
            systemSrcs += threadFileName + " ";
        }
    }
    std::string ioFileName = fileNamePrefix+"_"+ioBenchmarkSuffix;
    systemSrcs += ioFileName + ".c";
    systemSrcs += " " + std::string(VITIS_NUMA_ALLOC_HELPERS) + ".c";

    std::string kernelFileName = fileNamePrefix+"_"+ioBenchmarkSuffix+"_kernel.c";
    std::string driverFileName = fileNamePrefix+"_"+ioBenchmarkSuffix+"_driver.c";

    systemSrcs += " " + kernelFileName;
    systemSrcs += " " + driverFileName;

    std::string makefileContent =   "BUILD_DIR=build\n"
                                    "\n"
                                    "#Compiler Parameters\n"
                                    "CFLAGS = -Ofast -g -std=gnu11 -march=native -masm=att\n";
    makefileContent += "LIB=-pthread -lm\n";
    if(includeLrt){
        makefileContent += "LIB+= -lrt\n";
    }
    makefileContent += "UNAME:=$(shell uname)\n"
                       "ifneq ($(UNAME), Darwin)\n"
                       "LIB+= -latomic\n"
                       "endif\n";
    if(includePAPI){
        makefileContent += "LIB+= -lpapi\n";
    }
    makefileContent += "\n"
                       "DEFINES=\n"
                       "DEPENDS=\n";

    for(int i = 0; i<additionalSystemSrc.size(); i++){
        systemSrcs += " " + additionalSystemSrc[i];
    }
    //TODO: Get the partition files which need to be compiled as we cannot rely on the cpp extension to seperate them

    makefileContent += "\n"
                       "SRCS=" + systemSrcs + "\n"
                       "OBJS=$(patsubst %.c,$(BUILD_DIR)/%.o,$(SRCS))\n"
                       "\n"
                       "#Production\n"
                       "all: benchmark_" + fileNamePrefix + "_" + ioBenchmarkSuffix + "\n"
                       "\n"
                       "benchmark_" + fileNamePrefix + "_" + ioBenchmarkSuffix + ": $(OBJS) \n"
                       "\t$(CC) $(CFLAGS) $(INC) $(DEFINES) -o benchmark_" + fileNamePrefix + "_" + ioBenchmarkSuffix + " $(OBJS) $(LIB)\n"
                       "\n"
                       "$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)/\n"
                       "\t$(CC) $(CFLAGS) -c $(INC) $(DEFINES) -o $@ $<\n"
                       "\n"
                       "$(BUILD_DIR)/:\n"
                       "\tmkdir -p $@\n"
                       "\n"
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

std::vector<std::string> MultiThreadEmit::getPartitionStateStructTypeDef(std::vector<Variable> partitionStateVars, int partitionNum){
    std::vector<std::string> structDef;

    structDef.push_back("typedef struct{");

    for(unsigned long j = 0; j<partitionStateVars.size(); j++){
        //cFile << "_Thread_local static " << stateVars[j].getCVarDecl(false, true, true) << ";" << std::endl;
        structDef.push_back(partitionStateVars[j].getCVarDecl(false, true, false, true, false, false) + ";");

        if(partitionStateVars[j].getDataType().isComplex()){
            structDef.push_back(partitionStateVars[j].getCVarDecl(true, true, false, true, false, false) + ";");
        }
    }

    std::string typeName = "Partition"+(partitionNum >= 0?GeneralHelper::to_string(partitionNum):"N"+GeneralHelper::to_string(-partitionNum))+"_state_t";

    structDef.push_back("} " + typeName + ";");

    return structDef;
}

void MultiThreadEmit::emitPartitionThreadC(int partitionNum, std::vector<std::shared_ptr<Node>> nodesToEmit,
                                                     std::vector<std::shared_ptr<ThreadCrossingFIFO>> inputFIFOs,
                                                     std::vector<std::shared_ptr<ThreadCrossingFIFO>> outputFIFOs,
                                                     std::set<std::shared_ptr<ClockDomain>> clockDomainsInPartition,
                                                     std::string path, std::string fileNamePrefix,
                                                     std::string designName, SchedParams::SchedType schedType,
                                                     std::shared_ptr<MasterOutput> outputMaster,
                                                     unsigned long blockSizeBase, std::string fifoHeaderFile,
                                                     std::string fifoSupportFile, bool threadDebugPrint, bool printTelem,
                                                     EmitterHelpers::TelemetryLevel telemLevel,
                                                     int telemReportFreqBlockFreq, double reportPeriodSeconds,
                                                     std::string telemDumpFilePrefix, bool telemAvg,
                                                     std::string papiHelperHeader,
                                                     PartitionParams::FIFOIndexCachingBehavior fifoIndexCachingBehavior,
                                                     ComputeIODoubleBufferType doubleBuffer){
    bool collectTelem = EmitterHelpers::shouldCollectTelemetry(telemLevel);
    bool collectPAPI = EmitterHelpers::usesPAPI(telemLevel);
    bool collectPAPIComputeOnly = EmitterHelpers::papiComputeOnly(telemLevel);
    bool collectBreakdownTelem = EmitterHelpers::telemetryBreakdown(telemLevel);

    //Check input FIFOs
    bool fifoInPlace = false;
    for(int i = 0; i<inputFIFOs.size(); i++){
        //TODO: Support mix of in place and non-in-place FIFOs
        if(i == 0){
            fifoInPlace = inputFIFOs[i]->isInPlace();
        }else if(fifoInPlace != inputFIFOs[i]->isInPlace()){
            throw std::runtime_error(ErrorHelpers::genErrorStr("Currently, all FIFOs must be either in place or not", inputFIFOs[i]));
        }
    }
    //Check output FIFOs
    for(int i = 0; i<outputFIFOs.size(); i++){
        //TODO: Support mix of in place and non-in-place FIFOs
        if(i == 0 && inputFIFOs.empty()){
            fifoInPlace = outputFIFOs[i]->isInPlace();
        }else if(fifoInPlace != outputFIFOs[i]->isInPlace()){
            throw std::runtime_error(ErrorHelpers::genErrorStr("Currently, all FIFOs must be either in place or not", outputFIFOs[i]));
        }
    }

    if(doubleBuffer != ComputeIODoubleBufferType::NONE && !fifoInPlace){
        //TODO: Possibly provide double buffer implementation without in-place FIFOs
        //      ie. by implementing the double buffering outside of the compute function
        throw std::runtime_error(ErrorHelpers::genErrorStr("Double Buffering Requires In-Place FIFOs"));
    }


    std::string fileName = fileNamePrefix+"_partition"+(partitionNum >= 0?GeneralHelper::to_string(partitionNum):"N"+GeneralHelper::to_string(-partitionNum));
    std::cout << "Emitting C File: " << path << "/" << fileName << ".h" << std::endl;
    //#### Emit .h file ####
    std::ofstream headerFile;
    headerFile.open(path+"/"+fileName+".h", std::ofstream::out | std::ofstream::trunc);

    std::string fileNameUpper =  GeneralHelper::toUpper(fileName);
    headerFile << "#ifndef " << fileNameUpper << "_H" << std::endl;
    headerFile << "#define " << fileNameUpper << "_H" << std::endl;

    std::set<std::string> includesHFile;

    includesHFile.insert("#include <stdint.h>");
    includesHFile.insert("#include <stdbool.h>");
    includesHFile.insert("#include <math.h>");
    includesHFile.insert("#include <pthread.h>");
    includesHFile.insert("#include \"" + std::string(VITIS_TYPE_NAME) + ".h\"");
    includesHFile.insert("#include \"" + fifoHeaderFile + "\"");
    if(!fifoSupportFile.empty()) {
        includesHFile.insert("#include \"" + fifoSupportFile + "\"");
    }

    //Include any external include statements required by nodes in the design
    for(int i = 0; i<nodesToEmit.size(); i++){
        std::set<std::string> nodeIncludes = nodesToEmit[i]->getExternalIncludes();
        includesHFile.insert(nodeIncludes.begin(), nodeIncludes.end());
    }

    for(auto it = includesHFile.begin(); it != includesHFile.end(); it++){
        headerFile << *it << std::endl;
    }

    //headerFile << "#include <thread.h>" << std::endl;
    headerFile << std::endl;

    //Need to emit blackbox headers first in case a blackbox type is referred to in the state structure
    //Insert BlackBox Headers
    std::vector<std::shared_ptr<BlackBox>> blackBoxes = EmitterHelpers::findBlackBoxes(nodesToEmit);
    if(!blackBoxes.empty()) {
        headerFile << "//==== BlackBox Headers ====" << std::endl;

        for(unsigned long i = 0; i<blackBoxes.size(); i++){
            headerFile << "//**** BEGIN BlackBox " << blackBoxes[i]->getFullyQualifiedName() << " Header ****" << std::endl;
            headerFile << blackBoxes[i]->getCppHeaderContent() << std::endl;
            headerFile << "//**** END BlackBox " << blackBoxes[i]->getFullyQualifiedName() << " Header ****" << std::endl;
        }
    }
    headerFile << std::endl;

    //Find nodes with state & global decls
    std::vector<std::shared_ptr<Node>> nodesWithState = EmitterHelpers::findNodesWithState(nodesToEmit);
    std::vector<std::shared_ptr<Node>> nodesWithGlobalDecl = EmitterHelpers::findNodesWithGlobalDecl(nodesToEmit);
    unsigned long numNodes = nodesToEmit.size();

    headerFile << "//==== State Definition ====" << std::endl;

    std::vector<Variable> stateVars;
    unsigned long nodesWithStateCount = nodesWithState.size();
    for(unsigned long i = 0; i<nodesWithStateCount; i++){
        std::vector<Variable> nodeStateVars = nodesWithState[i]->getCStateVars();
        stateVars.insert(stateVars.end(), nodeStateVars.begin(), nodeStateVars.end());
    }
    bool partitionHasState = !stateVars.empty();

    std::string stateStructTypeName = "";
    if(partitionHasState) {
        //Emit State Structure Definition
        std::vector<std::string> structDefn = getPartitionStateStructTypeDef(stateVars, partitionNum);
        for (int i = 0; i < structDefn.size(); i++) {
            headerFile << structDefn[i] << std::endl;
        }

        stateStructTypeName = "Partition" + (partitionNum >= 0 ? GeneralHelper::to_string(partitionNum) : "N" + GeneralHelper::to_string(-partitionNum)) + "_state_t";
    }

    headerFile << std::endl;

    if(blackBoxes.size() > 0) {
        headerFile << std::endl << "//==== BlackBox Post State Decls ====" << std::endl;
        for(unsigned long i = 0; i<blackBoxes.size(); i++){
            std::string decl = blackBoxes[i]->getDeclAfterState();

            if(!decl.empty()) {
                headerFile << "//**** BEGIN BlackBox " << blackBoxes[i]->getFullyQualifiedName() << " Post State Decls ****"
                           << std::endl;
                headerFile << decl;
                headerFile << "//**** END BlackBox " << blackBoxes[i]->getFullyQualifiedName() << " Post State Decls ****"
                           << std::endl;
            }
        }

        headerFile << std::endl;
    }


    //Output the Function Definition
    std::string computeFctnProtoArgs = getPartitionComputeCFunctionArgPrototype(inputFIFOs, outputFIFOs, stateStructTypeName, doubleBuffer);
    std::string computeFctnName = designName + "_partition"+(partitionNum >= 0?GeneralHelper::to_string(partitionNum):"N"+GeneralHelper::to_string(-partitionNum)) + "_compute";
    std::string computeFctnProto = "void " + computeFctnName + "(" + computeFctnProtoArgs + ")";

    headerFile << computeFctnProto << ";" << std::endl;

    //Output the thread function definition
    std::string threadFctnDecl = "void* " + designName + "_partition" + (partitionNum >= 0?GeneralHelper::to_string(partitionNum):"N"+GeneralHelper::to_string(-partitionNum)) + "_thread(void *args)";
    headerFile << threadFctnDecl << ";" << std::endl;

    //Output the reset function definition
    std::string resetFunctionName = designName + "_partition" +
                                    (partitionNum >= 0?GeneralHelper::to_string(partitionNum):"N"+GeneralHelper::to_string(-partitionNum)) +
                                    "_reset";
    std::string resetFunctionPrototype = "void " + resetFunctionName + "(";
    if(partitionHasState){
        resetFunctionPrototype += stateStructTypeName + " *" + VITIS_STATE_STRUCT_NAME;
    }
    resetFunctionPrototype += ")";
    headerFile << resetFunctionPrototype << ";" << std::endl;

    headerFile << "void " << designName + "_partition" << partitionNum << "_reset(" << (partitionHasState ? stateStructTypeName + " *" + VITIS_STATE_STRUCT_NAME : "") << ");" << std::endl;
    headerFile << std::endl;

    //Create the threadFunction argument structure
    std::pair<std::string, std::string> threadArgStructAndTypeName = MultiThreadEmit::getCThreadArgStructDefn(inputFIFOs, outputFIFOs, designName, partitionNum);
    std::string threadArgStruct = threadArgStructAndTypeName.first;
    std::string threadArgTypeName = threadArgStructAndTypeName.second;
    headerFile << threadArgStruct << std::endl;
    headerFile << std::endl;

    headerFile << "#endif" << std::endl;

    headerFile.close();

    std::cout << "Emitting C File: " << path << "/" << fileName << ".c" << std::endl;
    //#### Emit .c file ####
    std::ofstream cFile;
    cFile.open(path+"/"+fileName+".c", std::ofstream::out | std::ofstream::trunc);

    std::set<std::string> includesCFile;

    if(collectTelem){
        cFile << "#ifndef _GNU_SOURCE" << std::endl;
        cFile << "#define _GNU_SOURCE //For clock_gettime" << std::endl;
        cFile << "#endif" << std::endl;
    }

    if(threadDebugPrint || collectTelem) {
        includesCFile.insert("#include <stdio.h>");
    }

    includesCFile.insert("#include \"" + fileName + ".h\"");

    includesCFile.insert("#include \"vitisPlatformParams.h\""); //Used for cache size

    if(collectTelem){
        includesCFile.insert("#include \"" + fileNamePrefix + "_telemetry_helpers.h" + "\"");
    }

    if(collectPAPI){
        includesCFile.insert("#include \"" + papiHelperHeader + "\"");
    }

    //Include any external include statements required by nodes in the design
    for(int i = 0; i<nodesToEmit.size(); i++){
        std::set<std::string> nodeIncludes = nodesToEmit[i]->getExternalIncludes();
        includesCFile.insert(nodeIncludes.begin(), nodeIncludes.end());
    }

    for(auto it = includesCFile.begin(); it != includesCFile.end(); it++){
        cFile << *it << std::endl;
    }

    cFile << std::endl;

    cFile << "//==== Global Declarations ====" << std::endl;
    unsigned long nodesWithGlobalDeclCount = nodesWithGlobalDecl.size();
    for(unsigned long i = 0; i<nodesWithGlobalDeclCount; i++){
        cFile << nodesWithGlobalDecl[i]->getGlobalDecl() << std::endl;
    }

    cFile << std::endl;

    //Emit the reset constants
    cFile << "//==== State Var Reset Vals ====" << std::endl;
    for(unsigned long i = 0; i<nodesWithStateCount; i++) {
        std::vector<Variable> stateVars = nodesWithState[i]->getCStateVars();

        unsigned long numStateVars = stateVars.size();
        for(unsigned long j = 0; j<numStateVars; j++) {
            std::vector<std::string> rstConsts = stateVars[j].getResetConst(true);
            for(const std::string &constant : rstConsts){
                cFile << constant << std::endl;
            }
        }
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

    //TODO: Create and set counter variables for clock domains not operating in vector mode
    std::vector<std::shared_ptr<ClockDomain>> clockDomainsOrderedByRate;
    clockDomainsOrderedByRate.insert(clockDomainsOrderedByRate.begin(), clockDomainsInPartition.begin(), clockDomainsInPartition.end());
    struct {
        bool operator() (const std::shared_ptr<ClockDomain> &a,
                                   const std::shared_ptr<ClockDomain> &b) const {
            std::pair<int, int> aRate = a->getRateRelativeToBase();
            double aRateDouble = ((double) aRate.first) / aRate.second;
            std::pair<int, int> bRate = b->getRateRelativeToBase();
            double bRateDouble = ((double) aRate.first) / aRate.second;

            if (aRateDouble == bRateDouble) {
                return a->getId() < b->getId();
            }
            return aRateDouble < bRateDouble;
        };
    } clockDomainCompare;
    std::sort(clockDomainsOrderedByRate.begin(), clockDomainsOrderedByRate.end(), clockDomainCompare);

    bool emittedClockDomainInd = false;
    for(const std::shared_ptr<ClockDomain> &clockDomain : clockDomainsOrderedByRate){
        if(clockDomain->requiresDeclaringExecutionCount()){
            Variable countVar = clockDomain->getExecutionCountVariable(blockSizeBase);
            if(countVar.getDataType().isComplex()){
                throw std::runtime_error(ErrorHelpers::genErrorStr("Clock Domain Count Var expected to be real"));
            }

            if(!emittedClockDomainInd){
                cFile << "//Clock Domain Count Variables" << std::endl;
                emittedClockDomainInd = true;
            }

            cFile << countVar.getCVarDecl(false, false, true) << ";" << std::endl;
        }
    }

    //Emit operators
    if(schedType == SchedParams::SchedType::TOPOLOGICAL_CONTEXT){
        emitSelectOpsSchedStateUpdateContext(cFile, nodesToEmit, schedType, outputMaster);
    }else{
        throw std::runtime_error("Only TOPOLOGICAL_CONTEXT scheduler varient is supported for multi-threaded emit");
    }

    //TODO: Fix FIFO double buffering. It was broken by inserting sub-blocking which changed the way clock domain
    //      indexes were created and incremented.
    //      See https://github.com/ucb-cyarp/vitis/issues/97
    if(doubleBuffer != ComputeIODoubleBufferType::NONE){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Double Buffer FIFOs not currently supported after automated sub-blocking introduced.  See https://github.com/ucb-cyarp/vitis/issues/97"));
    }
//    std::vector<std::string> dblBufferExprs = computeIODoubleBufferEmit(inputFIFOs, outputFIFOs, clkDomainRate,
//                                                                        getClkDomainIndVarName(clkDomainRate, true),
//                                                                        getClkDomainIndVarName(clkDomainRate, false), doubleBuffer);

    cFile << "}" << std::endl; //Close compute function

    cFile << std::endl;

    cFile << resetFunctionPrototype << "{" << std::endl;
    cFile << "//==== Reset State Vars ====" << std::endl;
    for(unsigned long i = 0; i<nodesWithStateCount; i++){
        std::vector<Variable> stateVars = nodesWithState[i]->getCStateVars();
        //Emit State Vars
        unsigned long numStateVars = stateVars.size();
        for(unsigned long j = 0; j<numStateVars; j++){
            std::vector<std::string> rstStatements = stateVars[j].genReset();
            for(const std::string &rstStatement : rstStatements) {
                cFile << rstStatement << std::endl;
            }
        }
    }

    //Call the reset functions of blackboxes if they have state
    if(blackBoxes.size() > 0) {
        cFile << "//==== Reset BlackBoxes ====" << std::endl;

        for(unsigned long i = 0; i<blackBoxes.size(); i++){
            cFile << blackBoxes[i]->getResetFunctionCall() << ";" << std::endl;
        }
    }

    cFile << "}" << std::endl;
    cFile << std::endl;

    //Emit thread function
    cFile << threadFctnDecl << "{" << std::endl;
    //Copy ptrs from struct argument
    cFile << MultiThreadEmit::emitCopyCThreadArgs(inputFIFOs, outputFIFOs, "args", threadArgTypeName);

    //Allocate state
    //Will allocate on this thread's stack.  A check should have occured to indicate if multiple stacks could possibly share a cache line.
    if(partitionHasState){
        cFile << std::endl;
        cFile << "//Allocate state" << std::endl;
        cFile << stateStructTypeName << " " << VITIS_STATE_STRUCT_NAME << ";" << std::endl;
        cFile << std::endl;
    }

    //Call reset function to initialize state and black boxes
    cFile << "//Reset/init state"<< std::endl;
    cFile << resetFunctionName << "(";
    if(partitionHasState){
        cFile << "&" << VITIS_STATE_STRUCT_NAME;
    }
    cFile << ");" << std::endl;
    cFile << std::endl;

    if(collectTelem){
        cFile << "timespec_t timeResolution;" << std::endl;
        cFile << "clock_getres(CLOCK_MONOTONIC, &timeResolution);" << std::endl;
        cFile << "double timeResolutionDouble = timespecToDouble(&timeResolution);" << std::endl;

        if(printTelem) {
            cFile << "printf(\"Partition " << partitionNum << " Time Resolution: %8.6e\\n\", timeResolutionDouble);" << std::endl;
        }
        if(!telemDumpFilePrefix.empty()){
            cFile << "FILE* telemDumpFile = fopen(\"" << telemDumpFilePrefix << partitionNum << ".csv\", \"w\");" << std::endl;

            //Write Header Row
            cFile << "fprintf(telemDumpFile, \"TimeStamp_s,TimeStamp_ns,Rate_msps";
            if(collectBreakdownTelem){
                cFile << ",WaitingForInputFIFOs_s,ReadingInputFIFOs_s,WaitingForComputeToFinish_s,WaitingForOutputFIFOs_s,WritingOutputFIFOs_s,Telemetry_Misc_s";
            }

            cFile << ",TotalTime_s";

            if(collectPAPI){
                cFile << ",clock_cycles,instructions_retired,floating_point_operations_retired,l1_data_cache_accesses";
                if(collectPAPIComputeOnly){
                    cFile << ",timeWaitingForComputeToFinishPlusPAPI";
                }
            }
            cFile << "\\n\");" << std::endl;
        }
        cFile << std::endl;

        if(collectPAPI){
            cFile << "//Setup PAPI Event Set" << std::endl;
            cFile << "int papiEventSet = setupPapiThread();\n"
                     "startPapiCounters(papiEventSet);\n"
                     "long long clock_cycles = 0;\n"
                     "long long instructions_retired = 0;\n"
                     "long long floating_point_operations_retired = 0;\n"
                     "long long l1_data_cache_accesses = 0;" << std::endl;
            if(collectPAPIComputeOnly){
                //Need to track the time for computing and reading PAPI when reporting clock freq
                cFile << "double timeWaitingForComputeToFinishPlusPAPI = 0;" << std::endl;
            }
        }
        cFile << "//Start timer" << std::endl;
        cFile << "uint64_t rxSamples = 0;" << std::endl;
        cFile << "timespec_t startTime;" << std::endl;
        cFile << "asm volatile (\"\" ::: \"memory\"); //Stop Re-ordering of timer" << std::endl;
        cFile << "clock_gettime(CLOCK_MONOTONIC, &startTime);" << std::endl;
        cFile << "asm volatile (\"\" ::: \"memory\"); //Stop Re-ordering of timer" << std::endl;
        cFile << "timespec_t lastPrint;" << std::endl;
        cFile << "asm volatile (\"\" ::: \"memory\"); //Stop Re-ordering of timer" << std::endl;
        cFile << "clock_gettime(CLOCK_MONOTONIC, &lastPrint);" << std::endl;
        cFile << "asm volatile (\"\" ::: \"memory\"); //Stop Re-ordering of timer" << std::endl;
        if(collectBreakdownTelem) {
            cFile << "double timeTotal = 0;" << std::endl;
            cFile << "double timeWaitingForInputFIFOs = 0;" << std::endl;
            cFile << "double timeReadingInputFIFOs = 0;" << std::endl;
            cFile << "double timeWaitingForComputeToFinish = 0;" << std::endl;
            cFile << "double timeWaitingForOutputFIFOs = 0;" << std::endl;
            cFile << "double timeWritingOutputFIFOs = 0;" << std::endl;
        }
        cFile << "bool collectTelem = false;" << std::endl;
        cFile << "int telemCheckCount = 0;" << std::endl;
    }

    //Create Local Vars
    std::vector<std::string> cachedVarDeclsInputFIFOs = MultiThreadEmit::createAndInitFIFOLocalVars(
            inputFIFOs);
    for(unsigned long i = 0; i<cachedVarDeclsInputFIFOs.size(); i++){
        cFile << cachedVarDeclsInputFIFOs[i] << std::endl;
    }

    std::vector<std::string> cachedVarDeclsOutputFIFOs = MultiThreadEmit::createAndInitFIFOLocalVars(
            outputFIFOs);
    for(unsigned long i = 0; i<cachedVarDeclsOutputFIFOs.size(); i++){
        cFile << cachedVarDeclsOutputFIFOs[i] << std::endl;
    }

    //Create temp entries for FIFO inputs
    if(!fifoInPlace) {
        std::vector<std::string> tmpReadDecls = MultiThreadEmit::createFIFOReadTemps(inputFIFOs);
        for (int i = 0; i < tmpReadDecls.size(); i++) {
            cFile << tmpReadDecls[i] << std::endl;
        }
    }

    //Create temp entries for outputs
    if(!fifoInPlace) {
        std::vector<std::string> tmpWriteDecls = MultiThreadEmit::createFIFOWriteTemps(outputFIFOs);
        for (int i = 0; i < tmpWriteDecls.size(); i++) {
            cFile << tmpWriteDecls[i] << std::endl;
        }
    }

    //Create input double buffer entries (if applicable)
    std::vector<std::string> inputDoubleBufferEntries = createFIFODoubleBufferReadVars(inputFIFOs, doubleBuffer);
    for (int i = 0; i < inputDoubleBufferEntries.size(); i++) {
        cFile << inputDoubleBufferEntries[i] << std::endl;
    }

    //Create output double buffer entries (if applicable)
    std::vector<std::string> outputDoubleBufferEntries = createFIFODoubleBufferWriteVars(outputFIFOs, doubleBuffer);
    for (int i = 0; i < outputDoubleBufferEntries.size(); i++) {
        cFile << outputDoubleBufferEntries[i] << std::endl;
    }

    //TODO: For double buffer, handle the initial load and store
    if(doubleBuffer == ComputeIODoubleBufferType::INPUT_AND_OUTPUT || doubleBuffer == ComputeIODoubleBufferType::INPUT) {
        cFile << std::endl;
        cFile << "{" << std::endl; //Open a scope for this
        cFile << "//Priming Double Buffer Input" << std::endl;

        //Need to initially read from the input FIFO without running compute
        //First, wait for input FIFOs
        cFile << MultiThreadEmit::emitFIFOChecks(inputFIFOs, false, "inputFIFOsReady", false, true, false, fifoIndexCachingBehavior); //Include pthread_testcancel check

        //TODO: currently assumes inPlace FIFOs is true, modify if this changes in the future
        std::vector<std::string> readFIFOExprs = MultiThreadEmit::readFIFOsToTemps(inputFIFOs, false, false, false);
        for (int i = 0; i < readFIFOExprs.size(); i++) {
            cFile << readFIFOExprs[i] << std::endl;
        }

        //Copy the values from the input FIFO _readTmp ptrs to the _current buffers
        for(auto inputFIFO : inputFIFOs){
            std::string sharedPtr = inputFIFO->getName() + "_readTmp";
            std::string currentPtr = inputFIFO->getName() + "_current";

            //Emit the copy
            cFile << "*" << currentPtr << "=*" << sharedPtr << ";" << std::endl;
        }

        //Need to update FIFOs with the read
        std::vector<std::string> readStatePush = pushReadFIFOsStatus(inputFIFOs);
        for (int i = 0; i < readStatePush.size(); i++) {
            cFile << readStatePush[i] << std::endl;
        }
        cFile << "}" << std::endl; //Close scope
    }

    if(doubleBuffer == ComputeIODoubleBufferType::INPUT_AND_OUTPUT || doubleBuffer == ComputeIODoubleBufferType::OUTPUT) {
        cFile << std::endl;
        cFile << "{" << std::endl; //Open a scope for this
        cFile << "//Priming Double Buffer Output" << std::endl;

        //Need to run an initial run of the compute function to get an output which can be written in the next itteration
        //To do this, will pass _prev as both the prev ptr ans the shared ptr.  This will cause prev to be written into itself instead of _prev being written to the FIFO
        //The _prev value will be discarded in any case and will become the buffer where the next itteration's compute output will be written

        //First, wait for input FIFOs
        cFile << MultiThreadEmit::emitFIFOChecks(inputFIFOs, false, "inputFIFOsReady", false, true, false, fifoIndexCachingBehavior); //Include pthread_testcancel check

        //Do not need to wait for output FIFOs since we are not actually writing into them

        //TODO: currently assumes inPlace FIFOs is true, modify if this changes in the future
        std::vector<std::string> readFIFOExprs = MultiThreadEmit::readFIFOsToTemps(inputFIFOs, false, false, false);
        for (int i = 0; i < readFIFOExprs.size(); i++) {
            cFile << readFIFOExprs[i] << std::endl;
        }

        //Run compute
        std::string stateArg = "";
        if(partitionHasState){
            stateArg = std::string("&") + VITIS_STATE_STRUCT_NAME;
        }
        std::string call = getCallPartitionComputeCFunction(computeFctnName, inputFIFOs, outputFIFOs,
                                                            fifoInPlace, stateArg, doubleBuffer,
                                                            "_readTmp",
                                                            "_prev", //this is normally "_writeTmp" but we do not actually want to write
                                                            "_next",
                                                            "_current",
                                                            "_current",
                                                            "_prev");
        cFile << "//Calling compute function to collect outputs in local buffer.  Not writing to output FIFOs since the current local buffer state for outputs is garbage" << std::endl;
        cFile << call << std::endl;

        //Update the input FIFOs but not the output FIFOs as we did not actually write
        std::vector<std::string> readStatePush = pushReadFIFOsStatus(inputFIFOs);
        for (int i = 0; i < readStatePush.size(); i++) {
            cFile << readStatePush[i] << std::endl;
        }

        cFile << "//Swapping double buffer ptrs" << std::endl;
        //Need to swap the buffers for reading and writing before going into the main loop
        std::vector<std::string> swapInputDoubleBufferPtrs = swapReadDoubleBufferPtrs(inputFIFOs, doubleBuffer);
        for (int i = 0; i < swapInputDoubleBufferPtrs.size(); i++) {
            cFile << swapInputDoubleBufferPtrs[i] << std::endl;
        }
        std::vector<std::string> swapOutputDoubleBufferPtrs = swapWriteDoubleBufferPtrs(outputFIFOs, doubleBuffer);
        for (int i = 0; i < swapOutputDoubleBufferPtrs.size(); i++) {
            cFile << swapOutputDoubleBufferPtrs[i] << std::endl;
        }

        cFile << "}" << std::endl; //Close scope
    }


    //Create Loop
    cFile << "//Begin Core Loop" << std::endl;
    cFile << "while(1){" << std::endl;

    //Move telemetry reporting to start of loop for more consistent telemetry reporting (no longer inside a single transaction)
    //The start time is before any of the telemetry is printed/written so, what is effectively captured is the time to report telemetry
    //for the last interval + the time to evaluate the current interval.  In this case, nothing is double counted.
    if(collectTelem){
        //Emit timer reporting
        cFile << "if(telemCheckCount > " << telemReportFreqBlockFreq << "){" << std::endl;
        cFile << "timespec_t currentTime;" << std::endl;
        cFile << "asm volatile (\"\" ::: \"memory\"); //Stop Re-ordering of timer" << std::endl;
        cFile << "clock_gettime(CLOCK_MONOTONIC, &currentTime);" << std::endl;
        cFile << "asm volatile (\"\" ::: \"memory\"); //Stop Re-ordering of timer" << std::endl;
        cFile << "double duration = difftimespec(&currentTime, &lastPrint);" << std::endl;
        cFile << "if(duration >= " << reportPeriodSeconds << "){" << std::endl;

        //If Collecting PAPI for entire thread (not isolated just to compute), collect here.  We want to avoid hitting PAPI too often since it involves a system call
        if(collectPAPI && !collectPAPIComputeOnly){
            cFile << "performance_counter_data_t counterData;" << std::endl;
            cFile << "asm volatile(\"\" ::: \"memory\"); //Stop Re-ordering of PAPI Read" << std::endl;
            cFile << "readResetPapiCounters(&counterData, papiEventSet);" << std::endl;
            cFile << "asm volatile(\"\" ::: \"memory\"); //Stop Re-ordering of PAPI Read" << std::endl;
            cFile << "clock_cycles += counterData.clock_cycles;\n"
                     "instructions_retired += counterData.instructions_retired;\n"
                     "floating_point_operations_retired += counterData.floating_point_operations_retired;\n"
                     "l1_data_cache_accesses += counterData.l1_data_cache_accesses;" << std::endl;
        }

        cFile << "lastPrint = currentTime;" << std::endl;
        cFile << "double durationSinceStart = difftimespec(&currentTime, &startTime);" << std::endl;
        cFile << "double rateMSps = ((double)rxSamples)/durationSinceStart/1000000;" << std::endl;
        if(collectBreakdownTelem) {
            cFile << "double durationTelemMisc = durationSinceStart-timeTotal;" << std::endl;
        }
        if (printTelem) {
            //Print the telemetry information to stdout
            cFile << "printf(\"Current " << designName << " [" << partitionNum << "]  Rate: %10.5f\\n\"" << std::endl;
            if(collectBreakdownTelem){
                cFile << "\"\\t[" << partitionNum << "] Waiting for Input FIFOs:        %10.5f (%8.4f%%)\\n\"" << std::endl;
                cFile << "\"\\t[" << partitionNum << "] Reading Input FIFOs:            %10.5f (%8.4f%%)\\n\"" << std::endl;
                cFile << "\"\\t[" << partitionNum << "] Waiting For Compute to Finish:  %10.5f (%8.4f%%)\\n\"" << std::endl;
                cFile << "\"\\t[" << partitionNum << "] Waiting for Output FIFOs:       %10.5f (%8.4f%%)\\n\"" << std::endl;
                cFile << "\"\\t[" << partitionNum << "] Writing Output FIFOs:           %10.5f (%8.4f%%)\\n\"" << std::endl;
                cFile << "\"\\t[" << partitionNum << "] Telemetry/Misc:                 %10.5f (%8.4f%%)\\n\"" << std::endl;
            }
            if(collectPAPI){
                cFile << "\"\\t\\t[" << partitionNum << "] Cycles:                       %10lld\\n\"" << std::endl;
                cFile << "\"\\t\\t[" << partitionNum << "] Instructions:                 %10lld (%4.3f)\\n\"" << std::endl;
                cFile << "\"\\t\\t[" << partitionNum << "] Floating Point Operations:    %10lld (%4.3f)\\n\"" << std::endl;
                cFile << "\"\\t\\t[" << partitionNum << "] L1 Data Cache Accesses:       %10lld (%4.3f)\\n\"" << std::endl;
                cFile << "\"\\t\\t[" << partitionNum << "] Clock Rate (MHz):             %10f\\n\"" << std::endl;
                if(collectPAPIComputeOnly){
                    cFile << "\"\\t\\t[" << partitionNum << "] Time for Compute + PAPI Read: %10f\\n\"" << std::endl;
                }
            }

            cFile << ", rateMSps" << std::endl;
            if(collectBreakdownTelem){
                cFile << ", timeWaitingForInputFIFOs, timeWaitingForInputFIFOs/durationSinceStart*100, " << std::endl;
                cFile << "timeReadingInputFIFOs, timeReadingInputFIFOs/durationSinceStart*100, " << std::endl;
                cFile << "timeWaitingForComputeToFinish, timeWaitingForComputeToFinish/durationSinceStart*100, " << std::endl;
                cFile << "timeWaitingForOutputFIFOs, timeWaitingForOutputFIFOs/durationSinceStart*100, " << std::endl;
                cFile << "timeWritingOutputFIFOs, timeWritingOutputFIFOs/durationSinceStart*100, " << std::endl;
                cFile << "durationTelemMisc, durationTelemMisc/durationSinceStart*100" << std::endl;
            }
            if(collectPAPI){
                cFile << ",clock_cycles," << std::endl;
                cFile << "instructions_retired,                ((double) instructions_retired)/clock_cycles," << std::endl;
                cFile << "floating_point_operations_retired,   ((double) floating_point_operations_retired)/clock_cycles," << std::endl;
                cFile << "l1_data_cache_accesses,              ((double) l1_data_cache_accesses)/clock_cycles," << std::endl;
                if(!collectPAPIComputeOnly) {
                    cFile << "((double) clock_cycles)/durationSinceStart/1000000" << std::endl;
                }else{
                    cFile << "((double) clock_cycles)/timeWaitingForComputeToFinishPlusPAPI/1000000" << std::endl;
                    cFile << ", timeWaitingForComputeToFinishPlusPAPI" << std::endl;
                }
            }
            cFile << ");" << std::endl;
        }
        if (!telemDumpFilePrefix.empty()) {
            //Write the telemetry to the file
            //The file includes the timestamp at the time it was written.  This is used to align telemetry from multiple threads
            //The partition number is included in the filename and is not written to the file
            cFile << "fprintf(telemDumpFile, \"%ld,%ld,%e";
            if(collectBreakdownTelem){
                cFile << ",%e,%e,%e,%e,%e,%e";
            }
            cFile << ",%e";
            if(collectPAPI) {
                cFile << ",%lld,%lld,%lld,%lld";
                if(collectPAPIComputeOnly){
                    cFile << ",%e";
                }
            }

            cFile << "\\n\", currentTime.tv_sec, currentTime.tv_nsec, rateMSps";

            if(collectBreakdownTelem){
                cFile << ", timeWaitingForInputFIFOs, timeReadingInputFIFOs,"
                         "timeWaitingForComputeToFinish, timeWaitingForOutputFIFOs, timeWritingOutputFIFOs, "
                         "durationTelemMisc";
            }

            cFile << ", durationSinceStart";

            if(collectPAPI) {
                cFile << ",clock_cycles,instructions_retired,floating_point_operations_retired,l1_data_cache_accesses";
                if(collectPAPIComputeOnly){
                    cFile << ",timeWaitingForComputeToFinishPlusPAPI";
                }
            }
            cFile << ");" << std::endl;

            //flush the file
            cFile << "fflush(telemDumpFile);" << std::endl;
            cFile << std::endl;
        }

        if (!telemAvg) {
            //Reset the counters for the next collection interval.
            cFile << "startTime = currentTime;" << std::endl;
            cFile << "rxSamples = 0;" << std::endl;
            if(collectBreakdownTelem) {
                cFile << "timeTotal = 0;" << std::endl;
                cFile << "timeWaitingForInputFIFOs = 0;" << std::endl;
                cFile << "timeReadingInputFIFOs = 0;" << std::endl;
                cFile << "timeWaitingForComputeToFinish = 0;" << std::endl;
                cFile << "timeWaitingForOutputFIFOs = 0;" << std::endl;
                cFile << "timeWritingOutputFIFOs = 0;" << std::endl;
            }
            if(collectPAPI) {
                cFile << "clock_cycles = 0;" << std::endl;
                cFile << "instructions_retired = 0;" << std::endl;
                cFile << "floating_point_operations_retired = 0;" << std::endl;
                cFile << "l1_data_cache_accesses = 0;" << std::endl;
                if(collectPAPIComputeOnly){
                    cFile << "timeWaitingForComputeToFinishPlusPAPI = 0;" << std::endl;
                }
            }
        }
        cFile << "}" << std::endl;
        cFile << "telemCheckCount=0;" << std::endl;
        cFile << "}else{" << std::endl;
        cFile << "telemCheckCount++;" << std::endl;
        cFile << "}" << std::endl;
    }

    if(threadDebugPrint) {
        cFile << "printf(\"Partition " + GeneralHelper::to_string(partitionNum) + " waiting for inputs ...\\n\");"
              << std::endl;
    }

    //=== Check Input FIFOs ===
    if(collectBreakdownTelem) {
        cFile << "timespec_t waitingForInputFIFOsStart;" << std::endl;
        cFile << "asm volatile (\"\" ::: \"memory\"); //Stop Re-ordering of timer" << std::endl;
        cFile << "clock_gettime(CLOCK_MONOTONIC, &waitingForInputFIFOsStart);" << std::endl;
        cFile << "asm volatile (\"\" ::: \"memory\"); //Stop Re-ordering of timer" << std::endl;
    }

    //Check FIFO input FIFOs (will spin until ready)
    cFile << MultiThreadEmit::emitFIFOChecks(inputFIFOs, false, "inputFIFOsReady", false, true, false, fifoIndexCachingBehavior); //Include pthread_testcancel check

    //This is a special case where the duration for this cycle is calculated later (after reporting).  That way,
    //each metric has undergone the same number of cycles
    if(collectBreakdownTelem) {
        cFile << "timespec_t waitingForInputFIFOsStop;" << std::endl;
        cFile << "asm volatile (\"\" ::: \"memory\"); //Stop Re-ordering of timer" << std::endl;
        cFile << "clock_gettime(CLOCK_MONOTONIC, &waitingForInputFIFOsStop);" << std::endl;
        cFile << "asm volatile (\"\" ::: \"memory\"); //Stop Re-ordering of timer" << std::endl;
        cFile
                << "double durationWaitingForInputFIFOs = difftimespec(&waitingForInputFIFOsStop, &waitingForInputFIFOsStart);"
                << std::endl;
        cFile << "timeWaitingForInputFIFOs += durationWaitingForInputFIFOs;" << std::endl;
        cFile << "timeTotal += durationWaitingForInputFIFOs;" << std::endl;
    }

    //=== Check Write FIFO (for in-place)
    if(fifoInPlace){
        //Check output FIFOs (will spin until ready)
        if(threadDebugPrint) {
            cFile << "printf(\"Partition " + GeneralHelper::to_string(partitionNum) +
                     " waiting for room in output FIFOs ...\\n\");" << std::endl;
        }

        if(collectBreakdownTelem) {
            cFile << "timespec_t waitingForOutputFIFOsStart;" << std::endl;
            cFile << "asm volatile (\"\" ::: \"memory\"); //Stop Re-ordering of timer" << std::endl;
            cFile << "clock_gettime(CLOCK_MONOTONIC, &waitingForOutputFIFOsStart);" << std::endl;
            cFile << "asm volatile (\"\" ::: \"memory\"); //Stop Re-ordering of timer" << std::endl;
        }

        cFile << MultiThreadEmit::emitFIFOChecks(outputFIFOs, true, "outputFIFOsReady", false, true, false, fifoIndexCachingBehavior); //Include pthread_testcancel check

        if(collectBreakdownTelem) {
            cFile << "timespec_t waitingForOutputFIFOsStop;" << std::endl;
            cFile << "asm volatile (\"\" ::: \"memory\"); //Stop Re-ordering of timer" << std::endl;
            cFile << "clock_gettime(CLOCK_MONOTONIC, &waitingForOutputFIFOsStop);" << std::endl;
            cFile << "asm volatile (\"\" ::: \"memory\"); //Stop Re-ordering of timer" << std::endl;
            cFile << "double durationWaitingForOutputFIFOs = difftimespec(&waitingForOutputFIFOsStop, &waitingForOutputFIFOsStart);" << std::endl;
            cFile << "timeWaitingForOutputFIFOs += durationWaitingForOutputFIFOs;" << std::endl;
            cFile << "timeTotal += durationWaitingForOutputFIFOs;" << std::endl;
        }
    }

    //=== Read FIFOs (and write for in-place) ===
    if(fifoInPlace){
        //Need to do both FIFO read (get read ptr) and FIFO write
        std::vector<std::string> readFIFOExprs = MultiThreadEmit::readFIFOsToTemps(inputFIFOs, false, false, false);
        for (int i = 0; i < readFIFOExprs.size(); i++) {
            cFile << readFIFOExprs[i] << std::endl;
        }
        std::vector<std::string> writeFIFOExprs = MultiThreadEmit::writeFIFOsFromTemps(outputFIFOs, false, false, false);
        for (int i = 0; i < writeFIFOExprs.size(); i++) {
            cFile << writeFIFOExprs[i] << std::endl;
        }

        if(collectTelem){
            cFile << "rxSamples += " << blockSizeBase << ";" << std::endl;
        }
    }else {
        if(collectBreakdownTelem){
            //Now, time how long it takes to read the FIFO
            cFile << "timespec_t readingInputFIFOsStart;" << std::endl;
            cFile << "asm volatile (\"\" ::: \"memory\"); //Stop Re-ordering of timer" << std::endl;
            cFile << "clock_gettime(CLOCK_MONOTONIC, &readingInputFIFOsStart);" << std::endl;
            cFile << "asm volatile (\"\" ::: \"memory\"); //Stop Re-ordering of timer" << std::endl;
        }

        //Read input FIFOs
        if(threadDebugPrint) {
            cFile << "printf(\"Partition " + GeneralHelper::to_string(partitionNum) + " reading inputs ...\\n\");"
                  << std::endl;
        }

        std::vector<std::string> readFIFOExprs = MultiThreadEmit::readFIFOsToTemps(inputFIFOs, false, true, false);
        for (int i = 0; i < readFIFOExprs.size(); i++) {
            cFile << readFIFOExprs[i] << std::endl;
        }

        if(collectBreakdownTelem) {
            cFile << "timespec_t readingInputFIFOsStop;" << std::endl;
            cFile << "asm volatile (\"\" ::: \"memory\"); //Stop Re-ordering of timer" << std::endl;
            cFile << "clock_gettime(CLOCK_MONOTONIC, &readingInputFIFOsStop);" << std::endl;
            cFile << "asm volatile (\"\" ::: \"memory\"); //Stop Re-ordering of timer" << std::endl;
            cFile << "double durationReadingInputFIFOs = difftimespec(&readingInputFIFOsStop, &readingInputFIFOsStart);" << std::endl;
            cFile << "timeReadingInputFIFOs += durationReadingInputFIFOs;" << std::endl;
            cFile << "timeTotal += durationReadingInputFIFOs;" << std::endl;
        }
        if(collectTelem){
            cFile << "rxSamples += " << blockSizeBase << ";" << std::endl;
        }
    }

    if(threadDebugPrint) {
        cFile << "printf(\"Partition " + GeneralHelper::to_string(partitionNum) + " computing ...\\n\");" << std::endl;
    }

    //If collecting the telemetry breakdown, do this
    if(collectBreakdownTelem) {
        cFile << "timespec_t waitingForComputeToFinishStart;" << std::endl;
        if(collectPAPIComputeOnly) {
            cFile << "asm volatile (\"\" ::: \"memory\"); //Stop Re-ordering of PAPI reset" << std::endl;
            cFile << "resetPapiCounters(papiEventSet); //Including timer read in the performance counter segment since clock_gettime should require very few cycles but PAPI requires more.  " << std::endl;
        }
        cFile << "asm volatile (\"\" ::: \"memory\"); //Stop Re-ordering of timer" << std::endl;
        cFile << "clock_gettime(CLOCK_MONOTONIC, &waitingForComputeToFinishStart);" << std::endl;
        cFile << "asm volatile (\"\" ::: \"memory\"); //Stop Re-ordering of timer" << std::endl;
    }else if(collectPAPIComputeOnly){
        //If not collecting breakdown telemetry but collecting papi durring the compute function only
        cFile << "asm volatile (\"\" ::: \"memory\"); //Stop Re-ordering of PAPI reset" << std::endl;
        cFile << "resetPapiCounters(papiEventSet); //Including timer read in the performance counter segment since clock_gettime should require very few cycles but PAPI requires more.  " << std::endl;
        cFile << "asm volatile (\"\" ::: \"memory\"); //Stop Re-ordering of PAPI reset" << std::endl;
    }

    //Call compute function (recall that the compute function is declared with outputs as references)
    std::string stateArg = "";
    if(partitionHasState){
        stateArg = std::string("&") + VITIS_STATE_STRUCT_NAME;
    }
    std::string call = getCallPartitionComputeCFunction(computeFctnName, inputFIFOs, outputFIFOs, fifoInPlace,
                                                        stateArg, doubleBuffer,
                                                        "_readTmp",
                                                        "_writeTmp",
                                                        "_next",
                                                        "_current",
                                                        "_current",
                                                        "_prev");
    cFile << call << std::endl;

    //If collecting the telemetry breakdown, do this
    if(collectBreakdownTelem) {
        cFile << "timespec_t waitingForComputeToFinishStop;" << std::endl;
        cFile << "asm volatile (\"\" ::: \"memory\"); //Stop Re-ordering of timer" << std::endl;
        cFile << "clock_gettime(CLOCK_MONOTONIC, &waitingForComputeToFinishStop);" << std::endl;
        cFile << "asm volatile (\"\" ::: \"memory\"); //Stop Re-ordering of timer" << std::endl;
        if(collectPAPIComputeOnly) {
            cFile << "performance_counter_data_t counterData;" << std::endl;
            cFile << "readPapiCounters(&counterData, papiEventSet);" << std::endl;
            cFile << "asm volatile(\"\" ::: \"memory\"); //Stop Re-ordering of PAPI Read" << std::endl;
            cFile << "timespec_t waitingForComputeToFinishPlusPapiStop;" << std::endl;
            cFile << "clock_gettime(CLOCK_MONOTONIC, &waitingForComputeToFinishPlusPapiStop);" << std::endl;
            cFile << "asm volatile (\"\" ::: \"memory\"); //Stop Re-ordering of timer" << std::endl;
        }
        cFile << "double durationWaitingForComputeToFinish = difftimespec(&waitingForComputeToFinishStop, &waitingForComputeToFinishStart);" << std::endl;
        cFile << "timeWaitingForComputeToFinish += durationWaitingForComputeToFinish;" << std::endl;
        cFile << "timeTotal += durationWaitingForComputeToFinish;" << std::endl;
        if(collectPAPIComputeOnly) {
            cFile << "clock_cycles += counterData.clock_cycles;\n"
                     "instructions_retired += counterData.instructions_retired;\n"
                     "floating_point_operations_retired += counterData.floating_point_operations_retired;\n"
                     "l1_data_cache_accesses += counterData.l1_data_cache_accesses;" << std::endl;
            cFile << "double durationWaitingForComputePlusPapiToFinish = difftimespec(&waitingForComputeToFinishPlusPapiStop, &waitingForComputeToFinishStart);" << std::endl;
            cFile << "timeWaitingForComputeToFinishPlusPAPI += durationWaitingForComputePlusPapiToFinish;" << std::endl;
        }
    }else if(collectPAPIComputeOnly){
        cFile << "performance_counter_data_t counterData;" << std::endl;
        cFile << "asm volatile(\"\" ::: \"memory\"); //Stop Re-ordering of PAPI Read" << std::endl;
        cFile << "readPapiCounters(&counterData, papiEventSet);" << std::endl;
        cFile << "asm volatile(\"\" ::: \"memory\"); //Stop Re-ordering of PAPI Read" << std::endl;
        cFile << "clock_cycles += counterData.clock_cycles;\n"
                 "instructions_retired += counterData.instructions_retired;\n"
                 "floating_point_operations_retired += counterData.floating_point_operations_retired;\n"
                 "l1_data_cache_accesses += counterData.l1_data_cache_accesses;" << std::endl;
    }

    //Only do FIFO output check here if not in-place (done before compute if in place)
    if(!fifoInPlace){
        //Check output FIFOs (will spin until ready)
        if(threadDebugPrint) {
            cFile << "printf(\"Partition " + GeneralHelper::to_string(partitionNum) +
                     " waiting for room in output FIFOs ...\\n\");" << std::endl;
        }

        if(collectBreakdownTelem) {
            cFile << "timespec_t waitingForOutputFIFOsStart;" << std::endl;
            cFile << "asm volatile (\"\" ::: \"memory\"); //Stop Re-ordering of timer" << std::endl;
            cFile << "clock_gettime(CLOCK_MONOTONIC, &waitingForOutputFIFOsStart);" << std::endl;
            cFile << "asm volatile (\"\" ::: \"memory\"); //Stop Re-ordering of timer" << std::endl;
        }

        cFile << MultiThreadEmit::emitFIFOChecks(outputFIFOs, true, "outputFIFOsReady", false, true, false, fifoIndexCachingBehavior); //Include pthread_testcancel check

        if(collectBreakdownTelem) {
            cFile << "timespec_t waitingForOutputFIFOsStop;" << std::endl;
            cFile << "asm volatile (\"\" ::: \"memory\"); //Stop Re-ordering of timer" << std::endl;
            cFile << "clock_gettime(CLOCK_MONOTONIC, &waitingForOutputFIFOsStop);" << std::endl;
            cFile << "asm volatile (\"\" ::: \"memory\"); //Stop Re-ordering of timer" << std::endl;
            cFile << "double durationWaitingForOutputFIFOs = difftimespec(&waitingForOutputFIFOsStop, &waitingForOutputFIFOsStart);" << std::endl;
            cFile << "timeWaitingForOutputFIFOs += durationWaitingForOutputFIFOs;" << std::endl;
            cFile << "timeTotal += durationWaitingForOutputFIFOs;" << std::endl;
        }

        //Write result to FIFOs
        if(threadDebugPrint) {
            cFile << "printf(\"Partition " + GeneralHelper::to_string(partitionNum) + " writing outputs ...\\n\");"
                  << std::endl;
        }

        //If the FIFOs are not in place
        if (collectBreakdownTelem) {
            cFile << "timespec_t writingOutputFIFOsStart;" << std::endl;
            cFile << "asm volatile (\"\" ::: \"memory\"); //Stop Re-ordering of timer" << std::endl;
            cFile << "clock_gettime(CLOCK_MONOTONIC, &writingOutputFIFOsStart);" << std::endl;
            cFile << "asm volatile (\"\" ::: \"memory\"); //Stop Re-ordering of timer" << std::endl;
        }

        std::vector<std::string> writeFIFOExprs = MultiThreadEmit::writeFIFOsFromTemps(outputFIFOs, false, true, false);
        for (int i = 0; i < writeFIFOExprs.size(); i++) {
            cFile << writeFIFOExprs[i] << std::endl;
        }

        if (collectBreakdownTelem) {
            cFile << "timespec_t writingOutputFIFOsStop;" << std::endl;
            cFile << "asm volatile (\"\" ::: \"memory\"); //Stop Re-ordering of timer" << std::endl;
            cFile << "clock_gettime(CLOCK_MONOTONIC, &writingOutputFIFOsStop);" << std::endl;
            cFile << "asm volatile (\"\" ::: \"memory\"); //Stop Re-ordering of timer" << std::endl;
            cFile
                    << "double durationWritingOutputFIFOs = difftimespec(&writingOutputFIFOsStop, &writingOutputFIFOsStart);"
                    << std::endl;
            cFile << "timeWritingOutputFIFOs += durationWritingOutputFIFOs;" << std::endl;
            cFile << "timeTotal += durationWritingOutputFIFOs;" << std::endl;
        }

        if(threadDebugPrint) {
            cFile << "printf(\"Partition " + GeneralHelper::to_string(partitionNum) + " done writing outputs ...\\n\");"
                  << std::endl;
        }
    }

    //==== If in-place, push the updates to the FIFOs here ====
    if(fifoInPlace){
        std::vector<std::string> readStatePush = pushReadFIFOsStatus(inputFIFOs);
        for (int i = 0; i < readStatePush.size(); i++) {
            cFile << readStatePush[i] << std::endl;
        }

        std::vector<std::string> writeStatePush = pushWriteFIFOsStatus(outputFIFOs);
        for (int i = 0; i < writeStatePush.size(); i++) {
            cFile << writeStatePush[i] << std::endl;
        }
    }

    //Swap double buffer arrays here
    if(doubleBuffer != ComputeIODoubleBufferType::NONE){
        cFile << "//Swap Double Buffer Ptrs" << std::endl;
    }
    std::vector<std::string> swapInputDoubleBufferPtrs = swapReadDoubleBufferPtrs(inputFIFOs, doubleBuffer);
    for (int i = 0; i < swapInputDoubleBufferPtrs.size(); i++) {
        cFile << swapInputDoubleBufferPtrs[i] << std::endl;
    }
    std::vector<std::string> swapOutputDoubleBufferPtrs = swapWriteDoubleBufferPtrs(outputFIFOs, doubleBuffer);
    for (int i = 0; i < swapOutputDoubleBufferPtrs.size(); i++) {
        cFile << swapOutputDoubleBufferPtrs[i] << std::endl;
    }

    if(collectTelem) {
        cFile << "if(!collectTelem){" << std::endl;
        cFile << "//Reset timer after processing first samples.  Removes startup time from telemetry" << std::endl;
        cFile << "rxSamples = 0;" << std::endl;
        cFile << "asm volatile (\"\" ::: \"memory\"); //Stop Re-ordering of timer" << std::endl;
        cFile << "clock_gettime(CLOCK_MONOTONIC, &startTime);" << std::endl;
        cFile << "asm volatile (\"\" ::: \"memory\"); //Stop Re-ordering of timer" << std::endl;
        if(collectPAPI && !collectPAPIComputeOnly) {
            cFile << "resetPapiCounters(papiEventSet); //Doing this after Timer fetch since, when collecting PAPI for entire thread execution, PAPI is reset after the timer is checked (to prevent calling PAPI too often)" << std::endl;
            cFile << "asm volatile (\"\" ::: \"memory\"); //Stop Re-ordering of PAPI reset" << std::endl;
        }
        cFile << "lastPrint = startTime;" << std::endl;
        if(collectBreakdownTelem) {
            cFile << "timeTotal = 0;" << std::endl;
            cFile << "timeWaitingForInputFIFOs = 0;" << std::endl;
            cFile << "timeReadingInputFIFOs = 0;" << std::endl;
            cFile << "timeWaitingForComputeToFinish = 0;" << std::endl;
            cFile << "timeWaitingForOutputFIFOs = 0;" << std::endl;
            cFile << "timeWritingOutputFIFOs = 0;" << std::endl;
        }
        if(collectPAPI) {
            cFile << "clock_cycles = 0;\n"
                     "instructions_retired = 0;\n"
                     "l1_data_cache_accesses = 0;\n"
                     "floating_point_operations_retired = 0;" << std::endl;
            if(collectPAPIComputeOnly){
                cFile << "timeWaitingForComputeToFinishPlusPAPI = 0;" << std::endl;
            }
        }
        cFile << "collectTelem = true;" << std::endl;
        cFile << "}" << std::endl;
    }

    //Close loop
    cFile << "}" << std::endl;

    if(collectPAPI) {
        cFile << "stopPapiCounters(papiEventSet);" << std::endl;
    }

    if(collectTelem && !telemDumpFilePrefix.empty()){
        cFile << "fclose(telemDumpFile);" << std::endl;
    }

    cFile << "return NULL;" << std::endl;

    //Close function
    cFile << "}" << std::endl;

    cFile.close();
}

std::string MultiThreadEmit::getPartitionComputeCFunctionArgPrototype(
        std::vector<std::shared_ptr<ThreadCrossingFIFO>> inputFIFOs,
        std::vector<std::shared_ptr<ThreadCrossingFIFO>> outputFIFOs,
        std::string stateTypeName, ComputeIODoubleBufferType doubleBuffer){
    std::string prototype = "";

    if(!stateTypeName.empty()){
        //The first argument will be a pointer to the partition state structure
        prototype += stateTypeName + " *" + VITIS_STATE_STRUCT_NAME;
    }

    std::vector<Variable> inputVars;
    std::vector<bool> inputIsScalar;
    for(int i = 0; i<inputFIFOs.size(); i++){
        for(int j = 0; j<inputFIFOs[i]->getInputPorts().size(); j++){
            Variable var = inputFIFOs[i]->getCStateVarExpandedForBlockSize(j);
            inputIsScalar.push_back(var.getDataType().isScalar());
            inputVars.push_back(var);
        }
    }

    //TODO: Assuming port numbers do not have a discontinuity.  Validate this assumption.
    for(unsigned long i = 0; i<inputVars.size(); i++){
        Variable var = inputVars[i];

        //Pass as not volatile
        var.setAtomicVar(false);

        if(i > 0 || !stateTypeName.empty()){
            prototype += ", ";
        }

        if(doubleBuffer == ComputeIODoubleBufferType::INPUT_AND_OUTPUT || doubleBuffer == ComputeIODoubleBufferType::INPUT){
            //Create vars for the sharedPtr (const)
            //                the currentBuffer (const)
            //            and the nextBuffer (not const)

            Variable sharedPtr = var;
            sharedPtr.setName(sharedPtr.getName()+"_shared");
            Variable currentPtr = var; //Since we are computing out of the "current" array, the name will be unchanged to avoid needing changes to the underlying FIFO node emit
            Variable nextPtr = var;
            nextPtr.setName(nextPtr.getName()+"_next");

            if(inputIsScalar[i]) { //force to be a ptr
                prototype += "const " + sharedPtr.getCPtrDecl(false);
            }else{
                prototype += "const " + sharedPtr.getCVarDecl(false, true, false, true);
            }
            prototype += ", const " + currentPtr.getCVarDecl(false, true, false, true); //It allowed to be a scalar
            if(inputIsScalar[i]) { //force to be a ptr
                prototype += ", " + nextPtr.getCPtrDecl(false);
            }else{
                prototype += ", " + nextPtr.getCVarDecl(false, true, false, true);
            }

            //Check if complex
            if(var.getDataType().isComplex()){
                if(inputIsScalar[i]) { //force to be a ptr
                    prototype += ", const " + sharedPtr.getCPtrDecl(true);
                }else {
                    prototype += ", const " + sharedPtr.getCVarDecl(true, true, false, true);
                }
                prototype += ", const " + currentPtr.getCVarDecl(true, true, false, true); //It allowd to be a scalar
                if(inputIsScalar[i]) { //force to be a ptr
                    prototype += ", " + nextPtr.getCPtrDecl(true);
                }else {
                    prototype += ", " + nextPtr.getCVarDecl(true, true, false, true);
                }
            }
        }else {
            prototype += "const " + var.getCVarDecl(false, true, false, true);

            //Check if complex
            if(var.getDataType().isComplex()){
                prototype += ", const " + var.getCVarDecl(true, true, false, true);
            }
        }
    }

    //Add outputs
    std::vector<Variable> outputVars;
    for(int i = 0; i<outputFIFOs.size(); i++){
        for(int j = 0; j<outputFIFOs[i]->getInputPorts().size(); j++){
            Variable var = outputFIFOs[i]->getCStateInputVarExpandedForBlockSize(j);
            outputVars.push_back(var);
        }
    }

    //TODO: Assuming port numbers do not have a discontinuity.  Validate this assumption.

    for(unsigned long i = 0; i<outputVars.size(); i++){
        Variable var = outputVars[i];

        //Pass as not volatile
        var.setAtomicVar(false);

        if(i > 0 || inputVars.size()>0 || !stateTypeName.empty()) {
            prototype += ", ";
        }

        if(doubleBuffer == ComputeIODoubleBufferType::INPUT_AND_OUTPUT || doubleBuffer == ComputeIODoubleBufferType::OUTPUT){
            //Create vars for the sharedPtr (not const)
            //                the currentBuffer (not const)
            //            and the prevBuffer (const)

            Variable sharedPtr = var;
            sharedPtr.setName(sharedPtr.getName()+"_shared");
            Variable currentPtr = var; //Since we are computing into the "current" array, the name will be unchanged to avoid needing changes to the underlying FIFO node emit
            Variable prevPtr = var;
            prevPtr.setName(prevPtr.getName()+"_prev");

            //The output variables are always pointers
            if(var.getDataType().isScalar()) {
                //Force a pointer for scalar values
                prototype += sharedPtr.getCPtrDecl(false);
                prototype += ", " + currentPtr.getCPtrDecl(false);
                prototype += ", const " + prevPtr.getCPtrDecl(false);
            }else{
                prototype += sharedPtr.getCVarDecl(false, true, false, true);
                prototype += ", " + currentPtr.getCVarDecl(false, true, false, true);
                prototype += ", const " + prevPtr.getCVarDecl(false, true, false, true);
            }

            //Check if complex
            if(var.getDataType().isComplex()){
                prototype += ", ";
                if(var.getDataType().isScalar()) {
                    prototype += sharedPtr.getCPtrDecl(true);
                    prototype += ", " + currentPtr.getCPtrDecl(true);
                    prototype += ", const " + prevPtr.getCPtrDecl(true);
                }else {
                    prototype += sharedPtr.getCVarDecl(true, true, false, true);
                    prototype += ", " + currentPtr.getCVarDecl(true, true, false, true);
                    prototype += ", const " + prevPtr.getCVarDecl(true, true, false, true);
                }
            }
        }else{
            //The output variables are always pointers
            if(var.getDataType().isScalar()) {
                //Force a pointer for scalar values
                prototype += var.getCPtrDecl(false);
            }else{
                prototype += var.getCVarDecl(false, true, false, true);
            }

            //Check if complex
            if(var.getDataType().isComplex()){
                prototype += ", ";
                if(var.getDataType().isScalar()) {
                    prototype += var.getCPtrDecl(true);
                }else {
                    prototype += var.getCVarDecl(true, true, false, true);
                }
            }
        }
    }

    return prototype;
}

std::string MultiThreadEmit::getCallPartitionComputeCFunction(std::string computeFctnName,
                                                                        std::vector<std::shared_ptr<ThreadCrossingFIFO>> inputFIFOs,
                                                                        std::vector<std::shared_ptr<ThreadCrossingFIFO>> outputFIFOs,
                                                                        bool fifoInPlace,
                                                                        std::string stateStructParam,
                                                                        ComputeIODoubleBufferType doubleBuffer,
                                                                        std::string inputFIFOSuffix, //"_readTmp"
                                                                        std::string outputFIFOSuffix, //"_writeTmp"
                                                                        std::string doubleBufferInputNextSuffix,
                                                                        std::string doubleBufferInputCurrentSuffix,
                                                                        std::string doubleBufferOutputCurrentSuffix,
                                                                        std::string doubleBufferOutputPrevSuffix){
    std::string call = computeFctnName + "(";

    //TODO: Update for double buffer

    if(!stateStructParam.empty()){
        call += stateStructParam;
    }

    bool foundInputVar = !stateStructParam.empty();
    //TODO: Assuming port numbers do not have a discontinuity.  Validate this assumption.
    for(unsigned long i = 0; i<inputFIFOs.size(); i++) {
        for (unsigned long portNum = 0; portNum < inputFIFOs[i]->getInputPorts().size(); portNum++) {
            Variable var = inputFIFOs[i]->getCStateVarExpandedForBlockSize(portNum);
            std::string tmpName = inputFIFOs[i]->getName() + inputFIFOSuffix;

            if (foundInputVar) {
                call += ", ";
            }
            call += tmpName + (fifoInPlace ? "->" : ".") + "port" + GeneralHelper::to_string(portNum) + "_real";

            //if using double buffer, the order of operands is, shared (readTmp), current, next, shared (imag), current (imag), next (imag)
            //Note that scalar next operand need to be passed as ptr so a '&' may be required
            //The recepy for the names are fifo_name + suffix
            std::string currentName = inputFIFOs[i]->getName() + doubleBufferInputCurrentSuffix;
            std::string nextName = inputFIFOs[i]->getName() + doubleBufferInputNextSuffix;

            if(doubleBuffer == ComputeIODoubleBufferType::INPUT_AND_OUTPUT || doubleBuffer == ComputeIODoubleBufferType::INPUT){
                call += ", " + currentName + "->port" + GeneralHelper::to_string(portNum) + "_real"; //Allowed to be a scaler
                call += ", " + (var.getDataType().isScalar() ? std::string("&") : std::string("")) + nextName + "->port" + GeneralHelper::to_string(portNum) + "_real";
            }

            foundInputVar = true;

            //Check if complex
            if (var.getDataType().isComplex()) {
                call += ", " + tmpName + (fifoInPlace ? "->" : ".") + "port" + GeneralHelper::to_string(portNum) + "_imag";

                if(doubleBuffer == ComputeIODoubleBufferType::INPUT_AND_OUTPUT || doubleBuffer == ComputeIODoubleBufferType::INPUT){
                    call += ", " + currentName + "->port" + GeneralHelper::to_string(portNum) + "_imag"; //Allowed to be a scaler
                    call += ", " + (var.getDataType().isScalar() ? std::string("&") : std::string("")) + nextName + "->port" + GeneralHelper::to_string(portNum) + "_imag";
                }
            }
        }
    }

    //Add output
    if(foundInputVar){
        call += ", ";
    }

    bool foundOutputVar = false;

    //TODO: Assuming port numbers do not have a discontinuity.  Validate this assumption.

    for(unsigned long i = 0; i<outputFIFOs.size(); i++) {
        for (unsigned long portNum = 0; portNum < outputFIFOs[i]->getInputPorts().size(); portNum++) {
            Variable var = outputFIFOs[i]->getCStateVarExpandedForBlockSize(portNum);

            DataType varDatatypeExpanded = var.getDataType(); //Should already be expanded

            std::string tmpName = "";
            if (varDatatypeExpanded.numberOfElements() == 1) {
                tmpName += "&";
            }
            tmpName += outputFIFOs[i]->getName() + outputFIFOSuffix;

            if (foundOutputVar) {
                call += ", ";
            }

            foundOutputVar = true;

            call += tmpName + (fifoInPlace ? "->" : ".") + "port" + GeneralHelper::to_string(portNum) + "_real";

            //if using double buffer, the order of operands is, shared (readTmp), current, prev, shared (imag), current (imag), prev (imag)
            //Note that all of these values need to be ptrs
            std::string currentName = outputFIFOs[i]->getName() + doubleBufferOutputCurrentSuffix;
            std::string prevName = outputFIFOs[i]->getName() + doubleBufferOutputPrevSuffix;

            if(doubleBuffer == ComputeIODoubleBufferType::INPUT_AND_OUTPUT || doubleBuffer == ComputeIODoubleBufferType::OUTPUT){
                call += ", " + (var.getDataType().isScalar() ? std::string("&") : std::string("")) + currentName + "->port" + GeneralHelper::to_string(portNum) + "_real";
                call += ", " + (var.getDataType().isScalar() ? std::string("&") : std::string("")) + prevName    + "->port" + GeneralHelper::to_string(portNum) + "_real";
            }

            //Check if complex
            if (var.getDataType().isComplex()) {
                call += ", " + tmpName + (fifoInPlace ? "->" : ".") + "port" + GeneralHelper::to_string(portNum) + "_imag";

                if(doubleBuffer == ComputeIODoubleBufferType::INPUT_AND_OUTPUT || doubleBuffer == ComputeIODoubleBufferType::OUTPUT){
                    call += ", " + (var.getDataType().isScalar() ? std::string("&") : std::string("")) + currentName + "->port" + GeneralHelper::to_string(portNum) + "_imag";
                    call += ", " + (var.getDataType().isScalar() ? std::string("&") : std::string("")) + prevName    + "->port" + GeneralHelper::to_string(portNum) + "_imag";
                }
            }
        }
    }

    call += ");\n";

    return call;
}
std::vector<std::string> MultiThreadEmit::computeIODoubleBufferEmitHelper(
                                                         std::vector<std::shared_ptr<ThreadCrossingFIFO>> fifos,
                                                         std::pair<int, int> filterRate,
                                                         std::string counterVarName,
                                                         std::string indexVarName,
                                                         ComputeIODoubleBufferType doubleBuffer,
                                                         bool &openedCounterCheck,
                                                         bool input){
    std::vector<std::string> statements;

    bool doubleBufferEmit;
    if(input){
        doubleBufferEmit = doubleBuffer == ComputeIODoubleBufferType::INPUT_AND_OUTPUT || doubleBuffer == ComputeIODoubleBufferType::INPUT;
    }else{
        doubleBufferEmit = doubleBuffer == ComputeIODoubleBufferType::INPUT_AND_OUTPUT || doubleBuffer == ComputeIODoubleBufferType::OUTPUT;
    }

    if(doubleBufferEmit){
        //Scan through input FIFOs
        for(auto fifo : fifos){
            std::vector<std::shared_ptr<ClockDomain>> clkDomains = fifo->getClockDomains();
            //Scan through the different ports
            for(int i = 0; i<clkDomains.size(); i++){
                std::pair<int, int> rate = std::pair<int, int>(1, 1); //Handles the case when no clock domain is returned
                if(clkDomains[i]){
                    rate = clkDomains[i]->getRateRelativeToBase();
                }

                if(rate == filterRate){
                    //Copy from shared to next
                    bool isScalar = fifo->getCStateVarExpandedForBlockSize(i).getDataType().isScalar();
                    Variable var;
                    if(input) {
                        var = fifo->getCStateVarExpandedForBlockSize(i);
                    }else{
                        var = fifo->getCStateInputVarExpandedForBlockSize(i);
                    }
                    Variable fromBuffer = var;
                    Variable toBuffer = var;

                    if(input) {
                        fromBuffer.setName(fromBuffer.getName() + "_shared");
                        toBuffer.setName(toBuffer.getName() + "_next");
                    }else{
                        fromBuffer.setName(fromBuffer.getName() + "_prev");
                        toBuffer.setName(toBuffer.getName() + "_shared");
                    }

                    if(filterRate.second != 1 && !openedCounterCheck){
                        //This is either a downsample or rational resample (with downsample component) domain, the counter needs to be checked
                        //before performing the copy
                        statements.push_back("if(" + counterVarName + " == 0){");
                        openedCounterCheck = true;
                    }

                    if(filterRate.first != 1){
                        //This is an upsample or rational resample (with upsample component) domain
                        //Will need to copy multiple elements in a for loop
                        //On top of that, each element may be a vecotr/matrix requiring a for loop

                        //The block dimension should be the first so that will be what needs to be offset by the given
                        //index and have the dimension set to the amount to copy

                        //In this case, the block size needs to be >1
                        //TODO: remove sanity check
                        if(fifo->getBlockSizeCreateIfNot(i) <= 1){
                            throw std::runtime_error(ErrorHelpers::genErrorStr("Clock domains with an upsample component must have a block size > 1"));
                        }

                        int elementsToCopy = filterRate.first;

                        //Adjust the first dimension (block dimension) of the dimensions expanded for the block size
                        std::vector<int> dimensions = var.getDataType().getDimensions();
                        dimensions[0]=elementsToCopy;

                        //Create nested loops
                        std::tuple<std::vector<std::string>, std::vector<std::string>, std::vector<std::string>> forLoopStrs =
                                EmitterHelpers::generateVectorMatrixForLoops(dimensions);

                        std::vector<std::string> forLoopOpen = std::get<0>(forLoopStrs);
                        std::vector<std::string> forLoopIndexVars = std::get<1>(forLoopStrs);
                        std::vector<std::string> forLoopClose = std::get<2>(forLoopStrs);

                        //Emit the for loop opening
                        statements.insert(statements.end(), forLoopOpen.begin(), forLoopOpen.end());

                        //Emit the copy
                        //Add the offset to the first for loop index
                        forLoopIndexVars[0] = indexVarName + "+" + forLoopIndexVars[0];
                        statements.push_back(toBuffer.getCVarName(false) +
                                             EmitterHelpers::generateIndexOperation(forLoopIndexVars) + " = " +
                                             fromBuffer.getCVarName(false) +
                                             EmitterHelpers::generateIndexOperation(forLoopIndexVars) + ";");

                        if(var.getDataType().isComplex()){
                            statements.push_back(toBuffer.getCVarName(true) +
                                                 EmitterHelpers::generateIndexOperation(forLoopIndexVars) + " = " +
                                                 fromBuffer.getCVarName(true) +
                                                 EmitterHelpers::generateIndexOperation(forLoopIndexVars) + ";");
                        }

                        //Emit the for loop closing
                        statements.insert(statements.end(), forLoopClose.begin(), forLoopClose.end());
                    }else {
                        //In this case, there is not an upsample component to this domain and only one element is copied
                        //at a time

                        //If the element is a scalar, it is a straight copy of the single element with no for loop required

                        //Note, the shared buffer and next buffer are guaranteed to be arrays or ptrs (regardless if these are inputs or outputs)
                        if(isScalar){
                            //If the block size is 1, this is a simple copy between dereferenced ptrs
                            if(fifo->getBlockSizeCreateIfNot(i) == 1) {
                                //If the block size is >1, this is a indexed operation
                                statements.push_back("*" + toBuffer.getCVarName(false) + " = " +
                                                     "*" + fromBuffer.getCVarName(false) + ";");

                                if(var.getDataType().isComplex()){
                                    statements.push_back("*" + toBuffer.getCVarName(true) + " = " +
                                                         "*" + fromBuffer.getCVarName(true) + ";");
                                }
                            }else{
                                std::vector<std::string> indExpr = {indexVarName};
                                statements.push_back(toBuffer.getCVarName(false) +
                                                     EmitterHelpers::generateIndexOperation(indExpr) + " = " +
                                                     fromBuffer.getCVarName(false) +
                                                     EmitterHelpers::generateIndexOperation(indExpr) + ";");
                                if(var.getDataType().isComplex()){
                                    statements.push_back(toBuffer.getCVarName(true) +
                                                         EmitterHelpers::generateIndexOperation(indExpr) + " = " +
                                                         fromBuffer.getCVarName(true) +
                                                         EmitterHelpers::generateIndexOperation(indExpr) + ";");
                                }
                            }
                        }else{
                            //Otherwise, we need a for loop to copy the single element
                            std::tuple<std::vector<std::string>, std::vector<std::string>, std::vector<std::string>> forLoopStrs =
                                    EmitterHelpers::generateVectorMatrixForLoops(var.getDataType().getDimensions());

                            std::vector<std::string> forLoopOpen = std::get<0>(forLoopStrs);
                            std::vector<std::string> forLoopIndexVars = std::get<1>(forLoopStrs);
                            std::vector<std::string> forLoopClose = std::get<2>(forLoopStrs);

                            //Emit the for loop opening
                            statements.insert(statements.end(), forLoopOpen.begin(), forLoopOpen.end());

                            //If the block size is 1, we do not modify the 1st dimension expression with the provided index
                            //    The dimension will not have been expanded
                            //If the block size is >1, the dimension will have been expanded and the first indexing op needs
                            //to be augmented with the index var
                            if(fifo->getBlockSizeCreateIfNot(i) > 1){
                                forLoopIndexVars[0] = indexVarName + "+" + forLoopIndexVars[0];
                            }

                            //Emit the copy
                            statements.push_back(toBuffer.getCVarName(false) +
                                                 EmitterHelpers::generateIndexOperation(forLoopIndexVars) + " = " +
                                                 fromBuffer.getCVarName(false) +
                                                 EmitterHelpers::generateIndexOperation(forLoopIndexVars) + ";");
                            if(var.getDataType().isComplex()){
                                statements.push_back(toBuffer.getCVarName(true) +
                                                     EmitterHelpers::generateIndexOperation(forLoopIndexVars) + " = " +
                                                     fromBuffer.getCVarName(true) +
                                                     EmitterHelpers::generateIndexOperation(forLoopIndexVars) + ";");
                            }

                            //Emit the for loop closing
                            statements.insert(statements.end(), forLoopClose.begin(), forLoopClose.end());
                        }
                    }

                    //This is now handled outside since input and output FIFOs may both be in the same clock domain.
                    //Avoid needing to emit a redundant if statement.
//                    if(filterRate.second != 1){
//                        //close the if
//                        statements.push_back("}");
//                    }
                }
            }
        }
    }

    return statements;
}

std::vector<std::string> MultiThreadEmit::computeIODoubleBufferEmit(
        std::vector<std::shared_ptr<ThreadCrossingFIFO>> inputFIFOs,
        std::vector<std::shared_ptr<ThreadCrossingFIFO>> outputFIFOs,
        std::pair<int, int> filterRate,
        std::string counterVarName,
        std::string indexVarName,
        ComputeIODoubleBufferType doubleBuffer){

    std::vector<std::string> statements;

    if(doubleBuffer != ComputeIODoubleBufferType::NONE){
        statements.push_back("");
        statements.push_back("//Handle Double Buffer Copies");
    }

    bool openedCounterCheck = false;
    //---- Input FIFOs ----
    std::vector<std::string> inputFIFOStatements = computeIODoubleBufferEmitHelper(inputFIFOs, filterRate,
                                                                                   counterVarName, indexVarName,
                                                                                   doubleBuffer, openedCounterCheck,
                                                                                   true);
    statements.insert(statements.end(), inputFIFOStatements.begin(), inputFIFOStatements.end());

    //---- Output FIFOs ----
    std::vector<std::string> outputFIFOStatements = computeIODoubleBufferEmitHelper(outputFIFOs, filterRate,
                                                                                   counterVarName, indexVarName,
                                                                                   doubleBuffer, openedCounterCheck,
                                                                                   false);
    statements.insert(statements.end(), outputFIFOStatements.begin(), outputFIFOStatements.end());

    //---- Close Counter Check (if applicable) ----
    if(openedCounterCheck){
        statements.push_back("}");
    }

    return statements;
}

//NOTE: if scheduling the output master is desired, it must be included in the nodes to emit
void MultiThreadEmit::emitSelectOpsSchedStateUpdateContext(std::ofstream &cFile, std::vector<std::shared_ptr<Node>> &nodesToEmit, SchedParams::SchedType schedType, std::shared_ptr<MasterOutput> outputMaster){

    cFile << std::endl << "//==== Compute Operators ====" << std::endl;

    //Sort nodes by schedOrder.
    std::vector<std::shared_ptr<Node>> orderedNodes = nodesToEmit;
    std::sort(orderedNodes.begin(), orderedNodes.end(), Node::lessThanSchedOrder);

    std::shared_ptr<Node> zeroSchedNodeCmp = NodeFactory::createNode<MasterUnconnected>(); //Need a node to compare to
    zeroSchedNodeCmp->setSchedOrder(0);

    auto schedIt = std::lower_bound(orderedNodes.begin(), orderedNodes.end(), zeroSchedNodeCmp, Node::lessThanSchedOrder); //Binary search for the first node to be emitted (schedOrder 0 or greater)

    std::vector<std::shared_ptr<Node>> toBeEmittedInThisOrder;
    std::copy(schedIt, orderedNodes.end(), std::back_inserter(toBeEmittedInThisOrder));

    EmitterHelpers::emitOpsStateUpdateContext(cFile, schedType, toBeEmittedInThisOrder, outputMaster);
}

bool MultiThreadEmit::checkNoNodesInIO(std::vector<std::shared_ptr<Node>> nodes) {
    for(int i = 0; i<nodes.size(); i++){
        if(nodes[i]->getPartitionNum() == IO_PARTITION_NUM){
            bool isThreadCrossingFIFO = GeneralHelper::isType<Node, ThreadCrossingFIFO>(nodes[i]) != nullptr;
            bool isAllowedSubSystem = GeneralHelper::isType<Node, SubSystem>(nodes[i]) != nullptr && GeneralHelper::isType<Node, ContextFamilyContainer>(nodes[i]) == nullptr && GeneralHelper::isType<Node, ContextContainer>(nodes[i]) == nullptr;

            if(!(isThreadCrossingFIFO || isAllowedSubSystem)){
                return false;
            }
        }
    }

    return true;
}

void MultiThreadEmit::writeTelemConfigJSONFile(std::string path, std::string telemDumpPrefix,
                                                         std::string designName, std::map<int, int> partitionToCPU,
                                                         int ioPartitionNumber, std::string graphmlSchedFile) {
    std::string fileName = telemDumpPrefix + "telemConfig";\
    std::cout << "Emitting JSON File: " << path << "/" << fileName << ".json" << std::endl;

    std::ofstream configFile;
    configFile.open(path+"/"+fileName+".json", std::ofstream::out | std::ofstream::trunc);
    configFile << "{" << std::endl;
    configFile << "\t\"name\": \"" << designName << "\"," << std::endl;
    configFile << "\t\"ioTelemFiles\": {" << std::endl;
    configFile << "\t\t\""<< ioPartitionNumber << "\": \"" << telemDumpPrefix + "IO.csv" << "\"" << std::endl;
    configFile << "\t}," << std::endl;
    configFile << "\t\"computeTelemFiles\": {" << std::endl;

    bool foundCompute = false;
    for(auto it = partitionToCPU.begin(); it != partitionToCPU.end(); it++){
        if(it->first != ioPartitionNumber){
            if(foundCompute){
                configFile << "," << std::endl;
            }else{
                foundCompute = true;
            }

            std::string telemFile = telemDumpPrefix + GeneralHelper::to_string(it->first) + ".csv";
            configFile << "\t\t\"" << it->first << "\": \"" << telemFile << "\"";
        }
    }
    configFile << std::endl;
    configFile << "\t}," << std::endl;

    //Include
    configFile << "\t\"partitionToCPU\": {" << std::endl;
    for(auto it = partitionToCPU.begin(); it != partitionToCPU.end(); it++){
        if(it != partitionToCPU.begin()){
            configFile << "," << std::endl;
        }
        configFile << "\t\t\"" << it->first << "\": \"" << it->second << "\"";
    }
    configFile << std::endl;
    configFile << "\t}," << std::endl;
    configFile << "\t\"computeTimeMetricName\": \"WaitingForComputeToFinish_s\"," << std::endl;
    configFile << "\t\"totalTimeMetricName\": \"TotalTime_s\"," << std::endl;
    configFile << "\t\"timestampSecName\": \"TimeStamp_s\"," << std::endl;
    configFile << "\t\"timestampNSecName\": \"TimeStamp_ns\"," << std::endl;
    configFile << "\t\"rateMSPSName\": \"Rate_msps\"," << std::endl;
    configFile << "\t\"waitingForInputFIFOsMetricName\": \"WaitingForInputFIFOs_s\"," << std::endl;
    configFile << "\t\"readingInputFIFOsMetricName\": \"ReadingInputFIFOs_s\"," << std::endl;
    configFile << "\t\"waitingForOutputFIFOsMetricName\": \"WaitingForOutputFIFOs_s\"," << std::endl;
    configFile << "\t\"writingOutputFIFOsMetricName\": \"WritingOutputFIFOs_s\"," << std::endl;
    configFile << "\t\"telemetryMiscMetricName\": \"Telemetry_Misc_s\"," << std::endl;

    configFile << "\t\"schedGraphMLFile\": \"" << graphmlSchedFile << "\"" << std::endl;

    configFile << "}" << std::endl;

    configFile.close();
}

void MultiThreadEmit::writePlatformParameters(std::string path, std::string filename, int memAlignment){
    std::cout << "Emitting C File: " << path << "/" << filename << ".h" << std::endl;

    std::ofstream headerFile;
    headerFile.open(path+"/"+filename+".h", std::ofstream::out | std::ofstream::trunc);

    std::string fileNameUpper =  GeneralHelper::toUpper(filename);
    headerFile << "#ifndef " << fileNameUpper << "_H" << std::endl;
    headerFile << "#define " << fileNameUpper << "_H" << std::endl;
    headerFile << "#define VITIS_MEM_ALIGNMENT (" << memAlignment << ")" << std::endl;
    headerFile << "#endif" << std::endl;

    headerFile.close();
}

void MultiThreadEmit::writeNUMAAllocHelperFiles(std::string path, std::string filename){
    std::cout << "Emitting C File: " << path << "/" << filename << ".h" << std::endl;

    std::ofstream headerFile;
    headerFile.open(path+"/"+filename+".h", std::ofstream::out | std::ofstream::trunc);

    std::string fileNameUpper =  GeneralHelper::toUpper(filename);
    headerFile << "#ifndef " << fileNameUpper << "_H" << std::endl;
    headerFile << "#define " << fileNameUpper << "_H" << std::endl;
    headerFile << "#include <stdint.h>" << std::endl;
    headerFile << "#include <stdlib.h>" << std::endl;
    headerFile << "#include <stdbool.h>" << std::endl;
    headerFile << "#include <string.h>" << std::endl;
    headerFile << "#include \"" << VITIS_TYPE_NAME << ".h\"" << std::endl;

    headerFile << "void* vitis_malloc_core(size_t size, int core);" << std::endl;
    headerFile << "void* vitis__mm_malloc_core(size_t size, size_t alignment, int core);" << std::endl;
    headerFile << "void* vitis_aligned_alloc_core(size_t alignment, size_t size, int core);" << std::endl;
    headerFile << "void* vitis_aligned_alloc(size_t alignment, size_t size);" << std::endl;

    headerFile << "#endif" << std::endl;
    headerFile.close();

    std::ofstream cFile;
    cFile.open(path+"/"+filename+".c", std::ofstream::out | std::ofstream::trunc);
    cFile << "#ifndef _GNU_SOURCE" << std::endl;
    cFile << "#define _GNU_SOURCE" << std::endl;
    cFile << "#endif" << std::endl;
    cFile << "#include \"" << filename << ".h\"" << std::endl;
    cFile << "#include <mm_malloc.h>" << std::endl;
    cFile << "#include <stdio.h>" << std::endl;
    cFile << "#include <sched.h>" << std::endl;
    cFile << "#include <unistd.h>" << std::endl;
    cFile << "#include <pthread.h>" << std::endl;

    cFile << "typedef struct{\n"
             "    size_t size;\n"
             "    size_t alignment;\n"
             "} vitis_aligned_malloc_args_t;" << std::endl;
    cFile << std::endl;

    cFile << "#if __APPLE__\n"
             "//Apple does not support the type of core affinity assignment we use\n"
             "void* vitis_malloc_core(size_t size, int core){\n"
             "    printf(\"Warning, cannot allocate on specific core on Mac\");\n"
             "    return malloc(size);\n"
             "}\n"
             "\n"
             "void* vitis__mm_malloc_core(size_t size, size_t alignment, int core){\n"
             "    printf(\"Warning, cannot allocate on specific core on Mac\");\n"
             "    return _mm_malloc(size, alignment);\n"
             "}\n"
             "\n"
             "void* vitis_aligned_alloc_core(size_t alignment, size_t size, int core){\n"
             "    printf(\"Warning, cannot allocate on specific core on Mac\");\n"
             "    return vitis_aligned_alloc(alignment, size);"
             "}\n"
             "\n"
             "void* vitis_aligned_alloc(size_t alignment, size_t size){\n"
             "    printf(\"Warning, Mac does not support aligned_alloc, using posix_memalign instead)\");\n"
             "\n"
             "    size_t allocSize = size + (size%alignment == 0 ? 0 : alignment-(size%alignment));\n"
             "\n"
             "    void* ptr;\n"
             "    int status = posix_memalign(&ptr, alignment, allocSize);\n"
             "\n"
             "    if(status != 0){\n"
             "        ptr = NULL;\n"
             "    }\n"
             "\n"
             "    return ptr;\n"
             "}\n"
             "\n"
             "#else\n"
             "    //Worker Threads\n"
             "    void* vitis_malloc_core_thread(void* arg_uncast){\n"
             "        size_t* size = (size_t*) arg_uncast;\n"
             "\n"
             "        void* rtnVal = malloc(*size);\n"
             "        return rtnVal;\n"
             "    }\n"
             "\n"
             "    void* vitis__mm_malloc_core_thread(void* arg_uncast){\n"
             "        vitis_aligned_malloc_args_t* arg = (vitis_aligned_malloc_args_t*) arg_uncast;\n"
             "        size_t size = arg->size;\n"
             "        size_t alignment = arg->alignment;\n"
             "\n"
             "        void* rtnVal = _mm_malloc(size, alignment);\n"
             "        return rtnVal;\n"
             "    }\n"
             "\n"
             "    void* vitis_aligned_alloc_core_thread(void* arg_uncast){\n"
             "        vitis_aligned_malloc_args_t* arg = (vitis_aligned_malloc_args_t*) arg_uncast;\n"
             "        size_t size = arg->size;\n"
             "        size_t alignment = arg->alignment;\n"
             "\n"
             "        size_t allocSize = size + (size%alignment == 0 ? 0 : alignment-(size%alignment));\n"
             "\n"
             "        //There is a condition on aligned_alloc that the size must be a\n"
             "        //multiple of the alignment\n"
             "        void* rtnVal = aligned_alloc(alignment, allocSize);\n"
             "        return rtnVal;\n"
             "    }\n"
             "\n"
             "    void* vitis_malloc_core(size_t size, int core){\n"
             "        cpu_set_t cpuset;\n"
             "        pthread_t thread;\n"
             "        pthread_attr_t attr;\n"
             "        void *res;\n"
             "\n"
             "        int status;\n"
             "\n"
             "        //Create pthread attributes\n"
             "        status = pthread_attr_init(&attr);\n"
             "        if(status != 0)\n"
             "        {\n"
             "            printf(\"Could not create pthread attributes for malloc_core ... exiting\\n\");\n"
             "            exit(1);\n"
             "        }\n"
             "\n"
             "        //Set CPU affinity\n"
             "        CPU_ZERO(&cpuset);\n"
             "        CPU_SET(core, &cpuset);\n"
             "        status = pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpuset);\n"
             "        if(status != 0)\n"
             "        {\n"
             "            printf(\"Could not set thread core affinity for malloc_core ... exiting\\n\");\n"
             "            exit(1);\n"
             "        }\n"
             "\n"
             "        // - Start Thread\n"
             "        size_t* newSize = malloc(sizeof(size_t));\n"
             "        *newSize = size;\n"
             "\n"
             "        status = pthread_create(&thread, &attr, vitis_malloc_core_thread, newSize);\n"
             "        if(status != 0)\n"
             "        {\n"
             "            printf(\"Could not create a thread for malloc_core ... exiting\\n\");\n"
             "            perror(NULL);\n"
             "            exit(1);\n"
             "        }\n"
             "\n"
             "        //Wait for thread to finish\n"
             "        status = pthread_join(thread, &res);\n"
             "        if(status != 0)\n"
             "        {\n"
             "            printf(\"Could not join a thread for malloc_core ... exiting\\n\");\n"
             "            perror(NULL);\n"
             "            exit(1);\n"
             "        }\n"
             "\n"
             "        free(newSize);\n"
             "\n"
             "        return res;\n"
             "    }\n"
             "\n"
             "    void* vitis__mm_malloc_core(size_t size, size_t alignment, int core){\n"
             "        cpu_set_t cpuset;\n"
             "        pthread_t thread;\n"
             "        pthread_attr_t attr;\n"
             "        void *res;\n"
             "\n"
             "        int status;\n"
             "\n"
             "        //Create pthread attributes\n"
             "        status = pthread_attr_init(&attr);\n"
             "        if(status != 0)\n"
             "        {\n"
             "            printf(\"Could not create pthread attributes for malloc_core ... exiting\\n\");\n"
             "            exit(1);\n"
             "        }\n"
             "\n"
             "        //Set CPU affinity\n"
             "        CPU_ZERO(&cpuset);\n"
             "        CPU_SET(core, &cpuset);\n"
             "        status = pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpuset);\n"
             "        if(status != 0)\n"
             "        {\n"
             "            printf(\"Could not set thread core affinity for malloc_core ... exiting\\n\");\n"
             "            exit(1);\n"
             "        }\n"
             "\n"
             "        // - Start Thread\n"
             "        vitis_aligned_malloc_args_t* args = malloc(sizeof(vitis_aligned_malloc_args_t));\n"
             "        args->size = size;\n"
             "        args->alignment = alignment;\n"
             "\n"
             "        status = pthread_create(&thread, &attr, vitis__mm_malloc_core_thread, args);\n"
             "        if(status != 0)\n"
             "        {\n"
             "            printf(\"Could not create a thread for malloc_core ... exiting\\n\");\n"
             "            perror(NULL);\n"
             "            exit(1);\n"
             "        }\n"
             "\n"
             "        //Wait for thread to finish\n"
             "        status = pthread_join(thread, &res);\n"
             "        if(status != 0)\n"
             "        {\n"
             "            printf(\"Could not join a thread for malloc_core ... exiting\\n\");\n"
             "            perror(NULL);\n"
             "            exit(1);\n"
             "        }\n"
             "\n"
             "        free(args);\n"
             "\n"
             "        return res;\n"
             "    }\n"
             "\n"
             "    void* vitis_aligned_alloc_core(size_t alignment, size_t size, int core){\n"
             "        cpu_set_t cpuset;\n"
             "        pthread_t thread;\n"
             "        pthread_attr_t attr;\n"
             "        void *res;\n"
             "\n"
             "        int status;\n"
             "\n"
             "        //Create pthread attributes\n"
             "        status = pthread_attr_init(&attr);\n"
             "        if(status != 0)\n"
             "        {\n"
             "            printf(\"Could not create pthread attributes for aligned_alloc_core ... exiting\\n\");\n"
             "            exit(1);\n"
             "        }\n"
             "\n"
             "        //Set CPU affinity\n"
             "        CPU_ZERO(&cpuset);\n"
             "        CPU_SET(core, &cpuset);\n"
             "        status = pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpuset);\n"
             "        if(status != 0)\n"
             "        {\n"
             "            printf(\"Could not set thread core affinity for aligned_alloc_core ... exiting\\n\");\n"
             "            exit(1);\n"
             "        }\n"
             "\n"
             "        // - Start Thread\n"
             "        vitis_aligned_malloc_args_t* args = malloc(sizeof(vitis_aligned_malloc_args_t));\n"
             "        args->size = size;\n"
             "        args->alignment = alignment;\n"
             "\n"
             "        status = pthread_create(&thread, &attr, vitis_aligned_alloc_core_thread, args);\n"
             "        if(status != 0)\n"
             "        {\n"
             "            printf(\"Could not create a thread for aligned_alloc_core ... exiting\\n\");\n"
             "            perror(NULL);\n"
             "            exit(1);\n"
             "        }\n"
             "\n"
             "        //Wait for thread to finish\n"
             "        status = pthread_join(thread, &res);\n"
             "        if(status != 0)\n"
             "        {\n"
             "            printf(\"Could not join a thread for aligned_alloc_core ... exiting\\n\");\n"
             "            perror(NULL);\n"
             "            exit(1);\n"
             "        }\n"
             "\n"
             "        free(args);\n"
             "\n"
             "        return res;\n"
             "    }\n"
             "\n"
             "void* vitis_aligned_alloc(size_t alignment, size_t size){\n"
             "        size_t allocSize = size + (size%alignment == 0 ? 0 : alignment-(size%alignment));\n"
             "\n"
             "        //There is a condition on aligned_alloc that the size must be a\n"
             "        //multiple of the alignment\n"
             "        void* rtnVal = aligned_alloc(alignment, allocSize);\n"
             "        return rtnVal;\n"
             "}\n"
             "#endif" << std::endl;

    cFile.close();
}

std::string MultiThreadEmit::getClkDomainIndVarName(std::pair<int, int> clkDomainRate, bool counter) {
    std::string indVarName = BLOCK_IND_VAR_PREFIX;

    if(clkDomainRate != std::pair<int, int>(1, 1)){
        indVarName += "_N" + GeneralHelper::to_string(clkDomainRate.first) + "_D" + GeneralHelper::to_string(clkDomainRate.second);
    }

    if(counter){
        indVarName += "_C";
    }

    return indVarName;
}

std::string MultiThreadEmit::getClkDomainIndVarName(std::shared_ptr<ClockDomain> clkDomain, bool counter) {
    std::pair<int, int> rate = std::pair<int, int>(1, 1);

    if(clkDomain){
        rate = clkDomain->getRateRelativeToBase();
    }

    return getClkDomainIndVarName(rate, counter);
}

std::string MultiThreadEmit::computeIODoubleBufferTypeToString(ComputeIODoubleBufferType computeIODoubleBufferType){
    switch(computeIODoubleBufferType){
        case ComputeIODoubleBufferType::NONE:
            return "NONE";
        case ComputeIODoubleBufferType::INPUT_AND_OUTPUT:
            return "INPUT_AND_OUTPUT";
        case ComputeIODoubleBufferType::INPUT:
            return "INPUT";
        case ComputeIODoubleBufferType::OUTPUT:
            return "OUTPUT";
        default:
            throw std::runtime_error(ErrorHelpers::genErrorStr("Unknown MultiThreadEmit"));
    }
}

MultiThreadEmit::ComputeIODoubleBufferType MultiThreadEmit::parseComputeIODoubleBufferType(std::string computeIODoubleBufferType){
    if(computeIODoubleBufferType == "NONE" || computeIODoubleBufferType == "none"){
        return ComputeIODoubleBufferType::NONE;
    }else if(computeIODoubleBufferType == "INPUT_AND_OUTPUT" || computeIODoubleBufferType == "input_and_output"){
        return ComputeIODoubleBufferType::INPUT_AND_OUTPUT;
    }else if(computeIODoubleBufferType == "INPUT" || computeIODoubleBufferType == "input"){
        return ComputeIODoubleBufferType::INPUT;
    }else if(computeIODoubleBufferType == "OUTPUT" || computeIODoubleBufferType == "output"){
        return ComputeIODoubleBufferType::OUTPUT;
    }else{
        throw std::runtime_error(ErrorHelpers::genErrorStr("Unknown ComputeIODoubleBufferType: " + computeIODoubleBufferType));
    }
}