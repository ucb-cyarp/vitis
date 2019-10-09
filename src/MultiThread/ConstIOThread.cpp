//
// Created by Christopher Yarp on 10/4/19.
//

#include "ConstIOThread.h"

#include <iostream>
#include <fstream>
#include "GraphCore/Design.h"
#include "General/GeneralHelper.h"
#include "MultiThreadEmitterHelpers.h"
#include "General/EmitterHelpers.h"

void ConstIOThread::emitConstIOThreadC(std::vector<std::shared_ptr<ThreadCrossingFIFO>> inputFIFOs, std::vector<std::shared_ptr<ThreadCrossingFIFO>> outputFIFOs, std::string path, std::string fileNamePrefix, std::string designName, unsigned long blockSize, std::string fifoHeaderFile, bool threadDebugPrint){
    //Emit a thread for handeling the I/O

    //Note, a single input FIFO may correspond to multiple MasterOutput ports
    //Will deal with input/output FIFOs only and not MasterNode ports
    //A mapping of MasterNode ports to FIFOs is required for other drivers

    std::string fileName = fileNamePrefix+"_io_const";
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
    if(threadDebugPrint) {
        headerFile << "#include <stdio.h>" << std::endl;
    }
    headerFile << "#include \"" << VITIS_TYPE_NAME << ".h\"" << std::endl;
    headerFile << "#include \"" << fifoHeaderFile << "\"" << std::endl;
    headerFile << std::endl;

    //Create the threadFunction argument structure for the I/O thread (includes the references to FIFO shared vars)
    std::pair<std::string, std::string> threadArgStructAndTypeName = MultiThreadEmitterHelpers::getCThreadArgStructDefn(inputFIFOs, outputFIFOs, designName, IO_PARTITION_NUM);
    std::string threadArgStruct = threadArgStructAndTypeName.first;
    std::string threadArgTypeName = threadArgStructAndTypeName.second;
    headerFile << threadArgStruct << std::endl;
    headerFile << std::endl;

    //Output the function prototype for the I/O thread function
    std::string threadFctnDecl = "void* " + designName + "_io_const_thread(void *args)";
    headerFile << std::endl;
    headerFile << threadFctnDecl << ";" << std::endl;
    headerFile << "#endif" << std::endl;

    headerFile.close();

    //#### Emit .c File ####
    std::cout << "Emitting C File: " << path << "/" << fileName << ".c" << std::endl;
    std::ofstream ioThread;
    ioThread.open(path+"/"+fileName+".c", std::ofstream::out | std::ofstream::trunc);

    ioThread << "#include \"" << fileName << ".h" << "\"" << std::endl;
    ioThread << "#include \"intrin_bench_default_defines.h\"" << std::endl;
    ioThread << std::endl;

    ioThread << threadFctnDecl << "{" << std::endl;

    //Copy shared variables from the input argument structure
    ioThread << MultiThreadEmitterHelpers::emitCopyCThreadArgs(inputFIFOs, outputFIFOs, "args", threadArgTypeName);

    ioThread << "//Set Constant Arguments" << std::endl;
    std::vector<std::vector<NumericValue>> defaultArgs;
    for (unsigned long i = 0; i < outputFIFOs.size(); i++) {
        std::vector<NumericValue> defaultArgsForFifo;
        for(unsigned long portNum = 0; portNum<outputFIFOs[i]->getInputPorts().size(); portNum++) {
            DataType type = (*outputFIFOs[i]->getInputPort(portNum)->getArcs().begin())->getDataType(); // should already be validated to have an input port
            NumericValue val;
            val.setRealInt(1);
            val.setImagInt(1);
            val.setComplex(type.isComplex());
            val.setFractional(false);

            defaultArgsForFifo.push_back(val);
        }
        defaultArgs.push_back(defaultArgsForFifo);
    }

    //Create temp entries for outputs and initialize them with the constants
    std::vector<std::string> tmpWriteDecls = MultiThreadEmitterHelpers::createAndInitializeFIFOWriteTemps(outputFIFOs, defaultArgs);
    for(int i = 0; i<tmpWriteDecls.size(); i++){
        ioThread << tmpWriteDecls[i] << std::endl;
    }

    //Create the input FIFO temps
    std::vector<std::string> tmpReadDecls = MultiThreadEmitterHelpers::createFIFOReadTemps(inputFIFOs);
    for(int i = 0; i<tmpReadDecls.size(); i++){
        ioThread << tmpReadDecls[i] << std::endl;
    }

    ioThread << "int readCount = 0;" << std::endl;

    if(blockSize > 1){
        ioThread << "int iterLimit = STIM_LEN/" << blockSize << ";" << std::endl;
    }else{
        ioThread << "int iterLimit = STIM_LEN;" << std::endl;
    }
    ioThread << "for(int i = 0; i<iterLimit; ){" << std::endl;
    //Check Output FIFOs
    ioThread << MultiThreadEmitterHelpers::emitFIFOChecks(outputFIFOs, true, "outputFIFOsReady", false, true, false); //Only need a pthread_testcancel check on one FIFO check since this is nonblocking
    ioThread << "if(outputFIFOsReady){" << std::endl;
    //Write FIFOs
    std::vector<std::string> writeFIFOExprs = MultiThreadEmitterHelpers::writeFIFOsFromTemps(outputFIFOs);
    for(int i = 0; i<writeFIFOExprs.size(); i++){
        ioThread << writeFIFOExprs[i] << std::endl;
    }
    ioThread << "i++;" << std::endl;
    if(threadDebugPrint) {
        ioThread << "printf(\"I/O Sent\\n\");" << std::endl;
    }
    ioThread << "}" << std::endl;

    //Check input FIFOs
    ioThread << MultiThreadEmitterHelpers::emitFIFOChecks(inputFIFOs, false, "inputFIFOsReady", true, false, true); //pthread_testcancel check here
    ioThread << "if(inputFIFOsReady){" << std::endl;
    //Read input FIFOs
    std::vector<std::string> readFIFOExprs = MultiThreadEmitterHelpers::readFIFOsToTemps(inputFIFOs);
    for(int i = 0; i<readFIFOExprs.size(); i++){
        ioThread << readFIFOExprs[i] << std::endl;
    }
    ioThread << "readCount++;" << std::endl;
    if(threadDebugPrint) {
        ioThread << "printf(\"I/O Received\\n\");" << std::endl;
    }
    ioThread << "}" << std::endl; //Close if

    ioThread << "}" << std::endl; //Close for

    //Wait for reading to finish
    ioThread << "while(readCount<iterLimit){" << std::endl;
    ioThread << MultiThreadEmitterHelpers::emitFIFOChecks(inputFIFOs, false, "inputFIFOsReady", true, false, true); //pthread_testcancel check here (is blocking)
    ioThread << "if(inputFIFOsReady){" << std::endl;
    //Read input FIFOs
    std::vector<std::string> readFIFOExprsCleanup = MultiThreadEmitterHelpers::readFIFOsToTemps(inputFIFOs);
    for(int i = 0; i<readFIFOExprsCleanup.size(); i++){
        ioThread << readFIFOExprsCleanup[i] << std::endl;
    }
    ioThread << "readCount++;" << std::endl;
    if(threadDebugPrint) {
        ioThread << "printf(\"I/O Received\\n\");" << std::endl;
    }
    ioThread << "}" << std::endl; //end if
    ioThread << "}" << std::endl; //end while

    //Done reading
    ioThread << "return NULL;" << std::endl;
    ioThread << "}" << std::endl;
    ioThread.close();
}