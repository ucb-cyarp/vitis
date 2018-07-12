//
// Created by Christopher Yarp on 7/9/18.
//

#include <memory>
#include <vector>
#include <set>
#include "gtest/gtest.h"
#include "GraphMLTools/SimulinkGraphMLImporter.h"
#include "GraphCore/Design.h"
#include "GraphCore/Port.h"
#include "MasterNodes/MasterInput.h"
#include "MasterNodes/MasterOutput.h"
#include "MasterNodes/MasterUnconnected.h"
#include "PrimitiveNodes/Sum.h"
#include "PrimitiveNodes/Product.h"
#include "PrimitiveNodes/Delay.h"

#include "GraphTestHelper.h"
#include "SimpleDesignValidator.h"

TEST(SimulinkImport, SimpleDesign) {
    //==== Import File ====
    std::string inputFile = "./stimulus/simulink/basic/simple.graphml";
    std::unique_ptr<Design> design = SimulinkGraphMLImporter::importSimulinkGraphML(inputFile);

    SCOPED_TRACE("");
    SimpleDesignValidator::validate(*design);
}