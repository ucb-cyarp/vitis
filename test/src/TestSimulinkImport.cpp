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

#include "GraphTestHelper.h"

TEST(SimulinkImport, SimpleDesign) {
    //==== Import File ====
    std::string inputFile = "./stimulus/simulink/basic/simple.graphml";
    std::unique_ptr<Design> design = SimulinkGraphMLImporter::importSimulinkGraphML(inputFile);

    ASSERT_EQ(design->getNodes().size(), 4);

    //==== Traverse Design and Check ====

    //Start at Input
    std::shared_ptr<MasterInput> inputs = design->getInputMaster();
    ASSERT_EQ(inputs->getOutputPorts().size(), 2);

    std::shared_ptr<Port> inPort0 = inputs->getOutputPort(0);
    std::set<std::shared_ptr<Arc>> inPort0Arcs = inPort0->getArcs();
    ASSERT_EQ(inPort0Arcs.size(), 1);
    std::shared_ptr<Arc> inPort0Arc = *(inPort0->arcsBegin());

    //Check Arc
    std::shared_ptr<Port> srcPort = inPort0Arc->getSrcPort();
    ASSERT_EQ(inPort0, srcPort);

    GraphTestHelper::verifyArcLinks(inPort0Arc);
    DataType inPort0ArcDataType = inPort0Arc->getDataType();
    GraphTestHelper::verifyDataType(inPort0ArcDataType, false, false, false, 16, 0, 1);

    std::shared_ptr<Port> dstPort = inPort0Arc->getDstPort();


}