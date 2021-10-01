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
#include "Emitter/MultiThreadEmit.h"
#include "General/ErrorHelpers.h"
#include "General/FileIOHelpers.h"
#include "General/TopologicalSortParameters.h"
#include "Flows/MultiThreadGenerator.h"

int main(int argc, char* argv[]) {


    //Print usage help
    if(argc < 4)
    {
        std::cout << "multiThreadedGenerator: Emit a design stored in a Vitis GraphML File to a Multi Threaded C Function" << std::endl;
        std::cout << std::endl;
        std::cout << "Usage: " << std::endl;
        std::cout << "    multiThreadedGenerator inputfile.graphml outputDir designName --partitioner <PARTITIONER> " << std::endl;
        std::cout << "                           --fifoType <FIFO_TYPE> --schedHeur <SCHED_HEUR> --randSeed <SCHED_RAND_SEED> " << std::endl;
        std::cout << "                           --blockSize <BLOCK_SIZE> --subBlockSize <SUB_BLOCK_SIZE>" << std::endl;
        std::cout << "                           --fifoLength <FIFO_LENGTH> --ioFifoSize <IO_FIFO_SIZE> " << std::endl;
        std::cout << "                           --partitionMap <PARTITION_MAP> <--emitGraphMLSched> <--printSched> " << std::endl;
        std::cout << "                           <--threadDebugPrint> <--printTelem> <--telemDumpPrefix> " << std::endl;
        std::cout << "                           --memAlignment <MEM_ALIGNMENT>" << std::endl;
        std::cout << "                           --fifoCachedIndexes <INDEX_CACHE_BEHAVIOR>" << std::endl;
        std::cout << "                           --fifoDoubleBuffering <FIFO_DOUBLE_BUFFERING>" << std::endl;
		std::cout << "                           <--useSCHED_FIFO> --pipeNameSuffix <PIPE_NAME_SUFFIX>" << std::endl;
        std::cout << "                           --telemLevel <TELEM_LEVEL> <--telemCheckBlockFreq> <--telemReportPeriodSec>" << std::endl;
        std::cout << std::endl;
        std::cout << "Possible PARTITIONER:" << std::endl;
        std::cout << "    manual <DEFAULT> = Partitioning is accomplished manually using VITIS_PARTITION directives" << std::endl;
        std::cout << std::endl;
        std::cout << "Possible FIFO_TYPE:" << std::endl;
        std::cout << "    lockeless_x86 = Lockless single producer, single consumer, FIFOs suitable for x86 based systems (coping to/from local buffers)" << std::endl;
        std::cout << "    lockeless_inplace_x86 <DEFAULT> = Lockless single producer, single consumer, FIFOs suitable for x86 based systems (using in place operations)" << std::endl;
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
        std::cout << "Possible SUB_BLOCK_SIZE (sub block size in samples):" << std::endl;
        std::cout << "    unsigned long subBlockSize <DEFAULT = 1>" << std::endl;
        std::cout << std::endl;
        std::cout << "Possible FIFO_LENGTH (length of FIFOs in blocks):" << std::endl;
        std::cout << "    unsigned long fifoLength <DEFAULT = 16>" << std::endl;
        std::cout << std::endl;
        std::cout << "Possible IO_FIFO_SIZE (size of IO FIFO in blocks - shared memory FIFOs only):" << std::endl;
        std::cout << "    unsigned long fifoLength <DEFAULT = 16>" << std::endl;
        std::cout << std::endl;
        std::cout << "Possible PARTITION_MAP (mapping of partition numbers to logical CPUs):" << std::endl;
        std::cout << "    A comma separated array without spaces (ex. [0,1,2,3])" << std::endl;
        std::cout << "    The first element of the array corresponds to the I/O thread.  The subsequent elements" << std::endl;
        std::cout << "    correspond to partition 0, 1, 2, etc..." << std::endl;
        std::cout << "    An empty array ([] or no argument) results in code that does not restrict threads to specific CPUs" << std::endl;
        std::cout << std::endl;
        std::cout << "Possible MEM_ALIGNMENT (the alignment in bytes used when allocating FIFO buffers):" << std::endl;
        std::cout << "    unsigned long memAlignment <DEFAULT = 64>" << std::endl;
        std::cout << std::endl;
        std::cout << "Possible INDEX_CACHE_BEHAVIOR (FIFO index behavior):" << std::endl;
        std::cout << "    none <Default>          = the FIFO indexes are fetched for each block" << std::endl;
        std::cout << "    producer_consumer_cache = the producer and consumer do not fetch indexes unless it cannot be determined if the FIFO is available based on prior information" << std::endl;
        std::cout << "    producer_cache          = only the producer does not fetch indexes unless it cannot be determined if the FIFO is available based on prior information" << std::endl;
        std::cout << "    consumer_cache          = only the consumer does not fetch indexes unless it cannot be determined if the FIFO is available based on prior information" << std::endl;
        std::cout << std::endl;
        std::cout << "Possible FIFO_DOUBLE_BUFFERING (FIFO double buffering behavior - required in-place FIFO):" << std::endl;
        std::cout << "    none <Default>   = No double buffering" << std::endl;
        std::cout << "    input_and_output = Double buffering on both inputs and outputs" << std::endl;
        std::cout << "    input            = Double buffering on inputs only" << std::endl;
        std::cout << "    output           = Double buffering on outputs only" << std::endl;
        std::cout << std::endl;
        std::cout << "Possible TELEM_LEVEL (Telemetry Collection Level):" << std::endl;
        std::cout << "    none <Default>  = No telemetry collection" << std::endl;
        std::cout << "    breakdown       = Collects timing telemetry with a breakdown of the different phases of thread execution" << std::endl;
        std::cout << "    rateOnly        = Collects timing telemetry but only reports overall processing rate (not broken down into phases)" << std::endl;
        std::cout << "    papiBreakdown   = Collects timing telemetry with a breakdown of the different phases of thread execution.  PAPI counters are collected each time telemetry is reported and is not broken down into different phases of thread execution" << std::endl;
        std::cout << "    papiComputeOnly = Collects timing telemetry with a breakdown of the different phases of thread execution.  PAPI counters are collected only durring the compute phase of thread execution.  Due to isolating the compute segment, this mode incurs substantial telemetry overhead" << std::endl;
        std::cout << "    papiRateOnly    = Collects timing telemetry but only reports overall processing rate (not broken down into phases).  PAPI counters are collected each time telemetry is reported and is not broken down into different phases of thread execution" << std::endl;
        std::cout << "    ioBreakdown     = Collects timing telemetry in I/O thread only with a breakdown of the different phases of thread execution" << std::endl;
        std::cout << "    ioRateOnly      = Collects timing telemetry in I/O thread only only rate reported" << std::endl;
        std::cout << std::endl;
        return 1;
    }

    PartitionParams::PartitionType partitioner = PartitionParams::PartitionType::MANUAL;
    ThreadCrossingFIFOParameters::ThreadCrossingFIFOType fifoType = ThreadCrossingFIFOParameters::ThreadCrossingFIFOType::LOCKLESS_X86;
    SchedParams::SchedType sched = SchedParams::SchedType::TOPOLOGICAL_CONTEXT;//This is the only supported scheduler for multi-threaded emit
    TopologicalSortParameters::Heuristic heuristic = TopologicalSortParameters::Heuristic::BFS;
    unsigned long randSeed = 4;
    unsigned long blockSize = 1;
    unsigned long subBlockSize = 1;
    unsigned long fifoLength = 16;
    unsigned long ioFifoSize = 16;
    std::vector<int> partitionMap;
    unsigned long memAlignment = 64;

    bool emitGraphMLSched = false;
    bool printNodeSched = false;
    bool threadDebugPrint = false;
    bool printTelem = false;
    bool useSCHEDFIFO = false;
    std::string telemDumpPrefix = "";
    std::string pipeNameSuffix = "";
    PartitionParams::FIFOIndexCachingBehavior fifoIndexCachingBehavior = PartitionParams::FIFOIndexCachingBehavior::NONE;
    MultiThreadEmit::ComputeIODoubleBufferType fifoDoubleBuffer = MultiThreadEmit::ComputeIODoubleBufferType::NONE;

    EmitterHelpers::TelemetryLevel telemLevel = EmitterHelpers::TelemetryLevel::NONE;
    int telemCheckBlockFreq = 100;
    double telemReportPeriodSec = 1.0;

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
        }else if(strcmp(argv[i], "--subBlockSize") == 0) {
            i++;
            std::string argStr = argv[i];
            try {
                unsigned long parsedSubBlockSize = std::stoul(argStr);
                subBlockSize = parsedSubBlockSize;
                if(blockSize<1){
                    std::cerr << "Invalid command line option type: --subBlockSize must be >= 1.  Currently:  " << argv[i] << std::endl;
                    exit(1);
                }
            } catch (std::invalid_argument e) {
                std::cerr << "Invalid command line option type: --subBlockSize " << argv[i] << std::endl;
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
        }else if(strcmp(argv[i], "--ioFifoSize") == 0) {
            i++;
            std::string argStr = argv[i];
            try {
                unsigned long parsedIO_FIFOSize = std::stoul(argStr);
                ioFifoSize = parsedIO_FIFOSize;
                if(ioFifoSize<1){
                    std::cerr << "Invalid command line option type: --ioFifoSize must be >= 1.  Currently:  " << argv[i] << std::endl;
                    exit(1);
                }
            } catch (std::invalid_argument e) {
                std::cerr << "Invalid command line option type: --fifoLength " << argv[i] << std::endl;
                exit(1);
            }
        }else if(strcmp(argv[i], "--memAlignment") == 0) {
            i++;
            std::string argStr = argv[i];
            try {
                unsigned long parsedMemAlignment = std::stoul(argStr);
                memAlignment = parsedMemAlignment;
                if(parsedMemAlignment<1){
                    std::cerr << "Invalid command line option type: --memAlignment must be >= 1.  Currently:  " << argv[i] << std::endl;
                    exit(1);
                }
            } catch (std::invalid_argument e) {
                std::cerr << "Invalid command line option type: --memAlignment " << argv[i] << std::endl;
                exit(1);
            }
        }else if(strcmp(argv[i], "--telemLevel") == 0) {
            i++;
            std::string argStr = argv[i];
            try {
                EmitterHelpers::TelemetryLevel telemLevelParsed = EmitterHelpers::parseTelemetryLevelStr(argStr);
                telemLevel = telemLevelParsed;
            } catch (std::invalid_argument e) {
                std::cerr << "Invalid command line option type: --telemLevel " << argv[i] << std::endl;
                exit(1);
            }
        }else if(strcmp(argv[i], "--telemCheckBlockFreq") == 0){
            i++; //Get the actual argument
            try{
                int parsedTelemCheckBlockFreq = std::stoi(argv[i]);
                if(parsedTelemCheckBlockFreq > 0) {
                    telemCheckBlockFreq = parsedTelemCheckBlockFreq;
                }else{
                    std::cerr << "Invalid command line option type: --telemCheckBlockFreq must be > 0.  Currently:  " << argv[i] << std::endl;
                    exit(1);
                }
            }catch(std::runtime_error e){
                std::cerr << "Unknown command line option selection: --telemCheckBlockFreq " << argv[i] << std::endl;
                exit(1);
            }
        }else if(strcmp(argv[i], "--telemReportPeriodSec") == 0){
            i++; //Get the actual argument
            try{
                int parsedTelemReportPeriodSec = std::stod(argv[i]);
                if(parsedTelemReportPeriodSec >= 0) {
                    telemReportPeriodSec = parsedTelemReportPeriodSec;
                }else{
                    std::cerr << "Invalid command line option type: --telemReportPeriodSec must be >= 0.  Currently:  " << argv[i] << std::endl;
                    exit(1);
                }
            }catch(std::runtime_error e){
                std::cerr << "Unknown command line option selection: --telemCheckBlockFreq " << argv[i] << std::endl;
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
        }else if(strcmp(argv[i], "--fifoCachedIndexes") == 0){
            i++; //Get the actual argument
            try{
                PartitionParams::FIFOIndexCachingBehavior parsedFIFOIndexCacheBehavior = PartitionParams::parseFIFOIndexCachingBehavior(argv[i]);
                 fifoIndexCachingBehavior = parsedFIFOIndexCacheBehavior;
            }catch(std::runtime_error e){
                std::cerr << "Unknown command line option selection: --fifoCachedIndexes " << argv[i] << std::endl;
                exit(1);
            }
        }else if(strcmp(argv[i], "--fifoDoubleBuffering") == 0){
            i++; //Get the actual argument
            try{
                MultiThreadEmit::ComputeIODoubleBufferType parsedFifoDoubleBuffer = MultiThreadEmit::parseComputeIODoubleBufferType(argv[i]);
                fifoDoubleBuffer = parsedFifoDoubleBuffer;
            }catch(std::runtime_error e){
                std::cerr << "Unknown command line option selection: --fifoDoubleBuffering " << argv[i] << std::endl;
                exit(1);
            }
        }else if(strcmp(argv[i], "--telemDumpPrefix") == 0) {
            i++;
            telemDumpPrefix = argv[i];
        }else if(strcmp(argv[i], "--pipeNameSuffix") == 0) {
            i++;
            pipeNameSuffix = argv[i];
        }else if(strcmp(argv[i],  "--emitGraphMLSched") == 0){
            emitGraphMLSched = true;
        }else if(strcmp(argv[i],  "--printSched") == 0){
            printNodeSched = true;
        }else if(strcmp(argv[i],  "--threadDebugPrint") == 0){
            threadDebugPrint = true;
        }else if(strcmp(argv[i],  "--printTelem") == 0){
            printTelem = true;
        }else if(strcmp(argv[i],  "--useSCHED_FIFO") == 0){
            useSCHEDFIFO = true;
        }else{
            std::cerr << "Unknown command line option: " << argv[i] << std::endl;
            exit(1);
        }
    }

    //Double Buffer can only be used with in-place FIFOs
    if(fifoDoubleBuffer != MultiThreadEmit::ComputeIODoubleBufferType::NONE &&
       fifoType != ThreadCrossingFIFOParameters::ThreadCrossingFIFOType::LOCKLESS_INPLACE_X86){
        std::cerr << "Double Buffering Requires in-place FIFOs" << std::endl;
        return 1;
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

    //Validate after expansion
    design->validateNodes();

    //Print Partitioner and Scheduler
    std::cout << "PARTITIONER: " << PartitionParams::partitionTypeToString(partitioner) << std::endl;
    std::cout << "FIFO_TYPE: " << ThreadCrossingFIFOParameters::threadCrossingFIFOTypeToString(fifoType) << std::endl;
    std::cout << "FIFO_DOUBLE_BUFFERING: " << MultiThreadEmit::computeIODoubleBufferTypeToString(fifoDoubleBuffer) << std::endl;
    std::cout << "FIFO_INDEX_CACHE_BEHAVIOR: " << PartitionParams::fifoIndexCachingBehaviorToString(fifoIndexCachingBehavior) << std::endl;
    std::cout << "SCHED: " << SchedParams::schedTypeToString(sched) << std::endl;
    if(sched == SchedParams::SchedType::TOPOLOGICAL_CONTEXT || sched == SchedParams::SchedType::TOPOLOGICAL){
        std::cout << "SCHED_HEUR: " << TopologicalSortParameters::heuristicToString(topoParams.getHeuristic()) << std::endl;
        std::cout << "SCHED_RAND_SEED: " << topoParams.getRandSeed() << std::endl;
    }
    std::cout << "Block Size: " << blockSize << std::endl;
    std::cout << "Sub-Block Size: " << subBlockSize << std::endl;
    std::cout << "FIFO Size: " << fifoLength << std::endl;

    bool propagatePartitionsFromSubsystems = true;

    if(partitioner == PartitionParams::PartitionType::MANUAL){
        propagatePartitionsFromSubsystems = true;
    }else{
        //TODO: Other cases?
    }

    FileIOHelpers::createDirectoryIfDoesNotExist(outputDir, true);

    //Emit threads, kernel (starter function), benchmarking driver, and makefile
    try{
        MultiThreadGenerator::emitMultiThreadedC(*design, outputDir, designName, designName, sched, topoParams,
                                                 fifoType, emitGraphMLSched, printNodeSched, fifoLength, blockSize,
                                                 subBlockSize, propagatePartitionsFromSubsystems, partitionMap, threadDebugPrint,
                                                 ioFifoSize, printTelem, telemDumpPrefix, telemLevel,
                                                 telemCheckBlockFreq, telemReportPeriodSec, memAlignment,
                                                 useSCHEDFIFO, fifoIndexCachingBehavior, fifoDoubleBuffer,
                                                 pipeNameSuffix);
    }catch(std::exception& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }

    return 0;
}
