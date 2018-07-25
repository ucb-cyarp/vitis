//
// Created by Christopher Yarp on 7/24/18.
//

#include "NestedSubSystemValidator.h"

#include "PrimitiveNodes/Sum.h"
#include "PrimitiveNodes/Delay.h"
#include "PrimitiveNodes/Product.h"
#include "GraphCore/SubSystem.h"

void NestedSubSystemValidator::validate(Design &design) {
    //==== Check Design Counts ====
    ASSERT_EQ(design.getNodes().size(), 11); //11 Nodes besides the master nodes
    ASSERT_EQ(design.getTopLevelNodes().size(), 4); //4 Nodes
    ASSERT_EQ(design.getArcs().size(), 15); //15 arcs

    //==== Traverse Design and Check ====

    //++++ Check Master Node Sizes & Names ++++
    std::shared_ptr<MasterInput> inputs = design.getInputMaster();
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyPortArcCounts(inputs, 0, std::vector<unsigned long>{1, 4});

    }
    ASSERT_EQ(inputs->getName(), "Input Master");

    std::shared_ptr<MasterOutput> outputs = design.getOutputMaster();
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyPortArcCounts(outputs, 3, std::vector<unsigned long>{});
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
    //---<Sum>---
    std::shared_ptr<Node> sum = design.getNodeByNamePath(std::vector<std::string>{"Sum"});
    ASSERT_NE(sum, nullptr);
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyPortArcCounts(sum, 2, std::vector<unsigned long>{1});
        GraphTestHelper::assertCast<Node, Sum>(sum);
    }
    std::shared_ptr<Sum> sumCast = std::dynamic_pointer_cast<Sum>(sum);

    //Check Input Signs
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyVector(sumCast->getInputSign(), std::vector<bool>{true, true});
    }
    //---</Sum>----

    //---<Delay>---
    std::shared_ptr<Node> delay = design.getNodeByNamePath(std::vector<std::string>{"Delay"});
    ASSERT_NE(delay, nullptr);
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyPortArcCounts(delay, 1, std::vector<unsigned long>{2});
        GraphTestHelper::assertCast<Node, Delay>(delay);
    }
    std::shared_ptr<Delay> delayCast = std::dynamic_pointer_cast<Delay>(delay);

    //Check Delay Val and Init Cond
    {
        SCOPED_TRACE("");
        ASSERT_EQ(delayCast->getDelayValue(), 1);
        GraphTestHelper::verifyVector(delayCast->getInitCondition(), std::vector<NumericValue>{NumericValue((long int) 0)});
    }
    //---</Delay>----

    //---<subsys_1>---
    std::shared_ptr<Node> subsys_1 = design.getNodeByNamePath(std::vector<std::string>{"subsys_1"});
    ASSERT_NE(subsys_1, nullptr);
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyPortArcCounts(subsys_1, 0, std::vector<unsigned long>{}); //Subsystem is not directly connected to.  It serves as a container
        GraphTestHelper::assertCast<Node, SubSystem>(subsys_1);
    }
    //Children will be checked via hierarchical traversal to find nodes by name.  Also checked by full hierarchy traversal.
    //---</subsys_1>----

    //---<subsys_2>---
    std::shared_ptr<Node> subsys_2 = design.getNodeByNamePath(std::vector<std::string>{"subsys_2"});
    ASSERT_NE(subsys_2, nullptr);
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyPortArcCounts(subsys_2, 0, std::vector<unsigned long>{}); //Subsystem is not directly connected to.  It serves as a container
        GraphTestHelper::assertCast<Node, SubSystem>(subsys_2);
    }
    //Children will be checked via hierarchical traversal to find nodes by name.  Also checked by full hierarchy traversal.
    //---</subsys_2>----

    //####</Top Lvl>####

    //####<subsys_1>####
    //---<subsys_1/Product>---
    std::shared_ptr<Node> subsys_1_Product = design.getNodeByNamePath(std::vector<std::string>{"subsys_1", "Product"});
    ASSERT_NE(subsys_1_Product, nullptr);
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyPortArcCounts(subsys_1_Product, 2, std::vector<unsigned long>{1});
        GraphTestHelper::assertCast<Node, Product>(subsys_1_Product);
    }
    std::shared_ptr<Product> subsys_1_ProductCast = std::dynamic_pointer_cast<Product>(subsys_1_Product);

    //Check Input Ops
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyVector(subsys_1_ProductCast->getInputOp(), std::vector<bool>{true, true});
    }
    //---</subsys_1/Product>----

    //---<subsys_1/Delay1>---
    std::shared_ptr<Node> subsys_1_Delay1 = design.getNodeByNamePath(std::vector<std::string>{"subsys_1", "Delay1"});
    ASSERT_NE(subsys_1_Delay1, nullptr);
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyPortArcCounts(subsys_1_Delay1, 1, std::vector<unsigned long>{1});
        GraphTestHelper::assertCast<Node, Delay>(subsys_1_Delay1);
    }
    std::shared_ptr<Delay> subsys_1_Delay1Cast = std::dynamic_pointer_cast<Delay>(subsys_1_Delay1);

    //Check Delay Value and Init Connd
    {
        SCOPED_TRACE("");
        ASSERT_EQ(subsys_1_Delay1Cast->getDelayValue(), 1);
        GraphTestHelper::verifyVector(subsys_1_Delay1Cast->getInitCondition(), std::vector<NumericValue>{NumericValue((long int) 0)});
    }
    //---</subsys_1/Delay1>----
    //####</subsys_1>####

    //####<subsys_2>####
    //---<subsys_2/Product>---
    std::shared_ptr<Node> subsys_2_Product = design.getNodeByNamePath(std::vector<std::string>{"subsys_2", "Product"});
    ASSERT_NE(subsys_2_Product, nullptr);
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyPortArcCounts(subsys_2_Product, 2, std::vector<unsigned long>{1});
        GraphTestHelper::assertCast<Node, Product>(subsys_2_Product);
    }
    std::shared_ptr<Product> subsys_2_ProductCast = std::dynamic_pointer_cast<Product>(subsys_2_Product);

    //Check Input Ops
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyVector(subsys_2_ProductCast->getInputOp(), std::vector<bool>{true, true});
    }
    //---</subsys_2/Product>----

    //---<subsys_2/Delay1>---
    std::shared_ptr<Node> subsys_2_Delay1 = design.getNodeByNamePath(std::vector<std::string>{"subsys_2", "Delay1"});
    ASSERT_NE(subsys_2_Delay1, nullptr);
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyPortArcCounts(subsys_2_Delay1, 1, std::vector<unsigned long>{2});
        GraphTestHelper::assertCast<Node, Delay>(subsys_2_Delay1);
    }
    std::shared_ptr<Delay> subsys_2_Delay1Cast = std::dynamic_pointer_cast<Delay>(subsys_2_Delay1);

    //Check Delay Value and Init Connd
    {
        SCOPED_TRACE("");
        ASSERT_EQ(subsys_2_Delay1Cast->getDelayValue(), 1);
        GraphTestHelper::verifyVector(subsys_2_Delay1Cast->getInitCondition(), std::vector<NumericValue>{NumericValue((long int) 0)});
    }
    //---</subsys_2/Delay1>----

    //---<subsys_2/sub_sub_sys>---
    std::shared_ptr<Node> subsys_2_sub_sub_sys = design.getNodeByNamePath(std::vector<std::string>{"subsys_2", "sub_sub_sys"});
    ASSERT_NE(subsys_2_sub_sub_sys, nullptr);
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyPortArcCounts(subsys_2_sub_sub_sys, 0, std::vector<unsigned long>{}); //Subsystem is not directly connected to.  It serves as a container
        GraphTestHelper::assertCast<Node, SubSystem>(subsys_2_sub_sub_sys);
    }
    //Children will be checked via hierarchical traversal to find nodes by name.  Also checked by full hierarchy traversal.
    //---</subsys_2/sub_sub_sys>----
    //####</subsys_2>####

    //####<subsys_2/sub_sub_sys>####
    //---<subsys_2/sub_sub_sys/Product>---
    std::shared_ptr<Node> subsys_2_sub_sub_sys_Product = design.getNodeByNamePath(std::vector<std::string>{"subsys_2", "sub_sub_sys", "Product"});
    ASSERT_NE(subsys_2_sub_sub_sys_Product, nullptr);
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyPortArcCounts(subsys_2_sub_sub_sys_Product, 2, std::vector<unsigned long>{1});
        GraphTestHelper::assertCast<Node, Product>(subsys_2_sub_sub_sys_Product);
    }
    std::shared_ptr<Product> subsys_2_sub_sub_sys_ProductCast = std::dynamic_pointer_cast<Product>(subsys_2_sub_sub_sys_Product);

    //Check Input Ops
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyVector(subsys_2_sub_sub_sys_ProductCast->getInputOp(), std::vector<bool>{true, true});
    }
    //---</subsys_2/sub_sub_sys/Product>----

    //---<subsys_2/sub_sub_sys/Delay1>---
    std::shared_ptr<Node> subsys_2_sub_sub_sys_Delay1 = design.getNodeByNamePath(std::vector<std::string>{"subsys_2", "sub_sub_sys", "Delay1"});
    ASSERT_NE(subsys_2_sub_sub_sys_Delay1, nullptr);
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyPortArcCounts(subsys_2_sub_sub_sys_Delay1, 1, std::vector<unsigned long>{1});
        GraphTestHelper::assertCast<Node, Delay>(subsys_2_sub_sub_sys_Delay1);
    }
    std::shared_ptr<Delay> subsys_2_sub_sub_sys_Delay1Cast = std::dynamic_pointer_cast<Delay>(subsys_2_sub_sub_sys_Delay1);

    //Check Delay Value and Init Connd
    {
        SCOPED_TRACE("");
        ASSERT_EQ(subsys_2_Delay1Cast->getDelayValue(), 1);
        GraphTestHelper::verifyVector(subsys_2_sub_sub_sys_Delay1Cast->getInitCondition(), std::vector<NumericValue>{NumericValue((long int) 0)});
    }
    //---</subsys_2/sub_sub_sys/Delay1>----
    //####</subsys_2/sub_sub_sys>####

    //++++ Check arcs ++++
    //If we check all of them, the check ensures they are all in the design arc vector.
    //Since we checked the total number of arcs, and we validated each node's port occupancy,
    //we can be sure we are not missing arcs

    //Some common types in this design
    DataType real_uint16_width1 = DataType(false, false, false, 16, 0, 1);
    DataType real_uint32_width1 = DataType(false, false, false, 32, 0, 1);
    DataType real_ufix48_width1 = DataType(false, false, false, 48, 0, 1);

    unsigned long arcCheckCount = 0;

    //Master Input Arcs (6)
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyConnection(design, inputs, 0, sum, GraphTestHelper::InPortType::STANDARD, 0, real_uint16_width1, arcCheckCount);
    }
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyConnection(design, inputs, 1, subsys_1_Product, GraphTestHelper::InPortType::STANDARD, 1, real_uint16_width1, arcCheckCount);
    }
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyConnection(design, inputs, 1, sum, GraphTestHelper::InPortType::STANDARD, 1, real_uint16_width1, arcCheckCount);
    }
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyConnection(design, inputs, 1, subsys_2_Product, GraphTestHelper::InPortType::STANDARD, 1, real_uint16_width1, arcCheckCount);
    }
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyConnection(design, inputs, 1, subsys_2_sub_sub_sys_Product, GraphTestHelper::InPortType::STANDARD, 1, real_uint16_width1, arcCheckCount);
    }

    //Sum Arcs (1)
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyConnection(design, sum, 0, delay, GraphTestHelper::InPortType::STANDARD, 0, real_uint16_width1, arcCheckCount);
    }

    //Delay Arcs (2)
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyConnection(design, delay, 0, subsys_1_Product, GraphTestHelper::InPortType::STANDARD, 0, real_uint16_width1, arcCheckCount);
    }
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyConnection(design, delay, 0, subsys_2_Product, GraphTestHelper::InPortType::STANDARD, 0, real_uint16_width1, arcCheckCount);
    }

    //subsys_1/Product Arcs (1)
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyConnection(design, subsys_1_Product, 0, subsys_1_Delay1, GraphTestHelper::InPortType::STANDARD, 0, real_uint32_width1, arcCheckCount);
    }

    //subsys_1/Delay1 Arcs (1)
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyConnection(design, subsys_1_Delay1, 0, outputs, GraphTestHelper::InPortType::STANDARD, 0, real_uint32_width1, arcCheckCount);
    }

    //subsys_2/Product Arcs (1)
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyConnection(design, subsys_2_Product, 0, subsys_2_Delay1, GraphTestHelper::InPortType::STANDARD, 0, real_uint32_width1, arcCheckCount);
    }

    //subsys_2/Delay1 Arcs (2)
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyConnection(design, subsys_2_Delay1, 0, outputs, GraphTestHelper::InPortType::STANDARD, 1, real_uint32_width1, arcCheckCount);
    }
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyConnection(design, subsys_2_Delay1, 0, subsys_2_sub_sub_sys_Product, GraphTestHelper::InPortType::STANDARD, 0, real_uint32_width1, arcCheckCount);
    }

    //subsys_2/sub_sub_sys/Product Arcs (1)
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyConnection(design, subsys_2_sub_sub_sys_Product, 0, subsys_2_sub_sub_sys_Delay1, GraphTestHelper::InPortType::STANDARD, 0, real_ufix48_width1, arcCheckCount);
    }

    //subsys_2/sub_sub_sys/Delay1 Arcs (1)
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyConnection(design, subsys_2_sub_sub_sys_Delay1, 0, outputs, GraphTestHelper::InPortType::STANDARD, 2, real_ufix48_width1, arcCheckCount);
    }

    //Checked all 16 arcs
    ASSERT_EQ(arcCheckCount, design.getArcs().size()) << "Did not check all arcs in design or checked more arcs than what is in design";
}