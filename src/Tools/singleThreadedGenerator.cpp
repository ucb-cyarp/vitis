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

int main(int argc, char* argv[]) {
    SchedParams::SchedType sched = SchedParams::SchedType::TOPOLOGICAL_CONTEXT;

    if(argc < 4  || argc > 5)
    {
        std::cout << "singleThreadedGenerator: Emit a design stored in a Vitis GraphML File to a Single Threaded C Function" << std::endl;
        std::cout << std::endl;
        std::cout << "Usage: " << std::endl;
        std::cout << "    singleThreadedGenerator inputfile.graphml outputDir designName <SCHED>" << std::endl;
        std::cout << std::endl;
        std::cout << "Possible SCHED:" << std::endl;
        std::cout << "    bottomUp = Bottom Up Scheduler (Does not Support EnabledSubsystems)" << std::endl;
        std::cout << "    topological = Topological Sort Scheduler (Does not Support EnabledSubsystems)" << std::endl;
        std::cout << "    topological_context <DEFAULT> = Topological Sort Scheduler with Contexts" << std::endl;


        return 1;
    }else if(argc == 5){
        try{
            SchedParams::SchedType parsedSched = SchedParams::parseSchedTypeStr(argv[4]);
            sched = parsedSched;
        }catch(std::runtime_error e){
            throw std::runtime_error("Unknown Scheduler, Possible SCHED: bottomUp, topological");
        }
    }

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

    //Emit C
    try{
        design->generateSingleThreadedC(outputDir, designName, sched);
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