//
// Created by Christopher Yarp on 6/28/18.
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
        std::cout << "simulinkGraphMLImporter: Import Simulink GraphML File and Write a vitis GraphML File" << std::endl;
        std::cout << "Usage: " << std::endl;
        std::cout << "    simulinkGraphMLImporter inputfile.graphml outputfile.graphml" << std::endl;

        return 1;
    }

    std::string inputFilename = argv[1];
    std::string outputFilename = argv[2];

    std::cout << "Importing Simulink GraphML File:  " << inputFilename << std::endl;
    std::cout << "Converting to vitis GraphML File: " << outputFilename << std::endl;

    std::unique_ptr<Design> design;

    //Assign node and arc IDs if they were not assigned already (should be a rare occurance)
    design->assignNodeIDs();
    design->assignArcIDs();

    //Import
    try{
        design = GraphMLImporter::importGraphML(inputFilename, GraphMLDialect::SIMULINK_EXPORT);
    }catch(std::exception& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }

    //Export
    try{
        GraphMLExporter::exportGraphML(outputFilename, *design);
    }catch(std::exception& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }

    return 0;
}