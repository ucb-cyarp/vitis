//
// Created by Christopher Yarp on 10/7/21.
//

#include "DSPTestHelper.h"
#include "General/TopologicalSortParameters.h"
#include "General/FileIOHelpers.h"
#include "Flows/MultiThreadGenerator.h"
#include <iostream>

void DSPTestHelper::runMultithreadGenForSinglePartitionDefaultSettings(Design &design, std::string outputDir, std::string designName){
    PartitionParams::PartitionType partitioner = PartitionParams::PartitionType::MANUAL;
    ThreadCrossingFIFOParameters::ThreadCrossingFIFOType fifoType = ThreadCrossingFIFOParameters::ThreadCrossingFIFOType::LOCKLESS_X86;
    SchedParams::SchedType sched = SchedParams::SchedType::TOPOLOGICAL_CONTEXT;//This is the only supported scheduler for multi-threaded emit
    TopologicalSortParameters::Heuristic heuristic = TopologicalSortParameters::Heuristic::BFS;
    unsigned long randSeed = 4;
    unsigned long blockSize = 120;
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

    //Double Buffer can only be used with in-place FIFOs
    if(fifoDoubleBuffer != MultiThreadEmit::ComputeIODoubleBufferType::NONE &&
       fifoType != ThreadCrossingFIFOParameters::ThreadCrossingFIFOType::LOCKLESS_INPLACE_X86){
        throw std::runtime_error("Double Buffering Requires in-place FIFOs");
    }

    TopologicalSortParameters topoParams = TopologicalSortParameters(heuristic, randSeed);

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

    //Set to single partition
    design.setSinglePartition(0);

    MultiThreadGenerator::emitMultiThreadedC(design, outputDir, designName, designName, sched, topoParams,
                                             fifoType, emitGraphMLSched, printNodeSched, fifoLength, blockSize,
                                             subBlockSize, propagatePartitionsFromSubsystems, partitionMap, threadDebugPrint,
                                             ioFifoSize, printTelem, telemDumpPrefix, telemLevel,
                                             telemCheckBlockFreq, telemReportPeriodSec, memAlignment,
                                             useSCHEDFIFO, fifoIndexCachingBehavior, fifoDoubleBuffer,
                                             pipeNameSuffix);

}