//
// Created by Christopher Yarp on 7/5/18.
//

#include <iostream>
#include <string>
#include <memory>
#include "GraphMLTools/SimulinkGraphMLImporter.h"
#include "GraphCore/Design.h"

int main(int argc, char* argv[]) {

    if(argc != 2)
    {
        std::cout << "simulinkGraphmlDOMPrint: Print DOM Nodes of GraphML File" << std::endl;
        std::cout << "Usage: " << std::endl;
        std::cout << "    simulinkGraphmlDOMPrint inputfile.graphml" << std::endl;

        return 1;
    }

    std::string inputFilename = argv[1];

    std::cout << "Reading GraphML File:  " << inputFilename << std::endl;

    std::unique_ptr<Design> design = SimulinkGraphMLImporter::importSimulinkGraphML(inputFilename);

    return 0;
}