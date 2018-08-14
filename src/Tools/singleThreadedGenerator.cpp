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

    if(argc != 4)
    {
        std::cout << "singleThreadedGenerator: Emit a design stored in a Vitis GraphML File to a Single Threaded C Function" << std::endl;
        std::cout << "Usage: " << std::endl;
        std::cout << "    singleThreadedGenerator inputfile.graphml outputDir designName" << std::endl;

        return 1;
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
    design->expandToPrimitive();

    //Assign node and arc IDs (needed for expanded nodes)
    design->assignNodeIDs();
    design->assignArcIDs();

    //Emit C
    std::cout << "Emitting C File: " << outputDir << "/" << designName << ".h" << std::endl;
    std::cout << "Emitting C File: " << outputDir << "/" << designName << ".c" << std::endl;
    std::cout << "Emitting CPP File: " << outputDir << "/" << designName << "_benchmark_kernel.h" << std::endl;
    std::cout << "Emitting CPP File: " << outputDir << "/" << designName << "_benchmark_kernel.cpp" << std::endl;
    std::cout << "Emitting CPP File: " << outputDir << "/" << designName +"_benchmark_driver.cpp" << std::endl;
    std::cout << "Emitting Makefile: " << outputDir << "/Makefile_" << designName << std::endl;
    std::cout << "Emitting Makefile: " << outputDir << "/Makefile_noPCM_" << designName << std::endl;

    design->emitSingleThreadedC(outputDir, designName, designName);

    return 0;
}