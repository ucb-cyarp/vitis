//
// Created by Christopher Yarp on 10/4/19.
//

#include "StreamIOThread.h"
#include <iostream>
#include <fstream>
#include <regex>
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

    std::vector<Variable>  masterInputVars = EmitterHelpers::getCInputVariables(inputMaster);
    std::vector<Variable> masterOutputVars = EmitterHelpers::getCOutputVariables(outputMaster);

    //Sort I/O varaibles into bundles - each bundle will have a separate stream
    //the default bundle is bundle 0
    std::set<int> bundles;
    std::map<int, std::vector<Variable>> masterInputBundles;
    std::map<int, std::vector<Variable>> masterOutputBundles;
    sortIntoBundles(masterInputVars, masterOutputVars,masterInputBundles,masterOutputBundles, bundles);

    //For each bundle, create a IO Port structure definition
    for(auto it = masterInputBundles.begin(); it != masterInputBundles.end(); it++){
        std::string inputStructTypeName = designName+"_inputs_bundle_"+GeneralHelper::to_string(it->first)+"_t";
        headerFile << EmitterHelpers::getCIOPortStructDefn(it->second, inputStructTypeName, blockSize) << std::endl;
        headerFile << std::endl;
    }

    for(auto it = masterOutputBundles.begin(); it != masterOutputBundles.end(); it++){
        std::string inputStructTypeName = designName+"_outputs_bundle_"+GeneralHelper::to_string(it->first)+"_t";
        headerFile << EmitterHelpers::getCIOPortStructDefn(it->second, inputStructTypeName, blockSize) << std::endl;
        headerFile << std::endl;
    }

    headerFile << "#endif" << std::endl;
    headerFile.close();

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
        ioThread << "#include <sys/select.h>" << std::endl;
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

    ioThread << std::endl;
    if(streamType == StreamType::PIPE) {
        ioThread << "//Setup Pipes" << std::endl;
        ioThread << "int status;" << std::endl;
        //Create a pipe for each bundle
        //Create Linux Pipes

        for(auto it = masterInputBundles.begin(); it != masterInputBundles.end(); it++){
            std::string inputPipeFileName = "input_bundle_"+GeneralHelper::to_string(it->first)+".pipe";
            ioThread << "status = mkfifo(\"" + inputPipeFileName + "\", S_IRUSR | S_IWUSR);" << std::endl;
            ioThread << "if(status != 0){" << std::endl;
            ioThread << "printf(\"Unable to create input pipe ... exiting\\n\");" << std::endl;
            ioThread << "perror(NULL);" << std::endl;
            ioThread << "exit(1);" << std::endl;
            ioThread << "}" << std::endl;
            ioThread << std::endl;
        }

        for(auto it = masterOutputBundles.begin(); it != masterOutputBundles.end(); it++) {
            std::string outputPipeFileName = "output_bundle_" + GeneralHelper::to_string(it->first) + ".pipe";
            ioThread << "status = mkfifo(\"" + outputPipeFileName + "\", S_IRUSR | S_IWUSR);" << std::endl;
            ioThread << "if(status != 0){" << std::endl;
            ioThread << "printf(\"Unable to create output pipe ... exiting\\n\");" << std::endl;
            ioThread << "perror(NULL);" << std::endl;
            ioThread << "exit(1);" << std::endl;
            ioThread << "}" << std::endl;
            ioThread << std::endl;
        }

        //Open Linux pipes
        for(auto it = masterInputBundles.begin(); it != masterInputBundles.end(); it++) {
            std::string inputPipeFileName = "input_bundle_"+GeneralHelper::to_string(it->first)+".pipe";
            std::string inputPipeHandleName = "inputPipe_bundle_"+GeneralHelper::to_string(it->first);
            ioThread << "FILE *" + inputPipeHandleName + " = fopen(\"" + inputPipeFileName + "\", \"rb\");" << std::endl;
            ioThread << "if(" + inputPipeHandleName + " == NULL){" << std::endl;
            ioThread << "printf(\"Unable to open input pipe ... exiting\\n\");" << std::endl;
            ioThread << "perror(NULL);" << std::endl;
            ioThread << "exit(1);" << std::endl;
            ioThread << "}" << std::endl;
            ioThread << std::endl;
        }

        for(auto it = masterOutputBundles.begin(); it != masterOutputBundles.end(); it++) {
            std::string outputPipeFileName = "output_bundle_"+GeneralHelper::to_string(it->first)+".pipe";
            std::string outputPipeHandleName = "outputPipe_bundle_"+GeneralHelper::to_string(it->first);
            ioThread << "FILE *" + outputPipeHandleName + " = fopen(\"" + outputPipeFileName + "\", \"wb\");" << std::endl;
            ioThread << "if(" + outputPipeHandleName + " == NULL){" << std::endl;
            ioThread << "printf(\"Unable to open output pipe ... exiting\\n\");" << std::endl;
            ioThread << "perror(NULL);" << std::endl;
            ioThread << "exit(1);" << std::endl;
            ioThread << "}" << std::endl;
        }
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
        //TODO: note, sockets are bidirectional.  So, input and output bundles should be combined

        for(auto it = bundles.begin(); it != bundles.end(); it++) {
            //Open a socket for each bundle.  The port number is VITIS_SOCKET_LISTEN_PORT+bundleNumber
            std::string listenSocketName = "listenSocket_bundle_" + GeneralHelper::to_string(*it);

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
            std::string settingValName = "settingVal_bundle_" + GeneralHelper::to_string(*it);
            std::string socketSettingName = "socketSetting_bundle_" + GeneralHelper::to_string(*it);

            ioThread << "int " + settingValName + " = 1;" << std::endl;
            ioThread << "int " + socketSettingName + " = setsockopt(" << listenSocketName
                     << ", SOL_SOCKET, SO_REUSEADDR, &" + settingValName + ", sizeof " + settingValName + ");" << std::endl;

            ioThread << "if(" + socketSettingName + " != 0){" << std::endl;
            ioThread << "fprintf(stderr, \"Could not set SO_REUSEADDR for listen socket ... continuing anyway\\n\");"
                     << std::endl;
            ioThread << "perror(NULL);" << std::endl;
            ioThread << "}" << std::endl;
            ioThread << std::endl;

            //Set allowed connection address/port pairs
            std::string localAddrName = "localAddr_bundle_" + GeneralHelper::to_string(*it);
            std::string socketPort = "VITIS_SOCKET_LISTEN_PORT+" + GeneralHelper::to_string(*it);

            ioThread << "struct sockaddr_in " + localAddrName + ";" << std::endl;
            ioThread << localAddrName << ".sin_family=AF_INET;" << std::endl;
            ioThread << localAddrName << ".sin_port=htons(" + socketPort + ");" << std::endl;
            ioThread << localAddrName << ".sin_addr.s_addr=htonl(VITIS_SOCKET_LISTEN_ADDR);" << std::endl;
            ioThread << "printf(\"Bundle " << *it << " Listening on port: %d\\n\", " + socketPort + ");" << std::endl;
            ioThread << "printf(\"Bundle " << *it << " Waiting for connection ...\\n\");" << std::endl;
            ioThread << std::endl;

            //Bind the socket to the address/port
            std::string bindStatusName = "bindStatus_bundle_" + GeneralHelper::to_string(*it);
            ioThread << "int " + bindStatusName + " = bind(" << listenSocketName
                     << ", (struct sockaddr *)(&" + localAddrName + "), sizeof(" + localAddrName + "));" << std::endl;

            ioThread << "if(" + bindStatusName + " !=0){" << std::endl;
            ioThread << "fprintf(stderr, \"Could not bind socket\\n\");" << std::endl;
            ioThread << "perror(NULL);" << std::endl;
            ioThread << "exit(1);" << std::endl;
            ioThread << "}" << std::endl;
            ioThread << std::endl;

            //Set the socket to listen for incoming connections (limit to a queue of length 1)
            std::string listenStatusName = "listenStatus_bundle_" + GeneralHelper::to_string(*it);
            ioThread << "int " + listenStatusName + " = listen(" << listenSocketName << ", 1);" << std::endl;
            ioThread << "if(" + listenStatusName + " !=0){" << std::endl;
            ioThread << "fprintf(stderr, \"Could not listen on socket\\n\");" << std::endl;
            ioThread << "perror(NULL);" << std::endl;
            ioThread << "exit(1);" << std::endl;
            ioThread << "}" << std::endl;
            ioThread << std::endl;
        }

        //Accept incoming connections
        //Loop until all sockets are connected
        ioThread << "int maxFD = 0;" << std::endl;
        for(auto it = bundles.begin(); it != bundles.end(); it++) {
            std::string connectedStatusName = "connectedStatus_bundle_" + GeneralHelper::to_string(*it);
            std::string listenSocketName = "listenSocket_bundle_" + GeneralHelper::to_string(*it);
            std::string connectedSocketName = "connectedSocket_bundle_" + GeneralHelper::to_string(*it);
            ioThread << "bool " << connectedStatusName << "=false;" << std::endl;
            ioThread << "if(" +listenSocketName + ">maxFD){" << std::endl;
            ioThread << "maxFD=" +listenSocketName + ";" << std::endl;
            ioThread << "}" << std::endl;
            ioThread << "int " << connectedSocketName << "=-1;" << std::endl;
        }
        ioThread << "int connectedCount = 0;" << std::endl;
        ioThread << "while(connectedCount < " << bundles.size() << "){" << std::endl;
        //see https://linux.die.net/man/3/fd_set
        ioThread << "fd_set fdSet;" << std::endl;
        ioThread << "FD_ZERO(&fdSet);" << std::endl;
        for(auto it = bundles.begin(); it != bundles.end(); it++) {
            std::string listenSocketName = "listenSocket_bundle_" + GeneralHelper::to_string(*it);
            ioThread << "FD_SET(" << listenSocketName << ", &fdSet);" << std::endl;
        }
        //See: http://beej.us/guide/bgnet/html/multi/selectman.html
        ioThread << "int selectStatus = select(maxFD, &fdSet, NULL, NULL, NULL);" << std::endl;
        ioThread << "if(selectStatus == -1){" << std::endl;
        ioThread << "fprintf(stderr, \"Error while waiting for connections\\n\");" << std::endl;
        ioThread << "perror(NULL);" << std::endl;
        ioThread << "exit(1);" << std::endl;
        ioThread << "}" << std::endl;

        for(auto it = bundles.begin(); it != bundles.end(); it++) {
            std::string listenSocketName = "listenSocket_bundle_" + GeneralHelper::to_string(*it);
            std::string connectedSocketName = "connectedSocket_bundle_" + GeneralHelper::to_string(*it);
            std::string connectedStatusName = "connectedStatus_bundle_" + GeneralHelper::to_string(*it);
            ioThread << "if(!" << connectedStatusName << " && FD_ISSET(" << listenSocketName << ", &fdSet)){" << std::endl;
            ioThread << "struct sockaddr socketAdr;" << std::endl;
            ioThread << "socklen_t socketPort = sizeof(socketAdr);" << std::endl;
            ioThread << connectedSocketName << " = accept(" << listenSocketName << ", &socketAdr, &socketPort);"
                     << std::endl;
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
            //Using getpeername to get the name of the connected client
            //https://stackoverflow.com/questions/20472072/c-socket-get-ip-address-from-filedescriptor-returned-from-accept/20475352
            //http://man7.org/linux/man-pages/man2/getpeername.2.html
            //https://beej.us/guide/bgnet/html/multi/getpeernameman.html
            ioThread << "struct sockaddr_storage clientAddr;" << std::endl;
            ioThread << "struct sockaddr *clientAddrCastGeneral = (struct sockaddr *) &clientAddr;" << std::endl;
            ioThread << "socklen_t clientAddrSize = sizeof(clientAddr);" << std::endl;
            ioThread << "int peerNameStatus = getpeername(" << connectedSocketName
                     << ", clientAddrCastGeneral, &clientAddrSize);" << std::endl;
            ioThread << "if(peerNameStatus == 0) {" << std::endl;
            ioThread << "if(clientAddr.ss_family != AF_INET){" << std::endl;
            ioThread << "fprintf(stderr, \"Unexpected connection address type\\n\");" << std::endl;
            ioThread << "}else {" << std::endl;
            ioThread << "struct sockaddr_in *clientAddrCast = (struct sockaddr_in *) &clientAddr;" << std::endl;
            ioThread << "char connectionAddrStr[INET_ADDRSTRLEN];" << std::endl;
            ioThread << "char* connectionAddrStrPtr = &(connectionAddrStr[0]);" << std::endl;
            ioThread
                    << "const char* nameStr = inet_ntop(AF_INET, &clientAddrCast->sin_addr, connectionAddrStrPtr, INET_ADDRSTRLEN);"
                    << std::endl;
            ioThread << "if(nameStr != NULL) {" << std::endl;
            ioThread << "printf(\"Bundle " << *it << " Connection from %s:%d\\n\", nameStr, ntohs(clientAddrCast->sin_port));" << std::endl;
            ioThread << "}" << std::endl;
            ioThread << "}" << std::endl;
            ioThread << "}else{" << std::endl;
            ioThread << "fprintf(stderr, \"Unable to get client address\\n\");" << std::endl;
            ioThread << "perror(NULL);" << std::endl;
            ioThread << "}" << std::endl;
            ioThread << connectedStatusName << "=true;" << std::endl;
            ioThread << "connectedCount++;" << std::endl;
            ioThread << "}" << std::endl; //End connection if
        }
        ioThread << "}" << std::endl; //Close connect while
    }else{
        throw std::runtime_error(ErrorHelpers::genErrorStr("Unknown stream type during stream I/O emit"));
    }

    ioThread << std::endl;
    ioThread << "//Thread loop" << std::endl;

    ioThread << "while(true){" << std::endl;
    //Allocate temp Memory for linux pipe read

    for(auto it = masterInputBundles.begin(); it != masterInputBundles.end(); it++) {
        //This process needs to be repeated for each input bundle
        //TODO: This currently forces reading in a specific order, consider changing this?
        std::string linuxInputTmpName = "linuxInputTmp_bundle_"+GeneralHelper::to_string(it->first);
        std::string inputStructTypeName = designName+"_inputs_bundle_"+GeneralHelper::to_string(it->first)+"_t";

        ioThread << inputStructTypeName << " " << linuxInputTmpName << ";" << std::endl;

        //Read data from the input pipe
        ioThread << "{" << std::endl; //Open a scope for reading
        if (streamType == StreamType::PIPE) {
            std::string inputPipeHandleName = "inputPipe_bundle_"+GeneralHelper::to_string(it->first);
            ioThread << "int elementsRead = fread(&" << linuxInputTmpName << ", sizeof(" << inputStructTypeName
                     << "), 1, " << inputPipeHandleName << ");" << std::endl;
            ioThread << "if(elementsRead != 1 && feof(" << inputPipeHandleName << ")){" << std::endl;
            ioThread << "//Done with input (input pipe closed)" << std::endl;
            ioThread << "break;" << std::endl;
            ioThread << "} else if (elementsRead != 1 && ferror(" << inputPipeHandleName << ")){" << std::endl;
            ioThread << "printf(\"An error was encountered while reading the Input Linux Pipe\\n\");" << std::endl;
            ioThread << "perror(NULL);" << std::endl;
            ioThread << "exit(1);" << std::endl;
            ioThread << "} else if (elementsRead != 1){" << std::endl;
            ioThread << "printf(\"An unknown error was encountered while reading the Input Linux Pipe\\n\");"
                     << std::endl;
            ioThread << "exit(1);" << std::endl;
            ioThread << "}" << std::endl;
        } else if (streamType == StreamType::SOCKET) {
            //Dectecting a closed socket is covered in the following
            //https://stackoverflow.com/questions/3091010/recv-socket-function-returning-data-with-length-as-0
            //https://linux.die.net/man/3/recv
            //Read should block, especially if in MSG_WAITALL mode
            std::string connectedSocketName = "connectedSocket_bundle_" + GeneralHelper::to_string(it->first);
            ioThread << "int bytesRead = recv(" << connectedSocketName << ", &" << linuxInputTmpName << ", sizeof("
                     << inputStructTypeName << "), MSG_WAITALL);" << std::endl;
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
        } else {
            throw std::runtime_error(ErrorHelpers::genErrorStr("Unknown stream type during stream I/O emit"));
        }
        ioThread << "}" << std::endl; //Close the scope for reading
    }

    if(threadDebugPrint) {
        ioThread << "printf(\"I/O Input Received\\n\");" << std::endl;
    }

    //Fill write temps with data from stream
    std::map<int, std::vector<std::pair<std::shared_ptr<ThreadCrossingFIFO>, int>>> inputPortFifoMap = getInputPortFIFOMapping(outputFIFOs); //Note, outputFIFOs are the outputs of the I/O thread.  They carry the inputs to the system to the rest of the system
    for(auto it = masterInputBundles.begin(); it != masterInputBundles.end(); it++) {
        std::string linuxInputTmpName = "linuxInputTmp_bundle_"+GeneralHelper::to_string(it->first);
        copyIOInputsToFIFO(ioThread, it->second, inputPortFifoMap, linuxInputTmpName, blockSize);
    }

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
    for(auto it = masterOutputBundles.begin(); it != masterOutputBundles.end(); it++) {
        std::string linuxOutputTmpName = "linuxOutputTmp_bundle_"+GeneralHelper::to_string(it->first);
        std::string outputStructTypeName = designName+"_outputs_bundle_"+GeneralHelper::to_string(it->first)+"_t";
        ioThread << outputStructTypeName << " " << linuxOutputTmpName << ";" << std::endl;
    }

    //Copy output to tmp variable
    std::map<int, std::pair<std::shared_ptr<ThreadCrossingFIFO>, int>> outputPortFifoMap = getOutputPortFIFOMapping(outputMaster);
    for(auto it = masterOutputBundles.begin(); it != masterOutputBundles.end(); it++) {
        std::string linuxOutputTmpName = "linuxOutputTmp_bundle_"+GeneralHelper::to_string(it->first);
        copyFIFOToIOOutputs(ioThread, it->second, outputPortFifoMap, outputMaster, linuxOutputTmpName, blockSize);
    }

    for(auto it = masterOutputBundles.begin(); it != masterOutputBundles.end(); it++) {
        std::string linuxOutputTmpName = "linuxOutputTmp_bundle_"+GeneralHelper::to_string(it->first);
        std::string outputStructTypeName = designName+"_outputs_bundle_"+GeneralHelper::to_string(it->first)+"_t";

        ioThread << "{" << std::endl; //Open a scope for writing
        if (streamType == StreamType::PIPE) {
            //Write to linux pipe
            std::string outputPipeHandleName = "outputPipe_bundle_"+GeneralHelper::to_string(it->first);
            ioThread << "int elementsWritten = fwrite(&" << linuxOutputTmpName << ", sizeof(" << outputStructTypeName
                     << "), 1, " << outputPipeHandleName << ");" << std::endl;
            ioThread << "fflush(" << outputPipeHandleName << ");" << std::endl;
            ioThread << "if (elementsWritten != 1 && ferror(" << outputPipeHandleName << ")){" << std::endl;
            ioThread << "printf(\"An error was encountered while writing the Output Linux Pipe\\n\");" << std::endl;
            ioThread << "perror(NULL);" << std::endl;
            ioThread << "exit(1);" << std::endl;
            ioThread << "} else if (elementsWritten != 1){" << std::endl;
            ioThread << "printf(\"An unknown error was encountered while writing to Input Linux Pipe\\n\");"
                     << std::endl;
            ioThread << "exit(1);" << std::endl;
            ioThread << "}" << std::endl;
        } else if (streamType == StreamType::SOCKET) {
            //Write to socket
            std::string connectedSocketName = "connectedSocket_bundle_" + GeneralHelper::to_string(it->first);
            ioThread << "int bytesSent = send(" << connectedSocketName << ", &" << linuxOutputTmpName << ", sizeof("
                     << outputStructTypeName << "), 0);" << std::endl;
            ioThread << "if (bytesSent == -1){" << std::endl;
            ioThread << "printf(\"An error was encountered while writing the socket\\n\");" << std::endl;
            ioThread << "perror(NULL);" << std::endl;
            ioThread << "exit(1);" << std::endl;
            ioThread << "} else if (bytesSent != sizeof(" << outputStructTypeName << ")){" << std::endl;
            ioThread << "printf(\"An unknown error was encountered while writing to socket\\n\");" << std::endl;
            ioThread << "exit(1);" << std::endl;
            ioThread << "}" << std::endl;
        } else {
            throw std::runtime_error(ErrorHelpers::genErrorStr("Unknown stream type during stream I/O emit"));
        }
        ioThread << "}" << std::endl; //Close writing scope
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
        ioThread << "int closeStatus;" << std::endl;
        for(auto it = masterInputBundles.begin(); it != masterInputBundles.end(); it++) {
            std::string inputPipeHandleName = "inputPipe_bundle_"+GeneralHelper::to_string(it->first);
            ioThread << "closeStatus = fclose(" << inputPipeHandleName << ");" << std::endl;
            ioThread << "if(closeStatus != 0){" << std::endl;
            ioThread << "printf(\"Unable to close Linux Input Pipe\\n\");" << std::endl;
            ioThread << "}" << std::endl;
        }

        for(auto it = masterOutputBundles.begin(); it != masterOutputBundles.end(); it++) {
            std::string outputPipeHandleName = "outputPipe_bundle_"+GeneralHelper::to_string(it->first);
            ioThread << "closeStatus = fclose(" + outputPipeHandleName + ");" << std::endl;
            ioThread << "if(closeStatus != 0){" << std::endl;
            ioThread << "printf(\"Unable to close Linux Output Pipe\\n\");" << std::endl;
            ioThread << "}" << std::endl;
        }

        //Delete the pipes
        for(auto it = masterInputBundles.begin(); it != masterInputBundles.end(); it++) {
            std::string inputPipeFileName = "input_bundle_"+GeneralHelper::to_string(it->first)+".pipe";
            ioThread << "closeStatus = unlink(\"" << inputPipeFileName << "\");" << std::endl;
            ioThread << "if (closeStatus != 0){" << std::endl;
            ioThread << "printf(\"Could not delete the Linux Input Pipe\\n\");" << std::endl;
            ioThread << "perror(NULL);" << std::endl;
            ioThread << "}" << std::endl;
        }

        for(auto it = masterOutputBundles.begin(); it != masterOutputBundles.end(); it++) {
            std::string outputPipeFileName = "output_bundle_" + GeneralHelper::to_string(it->first) + ".pipe";
            ioThread << "closeStatus = unlink(\"" << outputPipeFileName << "\");" << std::endl;
            ioThread << "if (closeStatus != 0){" << std::endl;
            ioThread << "printf(\"Could not delete the Linux Output Pipe\\n\");" << std::endl;
            ioThread << "perror(NULL);" << std::endl;
            ioThread << "}" << std::endl;
        }
    }else if(streamType == StreamType::SOCKET){
        for(auto it = bundles.begin(); it != bundles.end(); it++) {
            std::string listenSocketName = "listenSocket_bundle_" + GeneralHelper::to_string(*it);
            std::string connectedSocketName = "connectedSocket_bundle_" + GeneralHelper::to_string(*it);
            ioThread << "int closeStatus = close(" << connectedSocketName << ");" << std::endl;
            ioThread << "if (closeStatus != 0){" << std::endl;
            ioThread << "printf(\"Could not close the connection socket\\n\");" << std::endl;
            ioThread << "perror(NULL);" << std::endl;
            ioThread << "}" << std::endl;
            ioThread << "closeStatus = close(" << listenSocketName << ");" << std::endl;
            ioThread << "if (closeStatus != 0){" << std::endl;
            ioThread << "printf(\"Could not close the listen socket\\n\");" << std::endl;
            ioThread << "perror(NULL);" << std::endl;
            ioThread << "}" << std::endl;
        }
    }else{
        throw std::runtime_error(ErrorHelpers::genErrorStr("Unknown stream type during stream I/O emit"));
    }

    //Done reading
    ioThread << "return NULL;" << std::endl;
    ioThread << "}" << std::endl;
    ioThread.close();
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

void StreamIOThread::emitSocketClientLib(std::shared_ptr<MasterInput> inputMaster, std::shared_ptr<MasterOutput> outputMaster, std::string path, std::string fileNamePrefix, std::string fifoHeaderFile, std::string designName) {
    std::string serverFilenamePostfix = "io_network_socket";
    std::string serverFileName = fileNamePrefix + "_" + serverFilenamePostfix;

    std::string filenamePostfix = "io_network_socket_client";
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
    headerFile << "#define VITIS_SOCKET_LISTEN_PORT " << DEFAULT_VITIS_SOCKET_LISTEN_PORT << std::endl;
    headerFile << "#include \"" << VITIS_TYPE_NAME << ".h\"" << std::endl;
    headerFile << "#include \"" << serverFileName << ".h\"" << std::endl;
    headerFile << "#include \"" << fifoHeaderFile << "\"" << std::endl;
    headerFile << std::endl;

    std::vector<Variable> masterInputVars = EmitterHelpers::getCInputVariables(inputMaster);
    std::vector<Variable> masterOutputVars = EmitterHelpers::getCOutputVariables(outputMaster);

    std::set<int> bundles;
    std::map<int, std::vector<Variable>> masterInputBundles;
    std::map<int, std::vector<Variable>> masterOutputBundles;
    sortIntoBundles(masterInputVars, masterOutputVars, masterInputBundles, masterOutputBundles, bundles);

    //Only need one of the connect/disconnect functions
    //Output function prototypes for the socket client
    headerFile << "//For connecting to the remote system" << std::endl;
    std::string connectFctnDecl = "int " + designName + "_" + filenamePostfix + "_connect(char* ipAddrStr)";
    headerFile << connectFctnDecl << ";" << std::endl;
    headerFile << std::endl;

    //Output function prototypes for the socket client
    headerFile << "//For disconnecting from the remote system" << std::endl;
    std::string disconnectFctnDecl = "void " + designName + "_" + filenamePostfix + "_disconnect(int socket)";
    headerFile << disconnectFctnDecl << ";" << std::endl;
    headerFile << std::endl;

    for(auto it = masterInputBundles.begin(); it != masterInputBundles.end(); it++) {
        //Need one for each input port bundle
        std::string inputStructTypeName = designName+"_inputs_bundle_"+GeneralHelper::to_string(it->first)+"_t";
        headerFile << "//For sending data to the remote system" << std::endl;
        std::string sendFctnDcl = "void " + designName + "_" + filenamePostfix + "_bundle_" + GeneralHelper::to_string(it->first) + "_send(int socket, " + inputStructTypeName + " *toSend)";
        headerFile << sendFctnDcl << ";" << std::endl;
        headerFile << std::endl;
    }

    for(auto it = masterOutputBundles.begin(); it != masterOutputBundles.end(); it++) {
        //Need one for each output port bundle
        std::string outputStructTypeName = designName+"_outputs_bundle_"+GeneralHelper::to_string(it->first)+"_t";
        headerFile << "//For receiving data from the remote system" << std::endl;
        headerFile << "//Returns true if data received, false if socket has been closed" << std::endl;
        std::string recvFctnDcl = "bool " + designName + "_" + filenamePostfix + "_bundle_" + GeneralHelper::to_string(it->first) + "_recv(int socket, " + outputStructTypeName + " *toRecv)";
        headerFile << recvFctnDcl << ";" << std::endl;
        headerFile << std::endl;
    }
    headerFile << "#endif" << std::endl;
    headerFile.close();

    //#### Emit .c File ####
    std::cout << "Emitting C File: " << path << "/" << fileName << ".c" << std::endl;
    std::ofstream ioThread;
    ioThread.open(path+"/"+fileName+".c", std::ofstream::out | std::ofstream::trunc);

    ioThread << "#include <stdio.h>" << std::endl;
    ioThread << "#include <stdlib.h>" << std::endl;
    ioThread << "#include <string.h>" << std::endl;
    ioThread << "#include <sys/stat.h>" << std::endl;
    ioThread << "#include <unistd.h>" << std::endl;
    ioThread << "#include <sys/types.h>" << std::endl;
    ioThread << "#include <sys/socket.h>" << std::endl;
    ioThread << "#include <netinet/in.h>" << std::endl;
    ioThread << "#include <arpa/inet.h>" << std::endl;
    ioThread << "#include \"" << fileName << ".h" << "\"" << std::endl;
    ioThread << "#ifndef VITIS_CLIENT_PRINTF" << std::endl;
    ioThread << "//Redefine if matlab to ssPrintf or mexPrintf" << std::endl;
    ioThread << "#define VITIS_CLIENT_PRINTF printf" << std::endl;
    ioThread << "#endif" << std::endl;
    ioThread << std::endl;

    ioThread << connectFctnDecl << "{" << std::endl;
    ioThread << "//Setup Socket Connection" << std::endl;
    //Open a socket for IPv4 and a Socket Byte Stream (bidirectional byte stream)
    std::string connectSocketName = "connectSock";
    ioThread << "int " << connectSocketName << " = socket(AF_INET, SOCK_STREAM, 0);" << std::endl;
    ioThread << "if(" << connectSocketName << " == -1){" << std::endl;
    ioThread << "VITIS_CLIENT_PRINTF(\"Could not create socket\\n\");" << std::endl;
    ioThread << "perror(NULL);" << std::endl;
    ioThread << "exit(1);" << std::endl;
    ioThread << "}" << std::endl;
    ioThread << std::endl;

    //Set connection
    ioThread << "struct sockaddr_in connectAddr;" << std::endl;
    ioThread << "connectAddr.sin_family=AF_INET;" << std::endl;
    ioThread << "connectAddr.sin_port=htons(VITIS_SOCKET_LISTEN_PORT);" << std::endl;
    ioThread << "connectAddr.sin_addr.s_addr=inet_addr(ipAddrStr);" << std::endl;
    ioThread << "VITIS_CLIENT_PRINTF(\"Connecting to addr: %s:%d\\n\", ipAddrStr, VITIS_SOCKET_LISTEN_PORT);" << std::endl;
    ioThread << std::endl;

    ioThread << "int connectStatus = connect(" << connectSocketName << ", (struct sockaddr *)(&connectAddr), sizeof(connectAddr));" << std::endl;
    ioThread << "if(connectStatus == -1){" << std::endl;
    ioThread << "VITIS_CLIENT_PRINTF(\"Unable to connect\\n\");" << std::endl;
    ioThread << "perror(NULL);" << std::endl;
    ioThread << "exit(1);" << std::endl;
    ioThread << "}" << std::endl;
    ioThread << "VITIS_CLIENT_PRINTF(\"Connected!\\n\");" << std::endl;
    ioThread << "return " << connectSocketName << ";" << std::endl;
    ioThread << "}" << std::endl;
    ioThread << std::endl;

    ioThread << disconnectFctnDecl << "{" << std::endl;
    ioThread << "int closeStatus = close(socket);" << std::endl;
    ioThread << "if (closeStatus != 0){" << std::endl;
    ioThread << "VITIS_CLIENT_PRINTF(\"Could not close the socket\\n\");" << std::endl;
    ioThread << "perror(NULL);" << std::endl;
    ioThread << "}" << std::endl;
    ioThread << "VITIS_CLIENT_PRINTF(\"Disconnected\\n\");" << std::endl;
    ioThread << "}" << std::endl;
    ioThread << std::endl;

    for(auto it = masterInputBundles.begin(); it != masterInputBundles.end(); it++) {
        std::string inputStructTypeName = designName+"_inputs_bundle_"+GeneralHelper::to_string(it->first)+"_t";
        std::string sendFctnDcl = "void " + designName + "_" + filenamePostfix + "_bundle_" + GeneralHelper::to_string(it->first) + "_send(int socket, " + inputStructTypeName + " *toSend)";

        ioThread << sendFctnDcl << "{" << std::endl;
        ioThread << "int bytesSent = send(socket, toSend, sizeof(" << inputStructTypeName << "), 0);" << std::endl;
        ioThread << "if (bytesSent == -1){" << std::endl;
        ioThread << "VITIS_CLIENT_PRINTF(\"An error was encountered while writing the socket\\n\");" << std::endl;
        ioThread << "perror(NULL);" << std::endl;
        ioThread << "exit(1);" << std::endl;
        ioThread << "} else if (bytesSent != sizeof(" << inputStructTypeName << ")){" << std::endl;
        ioThread << "VITIS_CLIENT_PRINTF(\"An unknown error was encountered while writing to socket\\n\");"
                 << std::endl;
        ioThread << "exit(1);" << std::endl;
        ioThread << "}" << std::endl;
        ioThread << "}" << std::endl;
        ioThread << std::endl;
    }

    for(auto it = masterOutputBundles.begin(); it != masterOutputBundles.end(); it++) {
        std::string outputStructTypeName = designName+"_outputs_bundle_"+GeneralHelper::to_string(it->first)+"_t";
        std::string recvFctnDcl = "bool " + designName + "_" + filenamePostfix + "_bundle_" + GeneralHelper::to_string(it->first) + "_recv(int socket, " + outputStructTypeName + " *toRecv)";

        ioThread << recvFctnDcl << "{" << std::endl;
        ioThread << "int bytesRead = recv(socket, toRecv, sizeof(" << outputStructTypeName << "), MSG_WAITALL);"
                 << std::endl;
        ioThread << "if(bytesRead == 0){" << std::endl;
        ioThread << "//Done with input (socket closed)" << std::endl;
        ioThread << "return false;" << std::endl;
        ioThread << "} else if (bytesRead == -1){" << std::endl;
        ioThread << "VITIS_CLIENT_PRINTF(\"An error was encountered while reading the socket\\n\");" << std::endl;
        ioThread << "perror(NULL);" << std::endl;
        ioThread << "exit(1);" << std::endl;
        ioThread << "} else if (bytesRead != sizeof(" << outputStructTypeName << ")){" << std::endl;
        ioThread << "VITIS_CLIENT_PRINTF(\"An unknown error was encountered while reading the Socket\\n\");"
                 << std::endl;
        ioThread << "exit(1);" << std::endl;
        ioThread << "}" << std::endl;
        ioThread << "return true;" << std::endl;
        ioThread << "}" << std::endl;
        ioThread << std::endl;
    }
    ioThread.close();
}

void StreamIOThread::sortIntoBundles(std::vector<Variable> masterInputVars, std::vector<Variable> masterOutputVars,
                                      std::map<int, std::vector<Variable>> &masterInputBundles,
                                      std::map<int, std::vector<Variable>> &masterOutputBundles, std::set<int> &bundles) {
    //Sort I/O varaibles into bundles - each bundle will have a separate stream
    //the default bundle is bundle 0

    std::string suffix = ".*_BUNDLE_([0-9]+)_.*";

    for (int i = 0; i < masterInputVars.size(); i++) {
        //Check the suffix of the variable name
        std::string varName = masterInputVars[i].getName();
        std::regex suffixRegexExpr(suffix);
        std::smatch matches;
        bool matched = std::regex_match(varName, matches, suffixRegexExpr);
        if (matched) {
            //Goes in the specified bundle
            int tgtBundle = std::stoi(matches[1]);
            masterInputBundles[tgtBundle].push_back(masterInputVars[i]);
            bundles.insert(tgtBundle);
        } else {
            //Goes in bundle 0
            masterInputBundles[0].push_back(masterInputVars[i]);
            bundles.insert(0);
        }
    }

    for (int i = 0; i < masterOutputVars.size(); i++) {
        //Check the suffix of the variable name
        std::string varName = masterOutputVars[i].getName();
        std::regex suffixRegexExpr(suffix);
        std::smatch matches;
        bool matched = std::regex_match(varName, matches, suffixRegexExpr);
        if (matched) {
            //Goes in the specified bundle
            int tgtBundle = std::stoi(matches[1]);
            masterOutputBundles[tgtBundle].push_back(masterOutputVars[i]);
            bundles.insert(tgtBundle);
        } else {
            //Goes in bundle 0
            masterOutputBundles[0].push_back(masterOutputVars[i]);
            bundles.insert(0);
        }
    }
}