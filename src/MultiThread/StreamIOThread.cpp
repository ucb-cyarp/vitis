//
// Created by Christopher Yarp on 10/4/19.
//

#include "StreamIOThread.h"
#include <iostream>
#include <fstream>
#include <regex>
#include "General/GeneralHelper.h"
#include "Emitter/MultiThreadEmit.h"
#include "General/ErrorHelpers.h"
#include "General/EmitterHelpers.h"
#include "MasterNodes/MasterOutput.h"
#include "MasterNodes/MasterInput.h"

void StreamIOThread::emitStreamIOThreadC(std::shared_ptr<MasterInput> inputMaster,
                                         std::shared_ptr<MasterOutput> outputMaster,
                                         std::vector<std::shared_ptr<ThreadCrossingFIFO>> inputFIFOs,
                                         std::vector<std::shared_ptr<ThreadCrossingFIFO>> outputFIFOs,
                                         std::string path, std::string fileNamePrefix, std::string designName,
                                         StreamType streamType, unsigned long blockSize, std::string fifoHeaderFile,
                                         std::string fifoSupportFile,
                                         int32_t ioFifoSize, //The size of the FIFO in blocks for the shared memory FIFO
                                         bool threadDebugPrint, bool printTelem,
                                         EmitterHelpers::TelemetryLevel telemLevel,
                                         int telemReportFreqBlockFreq, double telemReportPeriodSeconds,
                                         std::string telemDumpFilePrefix, bool telemAvg,
                                         PartitionParams::FIFOIndexCachingBehavior fifoIndexCachingBehavior,
                                         std::string streamNameSuffix) {

    bool collectTelem = EmitterHelpers::ioShouldCollectTelemetry(telemLevel);
    bool collectBreakdownTelem = EmitterHelpers::ioTelemetryBreakdown(telemLevel);

    unsigned long blockSizeBase = blockSize;

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
        case StreamType::POSIX_SHARED_MEM:
            filenamePostfix = "io_posix_shared_mem";
            break;
        default:
            throw std::runtime_error(ErrorHelpers::genErrorStr("Unknown stream type during stream I/O emit"));
    }

    std::string sharedFIFOHelperHeaderName;
    if(streamType == StreamType::POSIX_SHARED_MEM){
        //Emit the helper files
        sharedFIFOHelperHeaderName = emitSharedMemoryFIFOHelperFiles(path);
    }

    std::string fileName = fileNamePrefix + "_" + filenamePostfix;

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

    if(threadDebugPrint) {
        includesHFile.insert("#include <stdio.h>");
    }
    if(streamType == StreamType::SOCKET){
        includesHFile.insert("#define VITIS_SOCKET_LISTEN_PORT " + GeneralHelper::to_string(DEFAULT_VITIS_SOCKET_LISTEN_PORT));
        includesHFile.insert("#define VITIS_SOCKET_LISTEN_ADDR " + GeneralHelper::to_string(DEFAULT_VITIS_SOCKET_LISTEN_ADDR));
    }else if(streamType == StreamType::POSIX_SHARED_MEM){
        includesHFile.insert("#include \"" + sharedFIFOHelperHeaderName + "\"");
    }
    includesHFile.insert("#include \"" + GeneralHelper::to_string(VITIS_TYPE_NAME) + ".h\"");
    includesHFile.insert("#include \"" + fifoHeaderFile + "\"");

    //Include any external include statements required by nodes in the design
    for(int i = 0; i<inputFIFOs.size(); i++){
        std::set<std::string> nodeIncludes = inputFIFOs[i]->getExternalIncludes();
        includesHFile.insert(nodeIncludes.begin(), nodeIncludes.end());
    }

    for(int i = 0; i<outputFIFOs.size(); i++){
        std::set<std::string> nodeIncludes = outputFIFOs[i]->getExternalIncludes();
        includesHFile.insert(nodeIncludes.begin(), nodeIncludes.end());
    }

    for(auto it = includesHFile.begin(); it != includesHFile.end(); it++){
        headerFile << *it << std::endl;
    }

    headerFile << std::endl;

    //Create the threadFunction argument structure for the I/O thread (includes the references to FIFO shared vars)
    std::pair<std::string, std::string> threadArgStructAndTypeName = MultiThreadEmit::getCThreadArgStructDefn(inputFIFOs, outputFIFOs, designName, IO_PARTITION_NUM);
    std::string threadArgStruct = threadArgStructAndTypeName.first;
    std::string threadArgTypeName = threadArgStructAndTypeName.second;
    headerFile << threadArgStruct << std::endl;
    headerFile << std::endl;

    //Output the function prototype for the I/O thread function
    std::string threadFctnDecl = "void* " + designName + "_" + filenamePostfix + "_thread(void *args)";
    headerFile << std::endl;
    headerFile << threadFctnDecl << ";" << std::endl;
    headerFile << std::endl;

    std::pair<std::vector<Variable>, std::vector<std::pair<int, int>>> masterInputVarRatePairs = EmitterHelpers::getCInputVariables(inputMaster);
    std::pair<std::vector<Variable>, std::vector<std::pair<int, int>>> masterOutputVarRatePairs = EmitterHelpers::getCOutputVariables(outputMaster);

    std::vector<Variable> masterInputVars = masterInputVarRatePairs.first;
    std::vector<Variable> masterOutputVars = masterOutputVarRatePairs.first;

    std::vector<int> masterInputVarBlockSizes = EmitterHelpers::getBlockSizesFromRates(masterInputVarRatePairs.second, blockSize);
    std::vector<int> masterOutputVarBlockSizes = EmitterHelpers::getBlockSizesFromRates(masterOutputVarRatePairs.second, blockSize);

    //Sort I/O varaibles into bundles - each bundle will have a separate stream
    //the default bundle is bundle 0
    std::set<int> bundles;
    std::map<int, std::pair<std::vector<Variable>, std::vector<int>>> masterInputBundles;
    std::map<int, std::pair<std::vector<Variable>, std::vector<int>>> masterOutputBundles;
    sortIntoBundles(masterInputVars, masterOutputVars, masterInputVarBlockSizes, masterOutputVarBlockSizes, masterInputBundles, masterOutputBundles, bundles);

    //For each bundle, create a IO Port structure definition

    for(auto it = masterInputBundles.begin(); it != masterInputBundles.end(); it++){
        std::string inputStructTypeName = designName+"_inputs_bundle_"+GeneralHelper::to_string(it->first)+"_t";
        std::vector<int> blkSizes = it->second.second;

        std::string bundleBlockSizeName = GeneralHelper::toUpper(designName) + "_INPUT_BUNDLE" + GeneralHelper::to_string(it->first) + "_BLOCKSIZE";

        int blockSize = 0;
        for(unsigned long i = 0; i<blkSizes.size(); i++){
            if(i == 0){
                blockSize = blkSizes[i];
            }else{
                if(blockSize != blkSizes[i]){
                    std::cerr << "WARNING: A port of input bundle " << GeneralHelper::to_string(it->first) << " has a different rate than the other ports. " << bundleBlockSizeName << " may be unreliable as an indication of the block size" << std::endl;
                }
            }
        }

        headerFile << "#define " << bundleBlockSizeName << " (" << blockSize << ")" << std::endl;
        headerFile << EmitterHelpers::getCIOPortStructDefn(it->second.first, it->second.second, inputStructTypeName) << std::endl;
        headerFile << std::endl;
    }

    for(auto it = masterOutputBundles.begin(); it != masterOutputBundles.end(); it++){
        std::string outputStructTypeName = designName+"_outputs_bundle_"+GeneralHelper::to_string(it->first)+"_t";
        std::vector<int> blkSizes = it->second.second;

        std::string bundleBlockSizeName = GeneralHelper::toUpper(designName) + "_OUTPUT_BUNDLE" + GeneralHelper::to_string(it->first) + "_BLOCKSIZE";

        int blockSize = 0;
        for(unsigned long i = 0; i<blkSizes.size(); i++){
            if(i == 0){
                blockSize = blkSizes[i];
            }else{
                if(blockSize != blkSizes[i]){
                    std::cerr << "WARNING: A port of output bundle " << GeneralHelper::to_string(it->first) << " has a different rate than the other ports. " << bundleBlockSizeName << " may be unreliable as an indication of the block size" << std::endl;
                }
            }
        }

        headerFile << "#define " << bundleBlockSizeName << " (" << blockSize << ")" << std::endl;

        headerFile << EmitterHelpers::getCIOPortStructDefn(it->second.first, it->second.second, outputStructTypeName) << std::endl;
        headerFile << std::endl;
    }

    headerFile << "#endif" << std::endl;
    headerFile.close();

    //#### Emit .c File ####
    std::cout << "Emitting C File: " << path << "/" << fileName << ".c" << std::endl;
    std::ofstream ioThread;
    ioThread.open(path+"/"+fileName+".c", std::ofstream::out | std::ofstream::trunc);

    if(collectTelem){
        ioThread << "#ifndef _GNU_SOURCE" << std::endl;
        ioThread << "#define _GNU_SOURCE //For clock_gettime" << std::endl;
        ioThread << "#endif" << std::endl;
    }

    std::set<std::string> includesCFile;
    includesCFile.insert("#include <stdio.h>");
    includesCFile.insert("#include <stdlib.h>");
    includesCFile.insert("#include <string.h>");
    includesCFile.insert("#include <sys/stat.h>");
    includesCFile.insert("#include <unistd.h>");

    if(streamType == StreamType::SOCKET) {
        includesCFile.insert("#include <sys/types.h>");
        includesCFile.insert("#include <sys/socket.h>");
        includesCFile.insert("#include <netinet/in.h>");
        includesCFile.insert("#include <arpa/inet.h>");
        includesCFile.insert("#include <sys/select.h>");
    }

    includesCFile.insert("#include <time.h>");
    includesCFile.insert("#include \"" + fileName + ".h\"");
    if(streamType == StreamType::SOCKET || streamType == StreamType::PIPE) {
        includesCFile.insert("#include \"" + fileNamePrefix + "_filestream_helpers.h\""); //For File I/O helpers
    }
    if(collectTelem){
        includesCFile.insert("#include \"" + fileNamePrefix + "_telemetry_helpers.h\"");
    }

    if(!fifoSupportFile.empty()){
        includesCFile.insert("#include \"" + fifoSupportFile + "\"");
    }

    //Include any external include statements required by nodes in the design
    for(int i = 0; i<inputFIFOs.size(); i++){
        std::set<std::string> nodeIncludes = inputFIFOs[i]->getExternalIncludes();
        includesCFile.insert(nodeIncludes.begin(), nodeIncludes.end());
    }

    for(int i = 0; i<outputFIFOs.size(); i++){
        std::set<std::string> nodeIncludes = outputFIFOs[i]->getExternalIncludes();
        includesCFile.insert(nodeIncludes.begin(), nodeIncludes.end());
    }

    for(auto it = includesCFile.begin(); it != includesCFile.end(); it++){
        ioThread << *it << std::endl;
    }

    ioThread << std::endl;

    ioThread << threadFctnDecl << "{" << std::endl;

    if(collectTelem){
        ioThread << "timespec_t timeResolution;" << std::endl;
        ioThread << "clock_getres(CLOCK_MONOTONIC, &timeResolution);" << std::endl;
        ioThread << "double timeResolutionDouble = timespecToDouble(&timeResolution);" << std::endl;
        if(printTelem) {
            ioThread << "printf(\"Time Resolution: %8.6e\\n\", timeResolutionDouble);" << std::endl;
        }

        if(!telemDumpFilePrefix.empty()){
            ioThread << "FILE* telemDumpFile = fopen(\"" << telemDumpFilePrefix << "IO.csv\", \"w\");" << std::endl;

            //Write Header Row
            ioThread << "fprintf(telemDumpFile, \"TimeStamp_s,TimeStamp_ns,Rate_msps";
            if(collectBreakdownTelem){
                ioThread << ",timeReadingExtFIFO_s,timeWaitingForFIFOsToCompute_s,timeWritingFIFOsToCompute_s,timeWaitingForFIFOsFromCompute_s,timeReadingFIFOsFromCompute_s,timeWritingExtFIFO_s";
            }

            ioThread << ",TotalTime_s";

//            if(collectPAPI){
//                cFile << ",clock_cycles,instructions_retired,floating_point_operations_retired,vector_instructions_retired,l1_data_cache_accesses";
//            }
            ioThread << "\\n\");" << std::endl;
        }
    }

    //Copy shared variables from the input argument structure
    ioThread << MultiThreadEmit::emitCopyCThreadArgs(inputFIFOs, outputFIFOs, "args", threadArgTypeName);

    //Create temp entries for outputs and initialize them with the constants
    std::vector<std::string> tmpWriteDecls = MultiThreadEmit::createFIFOWriteTemps(outputFIFOs);
    for(int i = 0; i<tmpWriteDecls.size(); i++){
        ioThread << tmpWriteDecls[i] << std::endl;
    }

    //Create the input FIFO temps
    std::vector<std::string> tmpReadDecls = MultiThreadEmit::createFIFOReadTemps(inputFIFOs);
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
            std::string inputPipeFileName = "input_bundle_"+GeneralHelper::to_string(it->first)+streamNameSuffix+".pipe";
            ioThread << "status = mkfifo(\"" + inputPipeFileName + "\", S_IRUSR | S_IWUSR);" << std::endl;
            ioThread << "if(status != 0){" << std::endl;
            ioThread << "printf(\"Unable to create input pipe ... exiting\\n\");" << std::endl;
            ioThread << "perror(NULL);" << std::endl;
            ioThread << "exit(1);" << std::endl;
            ioThread << "}" << std::endl;
            ioThread << std::endl;
        }

        for(auto it = masterOutputBundles.begin(); it != masterOutputBundles.end(); it++) {
            std::string outputPipeFileName = "output_bundle_" + GeneralHelper::to_string(it->first) + streamNameSuffix + ".pipe";
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
            std::string inputPipeFileName = "input_bundle_"+GeneralHelper::to_string(it->first)+streamNameSuffix+".pipe";
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
            std::string outputPipeFileName = "output_bundle_"+GeneralHelper::to_string(it->first)+streamNameSuffix+".pipe";
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
        ioThread << "int selectStatus = select(maxFD+1, &fdSet, NULL, NULL, NULL);" << std::endl;
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

            //Since we are using the AF_INET family, the structure used is sockaddr_in
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
            ioThread << "int peerNameStatus = getpeername(" << connectedSocketName << ", clientAddrCastGeneral, &clientAddrSize);" << std::endl;
            ioThread << "if(peerNameStatus == 0) {" << std::endl;
            ioThread << "if(clientAddr.ss_family != AF_INET){" << std::endl;
            ioThread << "fprintf(stderr, \"Unexpected connection address type\\n\");" << std::endl;
            ioThread << "}else {" << std::endl;
            ioThread << "struct sockaddr_in *clientAddrCast = (struct sockaddr_in *) &clientAddr;" << std::endl;
            ioThread << "char connectionAddrStr[INET_ADDRSTRLEN];" << std::endl;
            ioThread << "char* connectionAddrStrPtr = &(connectionAddrStr[0]);" << std::endl;
            ioThread << "const char* nameStr = inet_ntop(AF_INET, &clientAddrCast->sin_addr, connectionAddrStrPtr, INET_ADDRSTRLEN);" << std::endl;
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
    }else if(streamType == StreamType::POSIX_SHARED_MEM) {
        ioThread << "//Setup FIFOs" << std::endl;
        //Create a pipe for each bundle
        //Producers should be initialized first - therefore, we start with outputs
        //Open Output FIFOs
        for(auto it = masterOutputBundles.begin(); it != masterOutputBundles.end(); it++) {
            std::string outputSharedName = designName+"_output_bundle_"+GeneralHelper::to_string(it->first)+streamNameSuffix;
            std::string outputFifoHandleName = "outputFIFO_bundle_"+GeneralHelper::to_string(it->first);
            std::string outputFIFOSizeName = outputFifoHandleName+"_fifoSize";
            std::string outputStructTypeName = designName+"_outputs_bundle_"+GeneralHelper::to_string(it->first)+"_t";
            ioThread << "sharedMemoryFIFO_t " + outputFifoHandleName + ";" << std::endl;
            ioThread << "initSharedMemoryFIFO(&" + outputFifoHandleName + ");" << std::endl;
            ioThread << "size_t " << outputFIFOSizeName << " = sizeof(" << outputStructTypeName << ")*" << ioFifoSize << ";" << std::endl;
            ioThread << "producerOpenInitFIFO(\"" << outputSharedName << "\", " << outputFIFOSizeName << ", &" << outputFifoHandleName << ");" << std::endl;
        }

        //Open Input FIFOs
        for(auto it = masterInputBundles.begin(); it != masterInputBundles.end(); it++) {
            std::string inputSharedName = designName+"_input_bundle_"+GeneralHelper::to_string(it->first)+streamNameSuffix;
            std::string inputFifoHandleName = "inputFIFO_bundle_"+GeneralHelper::to_string(it->first);
            std::string inputFIFOSizeName = inputFifoHandleName+"_fifoSize";
            std::string inputStructTypeName = designName+"_inputs_bundle_"+GeneralHelper::to_string(it->first)+"_t";
            ioThread << "sharedMemoryFIFO_t " + inputFifoHandleName + ";" << std::endl;
            ioThread << "initSharedMemoryFIFO(&" + inputFifoHandleName + ");" << std::endl;
            ioThread << "size_t " << inputFIFOSizeName << " = sizeof(" << inputStructTypeName << ")*" << ioFifoSize << ";" << std::endl;
            ioThread << "consumerOpenFIFOBlock(\"" << inputSharedName << "\", " << inputFIFOSizeName << ", &" << inputFifoHandleName << ");" << std::endl;
        }
    }else{
        throw std::runtime_error(ErrorHelpers::genErrorStr("Unknown stream type during stream I/O emit"));
    }

    if(collectTelem) {
        ioThread << "//Init timer" << std::endl;
        ioThread << "uint64_t rxSamples = 0;" << std::endl;
        ioThread << "timespec_t startTime;" << std::endl;
        ioThread << "asm volatile (\"\" ::: \"memory\"); //Stop Re-ordering of timer" << std::endl;
        ioThread << "clock_gettime(CLOCK_MONOTONIC, &startTime);" << std::endl;
        ioThread << "asm volatile (\"\" ::: \"memory\"); //Stop Re-ordering of timer" << std::endl;
        ioThread << "timespec_t lastPrint;" << std::endl;
        ioThread << "asm volatile (\"\" ::: \"memory\"); //Stop Re-ordering of timer" << std::endl;
        ioThread << "clock_gettime(CLOCK_MONOTONIC, &lastPrint);" << std::endl;
        ioThread << "asm volatile (\"\" ::: \"memory\"); //Stop Re-ordering of timer" << std::endl;
        if(collectBreakdownTelem) {
            ioThread << "double timeTotal = 0;" << std::endl;
            ioThread << "double timeReadingExtFIFO = 0;" << std::endl;
            ioThread << "double timeWaitingForFIFOsToCompute = 0;" << std::endl;
            ioThread << "double timeWritingFIFOsToCompute = 0;" << std::endl;
            ioThread << "double timeWaitingForFIFOsFromCompute = 0;" << std::endl;
            ioThread << "double timeReadingFIFOsFromCompute = 0;" << std::endl;
            ioThread << "double timeWritingExtFIFO = 0;" << std::endl;
        }
        ioThread << "bool collectTelem = false;" << std::endl;
        ioThread << "int telemCheckCount = 0;" << std::endl;
    }

    ioThread << std::endl;
    ioThread << "//Create I/O Status Vars and Buffers" << std::endl;

    for(auto it = masterInputBundles.begin(); it != masterInputBundles.end(); it++) {
        //This process needs to be repeated for each input bundle
        //TODO: This currently forces reading in a specific order, consider changing this?
        std::string linuxInputTmpName = "linuxInputTmp_bundle_"+GeneralHelper::to_string(it->first);
        std::string inputStructTypeName = designName+"_inputs_bundle_"+GeneralHelper::to_string(it->first)+"_t";
        std::string extInputBufferFilledName = "extInputBufferFilled_"+GeneralHelper::to_string(it->first);
        std::string toComputeFIFOFilledName = "toComputeFIFOFilled_"+GeneralHelper::to_string(it->first);

        ioThread << inputStructTypeName << " " << linuxInputTmpName << ";" << std::endl;
        ioThread << "bool " << extInputBufferFilledName << " = false;" << std::endl;
        ioThread << "bool " <<  toComputeFIFOFilledName << " = false;" << std::endl;

    }
    ioThread << std::endl;

    for(auto it = masterOutputBundles.begin(); it != masterOutputBundles.end(); it++) {
        std::string linuxOutputTmpName = "linuxOutputTmp_bundle_"+GeneralHelper::to_string(it->first);
        std::string outputStructTypeName = designName+"_outputs_bundle_"+GeneralHelper::to_string(it->first)+"_t";
        std::string fromComputeFIFOFilledName = "fromComputeFIFOFilled_"+GeneralHelper::to_string(it->first);
        std::string extOutputBufferFilledName = "extOutputBufferFilled_"+GeneralHelper::to_string(it->first);

        ioThread << outputStructTypeName << " " << linuxOutputTmpName << ";" << std::endl;

        ioThread << "bool " << fromComputeFIFOFilledName << " = false;" << std::endl;
        ioThread << "bool " << extOutputBufferFilledName << " = false;" << std::endl;
    }
    ioThread << std::endl;

    //Create Local FIFO Vars
    std::vector<std::string> cachedVarDeclsInputFIFOs = MultiThreadEmit::createAndInitFIFOLocalVars(
            inputFIFOs);
    for(unsigned long i = 0; i<cachedVarDeclsInputFIFOs.size(); i++){
        ioThread << cachedVarDeclsInputFIFOs[i] << std::endl;
    }

    std::vector<std::string> cachedVarDeclsOutputFIFOs = MultiThreadEmit::createAndInitFIFOLocalVars(
            outputFIFOs);
    for(unsigned long i = 0; i<cachedVarDeclsOutputFIFOs.size(); i++){
        ioThread << cachedVarDeclsOutputFIFOs[i] << std::endl;
    }

    ioThread << std::endl;

    ioThread << "//Thread loop" << std::endl;
    ioThread << "while(true){" << std::endl;
    //Allocate temp Memory for linux pipe read
    //++++ External Input to Compute ++++

    if(collectBreakdownTelem) {
        ioThread << "timespec_t readingFromExtStart;" << std::endl;
        ioThread << "asm volatile (\"\" ::: \"memory\"); //Stop Re-ordering of timer" << std::endl;
        ioThread << "clock_gettime(CLOCK_MONOTONIC, &readingFromExtStart);" << std::endl;
        ioThread << "asm volatile (\"\" ::: \"memory\"); //Stop Re-ordering of timer" << std::endl;
    }

    std::map<int, std::vector<std::pair<std::shared_ptr<ThreadCrossingFIFO>, int>>> inputPortFifoMap = getInputPortFIFOMapping(outputFIFOs); //Note, outputFIFOs are the outputs of the I/O thread.  They carry the inputs to the system to the compute threads

    ioThread << std::endl;
    for(auto it = masterInputBundles.begin(); it != masterInputBundles.end(); it++) {
        //This process needs to be repeated for each input bundle
        //TODO: This currently forces reading in a specific order, consider changing this?
        std::string linuxInputTmpName = "linuxInputTmp_bundle_"+GeneralHelper::to_string(it->first);
        std::string inputStructTypeName = designName+"_inputs_bundle_"+GeneralHelper::to_string(it->first)+"_t";
        std::string extInputBufferFilledName = "extInputBufferFilled_"+GeneralHelper::to_string(it->first);
        std::string toComputeFIFOFilledName = "toComputeFIFOFilled_"+GeneralHelper::to_string(it->first);

        //Copy ext input into the compute buffer if the external input buffer has data but the computeFIFO buffer does not
        //This allows a read from the external stream to occur if availible
        ioThread << "//Copy Between Input Buffers" << std::endl;
        ioThread << "if(" << extInputBufferFilledName << " && !" << toComputeFIFOFilledName << "){" << std::endl;

        copyIOInputsToFIFO(ioThread, it->second.first, inputPortFifoMap, linuxInputTmpName, it->second.second);
        ioThread << extInputBufferFilledName << " = false;" << std::endl;
        ioThread << toComputeFIFOFilledName << " = true;" << std::endl;
        ioThread << "}" << std::endl;

        ioThread << std::endl;
        ioThread << "//Check if space available for input" << std::endl;
        //Read data from the input pipe if data is availible and the buffer is free
        ioThread << "if(!" << extInputBufferFilledName << "){" << std::endl;
        //Check if FIFO is free
        if (streamType == StreamType::PIPE) {
            std::string inputPipeHandleName = "inputPipe_bundle_"+GeneralHelper::to_string(it->first);
            ioThread << "bool extDataAvail = isReadyForReading(" << inputPipeHandleName << ");" << std::endl;
        } else if (streamType == StreamType::SOCKET) {
            std::string connectedSocketName = "connectedSocket_bundle_" + GeneralHelper::to_string(it->first);
            ioThread << "bool extDataAvail = isReadyForReadingFD(" << connectedSocketName << ");" << std::endl;
        } else if (streamType == StreamType::POSIX_SHARED_MEM){
            std::string inputFifoHandleName = "inputFIFO_bundle_"+GeneralHelper::to_string(it->first);
            ioThread << "bool extDataAvail = isReadyForReading(&" << inputFifoHandleName << ");" << std::endl;
        } else {
            throw std::runtime_error(ErrorHelpers::genErrorStr("Unknown stream type during stream I/O emit"));
        }

        ioThread << std::endl;
        ioThread << "//Check if data is available from external input" << std::endl;
        ioThread << "if(extDataAvail){" << std::endl; //If data is availible, perform the blocking read
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
        } else if (streamType == StreamType::POSIX_SHARED_MEM){
            std::string inputFifoHandleName = "inputFIFO_bundle_"+GeneralHelper::to_string(it->first);
            ioThread << "int elementsRead = readFifo(&" << linuxInputTmpName << ", sizeof(" << inputStructTypeName
                     << "), 1, &" << inputFifoHandleName << ");" << std::endl;
            ioThread << "if(elementsRead != 1){" << std::endl;
            ioThread << "//Done with input (input pipe closed)" << std::endl;
            ioThread << "break;" << std::endl;
            ioThread << "}" << std::endl;
        } else {
            throw std::runtime_error(ErrorHelpers::genErrorStr("Unknown stream type during stream I/O emit"));
        }

        //This read is blocking so can set the receive status unconditionally
        ioThread << extInputBufferFilledName << " = true;" << std::endl;

        //Only counts the data received for the one of the bundles
        if(collectTelem && it == masterInputBundles.begin()) {
            ioThread << "rxSamples += " << blockSizeBase << ";" << std::endl;
        }

        if(threadDebugPrint) {
            ioThread << "printf(\"I/O Input Received on bundle " << GeneralHelper::to_string(it->first) << "\\n\");" << std::endl;
        }

        ioThread << "}" << std::endl; //Close the scope for if data was availible
        ioThread << "}" << std::endl; //Close the scope for if the buffer was free
    }

    //This is a special case where the duration for this cycle is calculated later (after reporting).  That way,
    //each metric has undergone the same number of cycles
    if(collectBreakdownTelem) {
        ioThread << "timespec_t readingFromExtStop;" << std::endl;
        ioThread << "asm volatile (\"\" ::: \"memory\"); //Stop Re-ordering of timer" << std::endl;
        ioThread << "clock_gettime(CLOCK_MONOTONIC, &readingFromExtStop);" << std::endl;
        ioThread << "asm volatile (\"\" ::: \"memory\"); //Stop Re-ordering of timer" << std::endl;
    }

    if(collectTelem) {
        //Emit timer reporting
        ioThread << "if(telemCheckCount > " << telemReportFreqBlockFreq << "){" << std::endl;
        ioThread << "timespec_t currentTime;" << std::endl;
        ioThread << "asm volatile (\"\" ::: \"memory\"); //Stop Re-ordering of timer" << std::endl;
        ioThread << "clock_gettime(CLOCK_MONOTONIC, &currentTime);" << std::endl;
        ioThread << "asm volatile (\"\" ::: \"memory\"); //Stop Re-ordering of timer" << std::endl;
        ioThread << "double duration = difftimespec(&currentTime, &lastPrint);" << std::endl;
        ioThread << std::endl;
        ioThread << "//Display Telemetry" << std::endl;
        ioThread << "if(duration >= " << telemReportPeriodSeconds << "){" << std::endl;
        ioThread << "lastPrint = currentTime;" << std::endl;
        ioThread << "double durationSinceStart = difftimespec(&currentTime, &startTime);" << std::endl;
        ioThread << "double rateMSps = ((double)rxSamples)/durationSinceStart/1000000;" << std::endl;
        if(collectBreakdownTelem) {
            ioThread << "double durationTelemMisc = durationSinceStart-timeTotal;" << std::endl;
        }

        if(printTelem) {
            ioThread << "printf(\"Current " << designName << " Rate: %10.5f\\n\"" << std::endl;
            if (collectBreakdownTelem) {
                ioThread << "\"\\tWaiting/Reading/Shuffle I/O FIFOs: %10.5f (%8.4f%%)\\n\"" << std::endl;
                ioThread << "\"\\tWaiting For FIFOs to Compute:      %10.5f (%8.4f%%)\\n\"" << std::endl;
                ioThread << "\"\\tWriting FIFOs to Compute:          %10.5f (%8.4f%%)\\n\"" << std::endl;
                ioThread << "\"\\tWaiting For FIFOs from Compute:    %10.5f (%8.4f%%)\\n\"" << std::endl;
                ioThread << "\"\\tReading FIFOs from Compute:        %10.5f (%8.4f%%)\\n\"" << std::endl;
                ioThread << "\"\\tWaiting/Shuffle/Writing I/O FIFOs: %10.5f (%8.4f%%)\\n\"" << std::endl;
                ioThread << "\"\\tTelemetry/Misc:                    %10.5f (%8.4f%%)\\n\"" << std::endl;
            }
            ioThread << ", rateMSps";
            if (collectBreakdownTelem) {
                ioThread << ", timeReadingExtFIFO, timeReadingExtFIFO/durationSinceStart*100, ";
                ioThread << "timeWaitingForFIFOsToCompute, timeWaitingForFIFOsToCompute/durationSinceStart*100, ";
                ioThread << "timeWritingFIFOsToCompute, timeWritingFIFOsToCompute/durationSinceStart*100, ";
                ioThread << "timeWaitingForFIFOsFromCompute, timeWaitingForFIFOsFromCompute/durationSinceStart*100, ";
                ioThread << "timeReadingFIFOsFromCompute, timeReadingFIFOsFromCompute/durationSinceStart*100, ";
                ioThread << "timeWritingExtFIFO, timeWritingExtFIFO/durationSinceStart*100, ";
                ioThread << "durationTelemMisc, durationTelemMisc/durationSinceStart*100";
            }
            ioThread << ");" << std::endl;
        }

        if (!telemDumpFilePrefix.empty()) {
            //Write the telemetry to the file
            //The file includes the timestamp at the time it was written.  This is used to align telemetry from multiple threads
            //The partition number is included in the filename and is not written to the file
            ioThread << "fprintf(telemDumpFile, \"%ld,%ld,%e";
            if(collectBreakdownTelem){
                ioThread << ",%e,%e,%e,%e,%e,%e,%e";
            }
            ioThread << ",%e";

//            if(collectPAPI) {
//                cFile << ",%lld,%lld,%lld,%lld,%lld";
//            }
            ioThread << "\\n\", currentTime.tv_sec, currentTime.tv_nsec, rateMSps";

            if(collectBreakdownTelem){
                ioThread << ", timeReadingExtFIFO, timeWaitingForFIFOsToCompute,"
                            "timeWritingFIFOsToCompute, timeWaitingForFIFOsFromCompute, "
                            "timeReadingFIFOsFromCompute, timeWritingExtFIFO, durationTelemMisc";
            }

            ioThread << ", durationSinceStart";

//            if(collectPAPI) {
//                cFile << ",clock_cycles,instructions_retired,floating_point_operations_retired,vector_instructions_retired,l1_data_cache_accesses";
//            }
            ioThread << ");" << std::endl;

            //flush the file
            ioThread << "fflush(telemDumpFile);" << std::endl;
            ioThread << std::endl;
        }

        if (!telemAvg) {
            //Reset the counters for the next collection interval.
            ioThread << "startTime = currentTime;" << std::endl;
            ioThread << "rxSamples = 0;" << std::endl;
            if(collectBreakdownTelem) {
                ioThread << "timeTotal = 0;" << std::endl;
                ioThread << "timeReadingExtFIFO = 0;" << std::endl;
                ioThread << "timeWaitingForFIFOsToCompute = 0;" << std::endl;
                ioThread << "timeWritingFIFOsToCompute = 0;" << std::endl;
                ioThread << "timeWaitingForFIFOsFromCompute = 0;" << std::endl;
                ioThread << "timeReadingFIFOsFromCompute = 0;" << std::endl;
                ioThread << "timeWritingExtFIFO = 0;" << std::endl;
            }
        }
        ioThread << "}" << std::endl;
        ioThread << "telemCheckCount = 0;" << std::endl;
        ioThread << "}else{" << std::endl;
        ioThread << "telemCheckCount++;" << std::endl;
        ioThread << "}" << std::endl;

        if(collectBreakdownTelem) {
            //Now, finish timeReadingExtFIFO
            ioThread << "double readingFromExtDuration = difftimespec(&readingFromExtStop, &readingFromExtStart);" << std::endl;
            ioThread << "timeReadingExtFIFO += readingFromExtDuration;" << std::endl;
            ioThread << "timeTotal += readingFromExtDuration;" << std::endl;
        }
    }

    //Copying to the  is also part of reading from the external input
    if(collectBreakdownTelem) {
        ioThread << "asm volatile (\"\" ::: \"memory\"); //Stop Re-ordering of timer" << std::endl;
        ioThread << "clock_gettime(CLOCK_MONOTONIC, &readingFromExtStart);" << std::endl;
        ioThread << "asm volatile (\"\" ::: \"memory\"); //Stop Re-ordering of timer" << std::endl;
    }

    ioThread << std::endl;

    //NOTE: Because access to FIFOs is non-blocking and buffering is handled differently in I/O, we will force
    //TODO: Inspect if this becomes the bottleneck

    //Fill write temps with data from stream if there is room in the buffer and data is availible
    for(auto it = masterInputBundles.begin(); it != masterInputBundles.end(); it++) {
        std::string linuxInputTmpName = "linuxInputTmp_bundle_"+GeneralHelper::to_string(it->first);
        std::string extInputBufferFilledName = "extInputBufferFilled_"+GeneralHelper::to_string(it->first);
        std::string toComputeFIFOFilledName = "toComputeFIFOFilled_"+GeneralHelper::to_string(it->first);

        //Copy ext input into the compute buffer if possible
        ioThread << "//Copy Between Input Buffers" << std::endl;
        ioThread << "if(" << extInputBufferFilledName << " && !" << toComputeFIFOFilledName << "){" << std::endl;
        copyIOInputsToFIFO(ioThread, it->second.first, inputPortFifoMap, linuxInputTmpName, it->second.second);
        ioThread << extInputBufferFilledName << " = false;" << std::endl;
        ioThread << toComputeFIFOFilledName << " = true;" << std::endl;
        ioThread << "}" << std::endl;
    }

    if(collectBreakdownTelem) {
        ioThread << "asm volatile (\"\" ::: \"memory\"); //Stop Re-ordering of timer" << std::endl;
        ioThread << "clock_gettime(CLOCK_MONOTONIC, &readingFromExtStop);" << std::endl;
        ioThread << "asm volatile (\"\" ::: \"memory\"); //Stop Re-ordering of timer" << std::endl;
        ioThread << "readingFromExtDuration = difftimespec(&readingFromExtStop, &readingFromExtStart);" << std::endl;
        ioThread << "timeReadingExtFIFO += readingFromExtDuration;" << std::endl;
        ioThread << "timeTotal += readingFromExtDuration;" << std::endl;
    }

    //TODO: In the future make access to each FIFO independent (ie. if a FIFO is ready and data is available, write it even if other FIFOs are not ready or do not have data.  The current issue is that a given FIFO may have data from more than 1 external input

    if(collectBreakdownTelem) {
        ioThread << "timespec_t waitingForFIFOsToComputeStart;" << std::endl;
        ioThread << "asm volatile (\"\" ::: \"memory\"); //Stop Re-ordering of timer" << std::endl;
        ioThread << "clock_gettime(CLOCK_MONOTONIC, &waitingForFIFOsToComputeStart);" << std::endl;
        ioThread << "asm volatile (\"\" ::: \"memory\"); //Stop Re-ordering of timer" << std::endl;
    }

    //Only check output FIFO if data is available (construct the check here)
    ioThread << "//Check if data to be sent to compute" << std::endl;
    ioThread << "bool toComputeFIFOFilled_all = ";
    for(auto it = masterInputBundles.begin(); it != masterInputBundles.end(); it++) {
        std::string toComputeFIFOFilledName = "toComputeFIFOFilled_" + GeneralHelper::to_string(it->first);
        if(it != masterInputBundles.begin()){
            ioThread << " && ";
        }
        ioThread << toComputeFIFOFilledName;
    }
    ioThread << ";" << std::endl;

    if (collectBreakdownTelem) {
        ioThread << "timespec_t waitingForFIFOsToComputeStop;" << std::endl;
        ioThread << "asm volatile (\"\" ::: \"memory\"); //Stop Re-ordering of timer" << std::endl;
        ioThread << "clock_gettime(CLOCK_MONOTONIC, &waitingForFIFOsToComputeStop);" << std::endl;
        ioThread << "asm volatile (\"\" ::: \"memory\"); //Stop Re-ordering of timer" << std::endl;
        ioThread << "double waitingForIntFIFOWriteDuration = difftimespec(&waitingForFIFOsToComputeStop, &waitingForFIFOsToComputeStart);" << std::endl;
        ioThread << "timeWaitingForFIFOsToCompute += waitingForIntFIFOWriteDuration;" << std::endl;
        ioThread << "timeTotal += waitingForIntFIFOWriteDuration;" << std::endl;
    }
    //Check if data is availible to be written into FIFOs to compute
    ioThread << "if(toComputeFIFOFilled_all){" << std::endl;
    //Check if Room in Output FIFOs

    if(collectBreakdownTelem) {
        ioThread << "timespec_t waitingForFIFOsToComputeStart;" << std::endl;
        ioThread << "asm volatile (\"\" ::: \"memory\"); //Stop Re-ordering of timer" << std::endl;
        ioThread << "clock_gettime(CLOCK_MONOTONIC, &waitingForFIFOsToComputeStart);" << std::endl;
        ioThread << "asm volatile (\"\" ::: \"memory\"); //Stop Re-ordering of timer" << std::endl;
    }

    ioThread << "//Check if room in FIFOs to compute" << std::endl;
    ioThread << MultiThreadEmit::emitFIFOChecks(outputFIFOs, true, "outputFIFOsReady", false, false, false, fifoIndexCachingBehavior); //Only need a pthread_testcancel check on one FIFO check since this is nonblocking

    if (collectBreakdownTelem) {
        ioThread << "timespec_t waitingForFIFOsToComputeStop;" << std::endl;
        ioThread << "asm volatile (\"\" ::: \"memory\"); //Stop Re-ordering of timer" << std::endl;
        ioThread << "clock_gettime(CLOCK_MONOTONIC, &waitingForFIFOsToComputeStop);" << std::endl;
        ioThread << "asm volatile (\"\" ::: \"memory\"); //Stop Re-ordering of timer" << std::endl;
        ioThread << "double waitingForIntFIFOWriteDuration = difftimespec(&waitingForFIFOsToComputeStop, &waitingForFIFOsToComputeStart);" << std::endl;
        ioThread << "timeWaitingForFIFOsToCompute += waitingForIntFIFOWriteDuration;" << std::endl;
        ioThread << "timeTotal += waitingForIntFIFOWriteDuration;" << std::endl;
    }

    ioThread << "if(outputFIFOsReady){" << std::endl;

    if (collectBreakdownTelem) {
        ioThread << "timespec_t writingFIFOsToComputeStart;" << std::endl;
        ioThread << "asm volatile (\"\" ::: \"memory\"); //Stop Re-ordering of timer" << std::endl;
        ioThread << "clock_gettime(CLOCK_MONOTONIC, &writingFIFOsToComputeStart);" << std::endl;
        ioThread << "asm volatile (\"\" ::: \"memory\"); //Stop Re-ordering of timer" << std::endl;
    }

    //Write FIFOs
    ioThread << "//Write FIFOs to compute" << std::endl;
    std::vector<std::string> writeFIFOExprs = MultiThreadEmit::writeFIFOsFromTemps(outputFIFOs, false, true, true);
    for (int i = 0; i < writeFIFOExprs.size(); i++) {
        ioThread << writeFIFOExprs[i] << std::endl;
    }

    //Set status flags
    for(auto it = masterInputBundles.begin(); it != masterInputBundles.end(); it++) {
        std::string toComputeFIFOFilledName = "toComputeFIFOFilled_" + GeneralHelper::to_string(it->first);
        ioThread << toComputeFIFOFilledName << " = false;" << std::endl;
    }

    if (collectBreakdownTelem) {
        ioThread << "timespec_t writingFIFOsToComputeStop;" << std::endl;
        ioThread << "asm volatile (\"\" ::: \"memory\"); //Stop Re-ordering of timer" << std::endl;
        ioThread << "clock_gettime(CLOCK_MONOTONIC, &writingFIFOsToComputeStop);" << std::endl;
        ioThread << "asm volatile (\"\" ::: \"memory\"); //Stop Re-ordering of timer" << std::endl;
        ioThread
                << "double writingFIFOsToComputeDuration = difftimespec(&writingFIFOsToComputeStop, &writingFIFOsToComputeStart);"
                << std::endl;
        ioThread << "timeWritingFIFOsToCompute += writingFIFOsToComputeDuration;" << std::endl;
        ioThread << "timeTotal += writingFIFOsToComputeDuration;" << std::endl;
    }

    if(threadDebugPrint) {
        ioThread << "printf(\"I/O Passed to Compute\\n\");" << std::endl;
    }

    ioThread << "}" << std::endl; //Close writing to FIFO if ready
    ioThread << "}" << std::endl; //Close if data available to write

    //++++ From Compute to External Output ++++
    ioThread << std::endl; //Close if data available to write

    //Check input FIFOs
    if(collectBreakdownTelem) {
        ioThread << "timespec_t waitingForFIFOsFromComputeStart;" << std::endl;
        ioThread << "asm volatile (\"\" ::: \"memory\"); //Stop Re-ordering of timer" << std::endl;
        ioThread << "clock_gettime(CLOCK_MONOTONIC, &waitingForFIFOsFromComputeStart);" << std::endl;
        ioThread << "asm volatile (\"\" ::: \"memory\"); //Stop Re-ordering of timer" << std::endl;
    }

    //Only check input FIFOs if there is buffer space
    ioThread << "//Check if buffer space available for data from compute" << std::endl;
    ioThread << "bool fromComputeFIFOFilled_none = ";
    for(auto it = masterOutputBundles.begin(); it != masterOutputBundles.end(); it++) {
        std::string fromComputeFIFOFilledName = "fromComputeFIFOFilled_"+GeneralHelper::to_string(it->first);
        if(it != masterOutputBundles.begin()){
            ioThread << " && ";
        }
        ioThread << "!" << fromComputeFIFOFilledName;
    }
    ioThread << ";" << std::endl;

    if(collectBreakdownTelem) {
        ioThread << "timespec_t waitingForFIFOsFromComputeStop;" << std::endl;
        ioThread << "asm volatile (\"\" ::: \"memory\"); //Stop Re-ordering of timer" << std::endl;
        ioThread << "clock_gettime(CLOCK_MONOTONIC, &waitingForFIFOsFromComputeStop);" << std::endl;
        ioThread << "asm volatile (\"\" ::: \"memory\"); //Stop Re-ordering of timer" << std::endl;
        ioThread << "double waitingForFIFOsFromComputeDuration = difftimespec(&waitingForFIFOsFromComputeStop, &waitingForFIFOsFromComputeStart);" << std::endl;
        ioThread << "timeWaitingForFIFOsFromCompute += waitingForFIFOsFromComputeDuration;" << std::endl;
        ioThread << "timeTotal += waitingForFIFOsFromComputeDuration;" << std::endl;
    }

    //Check if buffer space is available
    ioThread << "if(fromComputeFIFOFilled_none){" << std::endl;

    //TODO: In the future make access to each FIFO independent (ie. if a FIFO is ready and data is available, write it even if other FIFOs are not ready or do not have data.  The current issue is that a given FIFO may have data from more than 1 external input

    if(collectBreakdownTelem) {
        ioThread << "timespec_t waitingForFIFOsFromComputeStart;" << std::endl;
        ioThread << "asm volatile (\"\" ::: \"memory\"); //Stop Re-ordering of timer" << std::endl;
        ioThread << "clock_gettime(CLOCK_MONOTONIC, &waitingForFIFOsFromComputeStart);" << std::endl;
        ioThread << "asm volatile (\"\" ::: \"memory\"); //Stop Re-ordering of timer" << std::endl;
    }

    ioThread << "//Check data available from compute" << std::endl;
    ioThread << MultiThreadEmit::emitFIFOChecks(inputFIFOs, false, "inputFIFOsReady", false, false, false, fifoIndexCachingBehavior); //pthread_testcancel check here

    if(collectBreakdownTelem) {
        ioThread << "asm volatile (\"\" ::: \"memory\"); //Stop Re-ordering of timer" << std::endl;
        ioThread << "clock_gettime(CLOCK_MONOTONIC, &waitingForFIFOsFromComputeStop);" << std::endl;
        ioThread << "asm volatile (\"\" ::: \"memory\"); //Stop Re-ordering of timer" << std::endl;
        ioThread << "double waitingForFIFOsFromComputeDuration = difftimespec(&waitingForFIFOsFromComputeStop, &waitingForFIFOsFromComputeStart);" << std::endl;
        ioThread << "timeWaitingForFIFOsFromCompute += waitingForFIFOsFromComputeDuration;" << std::endl;
        ioThread << "timeTotal += waitingForFIFOsFromComputeDuration;" << std::endl;
    }

    ioThread << "if(inputFIFOsReady){" << std::endl;
    //Data availible on FIFOs and room in buffers, read

    //Read input FIFOs
    ioThread << "//Read data from compute" << std::endl;
    if(collectBreakdownTelem) {
        ioThread << "timespec_t readingFIFOsFromComputeStart;" << std::endl;
        ioThread << "asm volatile (\"\" ::: \"memory\"); //Stop Re-ordering of timer" << std::endl;
        ioThread << "clock_gettime(CLOCK_MONOTONIC, &readingFIFOsFromComputeStart);" << std::endl;
        ioThread << "asm volatile (\"\" ::: \"memory\"); //Stop Re-ordering of timer" << std::endl;
    }

    std::vector<std::string> readFIFOExprs = MultiThreadEmit::readFIFOsToTemps(inputFIFOs, false, true, true);
    for(int i = 0; i<readFIFOExprs.size(); i++){
        ioThread << readFIFOExprs[i] << std::endl;
    }

    //Set status flags
    for(auto it = masterOutputBundles.begin(); it != masterOutputBundles.end(); it++) {
        std::string fromComputeFIFOFilledName = "fromComputeFIFOFilled_"+GeneralHelper::to_string(it->first);
        ioThread << fromComputeFIFOFilledName << " = true;" << std::endl;
    }

    if(collectBreakdownTelem) {
        ioThread << "timespec_t readingFIFOsFromComputeStop;" << std::endl;
        ioThread << "asm volatile (\"\" ::: \"memory\"); //Stop Re-ordering of timer" << std::endl;
        ioThread << "clock_gettime(CLOCK_MONOTONIC, &readingFIFOsFromComputeStop);" << std::endl;
        ioThread << "asm volatile (\"\" ::: \"memory\"); //Stop Re-ordering of timer" << std::endl;
        ioThread << "double readingFIFOsFromComputeDuration = difftimespec(&readingFIFOsFromComputeStop, &readingFIFOsFromComputeStart);" << std::endl;
        ioThread << "timeReadingFIFOsFromCompute += readingFIFOsFromComputeDuration;" << std::endl;
        ioThread << "timeTotal += readingFIFOsFromComputeDuration;" << std::endl;
    }

    if(threadDebugPrint) {
        ioThread << "printf(\"I/O Received from Compute\\n\");" << std::endl;
    }

    ioThread << "}" << std::endl; //End if data available on FIFOs
    ioThread << "}" << std::endl; //End if buffer space available

    //Copy output to tmp variable if possible (considered part of writing to ExtFIFO)
    if(collectBreakdownTelem) {
        ioThread << "timespec_t writingExtFIFOStart;" << std::endl;
        ioThread << "asm volatile (\"\" ::: \"memory\"); //Stop Re-ordering of timer" << std::endl;
        ioThread << "clock_gettime(CLOCK_MONOTONIC, &writingExtFIFOStart);" << std::endl;
        ioThread << "asm volatile (\"\" ::: \"memory\"); //Stop Re-ordering of timer" << std::endl;
    }

    std::map<int, std::pair<std::shared_ptr<ThreadCrossingFIFO>, int>> outputPortFifoMap = getOutputPortFIFOMapping(outputMaster);
    ioThread << "//Copy between buffers" << std::endl;
    for(auto it = masterOutputBundles.begin(); it != masterOutputBundles.end(); it++) {
        std::string linuxOutputTmpName = "linuxOutputTmp_bundle_"+GeneralHelper::to_string(it->first);
        std::string fromComputeFIFOFilledName = "fromComputeFIFOFilled_"+GeneralHelper::to_string(it->first);
        std::string extOutputBufferFilledName = "extOutputBufferFilled_"+GeneralHelper::to_string(it->first);

        //Copy ext input into the compute buffer if the external input buffer has data but the computeFIFO buffer does not
        //This allows a read from the external stream to occur if availible
        ioThread << "if(" << fromComputeFIFOFilledName << " && !" << extOutputBufferFilledName << "){" << std::endl;
        copyFIFOToIOOutputs(ioThread, it->second.first, outputPortFifoMap, outputMaster, linuxOutputTmpName, it->second.second);
        ioThread << fromComputeFIFOFilledName << " = false;" << std::endl;
        ioThread << extOutputBufferFilledName << " = true;" << std::endl;
        ioThread << "}" << std::endl;
    }

    ioThread << std::endl;

    if(collectBreakdownTelem) {
        ioThread << "timespec_t writingExtFIFOStop;" << std::endl;
        ioThread << "asm volatile (\"\" ::: \"memory\"); //Stop Re-ordering of timer" << std::endl;
        ioThread << "clock_gettime(CLOCK_MONOTONIC, &writingExtFIFOStop);" << std::endl;
        ioThread << "asm volatile (\"\" ::: \"memory\"); //Stop Re-ordering of timer" << std::endl;
        ioThread << "double writingExtFIFODuration = difftimespec(&writingExtFIFOStop, &writingExtFIFOStart);" << std::endl;
        ioThread << "timeWritingExtFIFO += writingExtFIFODuration;" << std::endl;
        ioThread << "timeTotal += writingExtFIFODuration;" << std::endl;
    }

    //Write external FIFOs
    if(collectBreakdownTelem) {
        ioThread << "asm volatile (\"\" ::: \"memory\"); //Stop Re-ordering of timer" << std::endl;
        ioThread << "clock_gettime(CLOCK_MONOTONIC, &writingExtFIFOStart);" << std::endl;
        ioThread << "asm volatile (\"\" ::: \"memory\"); //Stop Re-ordering of timer" << std::endl;
    }

    for(auto it = masterOutputBundles.begin(); it != masterOutputBundles.end(); it++) {
        std::string linuxOutputTmpName = "linuxOutputTmp_bundle_"+GeneralHelper::to_string(it->first);
        std::string outputStructTypeName = designName+"_outputs_bundle_"+GeneralHelper::to_string(it->first)+"_t";
        std::string extOutputBufferFilledName = "extOutputBufferFilled_"+GeneralHelper::to_string(it->first);

        //Only check external outputs ready if there is data to be sent
        ioThread << "//Check if data to write to external stream" << std::endl;
        ioThread << "if("<< extOutputBufferFilledName << "){" << std::endl;

        //Check if external output ready
        //Check if FIFO is free
        ioThread << "//Check if external stream ready" << std::endl;
        if (streamType == StreamType::PIPE) {
            std::string outputPipeHandleName = "outputPipe_bundle_"+GeneralHelper::to_string(it->first);
            ioThread << "bool extWriteReady = isReadyForWriting(" << outputPipeHandleName << ");" << std::endl;
        } else if (streamType == StreamType::SOCKET) {
            std::string connectedSocketName = "connectedSocket_bundle_" + GeneralHelper::to_string(it->first);
            ioThread << "bool extWriteReady = isReadyForWritingFD(" << connectedSocketName << ");" << std::endl;
        } else if (streamType == StreamType::POSIX_SHARED_MEM){
            std::string outputFifoHandleName = "outputFIFO_bundle_"+GeneralHelper::to_string(it->first);
            ioThread << "bool extWriteReady = isReadyForWriting(&" << outputFifoHandleName << ");" << std::endl;
        } else {
            throw std::runtime_error(ErrorHelpers::genErrorStr("Unknown stream type during stream I/O emit"));
        }

        ioThread << "if(extWriteReady){" << std::endl;
        //External stream is ready, perform a blocking write
        ioThread << "//Write to external stream" << std::endl;
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
        } else if (streamType == StreamType::POSIX_SHARED_MEM) {
            std::string outputFifoHandleName = "outputFIFO_bundle_"+GeneralHelper::to_string(it->first);
            ioThread << "int elementsWritten = writeFifo(&" << linuxOutputTmpName << ", sizeof(" << outputStructTypeName
                     << "), 1, &" << outputFifoHandleName << ");" << std::endl;
            ioThread << "if (elementsWritten != 1){" << std::endl;
            ioThread << "printf(\"An error was encountered while writing the Output FIFO\\n\");" << std::endl;
            ioThread << "perror(NULL);" << std::endl;
            ioThread << "exit(1);" << std::endl;
            ioThread << "}" << std::endl;
        } else {
            throw std::runtime_error(ErrorHelpers::genErrorStr("Unknown stream type during stream I/O emit"));
        }

        //Since write was blocking, can update status unconditionally
        ioThread << extOutputBufferFilledName << " = false;" << std::endl;

        ioThread << "}" << std::endl; //Close if output stream ready
        ioThread << "}" << std::endl; //Close if data available to be written
    }

    //This is a duplicate copy block which potentially allows another read from compute FIFOs to occur on the next cycle
    //Considered part of writing to external
    ioThread << "//Copy between buffers" << std::endl;
    for(auto it = masterOutputBundles.begin(); it != masterOutputBundles.end(); it++) {
        std::string linuxOutputTmpName = "linuxOutputTmp_bundle_"+GeneralHelper::to_string(it->first);
        std::string fromComputeFIFOFilledName = "fromComputeFIFOFilled_"+GeneralHelper::to_string(it->first);
        std::string extOutputBufferFilledName = "extOutputBufferFilled_"+GeneralHelper::to_string(it->first);

        //Copy ext input into the compute buffer if the external input buffer has data but the computeFIFO buffer does not
        //This allows a read from the external stream to occur if availible
        ioThread << "if(" << fromComputeFIFOFilledName << " && !" << extOutputBufferFilledName << "){" << std::endl;
        copyFIFOToIOOutputs(ioThread, it->second.first, outputPortFifoMap, outputMaster, linuxOutputTmpName, it->second.second);
        ioThread << fromComputeFIFOFilledName << " = false;" << std::endl;
        ioThread << extOutputBufferFilledName << " = true;" << std::endl;
        ioThread << "}" << std::endl;
    }

    if(collectBreakdownTelem) {
        ioThread << "asm volatile (\"\" ::: \"memory\"); //Stop Re-ordering of timer" << std::endl;
        ioThread << "clock_gettime(CLOCK_MONOTONIC, &writingExtFIFOStop);" << std::endl;
        ioThread << "asm volatile (\"\" ::: \"memory\"); //Stop Re-ordering of timer" << std::endl;
        ioThread << "writingExtFIFODuration = difftimespec(&writingExtFIFOStop, &writingExtFIFOStart);" << std::endl;
        ioThread << "timeWritingExtFIFO += writingExtFIFODuration;" << std::endl;
        ioThread << "timeTotal += writingExtFIFODuration;" << std::endl;
    }

    if(threadDebugPrint) {
        ioThread << "printf(\"I/O Output Sent\\n\");" << std::endl;
    }

    if(collectTelem) {
        ioThread << "if(!collectTelem){" << std::endl;
        ioThread << "//Reset timer after processing first samples.  Removes startup time from telemetry" << std::endl;
        ioThread << "rxSamples = 0;" << std::endl;
        ioThread << "asm volatile (\"\" ::: \"memory\"); //Stop Re-ordering of timer" << std::endl;
        ioThread << "clock_gettime(CLOCK_MONOTONIC, &startTime);" << std::endl;
        ioThread << "asm volatile (\"\" ::: \"memory\"); //Stop Re-ordering of timer" << std::endl;
        ioThread << "lastPrint = startTime;" << std::endl;
        if(collectBreakdownTelem) {
            ioThread << "timeTotal = 0;" << std::endl;
            ioThread << "timeReadingExtFIFO = 0;" << std::endl;
            ioThread << "timeWaitingForFIFOsToCompute = 0;" << std::endl;
            ioThread << "timeWritingFIFOsToCompute = 0;" << std::endl;
            ioThread << "timeWaitingForFIFOsFromCompute = 0;" << std::endl;
            ioThread << "timeReadingFIFOsFromCompute = 0;" << std::endl;
            ioThread << "timeWritingExtFIFO = 0;" << std::endl;
        }
        ioThread << "collectTelem = true;" << std::endl;
        ioThread << "}" << std::endl;
    }

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
        ioThread << "int closeStatus = 0;" << std::endl;
        for(auto it = bundles.begin(); it != bundles.end(); it++) {
            std::string listenSocketName = "listenSocket_bundle_" + GeneralHelper::to_string(*it);
            std::string connectedSocketName = "connectedSocket_bundle_" + GeneralHelper::to_string(*it);
            ioThread << "closeStatus = close(" << connectedSocketName << ");" << std::endl;
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
    } else if (streamType == StreamType::POSIX_SHARED_MEM) {
        for(auto it = masterOutputBundles.begin(); it != masterOutputBundles.end(); it++) {
            std::string outputFifoHandleName = "outputFIFO_bundle_"+GeneralHelper::to_string(it->first);
            ioThread << "cleanupProducer(&" << outputFifoHandleName << ");" << std::endl;
        }

        for(auto it = masterInputBundles.begin(); it != masterInputBundles.end(); it++) {
            std::string inputFifoHandleName = "inputFIFO_bundle_"+GeneralHelper::to_string(it->first);
            ioThread << "cleanupConsumer(&" << inputFifoHandleName << ");" << std::endl;
        }

    }else{
        throw std::runtime_error(ErrorHelpers::genErrorStr("Unknown stream type during stream I/O emit"));
    }

    if(collectTelem && !telemDumpFilePrefix.empty()){
        ioThread << "fclose(telemDumpFile);" << std::endl;
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

void StreamIOThread::copyIOInputsToFIFO(std::ofstream &ioThread, std::vector<Variable> masterInputVars, std::map<int, std::vector<std::pair<std::shared_ptr<ThreadCrossingFIFO>, int>>> inputPortFifoMap, std::string linuxInputPipeName, std::vector<int> masterInputBlockSizes) {
    for(int i = 0; i<masterInputVars.size(); i++){
        //The port in the FIFO structure is expanded according to the block size of the indevidual FIFO
        DataType dt = masterInputVars[i].getDataType().expandForBlock(masterInputBlockSizes[i]);

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

                //Note that dt is the dataype before expansion for block size.
                if(dt.isScalar()) {
                    ioThread << tmpName << ".port" << fifoPortNum << "_real = " << linuxInputPipeName << "."
                             << masterInputVars[i].getCVarName(false) << ";" << std::endl;
                    if (dt.isComplex()) {
                        ioThread << tmpName << ".port" << fifoPortNum << "_imag = " << linuxInputPipeName << "."
                                 << masterInputVars[i].getCVarName(true) << ";" << std::endl;
                    }
                }else{
                    //Array inputs/outputs are OK so long as they follow the C/C++ convention
                    //The input/output from this program is still a vector so any multidimensional array
                    //is flattened according to the C/C++ convention for contiguous memory access in a multidimensional array
                    //Note that the block is the left most index in the multidimensional array so blocks are grouped congiguously.
                    //Also note that the blocks are handled in the declaration of the stucture elements

                    //--- Confirm that this works with block sizes adjusted for port rates.  It looks like it should if the
                    //structure is updated properly
                    //---Addendum, it should work if the structure was updated correctly since sizeof with an array of know dimensions
                    //will return the number of bytes in the array and not the number of bytes in a single array element

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
                                         std::vector<int> masterOutputBlockSizes) {
    for(int i = 0; i<masterOutputVars.size(); i++){
        //The port in the FIFO structure is expanded according to the block size of the indevidual FIFO
        DataType dt = masterOutputVars[i].getDataType().expandForBlock(masterOutputBlockSizes[i]);

        //The index is the port number
        //Find if this element is used and where
        auto fifoPortIt = outputPortFifoMap.find(i);
        if(fifoPortIt != outputPortFifoMap.end()){
            //This port is used
            std::pair<std::shared_ptr<ThreadCrossingFIFO>, int> fifoPort = fifoPortIt->second;
            //Copy the values from the input structure to the appropriate (output) FIFO variables
            std::string tmpName = fifoPort.first->getName() + "_readTmp";
            int fifoPortNum = fifoPort.second;

            if(dt.isScalar()) {
                ioThread << linuxOutputPipeName << "." << masterOutputVars[i].getCVarName(false) << " = "
                         << tmpName << ".port" << fifoPortNum << "_real;" << std::endl;
                if (dt.isComplex()) {
                    ioThread << linuxOutputPipeName << "." << masterOutputVars[i].getCVarName(true) << " = "
                             << tmpName << ".port" << fifoPortNum << "_imag;" << std::endl;
                }
            }else{
                //Array inputs/outputs are OK so long as they follow the C/C++ convention
                //The input/output from this program is still a vector so any multidimensional array
                //is flattened according to the C/C++ convention for contiguous memory access in a multidimensional array
                //Note that the block is the left most index in the multidimensional array so blocks are grouped congiguously.
                //Also note that the blocks are handled in the declaration of the stucture elements
                ioThread << "memcpy(" << linuxOutputPipeName << "." << masterOutputVars[i].getCVarName(false)
                         << ", " << tmpName << ".port" << fifoPortNum << "_real" << ", sizeof(" << linuxOutputPipeName << "."
                         << masterOutputVars[i].getCVarName(false) << "));" << std::endl;
                if (dt.isComplex()) {
                    ioThread << "memcpy(" << linuxOutputPipeName << "." << masterOutputVars[i].getCVarName(true)
                             << ", " << tmpName << ".port" << fifoPortNum << "_imag" << ", sizeof(" << linuxOutputPipeName << "."
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

    std::pair<std::vector<Variable>, std::vector<std::pair<int, int>>> masterInputVarRatePairs = EmitterHelpers::getCInputVariables(inputMaster);
    std::pair<std::vector<Variable>, std::vector<std::pair<int, int>>> masterOutputVarRatePairs = EmitterHelpers::getCOutputVariables(outputMaster);
    std::vector<Variable> masterInputVars = masterInputVarRatePairs.first;
    std::vector<Variable> masterOutputVars = masterOutputVarRatePairs.first;

    //TODO: Optimize
    std::vector<int> masterInputBlockSizes = EmitterHelpers::getBlockSizesFromRates(masterInputVarRatePairs.second, 1);//Using a dummy blockSize
    std::vector<int> masterOutputBlockSizes = EmitterHelpers::getBlockSizesFromRates(masterOutputVarRatePairs.second, 1);//Using a dummy blockSize

    std::set<int> bundles;
    std::map<int, std::pair<std::vector<Variable>, std::vector<int>>> masterInputBundles;
    std::map<int, std::pair<std::vector<Variable>, std::vector<int>>> masterOutputBundles;
    sortIntoBundles(masterInputVars, masterOutputVars, masterInputBlockSizes, masterOutputBlockSizes,  masterInputBundles, masterOutputBundles, bundles);

    //Only need one of the connect/disconnect functions
    //Output function prototypes for the socket client
    headerFile << "//For connecting to the remote system" << std::endl;
    std::string connectFctnDecl = "int " + designName + "_" + filenamePostfix + "_connect(char* ipAddrStr, int bundleNum)";
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
    ioThread << "connectAddr.sin_port=htons(VITIS_SOCKET_LISTEN_PORT+bundleNum);" << std::endl;
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
                                     std::vector<int> inputBlockSizes, std::vector<int> outputBlockSizes,
                                     std::map<int, std::pair<std::vector<Variable>, std::vector<int>>> &masterInputBundles,
                                     std::map<int, std::pair<std::vector<Variable>, std::vector<int>>> &masterOutputBundles, std::set<int> &bundles) {
    //TODO: Possibly Update this to also export the relative clock rates of each input/output so that the block sizes
    //can be adapted
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
            masterInputBundles[tgtBundle].first.push_back(masterInputVars[i]);
            masterInputBundles[tgtBundle].second.push_back(inputBlockSizes[i]);
            bundles.insert(tgtBundle);
        } else {
            //Goes in bundle 0
            masterInputBundles[0].first.push_back(masterInputVars[i]);
            masterInputBundles[0].second.push_back(inputBlockSizes[i]);
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
            masterOutputBundles[tgtBundle].first.push_back(masterOutputVars[i]);
            masterOutputBundles[tgtBundle].second.push_back(outputBlockSizes[i]);
            bundles.insert(tgtBundle);
        } else {
            //Goes in bundle 0
            masterOutputBundles[0].first.push_back(masterOutputVars[i]);
            masterOutputBundles[0].second.push_back(outputBlockSizes[i]);
            bundles.insert(0);
        }
    }
}

std::string StreamIOThread::emitSharedMemoryFIFOHelperFiles(std::string path) {
    std::string fileName = "BerkeleySharedMemoryFIFO";
    std::cout << "Emitting C File: " << path << "/" << fileName << ".h" << std::endl;
    //#### Emit .h file ####
    std::ofstream headerFile;
    headerFile.open(path+"/"+fileName+".h", std::ofstream::out | std::ofstream::trunc);

    std::string fileNameUpper =  GeneralHelper::toUpper(fileName);

    headerFile << "//\n"
                  "// Project: BerkeleySharedMemoryFIFO\n"
                  "// Created by Christopher Yarp on 10/29/19.\n"
                  "// Availible at https://github.com/ucb-cyarp/BerkeleySharedMemoryFIFO\n"
                  "//\n"
                  "\n"
                  "// BSD 3-Clause License\n"
                  "//\n"
                  "// Copyright (c) 2019, Regents of the University of California\n"
                  "// All rights reserved.\n"
                  "//\n"
                  "// Redistribution and use in source and binary forms, with or without\n"
                  "// modification, are permitted provided that the following conditions are met:\n"
                  "//\n"
                  "// 1. Redistributions of source code must retain the above copyright notice, this\n"
                  "//    list of conditions and the following disclaimer.\n"
                  "//\n"
                  "// 2. Redistributions in binary form must reproduce the above copyright notice,\n"
                  "//    this list of conditions and the following disclaimer in the documentation\n"
                  "//    and/or other materials provided with the distribution.\n"
                  "//\n"
                  "// 3. Neither the name of the copyright holder nor the names of its\n"
                  "//    contributors may be used to endorse or promote products derived from\n"
                  "//    this software without specific prior written permission.\n"
                  "//\n"
                  "// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS \"AS IS\"\n"
                  "// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE\n"
                  "// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE\n"
                  "// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE\n"
                  "// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL\n"
                  "// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR\n"
                  "// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER\n"
                  "// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,\n"
                  "// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE\n"
                  "// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE." << std::endl;

    headerFile << "#ifndef " << fileNameUpper << "_H" << std::endl;
    headerFile << "#define " << fileNameUpper << "_H" << std::endl;

    headerFile << "#ifndef _DEFAULT_SOURCE\n"
                  "#define _DEFAULT_SOURCE\n"
                  "#endif\n"
                  "\n"
                  "#include <stdatomic.h>\n"
                  "#include <semaphore.h>\n"
                  "#include <sys/mman.h>\n"
                  "#include <unistd.h>\n"
                  "#include <stdbool.h>\n"
                  "\n"
                  "typedef struct{\n"
                  "    char *sharedName;\n"
                  "    int sharedFD;\n"
                  "    char* txSemaphoreName;\n"
                  "    char* rxSemaphoreName;\n"
                  "    sem_t *txSem;\n"
                  "    sem_t *rxSem;\n"
                  "    atomic_int_fast32_t* fifoCount;\n"
                  "    void* fifoBlock;\n"
                  "    void* fifoBuffer;\n"
                  "    size_t fifoSizeBytes;\n"
                  "    size_t fifoSharedBlockSizeBytes;\n"
                  "    size_t currentOffset;\n"
                  "    bool rxReady;\n"
                  "} sharedMemoryFIFO_t;\n"
                  "\n"
                  "void initSharedMemoryFIFO(sharedMemoryFIFO_t *fifo);\n"
                  "\n"
                  "int producerOpenInitFIFO(char *sharedName, size_t fifoSizeBytes, sharedMemoryFIFO_t *fifo);\n"
                  "\n"
                  "int consumerOpenFIFOBlock(char *sharedName, size_t fifoSizeBytes, sharedMemoryFIFO_t *fifo);\n"
                  "\n"
                  "//NOTE: this function blocks until numElements can be written into the FIFO\n"
                  "int writeFifo(void* src, size_t elementSize, int numElements, sharedMemoryFIFO_t *fifo);\n"
                  "\n"
                  "int readFifo(void* dst, size_t elementSize, int numElements, sharedMemoryFIFO_t *fifo);\n"
                  "\n"
                  "void cleanupProducer(sharedMemoryFIFO_t *fifo);\n"
                  "\n"
                  "void cleanupConsumer(sharedMemoryFIFO_t *fifo);\n"
                  "\n"
                  "bool isReadyForReading(sharedMemoryFIFO_t *fifo);\n"
                  "\n"
                  "bool isReadyForWriting(sharedMemoryFIFO_t *fifo);" << std::endl;
    headerFile << "#endif" << std::endl;
    headerFile.close();

    std::cout << "Emitting C File: " << path << "/" << fileName << ".c" << std::endl;
    //#### Emit .h file ####
    std::ofstream cFile;
    cFile.open(path+"/"+fileName+".c", std::ofstream::out | std::ofstream::trunc);

    cFile <<    "//\n"
                "// Project: BerkeleySharedMemoryFIFO\n"
                "// Created by Christopher Yarp on 10/29/19.\n"
                "// Availible at https://github.com/ucb-cyarp/BerkeleySharedMemoryFIFO\n"
                "//\n"
                "\n"
                "// BSD 3-Clause License\n"
                "//\n"
                "// Copyright (c) 2019, Regents of the University of California\n"
                "// All rights reserved.\n"
                "//\n"
                "// Redistribution and use in source and binary forms, with or without\n"
                "// modification, are permitted provided that the following conditions are met:\n"
                "//\n"
                "// 1. Redistributions of source code must retain the above copyright notice, this\n"
                "//    list of conditions and the following disclaimer.\n"
                "//\n"
                "// 2. Redistributions in binary form must reproduce the above copyright notice,\n"
                "//    this list of conditions and the following disclaimer in the documentation\n"
                "//    and/or other materials provided with the distribution.\n"
                "//\n"
                "// 3. Neither the name of the copyright holder nor the names of its\n"
                "//    contributors may be used to endorse or promote products derived from\n"
                "//    this software without specific prior written permission.\n"
                "//\n"
                "// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS \"AS IS\"\n"
                "// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE\n"
                "// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE\n"
                "// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE\n"
                "// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL\n"
                "// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR\n"
                "// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER\n"
                "// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,\n"
                "// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE\n"
                "// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE." << std::endl;

    cFile << "#include \"" + fileName + ".h\"" << std::endl;
    cFile << "#include <stdbool.h>\n"
             "#include <stdio.h>\n"
             "#include <stdlib.h>\n"
             "#include <string.h>\n"
             "#include <fcntl.h>\n"
             "#include <sys/types.h>\n"
             "\n"
             "void initSharedMemoryFIFO(sharedMemoryFIFO_t *fifo){\n"
             "    fifo->sharedName = NULL;\n"
             "    fifo->sharedFD = -1;\n"
             "    fifo->txSemaphoreName = NULL;\n"
             "    fifo->rxSemaphoreName = NULL;\n"
             "    fifo->txSem = NULL;\n"
             "    fifo->rxSem = NULL;\n"
             "    fifo->fifoCount = NULL;\n"
             "    fifo->fifoBlock = NULL;\n"
             "    fifo->fifoBuffer = NULL;\n"
             "    fifo->fifoSizeBytes = 0;\n"
             "    fifo->currentOffset = 0;\n"
             "    fifo->fifoSharedBlockSizeBytes = 0;\n"
             "    fifo->rxReady = false;\n"
             "}\n"
             "\n"
             "int producerOpenInitFIFO(char *sharedName, size_t fifoSizeBytes, sharedMemoryFIFO_t *fifo){\n"
             "    fifo->sharedName = sharedName;\n"
             "    fifo->fifoSizeBytes = fifoSizeBytes;\n"
             "    size_t sharedBlockSize = fifoSizeBytes + sizeof(atomic_int_fast32_t);\n"
             "    fifo->fifoSharedBlockSizeBytes = sharedBlockSize;\n"
             "\n"
             "    //The producer is responsible for initializing the FIFO and releasing the Tx semaphore\n"
             "    //Note: Both Tx and Rx use the O_CREAT mode to create the semaphore if it does not already exist\n"
             "    //The Rx semaphore is used to block the producer from continuing after FIFO init until a consumer is started and is ready\n"
             "    //---- Get access to the semaphore ----\n"
             "    int sharedNameLen = strlen(sharedName);\n"
             "    fifo->txSemaphoreName = malloc(sharedNameLen+5);\n"
             "    strcpy(fifo->txSemaphoreName, \"/\");\n"
             "    strcat(fifo->txSemaphoreName, sharedName);\n"
             "    strcat(fifo->txSemaphoreName, \"_TX\");\n"
             "    fifo->txSem = sem_open(fifo->txSemaphoreName, O_CREAT, S_IRWXU, 0); //Initialize to 0, the consumer will wait\n"
             "    if (fifo->txSem == SEM_FAILED){\n"
             "        printf(\"Unable to open tx semaphore\\n\");\n"
             "        perror(NULL);\n"
             "        exit(1);\n"
             "    }\n"
             "\n"
             "    fifo->rxSemaphoreName = malloc(sharedNameLen+5);\n"
             "    strcpy(fifo->rxSemaphoreName, \"/\");\n"
             "    strcat(fifo->rxSemaphoreName, sharedName);\n"
             "    strcat(fifo->rxSemaphoreName, \"_RX\");\n"
             "    fifo->rxSem = sem_open(fifo->rxSemaphoreName, O_CREAT, S_IRWXU, 0); //Initialize to 0, the consumer will wait\n"
             "    if (fifo->rxSem == SEM_FAILED){\n"
             "        printf(\"Unable to open rx semaphore\\n\");\n"
             "        perror(NULL);\n"
             "        exit(1);\n"
             "    }\n"
             "\n"
             "    //---- Init shared mem ----\n"
             "    fifo->sharedFD = shm_open(sharedName, O_CREAT | O_RDWR, S_IRWXU);\n"
             "    if (fifo->sharedFD == -1){\n"
             "        printf(\"Unable to open tx shm\\n\");\n"
             "        perror(NULL);\n"
             "        exit(1);\n"
             "    }\n"
             "\n"
             "    //Resize the shared memory\n"
             "    int status = ftruncate(fifo->sharedFD, sharedBlockSize);\n"
             "    if(status == -1){\n"
             "        printf(\"Unable to resize tx fifo\\n\");\n"
             "        perror(NULL);\n"
             "        exit(1);\n"
             "    }\n"
             "\n"
             "    fifo->fifoBlock = mmap(NULL, sharedBlockSize, PROT_READ | PROT_WRITE, MAP_SHARED, fifo->sharedFD, 0);\n"
             "    if (fifo->fifoBlock == MAP_FAILED){\n"
             "        printf(\"Rx mmap failed\\n\");\n"
             "        perror(NULL);\n"
             "        exit(1);\n"
             "    }\n"
             "\n"
             "    //---- Init the fifoCount ----\n"
             "    fifo->fifoCount = (atomic_int_fast32_t*) fifo->fifoBlock;\n"
             "    atomic_init(fifo->fifoCount, 0);\n"
             "\n"
             "    char* fifoBlockBytes = (char*) fifo->fifoBlock;\n"
             "    fifo->fifoBuffer = (void*) (fifoBlockBytes + sizeof(atomic_int_fast32_t));\n"
             "\n"
             "    //The semaphore is an implicit fence\n"
             "\n"
             "    //FIFO init done\n"
             "    //---- Release the semaphore ----\n"
             "    sem_post(fifo->txSem);\n"
             "\n"
             "    return sharedBlockSize;\n"
             "}\n"
             "\n"
             "int consumerOpenFIFOBlock(char *sharedName, size_t fifoSizeBytes, sharedMemoryFIFO_t *fifo){\n"
             "    fifo->sharedName = sharedName;\n"
             "    fifo->fifoSizeBytes = fifoSizeBytes;\n"
             "    size_t sharedBlockSize = fifoSizeBytes + sizeof(atomic_int_fast32_t);\n"
             "    fifo->fifoSharedBlockSizeBytes = sharedBlockSize;\n"
             "\n"
             "    //---- Get access to the semaphore ----\n"
             "    int sharedNameLen = strlen(sharedName);\n"
             "    fifo->txSemaphoreName = malloc(sharedNameLen+5);\n"
             "    strcpy(fifo->txSemaphoreName, \"/\");\n"
             "    strcat(fifo->txSemaphoreName, sharedName);\n"
             "    strcat(fifo->txSemaphoreName, \"_TX\");\n"
             "    fifo->txSem = sem_open(fifo->txSemaphoreName, O_CREAT, S_IRWXU, 0); //Initialize to 0, the consumer will wait\n"
             "    if (fifo->txSem == SEM_FAILED){\n"
             "        printf(\"Unable to open tx semaphore\\n\");\n"
             "        perror(NULL);\n"
             "        exit(1);\n"
             "    }\n"
             "\n"
             "    fifo->rxSemaphoreName = malloc(sharedNameLen+5);\n"
             "    strcpy(fifo->rxSemaphoreName, \"/\");\n"
             "    strcat(fifo->rxSemaphoreName, sharedName);\n"
             "    strcat(fifo->rxSemaphoreName, \"_RX\");\n"
             "    fifo->rxSem = sem_open(fifo->rxSemaphoreName, O_CREAT, S_IRWXU, 0); //Initialize to 0, the consumer will wait\n"
             "    if (fifo->rxSem == SEM_FAILED){\n"
             "        printf(\"Unable to open rx semaphore\\n\");\n"
             "        perror(NULL);\n"
             "        exit(1);\n"
             "    }\n"
             "\n"
             "    //Block on the semaphore while the producer is initializing\n"
             "    int status = sem_wait(fifo->txSem);\n"
             "    if(status == -1){\n"
             "        printf(\"Unable to wait on tx semaphore\\n\");\n"
             "        perror(NULL);\n"
             "        exit(1);\n"
             "    }\n"
             "\n"
             "    //The semaphore is an implicit fence\n"
             "\n"
             "    //---- Open shared mem ----\n"
             "    fifo->sharedFD = shm_open(sharedName, O_RDWR, S_IRWXU);\n"
             "    if(fifo->sharedFD == -1){\n"
             "        printf(\"Unable to open rx shm\\n\");\n"
             "        perror(NULL);\n"
             "        exit(1);\n"
             "    }\n"
             "\n"
             "    //No need to resize shared memory, the producer has already done that\n"
             "\n"
             "    fifo->fifoBlock = mmap(NULL, sharedBlockSize, PROT_READ | PROT_WRITE, MAP_SHARED, fifo->sharedFD, 0);\n"
             "    if(fifo->fifoBlock == MAP_FAILED){\n"
             "        printf(\"Rx mmap failed\\n\");\n"
             "        perror(NULL);\n"
             "        exit(1);\n"
             "    }\n"
             "\n"
             "    //---- Get appropriate pointers from the shared memory block ----\n"
             "    fifo->fifoCount = (atomic_int_fast32_t*) fifo->fifoBlock;\n"
             "\n"
             "    char* fifoBlockBytes = (char*) fifo->fifoBlock;\n"
             "    fifo->fifoBuffer = (void*) (fifoBlockBytes + sizeof(atomic_int_fast32_t));\n"
             "\n"
             "    //inform producer that consumer is ready\n"
             "    sem_post(fifo->rxSem);\n"
             "\n"
             "    return sharedBlockSize;\n"
             "}\n"
             "\n"
             "//currentOffset is updated by the call\n"
             "//currentOffset is in bytes\n"
             "//fifosize is in bytes\n"
             "//fifoCount is in bytes\n"
             "\n"
             "//returns number of elements written\n"
             "int writeFifo(void* src_uncast, size_t elementSize, int numElements, sharedMemoryFIFO_t *fifo){\n"
             "    char* dst = (char*) fifo->fifoBuffer;\n"
             "    char* src = (char*) src_uncast;\n"
             "\n"
             "    if(!fifo->rxReady) {\n"
             "        //---- Wait for consumer to join ---\n"
             "        sem_wait(fifo->rxSem);\n"
             "        fifo->rxReady = true;\n"
             "    }\n"
             "\n"
             "    bool hasRoom = false;\n"
             "\n"
             "    size_t bytesToWrite = elementSize*numElements;\n"
             "\n"
             "    while(!hasRoom){\n"
             "        int currentCount = atomic_load_explicit(fifo->fifoCount, memory_order_acquire);\n"
             "        int spaceInFIFO = fifo->fifoSizeBytes - currentCount;\n"
             "        //TODO: REMOVE\n"
             "        if(spaceInFIFO<0){\n"
             "            printf(\"FIFO had a negative count\");\n"
             "            exit(1);\n"
             "        }\n"
             "\n"
             "        if(bytesToWrite <= spaceInFIFO){\n"
             "            hasRoom = true;\n"
             "        }\n"
             "    }\n"
             "\n"
             "    //There is room in the FIFO, write into it\n"
             "    //Write up to the end of the buffer, wrap around if nessisary\n"
             "    size_t currentOffsetLocal = fifo->currentOffset;\n"
             "    size_t bytesToEnd = fifo->fifoSizeBytes - currentOffsetLocal;\n"
             "    size_t bytesToTransferFirst = bytesToEnd < bytesToWrite ? bytesToEnd : bytesToWrite;\n"
             "    memcpy(dst+currentOffsetLocal, src, bytesToTransferFirst);\n"
             "    currentOffsetLocal += bytesToTransferFirst;\n"
             "    if(currentOffsetLocal >= fifo->fifoSizeBytes){\n"
             "        //Wrap around\n"
             "        currentOffsetLocal = 0;\n"
             "\n"
             "        //Write remaining (if any)\n"
             "        size_t remainingBytesToTransfer = bytesToWrite - bytesToTransferFirst;\n"
             "        if(remainingBytesToTransfer>0){\n"
             "            //Know currentOffsetLocal is 0 so does not need to be added\n"
             "            //However, need to offset source by the number of bytes transfered before\n"
             "            memcpy(dst, src+bytesToTransferFirst, remainingBytesToTransfer);\n"
             "            currentOffsetLocal+=remainingBytesToTransfer;\n"
             "        }\n"
             "    }\n"
             "\n"
             "    //Update the current offset\n"
             "    fifo->currentOffset = currentOffsetLocal;\n"
             "\n"
             "    //Update the fifoCount, do not need the new value\n"
             "    atomic_fetch_add_explicit(fifo->fifoCount, bytesToWrite, memory_order_acq_rel);\n"
             "\n"
             "    return numElements;\n"
             "}\n"
             "\n"
             "int readFifo(void* dst_uncast, size_t elementSize, int numElements, sharedMemoryFIFO_t *fifo){\n"
             "    char* dst = (char*) dst_uncast;\n"
             "    char* src = (char*) fifo->fifoBuffer;\n"
             "\n"
             "    bool hasData = false;\n"
             "\n"
             "    size_t bytesToRead = elementSize*numElements;\n"
             "\n"
             "    while(!hasData){\n"
             "        int currentCount = atomic_load_explicit(fifo->fifoCount, memory_order_acquire);\n"
             "        //TODO: REMOVE\n"
             "        if(currentCount<0){\n"
             "            printf(\"FIFO had a negative count\");\n"
             "            exit(1);\n"
             "        }\n"
             "\n"
             "        if(currentCount >= bytesToRead){\n"
             "            hasData = true;\n"
             "        }\n"
             "    }\n"
             "\n"
             "    //There is enough data in the fifo to complete a read operation\n"
             "    //Read from the FIFO\n"
             "    //Read up to the end of the buffer and wrap if nessisary\n"
             "    size_t currentOffsetLocal = fifo->currentOffset;\n"
             "    size_t bytesToEnd = fifo->fifoSizeBytes - currentOffsetLocal;\n"
             "    size_t bytesToTransferFirst = bytesToEnd < bytesToRead ? bytesToEnd : bytesToRead;\n"
             "    memcpy(dst, src+currentOffsetLocal, bytesToTransferFirst);\n"
             "    currentOffsetLocal += bytesToTransferFirst;\n"
             "    if(currentOffsetLocal >= fifo->fifoSizeBytes){\n"
             "        //Wrap around\n"
             "        currentOffsetLocal = 0;\n"
             "\n"
             "        //Read remaining (if any)\n"
             "        size_t remainingBytesToTransfer = bytesToRead - bytesToTransferFirst;\n"
             "        if(remainingBytesToTransfer>0){\n"
             "            //Know currentOffsetLocal is 0 so does not need to be added to src\n"
             "            //However, need to offset dest by the number of bytes transfered before\n"
             "            memcpy(dst+bytesToTransferFirst, src, remainingBytesToTransfer);\n"
             "            currentOffsetLocal+=remainingBytesToTransfer;\n"
             "        }\n"
             "    }\n"
             "\n"
             "    //Update the current offset\n"
             "    fifo->currentOffset = currentOffsetLocal;\n"
             "\n"
             "    //Update the fifoCount, do not need the new value\n"
             "    atomic_fetch_sub_explicit(fifo->fifoCount, bytesToRead, memory_order_acq_rel);\n"
             "\n"
             "    return numElements;\n"
             "}\n"
             "\n"
             "void cleanupHelper(sharedMemoryFIFO_t *fifo){\n"
             "    void* fifoBlockCast = (void *) fifo->fifoBlock;\n"
             "    if(fifo->fifoBlock != NULL) {\n"
             "        int status = munmap(fifoBlockCast, fifo->fifoSharedBlockSizeBytes);\n"
             "        if (status == -1) {\n"
             "            printf(\"Error in tx munmap\\n\");\n"
             "            perror(NULL);\n"
             "        }\n"
             "    }\n"
             "\n"
             "    if(fifo->txSem != NULL) {\n"
             "        int status = sem_close(fifo->txSem);\n"
             "        if (status == -1) {\n"
             "            printf(\"Error in tx semaphore close\\n\");\n"
             "            perror(NULL);\n"
             "        }\n"
             "    }\n"
             "\n"
             "    if(fifo->rxSem != NULL) {\n"
             "        int status = sem_close(fifo->rxSem);\n"
             "        if (status == -1) {\n"
             "            printf(\"Error in rx semaphore close\\n\");\n"
             "            perror(NULL);\n"
             "        }\n"
             "    }\n"
             "}\n"
             "\n"
             "void cleanupProducer(sharedMemoryFIFO_t *fifo){\n"
             "    bool unlinkSharedBlock = fifo->fifoBlock != NULL;\n"
             "    bool unlinkTxSem = fifo->txSem != NULL;\n"
             "    bool unlinkRxSem = fifo->rxSem != NULL;\n"
             "\n"
             "    cleanupHelper(fifo);\n"
             "\n"
             "    if(unlinkSharedBlock) {\n"
             "        int status = shm_unlink(fifo->sharedName);\n"
             "        if (status == -1) {\n"
             "            printf(\"Error in tx fifo unlink\\n\");\n"
             "            perror(NULL);\n"
             "        }\n"
             "    }\n"
             "\n"
             "    if(unlinkTxSem) {\n"
             "        int status = sem_unlink(fifo->txSemaphoreName);\n"
             "        if (status == -1) {\n"
             "            printf(\"Error in tx semaphore unlink\\n\");\n"
             "            perror(NULL);\n"
             "        }\n"
             "    }\n"
             "\n"
             "    if(unlinkRxSem) {\n"
             "        int status = sem_unlink(fifo->rxSemaphoreName);\n"
             "        if (status == -1) {\n"
             "            printf(\"Error in rx semaphore unlink\\n\");\n"
             "            perror(NULL);\n"
             "        }\n"
             "    }\n"
             "\n"
             "    if(fifo->txSemaphoreName != NULL){\n"
             "        free(fifo->txSemaphoreName);\n"
             "    }\n"
             "\n"
             "    if(fifo->rxSemaphoreName != NULL){\n"
             "        free(fifo->rxSemaphoreName);\n"
             "    }\n"
             "}\n"
             "\n"
             "void cleanupConsumer(sharedMemoryFIFO_t *fifo) {\n"
             "    cleanupHelper(fifo);\n"
             "\n"
             "    if (fifo->txSemaphoreName != NULL) {\n"
             "        free(fifo->txSemaphoreName);\n"
             "    }\n"
             "\n"
             "    if (fifo->rxSemaphoreName != NULL) {\n"
             "        free(fifo->rxSemaphoreName);\n"
             "    }\n"
             "}\n"
             "\n"
             "bool isReadyForReading(sharedMemoryFIFO_t *fifo){\n"
             "    int32_t currentCount = atomic_load_explicit(fifo->fifoCount, memory_order_acquire);\n"
             "    return currentCount != 0;\n"
             "}\n"
             "\n"
             "bool isReadyForWriting(sharedMemoryFIFO_t *fifo){\n"
             "    if(!fifo->rxReady) {\n"
             "        //---- Check if consumer joined ----\n"
             "        int status = sem_trywait(fifo->rxSem);\n"
             "        if(status == 0){\n"
             "            fifo->rxReady = true;\n"
             "        }else{\n"
             "            //Consumer has not joined yet\n"
             "            return false;\n"
             "        }\n"
             "    }\n"
             "\n"
             "    int32_t currentCount = atomic_load_explicit(fifo->fifoCount, memory_order_acquire);\n"
             "    return currentCount < fifo->fifoSizeBytes;\n"
             "}" << std::endl;

    cFile.close();

    return fileName+".h";
}

std::string StreamIOThread::emitFileStreamHelpers(std::string path, std::string fileNamePrefix){
    std::string fileName = fileNamePrefix + "_filestream_helpers";
    std::cout << "Emitting C File: " << path << "/" << fileName << ".h" << std::endl;
    //#### Emit .h file ####
    std::ofstream headerFile;
    headerFile.open(path + "/" + fileName + ".h", std::ofstream::out | std::ofstream::trunc);

    std::string fileNameUpper =  GeneralHelper::toUpper(fileName);
    headerFile << "#ifndef " << fileNameUpper << "_H" << std::endl;
    headerFile << "#define " << fileNameUpper << "_H" << std::endl;
    headerFile << std::endl;

    headerFile << "#include <stdbool.h>" << std::endl;
    headerFile << "#include <stdio.h>" << std::endl;
    headerFile << std::endl;

    headerFile << "bool isReadyForReading(FILE* file);" << std::endl;
    headerFile << "bool isReadyForReadingFD(int fileFD);" << std::endl;
    headerFile << "bool isReadyForWriting(FILE* file);" << std::endl;
    headerFile << "bool isReadyForWritingFD(int fileFD);" << std::endl;
    headerFile << std::endl;

    headerFile << "#endif" << std::endl;
    headerFile.close();

    std::cout << "Emitting C File: " << path << "/" << fileName << ".c" << std::endl;
    //#### Emit .c file ####
    std::ofstream cFile;
    cFile.open(path + "/" + fileName + ".c", std::ofstream::out | std::ofstream::trunc);
    cFile << "#include \"" << fileName << ".h" << "\"" << std::endl;
    cFile << std::endl;

    cFile << "#include <stdio.h>" << std::endl;
    cFile << "#include <sys/select.h>" << std::endl;
    cFile << "#include <sys/time.h>" << std::endl;
    cFile << "#include <stdlib.h>" << std::endl;
    cFile << std::endl;
    cFile << "bool isReadyForReading(FILE* file) {" << std::endl;
    cFile << "int fileFD = fileno(file);" << std::endl;
    cFile << "return isReadyForReadingFD(fileFD);" << std::endl;
    cFile << "}" << std::endl;
    cFile << std::endl;
    cFile << "bool isReadyForReadingFD(int fileFD){" << std::endl;
    cFile << "fd_set fdSet;" << std::endl;
    cFile << "FD_ZERO(&fdSet);" << std::endl;
    cFile << "FD_SET(fileFD, &fdSet);" << std::endl;
    cFile << "int maxFD = fileFD;" << std::endl;
    cFile << std::endl;
    cFile << "//Timeout quickly" << std::endl;
    cFile << "struct timespec timeout;" << std::endl;
    cFile << "timeout.tv_sec = 0;" << std::endl;
    cFile << "timeout.tv_nsec = 0;" << std::endl;
    cFile << std::endl;
    cFile << "int selectStatus = pselect(maxFD+1, &fdSet, NULL, NULL, &timeout, NULL);" << std::endl;
    cFile << "if(selectStatus == -1){" << std::endl;
    cFile << "fprintf(stderr, \"Error while checking if a file is ready for reading\\n\");" << std::endl;
    cFile << "perror(NULL);" << std::endl;
    cFile << "exit(1);" << std::endl;
    cFile << "}" << std::endl;
    cFile << "return FD_ISSET(fileFD, &fdSet);" << std::endl;
    cFile << "}" << std::endl;
    cFile << std::endl;
    cFile << "bool isReadyForWriting(FILE* file) {" << std::endl;
    cFile << "int fileFD = fileno(file);" << std::endl;
    cFile << "return isReadyForWritingFD(fileFD);" << std::endl;
    cFile << "}" << std::endl;
    cFile << std::endl;
    cFile << "bool isReadyForWritingFD(int fileFD){" << std::endl;
    cFile << "fd_set fdSet;" << std::endl;
    cFile << "FD_ZERO(&fdSet);" << std::endl;
    cFile << "FD_SET(fileFD, &fdSet);" << std::endl;
    cFile << "int maxFD = fileFD;" << std::endl;
    cFile << std::endl;
    cFile << "//Timeout quickly" << std::endl;
    cFile << "struct timespec timeout;" << std::endl;
    cFile << "timeout.tv_sec = 0;" << std::endl;
    cFile << "timeout.tv_nsec = 0;" << std::endl;
    cFile << std::endl;
    cFile << "int selectStatus = pselect(maxFD+1, NULL, &fdSet, NULL, &timeout, NULL);" << std::endl;
    cFile << "if(selectStatus == -1){" << std::endl;
    cFile << "fprintf(stderr, \"Error while checking if a file is ready for reading\\n\");" << std::endl;
    cFile << "perror(NULL);" << std::endl;
    cFile << "exit(1);" << std::endl;
    cFile << "}" << std::endl;
    cFile << "return FD_ISSET(fileFD, &fdSet);" << std::endl;
    cFile << "}" << std::endl;
    cFile.close();

    return fileName+".h";
}