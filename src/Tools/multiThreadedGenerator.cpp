//
// Created by Christopher Yarp on 9/24/19.
//

#include <iostream>
#include <fstream>
#include <string>
#include <memory>
#include "GraphMLTools/GraphMLImporter.h"
#include "GraphMLTools/GraphMLExporter.h"
#include "GraphCore/Design.h"
#include "GraphMLTools/GraphMLDialect.h"
#include "MultiThread/PartitionParams.h"
#include "General/ErrorHelpers.h"

int main(int argc, char* argv[]) {


    //Print usage help
    if(argc < 4)
    {
        std::cout << "multiThreadedGenerator: Emit a design stored in a Vitis GraphML File to a Multi Threaded C Function" << std::endl;
        std::cout << std::endl;
        std::cout << "Usage: " << std::endl;
        std::cout << "    multiThreadedGenerator inputfile.graphml outputDir designName --partitioner <PARTITIONER> --fifoType <FIFO_TYPE> --schedHeur <SCHED_HEUR> --randSeed <SCHED_RAND_SEED> --blockSize <BLOCK_SIZE> --fifoLength <FIFO_LENGTH> --partitionMap <PARTITION_MAP> <--emitGraphMLSched> <--printSched> <--threadDebugPrint>" << std::endl;
        std::cout << std::endl;
        std::cout << "Possible PARTITIONER:" << std::endl;
        std::cout << "    manual <DEFAULT> = Partitioning is accomplished manually using VITIS_PARTITION directives" << std::endl;
        std::cout << std::endl;
        std::cout << "Possible FIFO_TYPE:" << std::endl;
        std::cout << "    lockeless_x86 <DEFAULT> = Lockless single producer, single consumer, FIFOs suitable for x86 based systems" << std::endl;
        std::cout << std::endl;
        std::cout << "Possible SCHED_HEUR:" << std::endl;
        std::cout << "    bfs <DEFAULT> = Breadth First Search Style" << std::endl;
        std::cout << "    dfs           = Depth First Search Style" << std::endl;
        std::cout << "    random        = Random Style" << std::endl;
        std::cout << std::endl;
        std::cout << "Possible SCHED_RAND_SEED (only applies to topological and topological_context):" << std::endl;
        std::cout << "    unsigned long seed <DEFAULT = 0>" << std::endl;
        std::cout << std::endl;
        std::cout << "Possible BLOCK_SIZE (block size in samples):" << std::endl;
        std::cout << "    unsigned long blockSize <DEFAULT = 1>" << std::endl;
        std::cout << std::endl;
        std::cout << "Possible FIFO_LENGTH (length of FIFOs in blocks):" << std::endl;
        std::cout << "    unsigned long fifoLength <DEFAULT = 16>" << std::endl;
        std::cout << std::endl;
        std::cout << "Possible PARTITION_MAP (mapping of partition numbers to logical CPUs):" << std::endl;
        std::cout << "    A comma separated array without spaces (ex. [0,1,2,3])" << std::endl;
        std::cout << "    The first element of the array corresponds to the I/O thread.  The subsequent elements" << std::endl;
        std::cout << "    correspond to partition 0, 1, 2, etc..." << std::endl;
        std::cout << "    An empty array ([]) corresponds to the default configuration:" << std::endl;
        std::cout << "    By default the I/O thread is placed on CPU0 and partitions are placed" << std::endl;
        std::cout << "    on the CPU that matches their partition number (ex. partition 1 is placed on CPU1)" << std::endl;
        std::cout << std::endl;

        return 1;
    }

    PartitionParams::PartitionType partitioner = PartitionParams::PartitionType::MANUAL;
    ThreadCrossingFIFOParameters::ThreadCrossingFIFOType fifoType = ThreadCrossingFIFOParameters::ThreadCrossingFIFOType::LOCKLESS_X86;
    SchedParams::SchedType sched = SchedParams::SchedType::TOPOLOGICAL_CONTEXT;//This is the only supported scheduler for multi-threaded emit
    TopologicalSortParameters::Heuristic heuristic = TopologicalSortParameters::Heuristic::BFS;
    unsigned long randSeed = 0;
    unsigned long blockSize = 1;
    unsigned long fifoLength = 16;
    std::vector<int> partitionMap;

    bool emitGraphMLSched = false;
    bool printNodeSched = false;
    bool threadDebugPrint = false;

    //Check for command line parameters
    for(unsigned long i = 4; i<argc; i++){
        if(strcmp(argv[i], "--partitioner") == 0) {
            i++; //Get the actual argument
            try {
                PartitionParams::PartitionType parsedPartitioner = PartitionParams::parsePartitionTypeStr(argv[i]);
                partitioner = parsedPartitioner;
            } catch (std::runtime_error e) {
                std::cerr << "Unknown command line option selection: --partitioner " << argv[i] << std::endl;
                exit(1);
            }
        }else if(strcmp(argv[i], "--fifoType") == 0) {
            i++; //Get the actual argument
            try {
                ThreadCrossingFIFOParameters::ThreadCrossingFIFOType parsedFIFOType = ThreadCrossingFIFOParameters::parseThreadCrossingFIFOType(argv[i]);
                fifoType = parsedFIFOType;
            } catch (std::runtime_error e) {
                std::cerr << "Unknown command line option selection: --fifoType " << argv[i] << std::endl;
                exit(1);
            }
        }else if(strcmp(argv[i], "--schedHeur") == 0){
            i++; //Get the actual argument
            try{
                TopologicalSortParameters::Heuristic parsedHeuristic = TopologicalSortParameters::parseHeuristicStr(argv[i]);
                heuristic = parsedHeuristic;
            }catch(std::runtime_error e){
                std::cerr << "Unknown command line option selection: --schedHeur " << argv[i] << std::endl;
                exit(1);
            }
        }else if(strcmp(argv[i], "--randSeed") == 0){
            i++;
            std::string argStr = argv[i];
            try{
                unsigned long parsedRandSeed = std::stoul(argStr);
                randSeed = parsedRandSeed;
            }catch(std::invalid_argument e){
                std::cerr << "Invalid command line option type: --randSeed " << argv[i] << std::endl;
                exit(1);
            }
        }else if(strcmp(argv[i], "--blockSize") == 0) {
            i++;
            std::string argStr = argv[i];
            try {
                unsigned long parsedBlockSize = std::stoul(argStr);
                blockSize = parsedBlockSize;
                if(blockSize<1){
                    std::cerr << "Invalid command line option type: --blockSize must be >= 1.  Currently:  " << argv[i] << std::endl;
                    exit(1);
                }
            } catch (std::invalid_argument e) {
                std::cerr << "Invalid command line option type: --blockSize " << argv[i] << std::endl;
                exit(1);
            }
        }else if(strcmp(argv[i], "--fifoLength") == 0) {
            i++;
            std::string argStr = argv[i];
            try {
                unsigned long parsedFIFOLength = std::stoul(argStr);
                fifoLength = parsedFIFOLength;
                if(fifoLength<1){
                    std::cerr << "Invalid command line option type: --fifoLength must be >= 1.  Currently:  " << argv[i] << std::endl;
                    exit(1);
                }
            } catch (std::invalid_argument e) {
                std::cerr << "Invalid command line option type: --fifoLength " << argv[i] << std::endl;
                exit(1);
            }
        }else if(strcmp(argv[i], "--partitionMap") == 0) {
            i++;
            std::string argStr = argv[i];
            try {
                std::vector<NumericValue> partitionMapTmp = NumericValue::parseXMLString(argStr);

                for(int j = 0; j<partitionMapTmp.size(); j++){
                    if(partitionMapTmp[j].isComplex()){
                        std::cerr << "Invalid command line option type: --partitionMap. CPU numbers must be real" << argv[i] << std::endl;
                    }

                    if(partitionMapTmp[j].isFractional()){
                        std::cerr << "Invalid command line option type: --partitionMap. CPU numbers must be integers" << argv[i] << std::endl;
                    }

                    if(partitionMapTmp[j].isSigned()){
                        std::cerr << "Invalid command line option type: --partitionMap. CPU numbers must be unsigned" << argv[i] << std::endl;
                    }

                    partitionMap.push_back(partitionMapTmp[j].getRealInt());
                }

            } catch (std::invalid_argument e) {
                std::cerr << "Invalid command line option type: --partitionMap " << argv[i] << std::endl;
                exit(1);
            }
        }else if(strcmp(argv[i],  "--emitGraphMLSched") == 0){
            emitGraphMLSched = true;
        }else if(strcmp(argv[i],  "--printSched") == 0){
            printNodeSched = true;
        }else if(strcmp(argv[i],  "--threadDebugPrint") == 0){
            threadDebugPrint = true;
        }else{
            std::cerr << "Unknown command line option: " << argv[i] << std::endl;
            exit(1);
        }
    }

    TopologicalSortParameters topoParams = TopologicalSortParameters(heuristic, randSeed);

    std::string inputFilename = argv[1];
    std::string outputDir = argv[2];
    std::string designName = argv[3];

    std::cout << "Importing vitis GraphML File: " << inputFilename << std::endl;

    std::unique_ptr<Design> design;

    //Import
    try{
        design = GraphMLImporter::importGraphML(inputFilename, GraphMLDialect::VITIS);
    }catch(std::exception& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }

    //Expand the design to primitives
    try{
        design->expandToPrimitive();
    }catch(std::exception& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }

    //Assign node and arc IDs (needed for expanded nodes)
    design->assignNodeIDs();
    design->assignArcIDs();

    //Print Partitioner and Scheduler
    std::cout << "PARTITIONER: " << PartitionParams::partitionTypeToString(partitioner) << std::endl;
    std::cout << "FIFO_TYPE: " << ThreadCrossingFIFOParameters::threadCrossingFIFOTypeToString(fifoType) << std::endl;
    std::cout << "SCHED: " << SchedParams::schedTypeToString(sched) << std::endl;
    if(sched == SchedParams::SchedType::TOPOLOGICAL_CONTEXT || sched == SchedParams::SchedType::TOPOLOGICAL){
        std::cout << "SCHED_HEUR: " << TopologicalSortParameters::heuristicToString(topoParams.getHeuristic()) << std::endl;
        std::cout << "SCHED_RAND_SEED: " << topoParams.getRandSeed() << std::endl;
    }
    std::cout << "Block Size: " << blockSize << std::endl;
    std::cout << "FIFO Size: " << fifoLength << std::endl;

    bool propagatePartitionsFromSubsystems = true;

    if(partitioner == PartitionParams::PartitionType::MANUAL){
        propagatePartitionsFromSubsystems = true;
    }else{
        //TODO: Other cases?
    }

    //Emit threads, kernel (starter function), benchmarking driver, and makefile
//    try{
        design->emitMultiThreadedC(outputDir, designName, designName, sched, topoParams, fifoType, emitGraphMLSched, printNodeSched, fifoLength, blockSize, propagatePartitionsFromSubsystems, partitionMap, threadDebugPrint);
//    }catch(std::exception& e) {
//        std::cerr << e.what() << std::endl;
//        return 1;
//    }

    return 0;
}