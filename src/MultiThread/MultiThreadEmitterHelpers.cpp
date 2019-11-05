//
// Created by Christopher Yarp on 9/3/19.
//

#include "MultiThreadEmitterHelpers.h"
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

void MultiThreadEmitterHelpers::findPartitionInputAndOutputFIFOs(
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

std::string MultiThreadEmitterHelpers::emitCopyCThreadArgs(std::vector<std::shared_ptr<ThreadCrossingFIFO>> inputFIFOs, std::vector<std::shared_ptr<ThreadCrossingFIFO>> outputFIFOs, std::string structName, std::string structTypeName){
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

std::string MultiThreadEmitterHelpers::emitFIFOChecks(std::vector<std::shared_ptr<ThreadCrossingFIFO>> fifos, bool checkFull, std::string checkVarName, bool shortCircuit, bool blocking, bool includeThreadCancelCheck){
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
//std::string MultiThreadEmitterHelpers::emitFIFOChecks(std::vector<std::shared_ptr<ThreadCrossingFIFO>> fifos, int partition, bool checkFull, std::string checkVarName, bool shortCircuit){
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

std::vector<std::string> MultiThreadEmitterHelpers::createFIFOReadTemps(std::vector<std::shared_ptr<ThreadCrossingFIFO>> fifos){
    std::vector<std::string> exprs;
    for(int i = 0; i<fifos.size(); i++) {
        std::string tmpName = fifos[i]->getName() + "_readTmp";
        std::string structType = fifos[i]->getFIFOStructTypeName();
        exprs.push_back(structType + " " + tmpName + ";");
    }

    return exprs;
}

std::vector<std::string> MultiThreadEmitterHelpers::createFIFOWriteTemps(std::vector<std::shared_ptr<ThreadCrossingFIFO>> fifos){
    std::vector<std::string> exprs;
    for(int i = 0; i<fifos.size(); i++) {
        std::string tmpName = fifos[i]->getName() + "_writeTmp";
        std::string structType = fifos[i]->getFIFOStructTypeName();
        exprs.push_back(structType + " " + tmpName + ";");
    }

    return exprs;
}

std::vector<std::string> MultiThreadEmitterHelpers::createAndInitializeFIFOWriteTemps(std::vector<std::shared_ptr<ThreadCrossingFIFO>> fifos, std::vector<std::vector<NumericValue>> defaultVal){
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

std::vector<std::string> MultiThreadEmitterHelpers::readFIFOsToTemps(std::vector<std::shared_ptr<ThreadCrossingFIFO>> fifos) {
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

std::vector<std::string> MultiThreadEmitterHelpers::writeFIFOsFromTemps(std::vector<std::shared_ptr<ThreadCrossingFIFO>> fifos) {
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

std::pair<std::string, std::string> MultiThreadEmitterHelpers::getCThreadArgStructDefn(std::vector<std::shared_ptr<ThreadCrossingFIFO>> inputFIFOs, std::vector<std::shared_ptr<ThreadCrossingFIFO>> outputFIFOs, std::string designName, int partitionNum){
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

std::string MultiThreadEmitterHelpers::emitFIFOStructHeader(std::string path, std::string fileNamePrefix, std::vector<std::shared_ptr<ThreadCrossingFIFO>> fifos) {
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

void MultiThreadEmitterHelpers::emitMultiThreadedBenchmarkKernel(std::map<std::pair<int, int>, std::vector<std::shared_ptr<ThreadCrossingFIFO>>> fifoMap,
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
    headerFile << "#include \"" << VITIS_TYPE_NAME << ".h\"" << std::endl;
    headerFile << "#include \"" << fifoHeaderFile << "\"" << std::endl;
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
    cFile.close();
}

void MultiThreadEmitterHelpers::emitMultiThreadedDriver(std::string path, std::string fileNamePrefix, std::string designName, int blockSize, std::string ioBenchmarkSuffix, std::vector<Variable> inputVars){
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

void MultiThreadEmitterHelpers::emitMultiThreadedMakefile(std::string path, std::string fileNamePrefix, std::string designName, int blockSize, std::set<int> partitions, std::string ioBenchmarkSuffix, bool includeLrt, std::vector<std::string> additionalSystemSrc){
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
                                    "SYSTEM_CFLAGS = -O3 -c -g -std=gnu11 -march=native -masm=att\n"
                                    "#Most kernels are allowed to be optomized.  Most assembly kernels use asm 'volitile' to force execution\n"
                                    "KERNEL_CFLAGS = -O3 -c -g -std=gnu11 -march=native -masm=att\n"
                                    "#For kernels that should not be optimized, the following is used\n"
                                    "KERNEL_NO_OPT_CFLAGS = -O0 -c -g -std=gnu11 -march=native -masm=att\n"
                                    "INC=-I $(COMMON_DIR) -I $(SRC_DIR)\n"
                                    "LIB_DIRS=-L $(COMMON_DIR)\n";
                                    if(includeLrt){
                     makefileContent += "LIB=-pthread -lrt -lProfilerCommon\n";
                                    }else {
                     makefileContent += "LIB=-pthread -lProfilerCommon\n";
                                    }
                 makefileContent += "\n"
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
                                    "SYSTEM_SRC = " + systemSrcs + ".c";
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

void MultiThreadEmitterHelpers::emitPartitionThreadC(int partitionNum, std::vector<std::shared_ptr<Node>> nodesToEmit, std::vector<std::shared_ptr<ThreadCrossingFIFO>> inputFIFOs, std::vector<std::shared_ptr<ThreadCrossingFIFO>> outputFIFOs, std::string path, std::string fileNamePrefix, std::string designName, SchedParams::SchedType schedType, std::shared_ptr<MasterOutput> outputMaster, unsigned long blockSize, std::string fifoHeaderFile, bool threadDebugPrint, bool printTelem){
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
    headerFile << "#include \"" << VITIS_TYPE_NAME << ".h\"" << std::endl;
    headerFile << "#include \"" << fifoHeaderFile << "\"" << std::endl;
    //headerFile << "#include <thread.h>" << std::endl;
    headerFile << std::endl;

    //Output the Function Definition
    headerFile << computeFctnProto << ";" << std::endl;
    headerFile << std::endl;

    //Create the threadFunction argument structure
    std::pair<std::string, std::string> threadArgStructAndTypeName = MultiThreadEmitterHelpers::getCThreadArgStructDefn(inputFIFOs, outputFIFOs, designName, partitionNum);
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
    if(printTelem){
        cFile << "#define _GNU_SOURCE //For clock_gettime" << std::endl;
    }
    if(threadDebugPrint || printTelem) {
        cFile << "#include <stdio.h>" << std::endl;
    }
    cFile << "#include \"" << fileName << ".h" << "\"" << std::endl;
    if(printTelem){
        cFile << "#include \"" << fileNamePrefix << "_telemetry_helpers.h" << "\"" << std::endl;
    }

    //Include any external include statements required by nodes in the design
    std::set<std::string> extIncludes;
    for(int i = 0; i<nodesToEmit.size(); i++){
        std::set<std::string> nodeIncludes = nodesToEmit[i]->getExternalIncludes();
        extIncludes.insert(nodeIncludes.begin(), nodeIncludes.end());
    }

    for(auto it = extIncludes.begin(); it != extIncludes.end(); it++){
        cFile << *it << std::endl;
    }

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
    cFile << MultiThreadEmitterHelpers::emitCopyCThreadArgs(inputFIFOs, outputFIFOs, "args", threadArgTypeName);

    //Insert timer init code
    double printDuration = 1; //TODO: Add option for this

    if(printTelem){
        cFile << "timespec_t timeResolution;" << std::endl;
        cFile << "clock_getres(CLOCK_MONOTONIC, &timeResolution);" << std::endl;
        cFile << "double timeResolutionDouble = timespecToDouble(&timeResolution);" << std::endl;
        cFile << "printf(\"Partition " << partitionNum << " Time Resolution: %8.6e\\n\", timeResolutionDouble);" << std::endl;
        cFile << std::endl;

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
        cFile << "double printDuration = " << printDuration << ";" << std::endl;
        cFile << "double timeTotal = 0;" << std::endl;
        cFile << "double timeWaitingForInputFIFOs = 0;" << std::endl;
        cFile << "double timeReadingInputFIFOs = 0;" << std::endl;
        cFile << "double timeWaitingForComputeToFinish = 0;" << std::endl;
        cFile << "double timeWaitingForOutputFIFOs = 0;" << std::endl;
        cFile << "double timeWritingOutputFIFOs = 0;" << std::endl;
    }

    //Create Loop
    cFile << "while(1){" << std::endl;

    if(threadDebugPrint) {
        cFile << "printf(\"Partition " + GeneralHelper::to_string(partitionNum) + " waiting for inputs ...\\n\");"
              << std::endl;
    }

    if(printTelem) {
        cFile << "timespec_t waitingForInputFIFOsStart;" << std::endl;
        cFile << "asm volatile (\"\" ::: \"memory\"); //Stop Re-ordering of timer" << std::endl;
        cFile << "clock_gettime(CLOCK_MONOTONIC, &waitingForInputFIFOsStart);" << std::endl;
        cFile << "asm volatile (\"\" ::: \"memory\"); //Stop Re-ordering of timer" << std::endl;
    }

    //Check FIFO input FIFOs (will spin until ready)
    cFile << MultiThreadEmitterHelpers::emitFIFOChecks(inputFIFOs, false, "inputFIFOsReady", false, true, true); //Include pthread_testcancel check

    //This is a special case where the duration for this cycle is calculated later (after reporting).  That way,
    //each metric has undergone the same number of cycles
    if(printTelem) {
        cFile << "timespec_t waitingForInputFIFOsStop;" << std::endl;
        cFile << "asm volatile (\"\" ::: \"memory\"); //Stop Re-ordering of timer" << std::endl;
        cFile << "clock_gettime(CLOCK_MONOTONIC, &waitingForInputFIFOsStop);" << std::endl;
        cFile << "asm volatile (\"\" ::: \"memory\"); //Stop Re-ordering of timer" << std::endl;

        //Emit timer reporting
        cFile << "rxSamples += " << blockSize << ";" << std::endl;
        cFile << "timespec_t currentTime;" << std::endl;
        cFile << "asm volatile (\"\" ::: \"memory\"); //Stop Re-ordering of timer" << std::endl;
        cFile << "clock_gettime(CLOCK_MONOTONIC, &currentTime);" << std::endl;
        cFile << "asm volatile (\"\" ::: \"memory\"); //Stop Re-ordering of timer" << std::endl;
        cFile << "double duration = difftimespec(&currentTime, &lastPrint);" << std::endl;
        cFile << "if(duration >= printDuration){" << std::endl;
        cFile << "lastPrint = currentTime;" << std::endl;
        cFile << "double durationSinceStart = difftimespec(&currentTime, &startTime);" << std::endl;
        cFile << "double rateMSps = ((double)rxSamples)/durationSinceStart/1000000;" << std::endl;
        cFile << "printf(\"Current " << designName << " [" << partitionNum << "]  Rate: %10.5f\\n\"" << std::endl;
        cFile << "\"\\t[" << partitionNum << "] Waiting for Input FIFOs:        %10.5f (%8.4f%%)\\n\"" << std::endl;
        cFile << "\"\\t[" << partitionNum << "] Reading Input FIFOs:            %10.5f (%8.4f%%)\\n\"" << std::endl;
        cFile << "\"\\t[" << partitionNum << "] Waiting For Compute to Finish:  %10.5f (%8.4f%%)\\n\"" << std::endl;
        cFile << "\"\\t[" << partitionNum << "] Waiting for Output FIFOs:       %10.5f (%8.4f%%)\\n\"" << std::endl;
        cFile << "\"\\t[" << partitionNum << "] Writing Output FIFOs:           %10.5f (%8.4f%%)\\n\"" << std::endl;
        cFile << "\"\\t[" << partitionNum << "] Total Time:                     %10.5f\\n\", " << std::endl;

        cFile << "rateMSps, ";
        cFile << "timeWaitingForInputFIFOs, timeWaitingForInputFIFOs/timeTotal*100, ";
        cFile << "timeReadingInputFIFOs, timeReadingInputFIFOs/timeTotal*100, ";
        cFile << "timeWaitingForComputeToFinish, timeWaitingForComputeToFinish/timeTotal*100, ";
        cFile << "timeWaitingForOutputFIFOs, timeWaitingForOutputFIFOs/timeTotal*100, ";
        cFile << "timeWritingOutputFIFOs, timeWritingOutputFIFOs/timeTotal*100, ";
        cFile << "timeTotal);" << std::endl;

        cFile << "}" << std::endl;

        //Now, finish timeReadingExtFIFO
        cFile << "double durationWaitingForInputFIFOs = difftimespec(&waitingForInputFIFOsStop, &waitingForInputFIFOsStart);" << std::endl;
        cFile << "timeWaitingForInputFIFOs += durationWaitingForInputFIFOs;" << std::endl;
        cFile << "timeTotal += durationWaitingForInputFIFOs;" << std::endl;

        cFile << "timespec_t readingInputFIFOsStart;" << std::endl;
        cFile << "asm volatile (\"\" ::: \"memory\"); //Stop Re-ordering of timer" << std::endl;
        cFile << "clock_gettime(CLOCK_MONOTONIC, &readingInputFIFOsStart);" << std::endl;
        cFile << "asm volatile (\"\" ::: \"memory\"); //Stop Re-ordering of timer" << std::endl;
    }

    //Create temp entries for FIFO inputs
    std::vector<std::string> tmpReadDecls = MultiThreadEmitterHelpers::createFIFOReadTemps(inputFIFOs);
    for(int i = 0; i<tmpReadDecls.size(); i++){
        cFile << tmpReadDecls[i] << std::endl;
    }

    //Read input FIFOs
    if(threadDebugPrint) {
        cFile << "printf(\"Partition " + GeneralHelper::to_string(partitionNum) + " reading inputs ...\\n\");"
              << std::endl;
    }
    std::vector<std::string> readFIFOExprs = MultiThreadEmitterHelpers::readFIFOsToTemps(inputFIFOs);
    for(int i = 0; i<readFIFOExprs.size(); i++){
        cFile << readFIFOExprs[i] << std::endl;
    }

    if(printTelem) {
        cFile << "timespec_t readingInputFIFOsStop;" << std::endl;
        cFile << "asm volatile (\"\" ::: \"memory\"); //Stop Re-ordering of timer" << std::endl;
        cFile << "clock_gettime(CLOCK_MONOTONIC, &readingInputFIFOsStop);" << std::endl;
        cFile << "asm volatile (\"\" ::: \"memory\"); //Stop Re-ordering of timer" << std::endl;
        cFile << "double durationReadingInputFIFOs = difftimespec(&readingInputFIFOsStop, &readingInputFIFOsStart);" << std::endl;
        cFile << "timeReadingInputFIFOs += durationReadingInputFIFOs;" << std::endl;
        cFile << "timeTotal += durationReadingInputFIFOs;" << std::endl;
    }

    //Create temp entries for outputs
    std::vector<std::string> tmpWriteDecls = MultiThreadEmitterHelpers::createFIFOWriteTemps(outputFIFOs);
    for(int i = 0; i<tmpWriteDecls.size(); i++){
        cFile << tmpWriteDecls[i] << std::endl;
    }

    if(threadDebugPrint) {
        cFile << "printf(\"Partition " + GeneralHelper::to_string(partitionNum) + " computing ...\\n\");" << std::endl;
    }

    if(printTelem) {
        cFile << "timespec_t waitingForComputeToFinishStart;" << std::endl;
        cFile << "asm volatile (\"\" ::: \"memory\"); //Stop Re-ordering of timer" << std::endl;
        cFile << "clock_gettime(CLOCK_MONOTONIC, &waitingForComputeToFinishStart);" << std::endl;
        cFile << "asm volatile (\"\" ::: \"memory\"); //Stop Re-ordering of timer" << std::endl;
    }

    //Call compute function (recall that the compute function is declared with outputs as references)
    std::string call = getCallPartitionComputeCFunction(computeFctnName, inputFIFOs, outputFIFOs, blockSize);
    cFile << call << std::endl;

    if(printTelem) {
        cFile << "timespec_t waitingForComputeToFinishStop;" << std::endl;
        cFile << "asm volatile (\"\" ::: \"memory\"); //Stop Re-ordering of timer" << std::endl;
        cFile << "clock_gettime(CLOCK_MONOTONIC, &waitingForComputeToFinishStop);" << std::endl;
        cFile << "asm volatile (\"\" ::: \"memory\"); //Stop Re-ordering of timer" << std::endl;
        cFile << "double durationWaitingForComputeToFinish = difftimespec(&waitingForComputeToFinishStop, &waitingForComputeToFinishStart);" << std::endl;
        cFile << "timeWaitingForComputeToFinish += durationWaitingForComputeToFinish;" << std::endl;
        cFile << "timeTotal += durationWaitingForComputeToFinish;" << std::endl;
    }

    //Check output FIFOs (will spin until ready)
    if(threadDebugPrint) {
        cFile << "printf(\"Partition " + GeneralHelper::to_string(partitionNum) +
                 " waiting for room in output FIFOs ...\\n\");" << std::endl;
    }

    if(printTelem) {
        cFile << "timespec_t waitingForOutputFIFOsStart;" << std::endl;
        cFile << "asm volatile (\"\" ::: \"memory\"); //Stop Re-ordering of timer" << std::endl;
        cFile << "clock_gettime(CLOCK_MONOTONIC, &waitingForOutputFIFOsStart);" << std::endl;
        cFile << "asm volatile (\"\" ::: \"memory\"); //Stop Re-ordering of timer" << std::endl;
    }

    cFile << MultiThreadEmitterHelpers::emitFIFOChecks(outputFIFOs, true, "outputFIFOsReady", false, true, true); //Include pthread_testcancel check

    if(printTelem) {
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

    if(printTelem) {
        cFile << "timespec_t writingOutputFIFOsStart;" << std::endl;
        cFile << "asm volatile (\"\" ::: \"memory\"); //Stop Re-ordering of timer" << std::endl;
        cFile << "clock_gettime(CLOCK_MONOTONIC, &writingOutputFIFOsStart);" << std::endl;
        cFile << "asm volatile (\"\" ::: \"memory\"); //Stop Re-ordering of timer" << std::endl;
    }

    std::vector<std::string> writeFIFOExprs = MultiThreadEmitterHelpers::writeFIFOsFromTemps(outputFIFOs);
    for(int i = 0; i<writeFIFOExprs.size(); i++){
        cFile << writeFIFOExprs[i] << std::endl;
    }

    if(printTelem) {
        cFile << "timespec_t writingOutputFIFOsStop;" << std::endl;
        cFile << "asm volatile (\"\" ::: \"memory\"); //Stop Re-ordering of timer" << std::endl;
        cFile << "clock_gettime(CLOCK_MONOTONIC, &writingOutputFIFOsStop);" << std::endl;
        cFile << "asm volatile (\"\" ::: \"memory\"); //Stop Re-ordering of timer" << std::endl;
        cFile << "double durationWritingOutputFIFOs = difftimespec(&writingOutputFIFOsStop, &writingOutputFIFOsStart);" << std::endl;
        cFile << "timeWritingOutputFIFOs += durationWritingOutputFIFOs;" << std::endl;
        cFile << "timeTotal += durationWritingOutputFIFOs;" << std::endl;
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

std::string MultiThreadEmitterHelpers::getPartitionComputeCFunctionArgPrototype(std::vector<std::shared_ptr<ThreadCrossingFIFO>> inputFIFOs, std::vector<std::shared_ptr<ThreadCrossingFIFO>> outputFIFOs, int blockSize){
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

std::string MultiThreadEmitterHelpers::getCallPartitionComputeCFunction(std::string computeFctnName, std::vector<std::shared_ptr<ThreadCrossingFIFO>> inputFIFOs, std::vector<std::shared_ptr<ThreadCrossingFIFO>> outputFIFOs, int blockSize){
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
void MultiThreadEmitterHelpers::emitSelectOpsSchedStateUpdateContext(std::ofstream &cFile, std::vector<std::shared_ptr<Node>> &nodesToEmit, SchedParams::SchedType schedType, std::shared_ptr<MasterOutput> outputMaster, int blockSize, std::string indVarName){

    cFile << std::endl << "//==== Compute Operators ====" << std::endl;

    //Sort nodes by schedOrder.
    std::vector<std::shared_ptr<Node>> orderedNodes = nodesToEmit;
    std::sort(orderedNodes.begin(), orderedNodes.end(), Node::lessThanSchedOrder);

    std::shared_ptr<Node> zeroSchedNodeCmp = NodeFactory::createNode<MasterUnconnected>(); //Need a node to compare to
    zeroSchedNodeCmp->setSchedOrder(0);

    auto schedIt = std::lower_bound(orderedNodes.begin(), orderedNodes.end(), zeroSchedNodeCmp, Node::lessThanSchedOrder); //Binary search for the first node to be emitted (schedOrder 0 or greater)

    std::vector<std::shared_ptr<Node>> toBeEmittedInThisOrder;
    std::copy(schedIt, orderedNodes.end(), std::back_inserter(toBeEmittedInThisOrder));

    EmitterHelpers::emitOpsStateUpdateContext(cFile, schedType, toBeEmittedInThisOrder, outputMaster, blockSize, indVarName);
}

bool MultiThreadEmitterHelpers::checkNoNodesInIO(std::vector<std::shared_ptr<Node>> nodes) {
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
