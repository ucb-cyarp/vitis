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

int main(int argc, char* argv[]) {
    SchedParams::SchedType sched = SchedParams::SchedType::TOPOLOGICAL_CONTEXT;


    if(argc < 4  || argc > 9)
    {
        std::cout << "singleThreadedGenerator: Emit a design stored in a Vitis GraphML File to a Single Threaded C Function" << std::endl;
        std::cout << std::endl;
        std::cout << "Usage: " << std::endl;
        std::cout << "    singleThreadedGenerator inputfile.graphml outputDir designName <SCHED> <SCHED_HEUR> <SCHED_RAND_SEED> <EMIT_GRAPHML_SCHED> <PRINT_SCHED>" << std::endl;
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
        std::cout << "Possible EMIT_GRAPHML_SCHED (only applies to topological and topological_context):" << std::endl;
        std::cout << "    EMIT_GRAPHML_SCHED  = Emit a GraphML file with the computed schedule as a node parameter" << std::endl;
        std::cout << "    <nothing> <DEFAULT> = Do not emit a GraphML file with the computed schedule" << std::endl;
        std::cout << std::endl;
        std::cout << "Possible PRINT_SCHED (only applies to topological and topological_context):" << std::endl;
        std::cout << "    PRINT_SCHED         = Print the node schedule to the console" << std::endl;
        std::cout << "    <nothing> <DEFAULT> = Do not print the node schedule" << std::endl;
        std::cout << std::endl;

        return 1;
    }

    TopologicalSortParameters::Heuristic heuristic = TopologicalSortParameters::Heuristic::BFS;
    unsigned long randSeed = 0;

    bool emitGraphMLSched = false;
    bool printNodeSched = false;

    for(unsigned long i = 4; i<argc; i++){
        bool found = false;

        try{
            SchedParams::SchedType parsedSched = SchedParams::parseSchedTypeStr(argv[i]);
            sched = parsedSched;
            found = true;
            continue;
        }catch(std::runtime_error e){
            ;
        }

        try{
            TopologicalSortParameters::Heuristic parsedHeuristic = TopologicalSortParameters::parseHeuristicStr(argv[i]);
            heuristic = parsedHeuristic;
            found = true;
            continue;
        }catch(std::runtime_error e){
            ;
        }

        std::string argStr = argv[i];
        try{
            unsigned long parsedRandSeed = std::stoul(argStr);
            randSeed = parsedRandSeed;
            found = true;
            continue;
        }catch(std::invalid_argument e){
            ;
        }

        if(argStr == "EMIT_GRAPHML_SCHED" || argStr == "emit_graphml_sched"){
            emitGraphMLSched = true;
            found = true;
            continue;
        }

        if(argStr == "PRINT_SCHED" || argStr == "print_sched"){
            printNodeSched = true;
            found = true;
            continue;
        }

        if(!found){
            throw std::runtime_error(ErrorHelpers::genErrorStr("Unable to parse parameter: " + argStr));
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

    //Print Scheduler
    std::cout << "SCHED: " << SchedParams::schedTypeToString(sched) << std::endl;

    if(sched == SchedParams::SchedType::TOPOLOGICAL_CONTEXT || sched == SchedParams::SchedType::TOPOLOGICAL){
        std::cout << "SCHED_HEUR: " << TopologicalSortParameters::heuristicToString(topoParams.getHeuristic()) << std::endl;
        std::cout << "SCHED_RAND_SEED: " << topoParams.getRandSeed() << std::endl;
    }

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

    //Emit C
    try{
        design->generateSingleThreadedC(outputDir, designName, sched, topoParams, emitGraphMLSched, printNodeSched);
    }catch(std::exception& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }

    std::cout << "Emitting CPP File: " << outputDir << "/" << designName << "_benchmark_kernel.h" << std::endl;
    std::cout << "Emitting CPP File: " << outputDir << "/" << designName << "_benchmark_kernel.cpp" << std::endl;
    std::cout << "Emitting CPP File: " << outputDir << "/" << designName +"_benchmark_driver.cpp" << std::endl;
    std::cout << "Emitting Makefile: " << outputDir << "/Makefile_" << designName << "_const" << std::endl;
    std::cout << "Emitting Makefile: " << outputDir << "/Makefile_noPCM_" << designName << "_const" << std::endl;

    std::cout << "Emitting CPP File: " << outputDir << "/" << designName << "_benchmark_kernel_mem.h" << std::endl;
    std::cout << "Emitting CPP File: " << outputDir << "/" << designName << "_benchmark_kernel_mem.cpp" << std::endl;
    std::cout << "Emitting CPP File: " << outputDir << "/" << designName +"_benchmark_driver_mem.cpp" << std::endl;
    std::cout << "Emitting Makefile: " << outputDir << "/Makefile_" << designName << "_mem" << std::endl;
    std::cout << "Emitting Makefile: " << outputDir << "/Makefile_noPCM_" << designName << "_mem" << std::endl;

    try{
        design->emitSingleThreadedCBenchmarkingDrivers(outputDir, designName, designName);
    }catch(std::exception& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }

    return 0;
}