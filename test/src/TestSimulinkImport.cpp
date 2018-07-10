//
// Created by Christopher Yarp on 7/9/18.
//

#include "gtest/gtest.h"
#include "GraphMLTools/SimulinkGraphMLImporter.h"

TEST(SimulinkImport, SimpleDesign) {
    std::string inputFile = "./stimulus/simulink/basic/simple.graphml";

    std::unique_ptr<Design> design = SimulinkGraphMLImporter::importSimulinkGraphML(inputFile);
}