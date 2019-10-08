//
// Created by Christopher Yarp on 10/4/19.
//

#include "StreamIOThread.h"
#include <iostream>
#include <fstream>
#include "General/GeneralHelper.h"
#include "MultiThreadEmitterHelpers.h"
#include "General/ErrorHelpers.h"
#include "General/EmitterHelpers.h"

void StreamIOThread::emitStreamIOThreadC(std::shared_ptr<MasterInput> inputMaster,
                                         std::shared_ptr<MasterOutput> outputMaster,
                                         std::vector<std::shared_ptr<ThreadCrossingFIFO>> inputFIFOs,
                                         std::vector<std::shared_ptr<ThreadCrossingFIFO>> outputFIFOs,
                                         std::string path, std::string fileNamePrefix, std::string designName,
                                         StreamType streamType, unsigned long blockSize, std::string fifoHeaderFile,
                                         bool threadDebugPrint) {

    //Emit a thread for handeling the I/O

    //Note, a single input FIFO may correspond to multiple MasterOutput ports
    //Will deal with input/output FIFOs only and not MasterNode ports
    //A mapping of MasterNode ports to FIFOs is required for other drivers

    std::string filenamePostfix;
    switch(streamType){
        case StreamType::PIPE:
            filenamePostfix = "io_linux_pipe";
            break;
        case StreamType::SOCKET:
            filenamePostfix = "io_network_socket";
            break;
        default:
            throw std::runtime_error(ErrorHelpers::genErrorStr("Unknown stream type during stream I/O emit"));
    }

    std::string fileName = fileNamePrefix + "_" + filenamePostfix;

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
    if(streamType == StreamType::SOCKET){
        headerFile << "#define VITIS_SOCKET_LISTEN_PORT " << DEFAULT_VITIS_SOCKET_LISTEN_PORT << std::endl;
        headerFile << "#define VITIS_SOCKET_LISTEN_ADDR " << DEFAULT_VITIS_SOCKET_LISTEN_ADDR << std::endl;
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
    std::string threadFctnDecl = "void* " + designName + "_" + filenamePostfix + "_thread(void *args)";
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
    ioThread << "#include <stdlib.h>" << std::endl;
    ioThread << "#include <string.h>" << std::endl;
    ioThread << "#include <sys/stat.h>" << std::endl;
    ioThread << "#include <unistd.h>" << std::endl;
    if(streamType == StreamType::SOCKET) {
        ioThread << "#include <sys/types.h>" << std::endl;
        ioThread << "#include <sys/socket.h>" << std::endl;
        ioThread << "#include <netinet/in.h>" << std::endl;
        ioThread << "#include <arpa/inet.h>" << std::endl;
    }
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

    std::string connectedSocketName = "acceptSocket";
    std::string listenSocketName = "listenSocket";
    ioThread << std::endl;
    if(streamType == StreamType::PIPE) {
        //Create Linux Pipes
        ioThread << "//Setup Pipes" << std::endl;
        ioThread << "int status = mkfifo(\"input.pipe\", S_IRUSR | S_IWUSR);" << std::endl;
        ioThread << "if(status != 0){" << std::endl;
        ioThread << "printf(\"Unable to create input pipe ... exiting\\n\");" << std::endl;
        ioThread << "perror(NULL);" << std::endl;
        ioThread << "exit(1);" << std::endl;
        ioThread << "}" << std::endl;
        ioThread << std::endl;

        ioThread << "status = mkfifo(\"output.pipe\", S_IRUSR | S_IWUSR);" << std::endl;
        ioThread << "if(status != 0){" << std::endl;
        ioThread << "printf(\"Unable to create output pipe ... exiting\\n\");" << std::endl;
        ioThread << "perror(NULL);" << std::endl;
        ioThread << "exit(1);" << std::endl;
        ioThread << "}" << std::endl;
        ioThread << std::endl;

        //Open Linux pipes
        ioThread << "FILE *inputPipe = fopen(\"input.pipe\", \"rb\");" << std::endl;
        ioThread << "if(inputPipe == NULL){" << std::endl;
        ioThread << "printf(\"Unable to open input pipe ... exiting\\n\");" << std::endl;
        ioThread << "perror(NULL);" << std::endl;
        ioThread << "exit(1);" << std::endl;
        ioThread << "}" << std::endl;
        ioThread << std::endl;

        ioThread << "FILE *outputPipe = fopen(\"output.pipe\", \"wb\");" << std::endl;
        ioThread << "if(outputPipe == NULL){" << std::endl;
        ioThread << "printf(\"Unable to open output pipe ... exiting\\n\");" << std::endl;
        ioThread << "perror(NULL);" << std::endl;
        ioThread << "exit(1);" << std::endl;
        ioThread << "}" << std::endl;
    }else if(streamType == StreamType::SOCKET){
        //Much of the code to open and bind sockets is borrowed from a lab I did in the Web Design Infrastructure class
        //at Santa Clara University using tutorials from http://beej.us/guide/bgnet and debug help from the following:
        //http://stackoverflow.com/questions/3855890/closing-a-listening-tcp-socket-in-c
        //http://stackoverflow.com/questions/6894092/question-about-inaddr-any
        //https://pubs.opengroup.org/onlinepubs/7908799/xns/arpainet.h.html
        //https://pubs.opengroup.org/onlinepubs/7908799/xns/syssocket.h.html
        //https://pubs.opengroup.org/onlinepubs/007908799/xns/netinetin.h.html
        //http://man7.org/linux/man-pages/man2/recv.2.html
        //https://linux.die.net/man/2/setsockopt
        //http://man7.org/linux/man-pages/man7/socket.7.html
        //http://man7.org/linux/man-pages/man7/ip.7.html

        ioThread << "//Setup Sockets" << std::endl;
        //Open a socket for IPv4 and a Socket Byte Stream (bidirectional byte stream)
        ioThread << "int " << listenSocketName << " = socket(AF_INET, SOCK_STREAM, 0);" << std::endl;
        ioThread << "if(" << listenSocketName << " == -1){" << std::endl;
        ioThread << "fprintf(stderr, \"Could not create listen socket\\n\");" << std::endl;
        ioThread << "perror(NULL);" << std::endl;
        ioThread << "exit(1);" << std::endl;
        ioThread << "}" << std::endl;
        ioThread << std::endl;

        //Allow the socket address to be re-used after the socket is closed (encountered this problem in another project
        //Where the socket was kept in TIME_WAIT after close unless this was specified
        ioThread << "int settingVal = 1;" << std::endl;
        ioThread << "int socketSetting = setsockopt(" << listenSocketName << ", SOL_SOCKET, SO_REUSEADDR, &settingVal, sizeof settingVal);" << std::endl;

        ioThread << "if(socketSetting != 0){" << std::endl;
        ioThread << "fprintf(stderr, \"Could not set SO_REUSEADDR for listen socket ... continuing anyway\\n\");" << std::endl;
        ioThread << "perror(NULL);" << std::endl;
        ioThread << "}" << std::endl;
        ioThread << std::endl;

        //Set allowed connection address/port pairs
        ioThread << "struct sockaddr_in localAddr;" << std::endl;
        ioThread << "localAddr.sin_family=AF_INET;" << std::endl;
        ioThread << "localAddr.sin_port=htons(VITIS_SOCKET_LISTEN_PORT);" << std::endl;
        ioThread << "localAddr.sin_addr.s_addr=htonl(VITIS_SOCKET_LISTEN_ADDR);" << std::endl;
        ioThread << "printf(\"Listen addr: %d\\n\", VITIS_SOCKET_LISTEN_ADDR);" << std::endl;
        ioThread << "printf(\"Listen port: %d\\n\", VITIS_SOCKET_LISTEN_PORT);" << std::endl;
        ioThread << "printf(\"Waiting for connection ...\\n\");" << std::endl;
        ioThread << std::endl;

        //Bind the socket to the address/port
        ioThread << "int bindStatus = bind(" << listenSocketName << ", (struct sockaddr *)(&localAddr), sizeof(localAddr));" << std::endl;

        ioThread << "if(bindStatus !=0){" << std::endl;
        ioThread << "fprintf(stderr, \"Could not bind socket\\n\");" << std::endl;
        ioThread << "perror(NULL);" << std::endl;
        ioThread << "exit(1);" << std::endl;
        ioThread << "}" << std::endl;
        ioThread << std::endl;

        //Set the socket to listen for incoming connections (limit to a queue of length 1)
        ioThread << "int listenStatus = listen(" << listenSocketName << ", 1);" << std::endl;
        ioThread << "if(listenStatus !=0){" << std::endl;
        ioThread << "fprintf(stderr, \"Could not listen on socket\\n\");" << std::endl;
        ioThread << "perror(NULL);" << std::endl;
        ioThread << "exit(1);" << std::endl;
        ioThread << "}" << std::endl;
        ioThread << std::endl;

        //Accept an incoming connection
        ioThread << "int " << connectedSocketName << "=-1;" << std::endl;
        ioThread << "struct sockaddr socketAdr;" << std::endl;
        ioThread << "socklen_t socketPort = sizeof(socketAdr);" << std::endl;
        ioThread << connectedSocketName << " = accept(" << listenSocketName << ", &socketAdr, &socketPort);" << std::endl;
        ioThread << "if(" << connectedSocketName << " == -1){" << std::endl;
        ioThread << "fprintf(stderr, \"Could not accept on listening socket\\n\");" << std::endl;
        ioThread << "perror(NULL);" << std::endl;
        ioThread << "exit(1);" << std::endl;
        ioThread << "}" << std::endl;
        ioThread << std::endl;

        //Since we are using the AF_INET family, the structure ysed is sockaddr_in
        //https://www.ibm.com/support/knowledgecenter/en/ssw_ibm_i_71/rzab6/cafinet.htm
        //https://stackoverflow.com/questions/1276294/getting-ipv4-address-from-a-sockaddr-structure
        //http://man7.org/linux/man-pages/man3/inet_ntop.3.html
        ioThread << "if(socketAdr.sa_family != AF_INET){" << std::endl;
        ioThread << "fprintf(stderr, \"Unexpected connection address type\\n\");" << std::endl;
        ioThread << "}else {" << std::endl;
        ioThread << "char connectionAddrStr[INET_ADDRSTRLEN];" << std::endl;
        ioThread << "char* connectionAddrStrPtr = &(connectionAddrStr[0]);" << std::endl;
        ioThread << "struct sockaddr_in* ipv4AddrStruct = (struct sockaddr_in*) &acceptSocket;" << std::endl;
        ioThread << "const char* nameStr = inet_ntop(AF_INET, ipv4AddrStruct, connectionAddrStrPtr, INET_ADDRSTRLEN);" << std::endl;
        ioThread << "if(nameStr != NULL) {" << std::endl;
        ioThread << "printf(\"Connection from %s:%d\\n\", nameStr, ntohs(ipv4AddrStruct->sin_port));" << std::endl;
        ioThread << "}" << std::endl;
        ioThread << "}" << std::endl;
    }else{
        throw std::runtime_error(ErrorHelpers::genErrorStr("Unknown stream type during stream I/O emit"));
    }

    ioThread << std::endl;
    ioThread << "//Thread loop" << std::endl;

    ioThread << "while(true){" << std::endl;
    //Allocate temp Memory for linux pipe read
    std::string linuxInputTmpName = "linuxInputTmp";
    ioThread << inputStructTypeName << " " << linuxInputTmpName << ";" << std::endl;

    //Read data from the input pipe
    if(streamType == StreamType::PIPE) {
        ioThread << "int elementsRead = fread(&" << linuxInputTmpName << ", sizeof(" << inputStructTypeName << "), 1, inputPipe);" << std::endl;
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
    }else if(streamType == StreamType::SOCKET){
        //Dectecting a closed socket is covered in the following
        //https://stackoverflow.com/questions/3091010/recv-socket-function-returning-data-with-length-as-0
        //https://linux.die.net/man/3/recv
        //Read should block, especially if in MSG_WAITALL mode
        ioThread << "int bytesRead = recv(" << connectedSocketName << ", &" << linuxInputTmpName << ", sizeof(" << inputStructTypeName << "), MSG_WAITALL);" << std::endl;
        ioThread << "if(bytesRead == 0){" << std::endl;
        ioThread << "//Done with input (socket closed)" << std::endl;
        ioThread << "break;" << std::endl;
        ioThread << "} else if (bytesRead == -1){" << std::endl;
        ioThread << "printf(\"An error was encountered while reading the socket\\n\");" << std::endl;
        ioThread << "perror(NULL);" << std::endl;
        ioThread << "exit(1);" << std::endl;
        ioThread << "} else if (bytesRead != sizeof(" << inputStructTypeName << ")){" << std::endl;
        ioThread << "printf(\"An unknown error was encountered while reading the Socket\\n\");" << std::endl;
        ioThread << "exit(1);" << std::endl;
        ioThread << "}" << std::endl;
    }else{
        throw std::runtime_error(ErrorHelpers::genErrorStr("Unknown stream type during stream I/O emit"));
    }

    if(threadDebugPrint) {
        ioThread << "printf(\"I/O Input Received\\n\");" << std::endl;
    }

    //Fill write temps with data from stream
    std::map<int, std::vector<std::pair<std::shared_ptr<ThreadCrossingFIFO>, int>>> inputPortFifoMap = getInputPortFIFOMapping(outputFIFOs); //Note, outputFIFOs are the outputs of the I/O thread.  They carry the inputs to the system to the rest of the system
    copyIOInputsToFIFO(ioThread, masterInputVars, inputPortFifoMap, linuxInputTmpName, blockSize);

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
    std::string linuxOutputTmpName = "linuxOutputTmp";
    ioThread << outputStructTypeName << " " << linuxOutputTmpName << ";" << std::endl;

    //Copy output to tmp variable
    std::map<int, std::pair<std::shared_ptr<ThreadCrossingFIFO>, int>> outputPortFifoMap = getOutputPortFIFOMapping(outputMaster);
    copyFIFOToIOOutputs(ioThread, masterOutputVars, outputPortFifoMap, outputMaster, linuxOutputTmpName, blockSize);

    if(streamType == StreamType::PIPE) {
        //Write to linux pipe
        ioThread << "int elementsWritten = fwrite(&" << linuxOutputTmpName << ", sizeof(" << outputStructTypeName << "), 1, outputPipe);" << std::endl;
        ioThread << "if (elementsWritten != 1 && ferror(outputPipe)){" << std::endl;
        ioThread << "printf(\"An error was encountered while writing the Output Linux Pipe\\n\");" << std::endl;
        ioThread << "perror(NULL);" << std::endl;
        ioThread << "exit(1);" << std::endl;
        ioThread << "} else if (elementsRead != 1){" << std::endl;
        ioThread << "printf(\"An unknown error was encountered while writing to Input Linux Pipe\\n\");" << std::endl;
        ioThread << "exit(1);" << std::endl;
        ioThread << "}" << std::endl;
    }else if(streamType == StreamType::SOCKET){
        //Write to socket
        ioThread << "int bytesSent = send(" << connectedSocketName << ", &" << linuxOutputTmpName << ", sizeof(" << outputStructTypeName << "), 0);" << std::endl;
        ioThread << "if (bytesSent == -1){" << std::endl;
        ioThread << "printf(\"An error was encountered while writing the socket\\n\");" << std::endl;
        ioThread << "perror(NULL);" << std::endl;
        ioThread << "exit(1);" << std::endl;
        ioThread << "} else if (bytesSent != sizeof(" << outputStructTypeName << ")){" << std::endl;
        ioThread << "printf(\"An unknown error was encountered while writing to socket\\n\");" << std::endl;
        ioThread << "exit(1);" << std::endl;
        ioThread << "}" << std::endl;
    }else{
        throw std::runtime_error(ErrorHelpers::genErrorStr("Unknown stream type during stream I/O emit"));
    }

    if(threadDebugPrint) {
        ioThread << "printf(\"I/O Output Sent\\n\");" << std::endl;
    }

    ioThread << "}" << std::endl; //Close if

    ioThread << "}" << std::endl; //Close while

    if(threadDebugPrint) {
        ioThread << "printf(\"Input Pipe Closed ... Exiting\\n\");" << std::endl;
    }


    //clean up streams
    if(streamType == StreamType::PIPE) {
        //The client should close the output FIFO first then the read FIFO
        //Close the pipes
        ioThread << "int closeStatus = fclose(inputPipe);" << std::endl;
        ioThread << "if(closeStatus != 0){" << std::endl;
        ioThread << "printf(\"Unable to close Linux Input Pipe\\n\");" << std::endl;
        ioThread << "}" << std::endl;
        ioThread << "closeStatus = fclose(outputPipe);" << std::endl;
        ioThread << "if(closeStatus != 0){" << std::endl;
        ioThread << "printf(\"Unable to close Linux Output Pipe\\n\");" << std::endl;
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
    }else if(streamType == StreamType::SOCKET){
        ioThread << "close(" << connectedSocketName << ");" << std::endl;
        ioThread << "close(" << listenSocketName << ");" << std::endl;
    }else{
        throw std::runtime_error(ErrorHelpers::genErrorStr("Unknown stream type during stream I/O emit"));
    }

    //Done reading
    ioThread << "return NULL;" << std::endl;
    ioThread << "}" << std::endl;

}

std::map<int, std::vector<std::pair<std::shared_ptr<ThreadCrossingFIFO>, int>>>
StreamIOThread::getInputPortFIFOMapping(std::vector<std::shared_ptr<ThreadCrossingFIFO>> inputFIFOs) {
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
StreamIOThread::getOutputPortFIFOMapping(std::shared_ptr<MasterOutput> outputMaster) {
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
StreamIOThread::getInputStructDefn(std::shared_ptr<MasterInput> inputMaster, std::string structTypeName, int blockSize) {

    std::vector<Variable> inputVariables = EmitterHelpers::getCInputVariables(inputMaster);
    return EmitterHelpers::getCIOPortStructDefn(inputVariables, structTypeName, blockSize);
}

std::string
StreamIOThread::getOutputStructDefn(std::shared_ptr<MasterOutput> outputMaster, std::string structTypeName,
                                    int blockSize) {
    std::vector<Variable> outputVars = EmitterHelpers::getCOutputVariables(outputMaster);
    return EmitterHelpers::getCIOPortStructDefn(outputVars, structTypeName, blockSize);
}

void StreamIOThread::copyIOInputsToFIFO(std::ofstream &ioThread, std::vector<Variable> masterInputVars, std::map<int, std::vector<std::pair<std::shared_ptr<ThreadCrossingFIFO>, int>>> inputPortFifoMap, std::string linuxInputPipeName, int blockSize) {
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

                if(dt.getWidth() == 1 && blockSize == 1) {
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

void StreamIOThread::copyFIFOToIOOutputs(std::ofstream &ioThread, std::vector<Variable> masterOutputVars,
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

            if(dt.getWidth() == 1 && blockSize == 1) {
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
