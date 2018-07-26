//
// Created by Christopher Yarp on 7/25/18.
//

#include "LUTDesignValidator.h"

void LUTDesignValidator::validate(Design &design) {
    //==== Check Design Counts ====
    ASSERT_EQ(design.getNodes().size(), 1); //1 Node besides the master nodes
    ASSERT_EQ(design.getTopLevelNodes().size(), 1); //1 Node
    ASSERT_EQ(design.getArcs().size(), 2); //2 arcs

    //==== Traverse Design and Check ====

    //++++ Check Master Node Sizes & Names ++++
    std::shared_ptr<MasterInput> inputs = design.getInputMaster();
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyPortArcCounts(inputs, 0, std::vector<unsigned long>{1});

    }
    ASSERT_EQ(inputs->getName(), "Input Master");

    std::shared_ptr<MasterOutput> outputs = design.getOutputMaster();
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyPortArcCounts(outputs, 1, std::vector<unsigned long>{});
    }
    ASSERT_EQ(outputs->getName(), "Output Master");

    std::shared_ptr<MasterOutput> terminator = design.getTerminatorMaster();
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyPortArcCounts(terminator, 0, std::vector<unsigned long>{});
    }
    ASSERT_EQ(terminator->getName(), "Terminator Master");

    std::shared_ptr<MasterOutput> vis = design.getVisMaster();
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyPortArcCounts(vis, 0, std::vector<unsigned long>{});
    }
    ASSERT_EQ(vis->getName(), "Visualization Master");

    std::shared_ptr<MasterUnconnected> unconnected = design.getUnconnectedMaster();
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyPortArcCounts(unconnected, 0, std::vector<unsigned long>{});
    }
    ASSERT_EQ(unconnected->getName(), "Unconnected Master");


    //++++ Verify port numbers of all nodes ++++
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyNodePortNumbers(design);
    }

    //++++ Check top level nodes ++++
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyNodesWithoutParentAreInTopLevelList(design);
    }

    //++++ Verify nodes in traversal are in design node vector (and no others) ++++
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyNodesInDesignFromHierarchicalTraversal(design);
    }

    //++++ Find nodes by name and check their properties & port occupancy ++++
    //####<Top Lvl>####
    //---<1-D Lookup Table>---
    std::shared_ptr<Node> lut = design.getNodeByNamePath(std::vector<std::string>{"1-D Lookup\nTable"});
    ASSERT_NE(lut, nullptr);
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyPortArcCounts(lut, 1, std::vector<unsigned long>{1});
        GraphTestHelper::assertCast<Node, LUT>(lut);
    }
    std::shared_ptr<LUT> lutCast = std::dynamic_pointer_cast<LUT>(lut);

    //Check Input Signs
    {
        SCOPED_TRACE("");
        ASSERT_EQ(lutCast->getBreakpoints().size(), 1); //1D Lookup Table
        double magnitudeEpsilon = 0.0001;
        GraphTestHelper::verifyVector(lutCast->getBreakpoints()[0], std::vector<NumericValue>{NumericValue((long int) -5),
                                                                                              NumericValue((long int) -4),
                                                                                              NumericValue((long int) -3),
                                                                                              NumericValue((long int) -2),
                                                                                              NumericValue((long int) -1),
                                                                                              NumericValue((long int) 0),
                                                                                              NumericValue((long int) 1),
                                                                                              NumericValue((long int) 2),
                                                                                              NumericValue((long int) 3),
                                                                                              NumericValue((long int) 4),
                                                                                              NumericValue((long int) 5)});
        GraphTestHelper::verifyVector(lutCast->getTableData(), std::vector<NumericValue>{NumericValue(std::atan(-5)),
                                                                                              NumericValue(std::atan(-4)),
                                                                                              NumericValue(std::atan(-3)),
                                                                                              NumericValue(std::atan(-2)),
                                                                                              NumericValue(std::atan(-1)),
                                                                                              NumericValue(std::atan(0)),
                                                                                              NumericValue(std::atan(1)),
                                                                                              NumericValue(std::atan(2)),
                                                                                              NumericValue(std::atan(3)),
                                                                                              NumericValue(std::atan(4)),
                                                                                              NumericValue(std::atan(5))},
                                                                                              magnitudeEpsilon);

    }
    //---</1-D Lookup Table>----
    //####</Top Lvl>####

    //++++ Check arcs ++++
    //If we check all of them, the check ensures they are all in the design arc vector.
    //Since we checked the total number of arcs, and we validated each node's port occupancy,
    //we can be sure we are not missing arcs

    //Some common types in this design
    DataType real_double_width1 = DataType(true, true, false, 64, 0, 1);

    unsigned long arcCheckCount = 0;

    //Master Input Arcs (1)
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyConnection(design, inputs, 0, lut, GraphTestHelper::InPortType::STANDARD, 0, real_double_width1, arcCheckCount);
    }

    //LUT Arcs (1)
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyConnection(design, lut, 0, outputs, GraphTestHelper::InPortType::STANDARD, 0, real_double_width1, arcCheckCount);
    }
}