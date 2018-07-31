//
// Created by Christopher Yarp on 7/18/18.
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

    if(argc != 3)
    {
        std::cout << "designExpander: Expand a design stored in a Vitis GraphML File" << std::endl;
        std::cout << "Usage: " << std::endl;
        std::cout << "    designExpander inputfile.graphml outputfile.graphml" << std::endl;

        return 1;
    }

    std::string inputFilename = argv[1];
    std::string outputFilename = argv[2];

    std::cout << "Importing vitis GraphML File: " << inputFilename << std::endl;
    std::cout << "Exporting vitis GraphML File: " << outputFilename << std::endl;

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

    //Export
    try{
        GraphMLExporter::exportGraphML(outputFilename, *design);
    }catch(std::exception& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }

    return 0;
}