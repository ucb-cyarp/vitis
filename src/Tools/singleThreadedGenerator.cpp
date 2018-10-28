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
    Design::SchedType sched = Design::SchedType::BOTTOM_UP;

    if(argc < 4  || argc > 5)
    {
        std::cout << "singleThreadedGenerator: Emit a design stored in a Vitis GraphML File to a Single Threaded C Function" << std::endl;
        std::cout << std::endl;
        std::cout << "Usage: " << std::endl;
        std::cout << "    singleThreadedGenerator inputfile.graphml outputDir designName <SCHED>" << std::endl;
        std::cout << std::endl;
        std::cout << "Possible SCHED:" << std::endl;
        std::cout << "    bottomUp <DEFAULT> = Bottom Up Scheduler" << std::endl;
        std::cout << "    topological = Topological Sort Scheduler" << std::endl;
        std::cout << "    topological_context = Topological Sort Scheduler with Contexts" << std::endl;


        return 1;
    }else if(argc == 5){
        try{
            Design::SchedType parsedSched = Design::parseSchedTypeStr(argv[4]);
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

    //Print Scheduler
    std::cout << "SCHED: " << Design::schedTypeToString(sched) << std::endl;

    //Emit C
    std::cout << "Emitting C File: " << outputDir << "/" << designName << ".h" << std::endl;
    std::cout << "Emitting C File: " << outputDir << "/" << designName << ".c" << std::endl;

    try{
        if(sched == Design::SchedType::BOTTOM_UP)
            design->emitSingleThreadedC(outputDir, designName, designName, false);
        else if(sched == Design::SchedType::TOPOLOGICAL) {
            design->scheduleTopologicalStort(true);
            design->verifyTopologicalOrder();
            design->emitSingleThreadedC(outputDir, designName, designName, true);
        }else if(sched == Design::SchedType::TOPOLOGICAL_CONTEXT){
            //TODO: Impelement
            throw std::runtime_error("Topological Context not yet implemented");
        }else{
            throw std::runtime_error("Unknown SCHED Type");
        }
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
    std::cout << "Emitting Makefile: " << outputDir << "/Makefile_noPCM_" << designName << "_const_mem" << std::endl;

    try{
        design->emitSingleThreadedCBenchmarkingDrivers(outputDir, designName, designName);
    }catch(std::exception& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }

    return 0;
}