//
// Created by Christopher Yarp on 6/28/18.
//

#include <iostream>
#include <string>
#include <memory>
#include "GraphMLTools/SimulinkGraphMLImporter.h"
#include "GraphCore/Design.h"

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

    std::unique_ptr<Design> design = SimulinkGraphMLImporter::importSimulinkGraphML(inputFilename);


    return 0;
}