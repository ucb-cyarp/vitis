//
// Created by Christopher Yarp on 8/9/18.
//

#include <iostream>
#include <fstream>
#include <string>
#include <memory>
#include "GraphMLTools/GraphMLImporter.h"
#include "GraphMLTools/GraphMLExporter.h"
#include "GraphCore/Design.h"
#include "GraphMLTools/GraphMLDialect.h"
#include "General/ErrorHelpers.h"
#include "General/FileIOHelpers.h"

int main(int argc, char* argv[]) {
    SchedParams::SchedType sched = SchedParams::SchedType::TOPOLOGICAL_CONTEXT;

    //Print usage help
    if(argc < 4)
    {
        std::cout << "singleThreadedGenerator: Emit a design stored in a Vitis GraphML File to a Single Threaded C Function" << std::endl;
        std::cout << std::endl;
        std::cout << "Usage: " << std::endl;
        std::cout << "    singleThreadedGenerator inputfile.graphml outputDir designName --sched <SCHED> --schedHeur <SCHED_HEUR> --randSeed <SCHED_RAND_SEED> --blockSize <BLOCK_SIZE> <--emitGraphMLSched> <--printSched>" << std::endl;
        std::cout << std::endl;
        std::cout << "Possible SCHED:" << std::endl;
        std::cout << "    bottomUp                      = Bottom Up Scheduler (Does not Support EnabledSubsystems)" << std::endl;
        std::cout << "    topological                   = Topological Sort Scheduler (Does not Support EnabledSubsystems)" << std::endl;
        std::cout << "    topological_context <DEFAULT> = Topological Sort Scheduler with Contexts" << std::endl;
        std::cout << std::endl;
        std::cout << "Possible SCHED_HEUR (only applies to topological and topological_context):" << std::endl;
        std::cout << "    bfs <DEFAULT> = Breadth First Search Style" << std::endl;
        std::cout << "    dfs           = Depth First Search Style" << std::endl;
        std::cout << "    random        = Random Style" << std::endl;
        std::cout << std::endl;
        std::cout << "Possible SCHED_RAND_SEED (only applies to topological and topological_context):" << std::endl;
        std::cout << "    unsigned long seed <DEFAULT = 0>" << std::endl;
        std::cout << std::endl;
        std::cout << "Possible BLOCK_SIZE:" << std::endl;
        std::cout << "    unsigned long blockSize <DEFAULT = 1>" << std::endl;
        std::cout << std::endl;

        return 1;
    }

    TopologicalSortParameters::Heuristic heuristic = TopologicalSortParameters::Heuristic::BFS;
    unsigned long randSeed = 0;
    unsigned long blockSize = 1;

    bool emitGraphMLSched = false;
    bool printNodeSched = false;

    //Check for command line parameters
    for(unsigned long i = 4; i<argc; i++){
        if(strcmp(argv[i],  "--sched") == 0) {
            i++; //Get the actual argument
            try{
                SchedParams::SchedType parsedSched = SchedParams::parseSchedTypeStr(argv[i]);
                sched = parsedSched;
            }catch(std::runtime_error e){
                std::cerr << "Unknown command line option selection: --sched " << argv[i] << std::endl;
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
        }else if(strcmp(argv[i],  "--emitGraphMLSched") == 0){
            emitGraphMLSched = true;
        }else if(strcmp(argv[i],  "--printSched") == 0){
            printNodeSched = true;
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

//    std::cerr << "Top Lvl Nodes: " << design->getTopLevelNodes().size() << std::endl;

    //Expand the design to primitives
    try{
        design->expandToPrimitive();
    }catch(std::exception& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }

//    std::cerr << "Top Lvl Nodes: " << design->getTopLevelNodes().size() << std::endl;

    //Assign node and arc IDs (needed for expanded nodes)
    design->assignNodeIDs();
    design->assignArcIDs();

    //Validate after expansion
    design->validateNodes();

    //Print Scheduler
    std::cout << "SCHED: " << SchedParams::schedTypeToString(sched) << std::endl;

    if(sched == SchedParams::SchedType::TOPOLOGICAL_CONTEXT || sched == SchedParams::SchedType::TOPOLOGICAL){
        std::cout << "SCHED_HEUR: " << TopologicalSortParameters::heuristicToString(topoParams.getHeuristic()) << std::endl;
        std::cout << "SCHED_RAND_SEED: " << topoParams.getRandSeed() << std::endl;
    }
    std::cout << "Block Size: " << blockSize << std::endl;

//    std::vector<std::shared_ptr<Node>> nodes = design->getNodes();
//    std::cout << "Nodes:" << std::endl;
//    for(unsigned long i = 0; i<nodes.size(); i++){
//        std::cout << "[" << i << "]: " << nodes[i]->getFullyQualifiedName() << std::endl;
//    }
//
//    std::vector<std::shared_ptr<Arc>> arcs = design->getArcs();
//    std::cout << "Arcs:" << std::endl;
//    for(unsigned long i = 0; i<nodes.size(); i++){
//        std::cout << "[" << i << "]: " << arcs[i]->getSrcPort()->getParent()->getFullyQualifiedName() << " -> " << arcs[i]->getDstPort()->getParent()->getFullyQualifiedName() << std::endl;
//    }

    FileIOHelpers::createDirectoryIfDoesNotExist(outputDir, true);

    //Emit C
    try{
        design->generateSingleThreadedC(outputDir, designName, sched, topoParams, blockSize, emitGraphMLSched, printNodeSched);
    }catch(std::exception& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }

    std::cout << "Emitting CPP File: " << outputDir << "/" << designName << "_benchmark_kernel.h" << std::endl;
    std::cout << "Emitting CPP File: " << outputDir << "/" << designName << "_benchmark_kernel.cpp" << std::endl;
    std::cout << "Emitting CPP File: " << outputDir << "/" << designName +"_benchmark_driver.cpp" << std::endl;
    std::cout << "Emitting Makefile: " << outputDir << "/Makefile_" << designName << "_const" << std::endl;

    std::cout << "Emitting CPP File: " << outputDir << "/" << designName << "_benchmark_kernel_mem.h" << std::endl;
    std::cout << "Emitting CPP File: " << outputDir << "/" << designName << "_benchmark_kernel_mem.cpp" << std::endl;
    std::cout << "Emitting CPP File: " << outputDir << "/" << designName +"_benchmark_driver_mem.cpp" << std::endl;
    std::cout << "Emitting Makefile: " << outputDir << "/Makefile_" << designName << "_mem" << std::endl;

    try{
        design->emitSingleThreadedCBenchmarkingDrivers(outputDir, designName, designName, blockSize);
    }catch(std::exception& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }

    return 0;
}