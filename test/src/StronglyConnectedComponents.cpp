//
// Created by Christopher Yarp on 2019-02-18.
//

#include "gtest/gtest.h"

#include "GraphCore/Design.h"
#include "GraphMLTools/GraphMLImporter.h"
#include "General/GraphAlgs.h"

#include "GraphTestHelper.h"

TEST(StronglyConnectedComponents, SimpleTestWithSubsystem) {
//==== Import Simulink File ====
std::string simulinkFile = "./stimulus/simulink/stronglyConnectedComponents/simpleConnectedComponentTest.graphml";
std::unique_ptr<Design> design;
{
SCOPED_TRACE("Importing Simulink File");
design = GraphMLImporter::importGraphML(simulinkFile, GraphMLDialect::SIMULINK_EXPORT);
}

//Expand the design to primitives
{
SCOPED_TRACE("Expanding to Primitives");
design->expandToPrimitive();
}

//Assign node and arc IDs (needed for expanded nodes)
design->assignNodeIDs();
design->assignArcIDs();

//Validate after expansion
design->validateNodes();

std::vector<std::shared_ptr<Node>> nodes = design->getNodes();
std::cout << "Nodes:" << std::endl;
for(const std::shared_ptr<Node> node : nodes){
    std::cout << node->getFullyQualifiedName() << " [ID: " << node->getId() << "]" << std::endl;
}
std::cout << std::endl;

//Get strongly connected components
std::set<std::shared_ptr<Node>> masterNodes = design->getMasterNodes();
std::set<std::set<std::shared_ptr<Node>>> stronglyConnectedComponents = GraphAlgs::findStronglyConnectedComponents(design->getNodes(), masterNodes);

int count = 0;
for(const std::set<std::shared_ptr<Node>> &connectedComponent : stronglyConnectedComponents){
    std::cout << "Connected Component: " << count << std::endl;
    count++;

    for(const std::shared_ptr<Node> &node : connectedComponent){
        std::cout << "\t" << node->getFullyQualifiedName() << " [ID: " << node->getId() << "]" << std::endl;
    }
}

//Begin verification
{
    SCOPED_TRACE("Check number of nodes");
    ASSERT_EQ(design->getNodes().size(), 15);
}

{
    SCOPED_TRACE("Check number of strongly connected components");
    ASSERT_EQ(stronglyConnectedComponents.size(), 9);
}

//Expected Strongly Connected Components
std::set<std::set<std::shared_ptr<Node>>> expectedSCCs;
std::set<std::shared_ptr<Node>> scc1 = {GraphTestHelper::findNodeByName(*design, "SCC1_Product1"),
                                        GraphTestHelper::findNodeByName(*design, "SCC1_Product2"),
                                        GraphTestHelper::findNodeByName(*design, "SCC1_Sum1"),
                                        GraphTestHelper::findNodeByName(*design, "SCC1_Delay1"),
                                        GraphTestHelper::findNodeByName(*design, "SCC1_Product3")};
expectedSCCs.insert(scc1);
std::set<std::shared_ptr<Node>> scc2 = {GraphTestHelper::findNodeByName(*design, "SCC2_Product1"),
                                        GraphTestHelper::findNodeByName(*design, "SCC2_Delay1"),
                                        GraphTestHelper::findNodeByName(*design, "SCC2_Product2")};
expectedSCCs.insert(scc2);
std::set<std::shared_ptr<Node>> scc3 = {GraphTestHelper::findNodeByName(*design, "SCCX_Constant1")};
expectedSCCs.insert(scc3);
std::set<std::shared_ptr<Node>> scc4 = {GraphTestHelper::findNodeByName(*design, "SCCX_Constant2")};
expectedSCCs.insert(scc4);
std::set<std::shared_ptr<Node>> scc5 = {GraphTestHelper::findNodeByName(*design, "SCCX_Constant3")};
expectedSCCs.insert(scc5);
std::set<std::shared_ptr<Node>> scc6 = {GraphTestHelper::findNodeByName(*design, "SCCX_Sum1")};
expectedSCCs.insert(scc6);
std::set<std::shared_ptr<Node>> scc7 = {GraphTestHelper::findNodeByName(*design, "SCCX_Product1")};
expectedSCCs.insert(scc7);
std::set<std::shared_ptr<Node>> scc8 = {GraphTestHelper::findNodeByName(*design, "SCCX_Subsystem1")};
expectedSCCs.insert(scc8);
std::set<std::shared_ptr<Node>> scc9 = {GraphTestHelper::findNodeByName(*design, "SCCX_Constant4")};
expectedSCCs.insert(scc9);

{
    SCOPED_TRACE("Check content of strongly connected components");
    ASSERT_EQ(stronglyConnectedComponents, expectedSCCs);
}
}