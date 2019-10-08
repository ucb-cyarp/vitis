//
// Created by Christopher Yarp on 10/4/19.
//

#include "LinuxPipeIOThread.h"
#include <iostream>
#include <fstream>
#include "General/GeneralHelper.h"
#include "MultiThreadEmitterHelpers.h"
#include "General/ErrorHelpers.h"
#include "General/EmitterHelpers.h"

void LinuxPipeIOThread::emitLinuxPipeIOThreadC(std::shared_ptr<MasterInput> inputMaster,
                                               std::shared_ptr<MasterOutput> outputMaster,
                                               std::vector<std::shared_ptr<ThreadCrossingFIFO>> inputFIFOs,
                                               std::vector<std::shared_ptr<ThreadCrossingFIFO>> outputFIFOs,
                                               std::string path, std::string fileNamePrefix, std::string designName,
                                               unsigned long blockSize, std::string fifoHeaderFile,
                                               bool threadDebugPrint) {

    //Emit a thread for handeling the I/O

    //Note, a single input FIFO may correspond to multiple MasterOutput ports
    //Will deal with input/output FIFOs only and not MasterNode ports
    //A mapping of MasterNode ports to FIFOs is required for other drivers

    std::string fileName = fileNamePrefix+"_io_linux_pipe";
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
    headerFile << "#include \"" << fifoHeaderFile << "\"" << std::endl;
    headerFile << std::endl;

    //Create the threadFunction argument structure for the I/O thread (includes the references to FIFO shared vars)
    std::pair<std::string, std::string> threadArgStructAndTypeName = MultiThreadEmitterHelpers::getCThreadArgStructDefn(inputFIFOs, outputFIFOs, designName, IO_PARTITION_NUM);
    std::string threadArgStruct = threadArgStructAndTypeName.first;
    std::string threadArgTypeName = threadArgStructAndTypeName.second;
    headerFile << threadArgStruct << std::endl;
    headerFile << std::endl;

    //Output the function prototype for the I/O thread function
    std::string threadFctnDecl = "void* " + designName + "_io_linux_pipe_thread(void *args)";
    headerFile << std::endl;
    headerFile << threadFctnDecl << ";" << std::endl;
    headerFile << std::endl;

    //Output the input and output structure definitions
    std::string inputStructTypeName = designName+"_inputs_t";
    std::string outputStructTypeName = designName+"_outputs_t";

    std::vector<Variable> masterInputVars = EmitterHelpers::getCInputVariables(inputMaster);
    std::vector<Variable> masterOutputVars = EmitterHelpers::getCOutputVariables(outputMaster);

    headerFile << EmitterHelpers::getCIOPortStructDefn(masterInputVars, inputStructTypeName, blockSize) << std::endl;
    headerFile << std::endl;
    headerFile << EmitterHelpers::getCIOPortStructDefn(masterOutputVars, outputStructTypeName, blockSize) << std::endl;

    headerFile << "#endif" << std::endl;

    //#### Emit .c File ####
    std::cout << "Emitting C File: " << path << "/" << fileName << ".c" << std::endl;
    std::ofstream ioThread;
    ioThread.open(path+"/"+fileName+".c", std::ofstream::out | std::ofstream::trunc);

    ioThread << "#include <stdio.h>" << std::endl;
    ioThread << "#include <sys/stat.h>" << std::endl;
    ioThread << "#include <unistd.h>" << std::endl;
    ioThread << "#include \"" << fileName << ".h" << "\"" << std::endl;
    ioThread << "#include \"intrin_bench_default_defines.h\"" << std::endl;
    ioThread << std::endl;

    ioThread << threadFctnDecl << "{" << std::endl;

    //Copy shared variables from the input argument structure
    ioThread << MultiThreadEmitterHelpers::emitCopyCThreadArgs(inputFIFOs, outputFIFOs, "args", threadArgTypeName);

    //Create temp entries for outputs and initialize them with the constants
    std::vector<std::string> tmpWriteDecls = MultiThreadEmitterHelpers::createFIFOWriteTemps(outputFIFOs);
    for(int i = 0; i<tmpWriteDecls.size(); i++){
        ioThread << tmpWriteDecls[i] << std::endl;
    }

    //Create the input FIFO temps
    std::vector<std::string> tmpReadDecls = MultiThreadEmitterHelpers::createFIFOReadTemps(inputFIFOs);
    for(int i = 0; i<tmpReadDecls.size(); i++){
        ioThread << tmpReadDecls[i] << std::endl;
    }

    //Create Linux Pipes
    ioThread << "int status = mkfifo(\"input.pipe\", S_IRUSR | S_IWUSR);" << std::endl;
    ioThread << "if(status != 0){" << std::endl;
    ioThread << "printf(\"Unable to create input pipe ... exiting\\n\");" << std::endl;
    ioThread << "perror(NULL);" << std::endl;
    ioThread << "exit(1);" << std::endl;
    ioThread << "}" << std::endl;

    ioThread << "status = mkfifo(\"output.pipe\", S_IRUSR | S_IWUSR);" << std::endl;
    ioThread << "if(status != 0){" << std::endl;
    ioThread << "printf(\"Unable to create output pipe ... exiting\\n\");" << std::endl;
    ioThread << "perror(NULL);" << std::endl;
    ioThread << "exit(1);" << std::endl;
    ioThread << "}" << std::endl;

    //Open Linux pipes
    ioThread << "FILE *inputPipe = fopen(\"input.pipe\", \"rb\");" << std::endl;
    ioThread << "if(inputPipe == NULL){" << std::endl;
    ioThread << "printf(\"Unable to open input pipe ... exiting\\n\");" << std::endl;
    ioThread << "perror(NULL);" << std::endl;
    ioThread << "exit(1);" << std::endl;
    ioThread << "}" << std::endl;

    ioThread << "FILE *outputPipe = fopen(\"output.pipe\", \"wb\");" << std::endl;
    ioThread << "if(outputPipe == NULL){" << std::endl;
    ioThread << "printf(\"Unable to open output pipe ... exiting\\n\");" << std::endl;
    ioThread << "perror(NULL);" << std::endl;
    ioThread << "exit(1);" << std::endl;
    ioThread << "}" << std::endl;

    ioThread << "while(true){" << std::endl;

    //Allocate temp Memory for linux pipe read
    std::string linuxInputPipeName = "linuxInputTmp";
    ioThread << inputStructTypeName << " " << linuxInputPipeName << ";" << std::endl;

    //Read data from the input pipe
    ioThread << "int elementsRead = fread(&" << linuxInputPipeName << ", sizeof(" << inputStructTypeName << "), 1, inputPipe);" << std::endl;
    ioThread << "if(elementsRead != 1 && feof(inputPipe)){" << std::endl;
    ioThread << "//Done with input (input pipe closed)" << std::endl;
    ioThread << "break;" << std::endl;
    ioThread << "} else if (elementsRead != 1 && ferror(inputPipe)){" << std::endl;
    ioThread << "printf(\"An error was encountered while reading the Input Linux Pipe\\n\");" << std::endl;
    ioThread << "perror(NULL);" << std::endl;
    ioThread << "exit(1);" << std::endl;
    ioThread << "} else if (elementsRead != 1){" << std::endl;
    ioThread << "printf(\"An unknown error was encountered while reading the Input Linux Pipe\\n\");" << std::endl;
    ioThread << "exit(1);" << std::endl;
    ioThread << "}" << std::endl;
    if(threadDebugPrint) {
        ioThread << "printf(\"I/O Input Received\\n\");" << std::endl;
    }

    //Fill write temps with data from pipe
    std::map<int, std::vector<std::pair<std::shared_ptr<ThreadCrossingFIFO>, int>>> inputPortFifoMap = getInputPortFIFOMapping(outputFIFOs); //Note, outputFIFOs are the outputs of the I/O thread.  They carry the inputs to the system to the rest of the system
    copyIOInputsToFIFO(ioThread, masterInputVars, inputPortFifoMap, linuxInputPipeName, blockSize);

    //Check Output FIFOs
    ioThread << MultiThreadEmitterHelpers::emitFIFOChecks(outputFIFOs, true, "outputFIFOsReady", false, true, false); //Only need a pthread_testcancel check on one FIFO check since this is nonblocking
    ioThread << "if(outputFIFOsReady){" << std::endl;

    //Write FIFOs
    std::vector<std::string> writeFIFOExprs = MultiThreadEmitterHelpers::writeFIFOsFromTemps(outputFIFOs);
    for(int i = 0; i<writeFIFOExprs.size(); i++){
        ioThread << writeFIFOExprs[i] << std::endl;
    }

    if(threadDebugPrint) {
        ioThread << "printf(\"I/O Passed to Compute\\n\");" << std::endl;
    }
    ioThread << "}" << std::endl;

    //Check input FIFOs
    ioThread << MultiThreadEmitterHelpers::emitFIFOChecks(inputFIFOs, false, "inputFIFOsReady", true, true, true); //pthread_testcancel check here
    ioThread << "if(inputFIFOsReady){" << std::endl;
    //Read input FIFOs
    std::vector<std::string> readFIFOExprs = MultiThreadEmitterHelpers::readFIFOsToTemps(inputFIFOs);
    for(int i = 0; i<readFIFOExprs.size(); i++){
        ioThread << readFIFOExprs[i] << std::endl;
    }
    if(threadDebugPrint) {
        ioThread << "printf(\"I/O Received from Compute\\n\");" << std::endl;
    }

    //Allocate temp Memory for linux pipe write
    std::string linuxOutputPipeName = "linuxOutputTmp";
    ioThread << outputStructTypeName << " " << linuxOutputPipeName << ";" << std::endl;

    //Copy output to tmp variable
    std::map<int, std::pair<std::shared_ptr<ThreadCrossingFIFO>, int>> outputPortFifoMap = getOutputPortFIFOMapping(outputMaster);
    copyFIFOToIOOutputs(ioThread, masterOutputVars, outputPortFifoMap, outputMaster, linuxOutputPipeName, blockSize);

    //Write to linux pipe
    ioThread << "int elementsWritten = fwrite(&" << linuxOutputPipeName << ", sizeof(" << outputStructTypeName << "), 1, outputPipe);" << std::endl;
    ioThread << "if (elementsWritten != 1 && ferror(outputPipe)){" << std::endl;
    ioThread << "printf(\"An error was encountered while writing the Output Linux Pipe\\n\");" << std::endl;
    ioThread << "perror(NULL);" << std::endl;
    ioThread << "exit(1);" << std::endl;
    ioThread << "} else if (elementsRead != 1){" << std::endl;
    ioThread << "printf(\"An unknown error was encountered while writing to Input Linux Pipe\\n\");" << std::endl;
    ioThread << "exit(1);" << std::endl;
    ioThread << "}" << std::endl;
    if(threadDebugPrint) {
        ioThread << "printf(\"I/O Output Sent\\n\");" << std::endl;
    }

    ioThread << "}" << std::endl; //Close if

    ioThread << "}" << std::endl; //Close while

    if(threadDebugPrint) {
        ioThread << "printf(\"Input Pipe Closed ... Exiting\\n\");" << std::endl;
    }

    //clean up pipes, input pipe closed
    //The client should close the output FIFO first then the read FIFO
    //Close the pipes
    ioThread << "int closeStatus = fclose(inputPipe);" << std::endl;
    ioThread << "if(closeStatus != 0){" << std::endl;
    ioThread << "printf(\"Unable to close Linux Input Pipe\\n\")";
    ioThread << "}" << std::endl;
    ioThread << "closeStatus = fclose(outputPipe);" << std::endl;
    ioThread << "if(closeStatus != 0){" << std::endl;
    ioThread << "printf(\"Unable to close Linux Output Pipe\\n\")";
    ioThread << "}" << std::endl;

    //Delete the pipes
    ioThread << "closeStatus = unlink(\"input.pipe\");" << std::endl;
    ioThread << "if (closeStatus != 0){" << std::endl;
    ioThread << "printf(\"Could not delete the Linux Input Pipe\\n\");" << std::endl;
    ioThread << "perror(NULL);" << std::endl;
    ioThread << "}" << std::endl;

    ioThread << "closeStatus = unlink(\"output.pipe\");" << std::endl;
    ioThread << "if (closeStatus != 0){" << std::endl;
    ioThread << "printf(\"Could not delete the Linux Output Pipe\\n\");" << std::endl;
    ioThread << "perror(NULL);" << std::endl;
    ioThread << "}" << std::endl;

    //Done reading
    ioThread << "return NULL;" << std::endl;
    ioThread << "}" << std::endl;

}

std::map<int, std::vector<std::pair<std::shared_ptr<ThreadCrossingFIFO>, int>>>
LinuxPipeIOThread::getInputPortFIFOMapping(std::vector<std::shared_ptr<ThreadCrossingFIFO>> inputFIFOs) {
    std::map<int, std::vector<std::pair<std::shared_ptr<ThreadCrossingFIFO>, int>>> portToFIFO;

    for(int i = 0; i<inputFIFOs.size(); i++){
        if(inputFIFOs[i]->getInputPorts().size()==0){
            throw std::runtime_error(ErrorHelpers::genErrorStr("FIFO must have at least 1 input port", inputFIFOs[i]));
        }
        std::vector<std::shared_ptr<InputPort>> fifoInputPorts = inputFIFOs[i]->getInputPorts();
        for(int j = 0; j<fifoInputPorts.size(); j++){
            if(fifoInputPorts[j]->getArcs().size() != 1){
                throw std::runtime_error(ErrorHelpers::genErrorStr("FIFO input ports must have exactly 1 input arc", inputFIFOs[i]));
            }

            std::shared_ptr<OutputPort> masterInputPort = fifoInputPorts[j]->getSrcOutputPort();
            //TODO: change if I/O merged with computation
            if(GeneralHelper::isType<Node, MasterInput>(masterInputPort->getParent()) == nullptr){
                throw std::runtime_error(ErrorHelpers::genErrorStr("I/O Input FIFOs should be connected to an input master node", inputFIFOs[i]));
            }

            int fifoPortNum = fifoInputPorts[j]->getPortNum();
            int masterInputPortNum = masterInputPort->getPortNum();

            std::pair<std::shared_ptr<ThreadCrossingFIFO>, int> newPair(inputFIFOs[i], fifoPortNum);
            portToFIFO[masterInputPortNum].push_back(newPair);
        }
    }

    return portToFIFO;
}

std::map<int, std::pair<std::shared_ptr<ThreadCrossingFIFO>, int>>
LinuxPipeIOThread::getOutputPortFIFOMapping(std::shared_ptr<MasterOutput> outputMaster) {
    std::vector<std::shared_ptr<InputPort>> outputMasterPorts = outputMaster->getInputPorts();
    std::map<int, std::pair<std::shared_ptr<ThreadCrossingFIFO>, int>> portToFIFO;

    for(int i = 0; i<outputMasterPorts.size(); i++){
        if(outputMasterPorts[i]->getArcs().size() != 1){
            throw std::runtime_error(ErrorHelpers::genErrorStr("Master Outputs must have exactly 1 incoming arc", outputMaster));
        }

        std::shared_ptr<OutputPort> srcPort = outputMasterPorts[i]->getSrcOutputPort();

        //TODO: Handle special case of input->output direct passthrough
        //Stopgap measure: suggest placing a zero input delay
        if(GeneralHelper::isType<Node, MasterInput>(srcPort->getParent()) != nullptr){
            throw std::runtime_error(ErrorHelpers::genErrorStr("Multithreaded emit does not currently support direct passthrough of inputs to outputs.  Try inserting a delay block with delay = 0", outputMaster));
        }

        //TODO: change if I/O merged with computation
        std::shared_ptr<ThreadCrossingFIFO> srcFIFO = GeneralHelper::isType<Node, ThreadCrossingFIFO>(srcPort->getParent());
        if(srcFIFO == nullptr){
            throw std::runtime_error(ErrorHelpers::genErrorStr("Output Master should only be connected to thread crossing FIFOs", outputMaster));
        }

        int masterOutputPortNum = outputMasterPorts[i]->getPortNum();
        int fifoPortNum = srcPort->getPortNum();

        std::pair<std::shared_ptr<ThreadCrossingFIFO>, int> fifoPortPair(srcFIFO, fifoPortNum);
        portToFIFO[masterOutputPortNum] = fifoPortPair;
    }

    return portToFIFO;
}

std::string
LinuxPipeIOThread::getInputStructDefn(std::shared_ptr<MasterInput> inputMaster, std::string structTypeName, int blockSize) {

    std::vector<Variable> inputVariables = EmitterHelpers::getCInputVariables(inputMaster);
    return EmitterHelpers::getCIOPortStructDefn(inputVariables, structTypeName, blockSize);
}

std::string
LinuxPipeIOThread::getOutputStructDefn(std::shared_ptr<MasterOutput> outputMaster, std::string structTypeName,
                                       int blockSize) {
    std::vector<Variable> outputVars = EmitterHelpers::getCOutputVariables(outputMaster);
    return EmitterHelpers::getCIOPortStructDefn(outputVars, structTypeName, blockSize);
}

void LinuxPipeIOThread::copyIOInputsToFIFO(std::ofstream &ioThread, std::vector<Variable> masterInputVars, std::map<int, std::vector<std::pair<std::shared_ptr<ThreadCrossingFIFO>, int>>> inputPortFifoMap, std::string linuxInputPipeName, int blockSize) {
    for(int i = 0; i<masterInputVars.size(); i++){
        DataType dt = masterInputVars[i].getDataType();

        //The index is the port number
        //Find if this element is used and where
        auto fifoVecIt = inputPortFifoMap.find(i);
        if(fifoVecIt != inputPortFifoMap.end()){
            //This port is used
            std::vector<std::pair<std::shared_ptr<ThreadCrossingFIFO>, int>> fifoPortVec = fifoVecIt->second;
            for(int j = 0; j<fifoPortVec.size(); j++) {
                //Copy the values from the input structure to the appropriate (output) FIFO variables
                std::string tmpName = fifoPortVec[j].first->getName() + "_writeTmp";
                int fifoPortNum = fifoPortVec[j].second;

                if(dt.getWidth() == 1) {
                    ioThread << tmpName << ".port" << fifoPortNum << "_real = " << linuxInputPipeName << "."
                             << masterInputVars[i].getCVarName(false) << ";" << std::endl;
                    if (dt.isComplex()) {
                        ioThread << tmpName << ".port" << fifoPortNum << "_imag = " << linuxInputPipeName << "."
                                 << masterInputVars[i].getCVarName(true) << ";" << std::endl;
                    }
                }else if((*fifoPortVec[j].first->getInputArcs().begin())->getDataType().getWidth() != 1 && blockSize != 1){
                    //TODO: Implement full support for 2D arrays.  Implemented in FIFOs but not in most primitive elements
                    throw std::runtime_error(ErrorHelpers::genErrorStr("Multi dimension arrays are not currently supported for master input ports"));
                }else{
                    //Simple array
                    ioThread << "memcpy(" << tmpName << ".port" << fifoPortNum << "_real, " << linuxInputPipeName << "."
                             << masterInputVars[i].getCVarName(false) << ", sizeof(" << linuxInputPipeName << "."
                             << masterInputVars[i].getCVarName(false) << "));" << std::endl;
                    if (dt.isComplex()) {
                        ioThread << "memcpy(" << tmpName << ".port" << fifoPortNum << "_imag, " << linuxInputPipeName << "."
                                 << masterInputVars[i].getCVarName(true) << ", sizeof(" << linuxInputPipeName << "."
                                 << masterInputVars[i].getCVarName(true) << "));" << std::endl;
                    }
                }
            }
        }
    }
}

void LinuxPipeIOThread::copyFIFOToIOOutputs(std::ofstream &ioThread, std::vector<Variable> masterOutputVars,
                                            std::map<int, std::pair<std::shared_ptr<ThreadCrossingFIFO>, int>> outputPortFifoMap,
                                            std::shared_ptr<MasterOutput> outputMaster, std::string linuxOutputPipeName,
                                            int blockSize) {
    for(int i = 0; i<masterOutputVars.size(); i++){
        DataType dt = masterOutputVars[i].getDataType();

        //The index is the port number
        //Find if this element is used and where
        auto fifoPortIt = outputPortFifoMap.find(i);
        if(fifoPortIt != outputPortFifoMap.end()){
            //This port is used
            std::pair<std::shared_ptr<ThreadCrossingFIFO>, int> fifoPort = fifoPortIt->second;
            //Copy the values from the input structure to the appropriate (output) FIFO variables
            std::string tmpName = fifoPort.first->getName() + "_readTmp";
            int fifoPortNum = fifoPort.second;

            if(dt.getWidth() == 1) {
                ioThread << linuxOutputPipeName << "." << masterOutputVars[i].getCVarName(false) << " = "
                         << tmpName << ".port" << fifoPortNum << "_real;" << std::endl;
                if (dt.isComplex()) {
                    ioThread << linuxOutputPipeName << "." << masterOutputVars[i].getCVarName(true) << " = "
                             << tmpName << ".port" << fifoPortNum << "_imag;" << std::endl;
                }
            }else if((*fifoPort.first->getInputArcs().begin())->getDataType().getWidth() != 1 && blockSize != 1){
                //TODO: Implement full support for 2D arrays.  Implemented in FIFOs but not in most primitive elements
                throw std::runtime_error(ErrorHelpers::genErrorStr("Multi dimension arrays are not currently supported for master output ports", outputMaster));
            }else{
                //Simple array
                ioThread << "memcpy(" << linuxOutputPipeName << "." << masterOutputVars[i].getCVarName(false)
                         << ", " << tmpName << ".port" << fifoPortNum << "_real" << ", sizeof(" << linuxOutputPipeName << "."
                         << masterOutputVars[i].getCVarName(false) << "));" << std::endl;
                if (dt.isComplex()) {
                    ioThread << "memcpy(" << linuxOutputPipeName << "." << masterOutputVars[i].getCVarName(true)
                             << ", " << tmpName << ".port" << fifoPortNum << "_real" << ", sizeof(" << linuxOutputPipeName << "."
                             << masterOutputVars[i].getCVarName(true) << "));" << std::endl;
                }
            }
        }else{
            throw std::runtime_error(ErrorHelpers::genErrorStr("All master output ports should have a connected FIFO", outputMaster));
        }
    }
}
