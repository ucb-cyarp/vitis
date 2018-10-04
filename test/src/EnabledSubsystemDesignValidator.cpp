//
// Created by Christopher Yarp on 7/26/18.
//

#include "EnabledSubsystemDesignValidator.h"

#include "GraphCore/SubSystem.h"
#include "GraphCore/EnabledSubSystem.h"
#include "PrimitiveNodes/Sum.h"
#include "PrimitiveNodes/Delay.h"
#include "PrimitiveNodes/Constant.h"
#include "PrimitiveNodes/Product.h"
#include "PrimitiveNodes/Compare.h"

void EnabledSubsystemDesignValidator::validate(Design &design) {
    //==== Check Design Counts ====
    ASSERT_EQ(design.getNodes().size(), 53); //53 Nodes besides the master nodes
    ASSERT_EQ(design.getTopLevelNodes().size(), 9); //1 Node
    ASSERT_EQ(design.getArcs().size(), 81); //2 arcs

    //==== Traverse Design and Check ====

    //++++ Check Master Node Sizes & Names ++++
    std::shared_ptr<MasterInput> inputs = design.getInputMaster();
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyPortArcCounts(inputs, 0, std::vector<unsigned long>{1, 9, 1});

    }
    ASSERT_EQ(inputs->getName(), "Input Master");
    ASSERT_EQ(inputs->getOutputPort(0)->getName(), "In1");
    ASSERT_EQ(inputs->getOutputPort(1)->getName(), "In2");
    ASSERT_EQ(inputs->getOutputPort(2)->getName(), "In3");

    std::shared_ptr<MasterOutput> outputs = design.getOutputMaster();
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyPortArcCounts(outputs, 10, std::vector<unsigned long>{});
    }
    ASSERT_EQ(outputs->getName(), "Output Master");
    //Note, the Out names to not match the port numbers for this example
    ASSERT_EQ(outputs->getInputPort(0)->getName(), "Out1");
    ASSERT_EQ(outputs->getInputPort(1)->getName(), "Out4");
    ASSERT_EQ(outputs->getInputPort(2)->getName(), "Out2");
    ASSERT_EQ(outputs->getInputPort(3)->getName(), "Out3");
    ASSERT_EQ(outputs->getInputPort(4)->getName(), "Out5");
    ASSERT_EQ(outputs->getInputPort(5)->getName(), "Out6");
    ASSERT_EQ(outputs->getInputPort(6)->getName(), "Out7");
    ASSERT_EQ(outputs->getInputPort(7)->getName(), "Out8");
    ASSERT_EQ(outputs->getInputPort(8)->getName(), "Out9");
    ASSERT_EQ(outputs->getInputPort(9)->getName(), "Out10");

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
    unsigned long nodeCheckCount = 0;

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

    //Check Params
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyVector(sumCast->getInputSign(), std::vector<bool>{true, true});
    }
    nodeCheckCount++;
    //---</Sum>----

    //---<Delay>---
    std::shared_ptr<Node> delay = design.getNodeByNamePath(std::vector<std::string>{"Delay"});
    ASSERT_NE(delay, nullptr);
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyPortArcCounts(delay, 1, std::vector<unsigned long>{6});
        GraphTestHelper::assertCast<Node, Delay>(delay);
    }
    std::shared_ptr<Delay> delayCast = std::dynamic_pointer_cast<Delay>(delay);

    //Check Params
    {
        SCOPED_TRACE("");
        ASSERT_EQ(delayCast->getDelayValue(), 1);
        GraphTestHelper::verifyVector(delayCast->getInitCondition(), std::vector<NumericValue>{NumericValue((long int) 0)});
    }
    nodeCheckCount++;
    //---</Delay>----

    //---<Compare To Constant>---
    std::shared_ptr<Node> compareToConstant = design.getNodeByNamePath(std::vector<std::string>{"Compare\nTo Constant"});
    ASSERT_NE(compareToConstant, nullptr);
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyPortArcCounts(compareToConstant, 0, std::vector<unsigned long>{}); //However, connections are to internal nodes and not to subsystem directly.
        GraphTestHelper::assertCast<Node, SubSystem>(compareToConstant);
    }
    nodeCheckCount++;
    //---</Compare To Constant>----

    //---<subsys_1>---
    std::shared_ptr<Node> subsys_1 = design.getNodeByNamePath(std::vector<std::string>{"subsys_1"});
    ASSERT_NE(subsys_1, nullptr);
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyPortArcCounts(subsys_1, 0, std::vector<unsigned long>{}); //Connections are to internal nodes and not to subsystem directly.
        GraphTestHelper::assertCast<Node, SubSystem>(subsys_1);
    }
    nodeCheckCount++;
    //---</subsys_1>----

    //---<subsys_1_en>---
    std::shared_ptr<Node> subsys_1_en = design.getNodeByNamePath(std::vector<std::string>{"subsys_1_en"});
    ASSERT_NE(subsys_1_en, nullptr);
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyPortArcCounts(subsys_1_en, 0, std::vector<unsigned long>{}); //Connections are to internal nodes and not to subsystem directly.
        GraphTestHelper::assertCast<Node, EnabledSubSystem>(subsys_1_en);
    }
    nodeCheckCount++;
    //---</subsys_1_en>----

    //---<subsys_2>---
    std::shared_ptr<Node> subsys_2 = design.getNodeByNamePath(std::vector<std::string>{"subsys_2"});
    ASSERT_NE(subsys_2, nullptr);
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyPortArcCounts(subsys_2, 0, std::vector<unsigned long>{}); //Connections are to internal nodes and not to subsystem directly.
        GraphTestHelper::assertCast<Node, SubSystem>(subsys_2);
    }
    nodeCheckCount++;
    //---</subsys_2>----

    //---<subsys_2_en1>---
    std::shared_ptr<Node> subsys_2_en1 = design.getNodeByNamePath(std::vector<std::string>{"subsys_2_en1"});
    ASSERT_NE(subsys_2_en1, nullptr);
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyPortArcCounts(subsys_2_en1, 0, std::vector<unsigned long>{}); //Connections are to internal nodes and not to subsystem directly.
        GraphTestHelper::assertCast<Node, EnabledSubSystem>(subsys_2_en1);
    }
    nodeCheckCount++;
    //---</subsys_2_en1>----

    //---<subsys_2_en2>---
    std::shared_ptr<Node> subsys_2_en2 = design.getNodeByNamePath(std::vector<std::string>{"subsys_2_en2"});
    ASSERT_NE(subsys_2_en2, nullptr);
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyPortArcCounts(subsys_2_en2, 0, std::vector<unsigned long>{}); //Connections are to internal nodes and not to subsystem directly.
        GraphTestHelper::assertCast<Node, EnabledSubSystem>(subsys_2_en2);
    }
    nodeCheckCount++;
    //---</subsys_2_en2>----

    //---<subsys_2_en3>---
    std::shared_ptr<Node> subsys_2_en3 = design.getNodeByNamePath(std::vector<std::string>{"subsys_2_en3"});
    ASSERT_NE(subsys_2_en3, nullptr);
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyPortArcCounts(subsys_2_en3, 0, std::vector<unsigned long>{}); //Connections are to internal nodes and not to subsystem directly.
        GraphTestHelper::assertCast<Node, SubSystem>(subsys_2_en3);
    }
    nodeCheckCount++;
    //---</subsys_2_en3>----
    //####</Top Lvl>####

    //####<Compare To Constant>####
    //---<Compare To Constant/Constant>---
    std::shared_ptr<Node> compareToConstant_Constant = design.getNodeByNamePath(std::vector<std::string>{"Compare\nTo Constant", "Constant"});
    ASSERT_NE(delay, nullptr);
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyPortArcCounts(compareToConstant_Constant, 0, std::vector<unsigned long>{1});
        GraphTestHelper::assertCast<Node, Constant>(compareToConstant_Constant);
    }
    std::shared_ptr<Constant> compareToConstant_ConstantCast = std::dynamic_pointer_cast<Constant>(compareToConstant_Constant);

    //Check Params
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyVector(compareToConstant_ConstantCast->getValue(), std::vector<NumericValue>{NumericValue((long int) 3)});
    }
    nodeCheckCount++;
    //---</Compare To Constant/Constant>----

    //---<Compare To Constant/Compare>---
    std::shared_ptr<Node> compareToConstant_Compare = design.getNodeByNamePath(std::vector<std::string>{"Compare\nTo Constant", "Compare"});
    ASSERT_NE(delay, nullptr);
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyPortArcCounts(compareToConstant_Compare, 2, std::vector<unsigned long>{16});
        GraphTestHelper::assertCast<Node, Compare>(compareToConstant_Compare);
    }
    std::shared_ptr<Compare> compareToConstant_CompareCast = std::dynamic_pointer_cast<Compare>(compareToConstant_Compare);

    //Check Params
    ASSERT_EQ(compareToConstant_CompareCast->getCompareOp(), Compare::CompareOp::LEQ);
    nodeCheckCount++;
    //---</Compare To Constant/Compare>----
    //####</Compare To Constant>####

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

    //Check Params
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyVector(subsys_1_ProductCast->getInputOp(), std::vector<bool>{true, true});
    }
    nodeCheckCount++;
    //---</subsys_1/Product>----

    //---<subsys_1/Delay>---
    std::shared_ptr<Node> subsys_1_Delay1 = design.getNodeByNamePath(std::vector<std::string>{"subsys_1", "Delay1"});
    ASSERT_NE(subsys_1_Delay1, nullptr);
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyPortArcCounts(subsys_1_Delay1, 1, std::vector<unsigned long>{1});
        GraphTestHelper::assertCast<Node, Delay>(subsys_1_Delay1);
    }
    std::shared_ptr<Delay> subsys_1_Delay1Cast = std::dynamic_pointer_cast<Delay>(subsys_1_Delay1);

    //Check Params
    {
        SCOPED_TRACE("");
        ASSERT_EQ(subsys_1_Delay1Cast->getDelayValue(), 1);
        GraphTestHelper::verifyVector(subsys_1_Delay1Cast->getInitCondition(), std::vector<NumericValue>{NumericValue((long int) 0)});
    }
    nodeCheckCount++;
    //---</subsys_1/Delay>----
    //####</subsys_1>####

    //####<subsys_1_en>####
    //**** EnableInputs (Take Name from Port in Simulink) ****
    //---<subsys_1_en/In1>---
    std::shared_ptr<Node> subsys_1_en_In1 = design.getNodeByNamePath(std::vector<std::string>{"subsys_1_en", "In1"});
    ASSERT_NE(subsys_1_en_In1, nullptr);
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyPortArcCounts(subsys_1_en_In1, 1, std::vector<unsigned long>{1}); //Connections are to internal nodes and not to subsystem directly.
        GraphTestHelper::assertCast<Node, EnableInput>(subsys_1_en_In1);
    }
    nodeCheckCount++;
    //---</subsys_1_en/In1>----

    //---<subsys_1_en/In2>---
    std::shared_ptr<Node> subsys_1_en_In2 = design.getNodeByNamePath(std::vector<std::string>{"subsys_1_en", "In2"});
    ASSERT_NE(subsys_1_en_In2, nullptr);
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyPortArcCounts(subsys_1_en_In2, 1, std::vector<unsigned long>{1}); //Connections are to internal nodes and not to subsystem directly.
        GraphTestHelper::assertCast<Node, EnableInput>(subsys_1_en_In2);
    }
    nodeCheckCount++;
    //---</subsys_1_en/In2>----

    //**** EnableOutput (Take Name from Port in Simulink) ****

    //---<subsys_1_en/Out1>---
    std::shared_ptr<Node> subsys_1_en_Out1 = design.getNodeByNamePath(std::vector<std::string>{"subsys_1_en", "Out1"});
    ASSERT_NE(subsys_1_en_Out1, nullptr);
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyPortArcCounts(subsys_1_en_Out1, 1, std::vector<unsigned long>{1}); //Connections are to internal nodes and not to subsystem directly.
        GraphTestHelper::assertCast<Node, EnableOutput>(subsys_1_en_Out1);
    }
    nodeCheckCount++;
    //---</subsys_1_en/Out1>----

    //**** Other Nodes ****
    //---<subsys_1_en/Product>---
    std::shared_ptr<Node> subsys_1_en_Product = design.getNodeByNamePath(std::vector<std::string>{"subsys_1_en", "Product"});
    ASSERT_NE(subsys_1_en_Product, nullptr);
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyPortArcCounts(subsys_1_en_Product, 2, std::vector<unsigned long>{1});
        GraphTestHelper::assertCast<Node, Product>(subsys_1_en_Product);
    }
    std::shared_ptr<Product> subsys_1_en_ProductCast = std::dynamic_pointer_cast<Product>(subsys_1_en_Product);

    //Check Params
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyVector(subsys_1_en_ProductCast->getInputOp(), std::vector<bool>{true, true});
    }
    nodeCheckCount++;
    //---</subsys_1_en/Product>----

    //---<subsys_1_en/Delay>---
    std::shared_ptr<Node> subsys_1_en_Delay1 = design.getNodeByNamePath(std::vector<std::string>{"subsys_1_en", "Delay1"});
    ASSERT_NE(subsys_1_en_Delay1, nullptr);
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyPortArcCounts(subsys_1_en_Delay1, 1, std::vector<unsigned long>{1});
        GraphTestHelper::assertCast<Node, Delay>(subsys_1_en_Delay1);
    }
    std::shared_ptr<Delay> subsys_1_en_Delay1Cast = std::dynamic_pointer_cast<Delay>(subsys_1_en_Delay1);

    //Check Params
    {
        SCOPED_TRACE("");
        ASSERT_EQ(subsys_1_en_Delay1Cast->getDelayValue(), 1);
        GraphTestHelper::verifyVector(subsys_1_en_Delay1Cast->getInitCondition(), std::vector<NumericValue>{NumericValue((long int) 0)});
    }
    nodeCheckCount++;
    //---</subsys_1_en/Delay>----
    //####</subsys_1_en>####

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

    //Check Params
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyVector(subsys_2_ProductCast->getInputOp(), std::vector<bool>{true, true});
    }
    nodeCheckCount++;
    //---</subsys_2/Product>----

    //---<subsys_2/Delay>---
    std::shared_ptr<Node> subsys_2_Delay1 = design.getNodeByNamePath(std::vector<std::string>{"subsys_2", "Delay1"});
    ASSERT_NE(subsys_2_Delay1, nullptr);
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyPortArcCounts(subsys_2_Delay1, 1, std::vector<unsigned long>{2});
        GraphTestHelper::assertCast<Node, Delay>(subsys_2_Delay1);
    }
    std::shared_ptr<Delay> subsys_2_Delay1Cast = std::dynamic_pointer_cast<Delay>(subsys_2_Delay1);

    //Check Params
    {
        SCOPED_TRACE("");
        ASSERT_EQ(subsys_2_Delay1Cast->getDelayValue(), 1);
        GraphTestHelper::verifyVector(subsys_2_Delay1Cast->getInitCondition(), std::vector<NumericValue>{NumericValue((long int) 0)});
    }
    nodeCheckCount++;
    //---</subsys_2/Delay>----

    //---<subsys_2/sub_sub_sys>---
    std::shared_ptr<Node> subsys_2_sub_sub_sys = design.getNodeByNamePath(std::vector<std::string>{"subsys_2", "sub_sub_sys"});
    ASSERT_NE(subsys_2_sub_sub_sys, nullptr);
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyPortArcCounts(subsys_2_sub_sub_sys, 0, std::vector<unsigned long>{}); //Connections are to internal nodes and not to subsystem directly.
        GraphTestHelper::assertCast<Node, SubSystem>(subsys_2_sub_sub_sys);
    }
    nodeCheckCount++;
    //---</subsys_2/sub_sub_sys>----

    //---<subsys_2/sub_sub_sys/Product>---
    std::shared_ptr<Node> subsys_2_sub_sub_sys_Product = design.getNodeByNamePath(std::vector<std::string>{"subsys_2", "sub_sub_sys", "Product"});
    ASSERT_NE(subsys_2_sub_sub_sys_Product, nullptr);
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyPortArcCounts(subsys_2_sub_sub_sys_Product, 2, std::vector<unsigned long>{1});
        GraphTestHelper::assertCast<Node, Product>(subsys_2_sub_sub_sys_Product);
    }
    std::shared_ptr<Product> subsys_2_sub_sub_sys_ProductCast = std::dynamic_pointer_cast<Product>(subsys_2_sub_sub_sys_Product);

    //Check Params
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyVector(subsys_2_sub_sub_sys_ProductCast->getInputOp(), std::vector<bool>{true, true});
    }
    nodeCheckCount++;
    //---</subsys_2/sub_sub_sys/Product>----

    //---<subsys_2/sub_sub_sys/Delay>---
    std::shared_ptr<Node> subsys_2_sub_sub_sys_Delay1 = design.getNodeByNamePath(std::vector<std::string>{"subsys_2", "sub_sub_sys", "Delay1"});
    ASSERT_NE(subsys_2_sub_sub_sys_Delay1, nullptr);
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyPortArcCounts(subsys_2_sub_sub_sys_Delay1, 1, std::vector<unsigned long>{1});
        GraphTestHelper::assertCast<Node, Delay>(subsys_2_sub_sub_sys_Delay1);
    }
    std::shared_ptr<Delay> subsys_2_sub_sub_sys_Delay1Cast = std::dynamic_pointer_cast<Delay>(subsys_2_sub_sub_sys_Delay1);

    //Check Params
    {
        SCOPED_TRACE("");
        ASSERT_EQ(subsys_2_sub_sub_sys_Delay1Cast->getDelayValue(), 1);
        GraphTestHelper::verifyVector(subsys_2_sub_sub_sys_Delay1Cast->getInitCondition(), std::vector<NumericValue>{NumericValue((long int) 0)});
    }
    nodeCheckCount++;
    //---</subsys_2/sub_sub_sys/Delay>----
    //####</subsys_2>####

    //####<subsys_2_en1>####
    //**** EnableInputs (Take Name from Port in Simulink) ****
    //---<subsys_2_en1/In1>---
    std::shared_ptr<Node> subsys_2_en1_In1 = design.getNodeByNamePath(std::vector<std::string>{"subsys_2_en1", "In1"});
    ASSERT_NE(subsys_2_en1_In1, nullptr);
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyPortArcCounts(subsys_2_en1_In1, 1, std::vector<unsigned long>{1}); //Connections are to internal nodes and not to subsystem directly.
        GraphTestHelper::assertCast<Node, EnableInput>(subsys_2_en1_In1);
    }
    nodeCheckCount++;
    //---</subsys_2_en1/In1>----

    //---<subsys_2_en1/In2>---
    std::shared_ptr<Node> subsys_2_en1_In2 = design.getNodeByNamePath(std::vector<std::string>{"subsys_2_en1", "In2"});
    ASSERT_NE(subsys_2_en1_In2, nullptr);
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyPortArcCounts(subsys_2_en1_In2, 1, std::vector<unsigned long>{2}); //Connections are to internal nodes and not to subsystem directly.
        GraphTestHelper::assertCast<Node, EnableInput>(subsys_2_en1_In2);
    }
    nodeCheckCount++;
    //---</subsys_2_en1/In2>----

    //**** EnableOutput (Take Name from Port in Simulink) ****
    //---<subsys_2_en1/Out1>---
    std::shared_ptr<Node> subsys_2_en1_Out1 = design.getNodeByNamePath(std::vector<std::string>{"subsys_2_en1", "Out1"});
    ASSERT_NE(subsys_2_en1_Out1, nullptr);
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyPortArcCounts(subsys_2_en1_Out1, 1, std::vector<unsigned long>{1}); //Connections are to internal nodes and not to subsystem directly.
        GraphTestHelper::assertCast<Node, EnableOutput>(subsys_2_en1_Out1);
    }
    nodeCheckCount++;
    //---</subsys_2_en1/Out1>----

    //---<subsys_2_en1/Out2>---
    std::shared_ptr<Node> subsys_2_en1_Out2 = design.getNodeByNamePath(std::vector<std::string>{"subsys_2_en1", "Out2"});
    ASSERT_NE(subsys_2_en1_Out2, nullptr);
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyPortArcCounts(subsys_2_en1_Out2, 1, std::vector<unsigned long>{1}); //Connections are to internal nodes and not to subsystem directly.
        GraphTestHelper::assertCast<Node, EnableOutput>(subsys_2_en1_Out2);
    }
    nodeCheckCount++;
    //---</subsys_2_en1/Out2>----

    //**** Other Nodes ****
    //---<subsys_2_en1/Product>---
    std::shared_ptr<Node> subsys_2_en1_Product = design.getNodeByNamePath(std::vector<std::string>{"subsys_2_en1", "Product"});
    ASSERT_NE(subsys_2_en1_Product, nullptr);
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyPortArcCounts(subsys_2_en1_Product, 2, std::vector<unsigned long>{1});
        GraphTestHelper::assertCast<Node, Product>(subsys_2_en1_Product);
    }
    std::shared_ptr<Product> subsys_2_en1_ProductCast = std::dynamic_pointer_cast<Product>(subsys_2_en1_Product);

    //Check Params
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyVector(subsys_2_en1_ProductCast->getInputOp(), std::vector<bool>{true, true});
    }
    nodeCheckCount++;
    //---</subsys_2_en1/Product>----

    //---<subsys_2_en1/Delay>---
    std::shared_ptr<Node> subsys_2_en1_Delay1 = design.getNodeByNamePath(std::vector<std::string>{"subsys_2_en1", "Delay1"});
    ASSERT_NE(subsys_2_en1_Delay1, nullptr);
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyPortArcCounts(subsys_2_en1_Delay1, 1, std::vector<unsigned long>{2});
        GraphTestHelper::assertCast<Node, Delay>(subsys_2_en1_Delay1);
    }
    std::shared_ptr<Delay> subsys_2_en1_Delay1Cast = std::dynamic_pointer_cast<Delay>(subsys_2_en1_Delay1);

    //Check Params
    {
        SCOPED_TRACE("");
        ASSERT_EQ(subsys_2_en1_Delay1Cast->getDelayValue(), 1);
        GraphTestHelper::verifyVector(subsys_2_en1_Delay1Cast->getInitCondition(), std::vector<NumericValue>{NumericValue((long int) 0)});
    }
    nodeCheckCount++;
    //---</subsys_2_en1/Delay>----

    //---<subsys_2_en1/sub_sub_sys>---
    std::shared_ptr<Node> subsys_2_en1_sub_sub_sys = design.getNodeByNamePath(std::vector<std::string>{"subsys_2_en1", "sub_sub_sys"});
    ASSERT_NE(subsys_2_en1_sub_sub_sys, nullptr);
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyPortArcCounts(subsys_2_en1_sub_sub_sys, 0, std::vector<unsigned long>{}); //Connections are to internal nodes and not to subsystem directly.
        GraphTestHelper::assertCast<Node, SubSystem>(subsys_2_en1_sub_sub_sys);
    }
    nodeCheckCount++;
    //---</subsys_2_en1/sub_sub_sys>----
    //####</subsys_2_en1>####

    //####<subsys_2_en1/sub_sub_sys>####
    //---<subsys_2_en1/sub_sub_sys/Product>---
    std::shared_ptr<Node> subsys_2_en1_sub_sub_sys_Product = design.getNodeByNamePath(std::vector<std::string>{"subsys_2_en1", "sub_sub_sys", "Product"});
    ASSERT_NE(subsys_2_en1_sub_sub_sys_Product, nullptr);
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyPortArcCounts(subsys_2_en1_sub_sub_sys_Product, 2, std::vector<unsigned long>{1});
        GraphTestHelper::assertCast<Node, Product>(subsys_2_en1_sub_sub_sys_Product);
    }
    std::shared_ptr<Product> subsys_2_en1_sub_sub_sys_ProductCast = std::dynamic_pointer_cast<Product>(subsys_2_en1_sub_sub_sys_Product);

    //Check Params
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyVector(subsys_2_en1_sub_sub_sys_ProductCast->getInputOp(), std::vector<bool>{true, true});
    }
    nodeCheckCount++;
    //---</subsys_2_en1/sub_sub_sys/Product>----

    //---<subsys_2_en1/sub_sub_sys/Delay>---
    std::shared_ptr<Node> subsys_2_en1_sub_sub_sys_Delay1 = design.getNodeByNamePath(std::vector<std::string>{"subsys_2_en1", "sub_sub_sys", "Delay1"});
    ASSERT_NE(subsys_2_en1_sub_sub_sys_Delay1, nullptr);
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyPortArcCounts(subsys_2_en1_sub_sub_sys_Delay1, 1, std::vector<unsigned long>{1});
        GraphTestHelper::assertCast<Node, Delay>(subsys_2_en1_sub_sub_sys_Delay1);
    }
    std::shared_ptr<Delay> subsys_2_en1_sub_sub_sys_Delay1Cast = std::dynamic_pointer_cast<Delay>(subsys_2_en1_sub_sub_sys_Delay1);

    //Check Params
    {
        SCOPED_TRACE("");
        ASSERT_EQ(subsys_2_en1_sub_sub_sys_Delay1Cast->getDelayValue(), 1);
        GraphTestHelper::verifyVector(subsys_2_en1_sub_sub_sys_Delay1Cast->getInitCondition(), std::vector<NumericValue>{NumericValue((long int) 0)});
    }
    nodeCheckCount++;
    //---</subsys_2_en1/sub_sub_sys/Delay>----
    //####</subsys_2_en1/sub_sub_sys>####

    //####<subsys_2_en2>####
    //**** EnableInputs (Take Name from Port in Simulink) ****
    //---<subsys_2_en2/In1>---
    std::shared_ptr<Node> subsys_2_en2_In1 = design.getNodeByNamePath(std::vector<std::string>{"subsys_2_en2", "In1"});
    ASSERT_NE(subsys_2_en2_In1, nullptr);
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyPortArcCounts(subsys_2_en2_In1, 1, std::vector<unsigned long>{1}); //Connections are to internal nodes and not to subsystem directly.
        GraphTestHelper::assertCast<Node, EnableInput>(subsys_2_en2_In1);
    }
    nodeCheckCount++;
    //---</subsys_2_en2/In1>----

    //---<subsys_2_en2/In2>---
    std::shared_ptr<Node> subsys_2_en2_In2 = design.getNodeByNamePath(std::vector<std::string>{"subsys_2_en2", "In2"});
    ASSERT_NE(subsys_2_en2_In2, nullptr);
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyPortArcCounts(subsys_2_en2_In2, 1, std::vector<unsigned long>{2}); //Connections are to internal nodes and not to subsystem directly.
        GraphTestHelper::assertCast<Node, EnableInput>(subsys_2_en2_In2);
    }
    nodeCheckCount++;
    //---</subsys_2_en2/In2>----

    //---<subsys_2_en2/In3>---
    std::shared_ptr<Node> subsys_2_en2_In3 = design.getNodeByNamePath(std::vector<std::string>{"subsys_2_en2", "In3"});
    ASSERT_NE(subsys_2_en2_In3, nullptr);
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyPortArcCounts(subsys_2_en2_In3, 1, std::vector<unsigned long>{3}); //Connection to enable nodes of nested enabled subsystem
        GraphTestHelper::assertCast<Node, EnableInput>(subsys_2_en2_In3);
    }
    nodeCheckCount++;
    //---</subsys_2_en2/In2>----

    //**** EnableOutputs (Take Name from Port in Simulink) ****
    //---<subsys_2_en2/Out1>---
    std::shared_ptr<Node> subsys_2_en2_Out1 = design.getNodeByNamePath(std::vector<std::string>{"subsys_2_en2", "Out1"});
    ASSERT_NE(subsys_2_en2_Out1, nullptr);
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyPortArcCounts(subsys_2_en2_Out1, 1, std::vector<unsigned long>{1}); //Connections are to internal nodes and not to subsystem directly.
        GraphTestHelper::assertCast<Node, EnableOutput>(subsys_2_en2_Out1);
    }
    nodeCheckCount++;
    //---</subsys_2_en2/Out1>----

    //---<subsys_2_en2/Out2>---
    std::shared_ptr<Node> subsys_2_en2_Out2 = design.getNodeByNamePath(std::vector<std::string>{"subsys_2_en2", "Out2"});
    ASSERT_NE(subsys_2_en2_Out2, nullptr);
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyPortArcCounts(subsys_2_en2_Out2, 1, std::vector<unsigned long>{1}); //Connections are to internal nodes and not to subsystem directly.
        GraphTestHelper::assertCast<Node, EnableOutput>(subsys_2_en2_Out2);
    }
    nodeCheckCount++;
    //---</subsys_2_en2/Out2>----

    //**** Other Nodes ****
    //---<subsys_2_en2/Product>---
    std::shared_ptr<Node> subsys_2_en2_Product = design.getNodeByNamePath(std::vector<std::string>{"subsys_2_en2", "Product"});
    ASSERT_NE(subsys_2_en2_Product, nullptr);
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyPortArcCounts(subsys_2_en2_Product, 2, std::vector<unsigned long>{1});
        GraphTestHelper::assertCast<Node, Product>(subsys_2_en2_Product);
    }
    std::shared_ptr<Product> subsys_2_en2_ProductCast = std::dynamic_pointer_cast<Product>(subsys_2_en2_Product);

    //Check Params
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyVector(subsys_2_en2_ProductCast->getInputOp(), std::vector<bool>{true, true});
    }
    nodeCheckCount++;
    //---</subsys_2_en2/Product>----

    //---<subsys_2_en2/Delay>---
    std::shared_ptr<Node> subsys_2_en2_Delay1 = design.getNodeByNamePath(std::vector<std::string>{"subsys_2_en2", "Delay1"});
    ASSERT_NE(subsys_2_en2_Delay1, nullptr);
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyPortArcCounts(subsys_2_en2_Delay1, 1, std::vector<unsigned long>{2});
        GraphTestHelper::assertCast<Node, Delay>(subsys_2_en2_Delay1);
    }
    std::shared_ptr<Delay> subsys_2_en2_Delay1Cast = std::dynamic_pointer_cast<Delay>(subsys_2_en2_Delay1);

    //Check Params
    {
        SCOPED_TRACE("");
        ASSERT_EQ(subsys_2_en2_Delay1Cast->getDelayValue(), 1);
        GraphTestHelper::verifyVector(subsys_2_en2_Delay1Cast->getInitCondition(), std::vector<NumericValue>{NumericValue((long int) 0)});
    }
    nodeCheckCount++;
    //---</subsys_2_en2/Delay>----

    //---<subsys_2_en2/sub_sub_sys>---
    std::shared_ptr<Node> subsys_2_en2_sub_sub_sys = design.getNodeByNamePath(std::vector<std::string>{"subsys_2_en2", "sub_sub_sys"});
    ASSERT_NE(subsys_2_en2_sub_sub_sys, nullptr);
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyPortArcCounts(subsys_2_en2_sub_sub_sys, 0, std::vector<unsigned long>{}); //Connections are to internal nodes and not to subsystem directly.
        GraphTestHelper::assertCast<Node, EnabledSubSystem>(subsys_2_en2_sub_sub_sys);
    }
    nodeCheckCount++;
    //---</subsys_2_en2/sub_sub_sys>----
    //####</subsys_2_en2>####

    //####<subsys_2_en2/sub_sub_sys>####
    //**** EnableInputs (Take Name from Port in Simulink) ****
    //---<subsys_2_en2/sub_sub_sys/In1>---
    std::shared_ptr<Node> subsys_2_en2_sub_sub_sys_In1 = design.getNodeByNamePath(std::vector<std::string>{"subsys_2_en2", "sub_sub_sys", "In1"});
    ASSERT_NE(subsys_2_en2_sub_sub_sys_In1, nullptr);
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyPortArcCounts(subsys_2_en2_sub_sub_sys_In1, 1, std::vector<unsigned long>{1}); //Connections are to internal nodes and not to subsystem directly.
        GraphTestHelper::assertCast<Node, EnableInput>(subsys_2_en2_sub_sub_sys_In1);
    }
    nodeCheckCount++;
    //---</subsys_2_en2/sub_sub_sys/In1>----

    //---<subsys_2_en2/sub_sub_sys/In2>---
    std::shared_ptr<Node> subsys_2_en2_sub_sub_sys_In2 = design.getNodeByNamePath(std::vector<std::string>{"subsys_2_en2", "sub_sub_sys", "In2"});
    ASSERT_NE(subsys_2_en2_sub_sub_sys_In2, nullptr);
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyPortArcCounts(subsys_2_en2_sub_sub_sys_In2, 1, std::vector<unsigned long>{1}); //Connections are to internal nodes and not to subsystem directly.
        GraphTestHelper::assertCast<Node, EnableInput>(subsys_2_en2_sub_sub_sys_In2);
    }
    nodeCheckCount++;
    //---</subsys_2_en2/sub_sub_sys/In2>----

    //**** EnableOutputs (Take Name from Port in Simulink) ****
    //---<subsys_2_en2/sub_sub_sys/Out1>---
    std::shared_ptr<Node> subsys_2_en2_sub_sub_sys_Out1 = design.getNodeByNamePath(std::vector<std::string>{"subsys_2_en2", "sub_sub_sys", "Out1"});
    ASSERT_NE(subsys_2_en2_sub_sub_sys_Out1, nullptr);
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyPortArcCounts(subsys_2_en2_sub_sub_sys_Out1, 1, std::vector<unsigned long>{1}); //Connections are to internal nodes and not to subsystem directly.
        GraphTestHelper::assertCast<Node, EnableOutput>(subsys_2_en2_sub_sub_sys_Out1);
    }
    nodeCheckCount++;
    //---</subsys_2_en2/sub_sub_sys/Out1>----

    //**** Other Nodes ****
    //---<subsys_2_en2/sub_sub_sys/Product>---
    std::shared_ptr<Node> subsys_2_en2_sub_sub_sys_Product = design.getNodeByNamePath(std::vector<std::string>{"subsys_2_en2", "sub_sub_sys", "Product"});
    ASSERT_NE(subsys_2_en2_sub_sub_sys_Product, nullptr);
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyPortArcCounts(subsys_2_en2_sub_sub_sys_Product, 2, std::vector<unsigned long>{1});
        GraphTestHelper::assertCast<Node, Product>(subsys_2_en2_sub_sub_sys_Product);
    }
    std::shared_ptr<Product> subsys_2_en2_sub_sub_sys_ProductCast = std::dynamic_pointer_cast<Product>(subsys_2_en2_sub_sub_sys_Product);

    //Check Params
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyVector(subsys_2_en2_sub_sub_sys_ProductCast->getInputOp(), std::vector<bool>{true, true});
    }
    nodeCheckCount++;
    //---</subsys_2_en2/sub_sub_sys/Product>----

    //---<subsys_2_en2/sub_sub_sys/Delay1>---
    std::shared_ptr<Node> subsys_2_en2_sub_sub_sys_Delay1 = design.getNodeByNamePath(std::vector<std::string>{"subsys_2_en2", "sub_sub_sys", "Delay1"});
    ASSERT_NE(subsys_2_en2_sub_sub_sys_Delay1, nullptr);
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyPortArcCounts(subsys_2_en2_sub_sub_sys_Delay1, 1, std::vector<unsigned long>{1});
        GraphTestHelper::assertCast<Node, Delay>(subsys_2_en2_sub_sub_sys_Delay1);
    }
    std::shared_ptr<Delay> subsys_2_en2_sub_sub_sys_Delay1Cast = std::dynamic_pointer_cast<Delay>(subsys_2_en2_sub_sub_sys_Delay1);

    //Check Params
    {
        SCOPED_TRACE("");
        ASSERT_EQ(subsys_2_en2_sub_sub_sys_Delay1Cast->getDelayValue(), 1);
        GraphTestHelper::verifyVector(subsys_2_en2_sub_sub_sys_Delay1Cast->getInitCondition(), std::vector<NumericValue>{NumericValue((long int) 0)});
    }
    nodeCheckCount++;
    //---</subsys_2_en2/sub_sub_sys/Delay1>----
    //####</subsys_2_en2/sub_sub_sys>####

    //####<subsys_2_en3>####
    //---<subsys_2_en3/Product>---
    std::shared_ptr<Node> subsys_2_en3_Product = design.getNodeByNamePath(std::vector<std::string>{"subsys_2_en3", "Product"});
    ASSERT_NE(subsys_2_en3_Product, nullptr);
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyPortArcCounts(subsys_2_en3_Product, 2, std::vector<unsigned long>{1});
        GraphTestHelper::assertCast<Node, Product>(subsys_2_en3_Product);
    }
    std::shared_ptr<Product> subsys_2_en3_ProductCast = std::dynamic_pointer_cast<Product>(subsys_2_en3_Product);

    //Check Params
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyVector(subsys_2_en3_ProductCast->getInputOp(), std::vector<bool>{true, true});
    }
    nodeCheckCount++;
    //---</subsys_2_en3/Product>----

    //---<subsys_2_en3/Delay1>---
    std::shared_ptr<Node> subsys_2_en3_Delay1 = design.getNodeByNamePath(std::vector<std::string>{"subsys_2_en3", "Delay1"});
    ASSERT_NE(subsys_2_en3_Delay1, nullptr);
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyPortArcCounts(subsys_2_en3_Delay1, 1, std::vector<unsigned long>{2});
        GraphTestHelper::assertCast<Node, Delay>(subsys_2_en3_Delay1);
    }
    std::shared_ptr<Delay> subsys_2_en3_Delay1Cast = std::dynamic_pointer_cast<Delay>(subsys_2_en3_Delay1);

    //Check Params
    {
        SCOPED_TRACE("");
        ASSERT_EQ(subsys_2_en3_Delay1Cast->getDelayValue(), 1);
        GraphTestHelper::verifyVector(subsys_2_en3_Delay1Cast->getInitCondition(), std::vector<NumericValue>{NumericValue((long int) 0)});
    }
    nodeCheckCount++;
    //---</subsys_2_en3/Delay1>----

    //---<subsys_2_en3/sub_sub_sys>---
    std::shared_ptr<Node> subsys_2_en3_sub_sub_sys = design.getNodeByNamePath(std::vector<std::string>{"subsys_2_en3", "sub_sub_sys"});
    ASSERT_NE(subsys_2_en3_sub_sub_sys, nullptr);
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyPortArcCounts(subsys_2_en3_sub_sub_sys, 0, std::vector<unsigned long>{}); //Connections are to internal nodes and not to subsystem directly.
        GraphTestHelper::assertCast<Node, EnabledSubSystem>(subsys_2_en3_sub_sub_sys);
    }
    nodeCheckCount++;
    //---</subsys_2_en3/sub_sub_sys>----
    //####</subsys_2_en3>####

    //####<subsys_2_en3/sub_sub_sys>####
    //**** EnableInputs (Take Name from Port in Simulink) ****
    //---<subsys_2_en3/sub_sub_sys/In1>---
    std::shared_ptr<Node> subsys_2_en3_sub_sub_sys_In1 = design.getNodeByNamePath(std::vector<std::string>{"subsys_2_en3", "sub_sub_sys", "In1"});
    ASSERT_NE(subsys_2_en3_sub_sub_sys_In1, nullptr);
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyPortArcCounts(subsys_2_en3_sub_sub_sys_In1, 1, std::vector<unsigned long>{1}); //Connections are to internal nodes and not to subsystem directly.
        GraphTestHelper::assertCast<Node, EnableInput>(subsys_2_en3_sub_sub_sys_In1);
    }
    nodeCheckCount++;
    //---</subsys_2_en3/sub_sub_sys/In1>----

    //---<subsys_2_en3/sub_sub_sys/In2>---
    std::shared_ptr<Node> subsys_2_en3_sub_sub_sys_In2 = design.getNodeByNamePath(std::vector<std::string>{"subsys_2_en3", "sub_sub_sys", "In2"});
    ASSERT_NE(subsys_2_en3_sub_sub_sys_In2, nullptr);
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyPortArcCounts(subsys_2_en3_sub_sub_sys_In2, 1, std::vector<unsigned long>{1}); //Connections are to internal nodes and not to subsystem directly.
        GraphTestHelper::assertCast<Node, EnableInput>(subsys_2_en3_sub_sub_sys_In2);
    }
    nodeCheckCount++;
    //---</subsys_2_en3/sub_sub_sys/In2>----

    //**** EnableOutputs (Take Name from Port in Simulink) ****
    //---<subsys_2_en3/sub_sub_sys/Out1>---
    std::shared_ptr<Node> subsys_2_en3_sub_sub_sys_Out1 = design.getNodeByNamePath(std::vector<std::string>{"subsys_2_en3", "sub_sub_sys", "Out1"});
    ASSERT_NE(subsys_2_en3_sub_sub_sys_Out1, nullptr);
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyPortArcCounts(subsys_2_en3_sub_sub_sys_Out1, 1, std::vector<unsigned long>{1}); //Connections are to internal nodes and not to subsystem directly.
        GraphTestHelper::assertCast<Node, EnableOutput>(subsys_2_en3_sub_sub_sys_Out1);
    }
    nodeCheckCount++;
    //---</subsys_2_en3/sub_sub_sys/Out1>----

    //**** Other Nodes ****
    //---<subsys_2_en3/sub_sub_sys/Product>---
    std::shared_ptr<Node> subsys_2_en3_sub_sub_sys_Product = design.getNodeByNamePath(std::vector<std::string>{"subsys_2_en3", "sub_sub_sys", "Product"});
    ASSERT_NE(subsys_2_en3_sub_sub_sys_Product, nullptr);
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyPortArcCounts(subsys_2_en3_sub_sub_sys_Product, 2, std::vector<unsigned long>{1});
        GraphTestHelper::assertCast<Node, Product>(subsys_2_en3_sub_sub_sys_Product);
    }
    std::shared_ptr<Product> subsys_2_en3_sub_sub_sys_ProductCast = std::dynamic_pointer_cast<Product>(subsys_2_en3_sub_sub_sys_Product);

    //Check Params
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyVector(subsys_2_en3_sub_sub_sys_ProductCast->getInputOp(), std::vector<bool>{true, true});
    }
    nodeCheckCount++;
    //---</subsys_2_en3/sub_sub_sys/Product>----

    //---<subsys_2_en3/sub_sub_sys/Delay1>---
    std::shared_ptr<Node> subsys_2_en3_sub_sub_sys_Delay1 = design.getNodeByNamePath(std::vector<std::string>{"subsys_2_en3", "sub_sub_sys", "Delay1"});
    ASSERT_NE(subsys_2_en3_Delay1, nullptr);
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyPortArcCounts(subsys_2_en3_sub_sub_sys_Delay1, 1, std::vector<unsigned long>{1});
        GraphTestHelper::assertCast<Node, Delay>(subsys_2_en3_sub_sub_sys_Delay1);
    }
    std::shared_ptr<Delay> subsys_2_en3_sub_sub_sys_Delay1Cast = std::dynamic_pointer_cast<Delay>(subsys_2_en3_sub_sub_sys_Delay1);

    //Check Params
    {
        SCOPED_TRACE("");
        ASSERT_EQ(subsys_2_en3_sub_sub_sys_Delay1Cast->getDelayValue(), 1);
        GraphTestHelper::verifyVector(subsys_2_en3_sub_sub_sys_Delay1Cast->getInitCondition(), std::vector<NumericValue>{NumericValue((long int) 0)});
    }
    nodeCheckCount++;
    //---</subsys_2_en3/sub_sub_sys/Delay1>----
    //####</subsys_2_en3/sub_sub_sys>####

    //Verify all nodes checked
    ASSERT_EQ(nodeCheckCount, design.getNodes().size());

    //++++ Check arcs ++++
    //If we check all of them, the check ensures they are all in the design arc vector.
    //Since we checked the total number of arcs, and we validated each node's port occupancy,
    //we can be sure we are not missing arcs

    //Some common types in this design
    DataType real_bool_width1 = DataType(false, false, false, 1, 0, 1);
    DataType real_uint16_width1 = DataType(false, false, false, 16, 0, 1);
    DataType real_uint32_width1 = DataType(false, false, false, 32, 0, 1);
    DataType real_ufix48_width1 = DataType(false, false, false, 48, 0, 1);

    unsigned long arcCheckCount = 0;

    //Master Input Arcs
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
        GraphTestHelper::verifyConnection(design, inputs, 1, subsys_1_Product, GraphTestHelper::InPortType::STANDARD, 1, real_uint16_width1, arcCheckCount);
    }
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyConnection(design, inputs, 1, subsys_1_en_In2, GraphTestHelper::InPortType::STANDARD, 0, real_uint16_width1, arcCheckCount);
    }

    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyConnection(design, inputs, 1, subsys_2_Product, GraphTestHelper::InPortType::STANDARD, 1, real_uint16_width1, arcCheckCount);
    }
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyConnection(design, inputs, 1, subsys_2_sub_sub_sys_Product, GraphTestHelper::InPortType::STANDARD, 1, real_uint16_width1, arcCheckCount);
    }
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyConnection(design, inputs, 1, subsys_2_en1_In2, GraphTestHelper::InPortType::STANDARD, 0, real_uint16_width1, arcCheckCount);
    }
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyConnection(design, inputs, 1, subsys_2_en2_In2, GraphTestHelper::InPortType::STANDARD, 0, real_uint16_width1, arcCheckCount);
    }
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyConnection(design, inputs, 1, subsys_2_en3_Product, GraphTestHelper::InPortType::STANDARD, 1, real_uint16_width1, arcCheckCount);
    }
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyConnection(design, inputs, 1, subsys_2_en3_sub_sub_sys_In2, GraphTestHelper::InPortType::STANDARD, 0, real_uint16_width1, arcCheckCount);
    }
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyConnection(design, inputs, 2, compareToConstant_Compare, GraphTestHelper::InPortType::STANDARD, 0, real_uint16_width1, arcCheckCount);
    }

    //Sum Arcs
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyConnection(design, sum, 0, delay, GraphTestHelper::InPortType::STANDARD, 0, real_uint16_width1, arcCheckCount);
    }

    //Delay Arcs
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyConnection(design, delay, 0, subsys_1_Product, GraphTestHelper::InPortType::STANDARD, 0, real_uint16_width1, arcCheckCount);
    }
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyConnection(design, delay, 0, subsys_1_en_In1, GraphTestHelper::InPortType::STANDARD, 0, real_uint16_width1, arcCheckCount);
    }
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyConnection(design, delay, 0, subsys_2_Product, GraphTestHelper::InPortType::STANDARD, 0, real_uint16_width1, arcCheckCount);
    }
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyConnection(design, delay, 0, subsys_2_en1_In1, GraphTestHelper::InPortType::STANDARD, 0, real_uint16_width1, arcCheckCount);
    }
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyConnection(design, delay, 0, subsys_2_en2_In1, GraphTestHelper::InPortType::STANDARD, 0, real_uint16_width1, arcCheckCount);
    }
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyConnection(design, delay, 0, subsys_2_en3_Product, GraphTestHelper::InPortType::STANDARD, 0, real_uint16_width1, arcCheckCount);
    }

    //Compare to Constant/Constant
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyConnection(design, compareToConstant_Constant, 0, compareToConstant_Compare, GraphTestHelper::InPortType::STANDARD, 1, real_uint16_width1, arcCheckCount);
    }

    //Compare to Constant/Compare
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyConnection(design, compareToConstant_Compare, 0, subsys_1_en_In1, GraphTestHelper::InPortType::ENABLE, 0, real_bool_width1, arcCheckCount);
    }
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyConnection(design, compareToConstant_Compare, 0, subsys_1_en_In2, GraphTestHelper::InPortType::ENABLE, 0, real_bool_width1, arcCheckCount);
    }
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyConnection(design, compareToConstant_Compare, 0, subsys_1_en_Out1, GraphTestHelper::InPortType::ENABLE, 0, real_bool_width1, arcCheckCount);
    }
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyConnection(design, compareToConstant_Compare, 0, subsys_2_en1_In1, GraphTestHelper::InPortType::ENABLE, 0, real_bool_width1, arcCheckCount);
    }
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyConnection(design, compareToConstant_Compare, 0, subsys_2_en1_In2, GraphTestHelper::InPortType::ENABLE, 0, real_bool_width1, arcCheckCount);
    }
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyConnection(design, compareToConstant_Compare, 0, subsys_2_en1_Out1, GraphTestHelper::InPortType::ENABLE, 0, real_bool_width1, arcCheckCount);
    }
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyConnection(design, compareToConstant_Compare, 0, subsys_2_en1_Out2, GraphTestHelper::InPortType::ENABLE, 0, real_bool_width1, arcCheckCount);
    }
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyConnection(design, compareToConstant_Compare, 0, subsys_2_en2_In1, GraphTestHelper::InPortType::ENABLE, 0, real_bool_width1, arcCheckCount);
    }
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyConnection(design, compareToConstant_Compare, 0, subsys_2_en2_In2, GraphTestHelper::InPortType::ENABLE, 0, real_bool_width1, arcCheckCount);
    }
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyConnection(design, compareToConstant_Compare, 0, subsys_2_en2_In3, GraphTestHelper::InPortType::ENABLE, 0, real_bool_width1, arcCheckCount);
    }
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyConnection(design, compareToConstant_Compare, 0, subsys_2_en2_In3, GraphTestHelper::InPortType::STANDARD, 0, real_bool_width1, arcCheckCount);
    }
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyConnection(design, compareToConstant_Compare, 0, subsys_2_en2_Out1, GraphTestHelper::InPortType::ENABLE, 0, real_bool_width1, arcCheckCount);
    }
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyConnection(design, compareToConstant_Compare, 0, subsys_2_en2_Out2, GraphTestHelper::InPortType::ENABLE, 0, real_bool_width1, arcCheckCount);
    }
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyConnection(design, compareToConstant_Compare, 0, subsys_2_en3_sub_sub_sys_In1, GraphTestHelper::InPortType::ENABLE, 0, real_bool_width1, arcCheckCount);
    }
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyConnection(design, compareToConstant_Compare, 0, subsys_2_en3_sub_sub_sys_In2, GraphTestHelper::InPortType::ENABLE, 0, real_bool_width1, arcCheckCount);
    }
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyConnection(design, compareToConstant_Compare, 0, subsys_2_en3_sub_sub_sys_Out1, GraphTestHelper::InPortType::ENABLE, 0, real_bool_width1, arcCheckCount);
    }

    //subsys_1/Product
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyConnection(design, subsys_1_Product, 0, subsys_1_Delay1, GraphTestHelper::InPortType::STANDARD, 0, real_uint32_width1, arcCheckCount);
    }

    //subsys_1/Delay1
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyConnection(design, subsys_1_Delay1, 0, outputs, GraphTestHelper::InPortType::STANDARD, 0, real_uint32_width1, arcCheckCount);
    }

    //subsys_1_en/ENABLE_PORTS
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyConnection(design, subsys_1_en_In1, 0, subsys_1_en_Product, GraphTestHelper::InPortType::STANDARD, 0, real_uint16_width1, arcCheckCount);
    }
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyConnection(design, subsys_1_en_In2, 0, subsys_1_en_Product, GraphTestHelper::InPortType::STANDARD, 1, real_uint16_width1, arcCheckCount);
    }
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyConnection(design, subsys_1_en_Out1, 0, outputs, GraphTestHelper::InPortType::STANDARD, 1, real_uint32_width1, arcCheckCount);
    }

    //subsys_1_en/Product
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyConnection(design, subsys_1_en_Product, 0, subsys_1_en_Delay1, GraphTestHelper::InPortType::STANDARD, 0, real_uint32_width1, arcCheckCount);
    }

    //subsys_1_en/Delay1
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyConnection(design, subsys_1_en_Delay1, 0, subsys_1_en_Out1, GraphTestHelper::InPortType::STANDARD, 0, real_uint32_width1, arcCheckCount);
    }

    //subsys_2/Product
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyConnection(design, subsys_2_Product, 0, subsys_2_Delay1, GraphTestHelper::InPortType::STANDARD, 0, real_uint32_width1, arcCheckCount);
    }

    //subsys_2/Delay1
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyConnection(design, subsys_2_Delay1, 0, subsys_2_sub_sub_sys_Product, GraphTestHelper::InPortType::STANDARD, 0, real_uint32_width1, arcCheckCount);
    }

    //subsys_2/sub_sub_sys/Product
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyConnection(design, subsys_2_sub_sub_sys_Product, 0, subsys_2_sub_sub_sys_Delay1, GraphTestHelper::InPortType::STANDARD, 0, real_ufix48_width1, arcCheckCount);
    }

    //subsys_2/sub_sub_sys/Delay1
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyConnection(design, subsys_2_sub_sub_sys_Delay1, 0, outputs, GraphTestHelper::InPortType::STANDARD, 3, real_ufix48_width1, arcCheckCount);
    }

    //subsys_2_en1/ENABLE PORTS
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyConnection(design, subsys_2_en1_In1, 0, subsys_2_en1_Product, GraphTestHelper::InPortType::STANDARD, 0, real_uint16_width1, arcCheckCount);
    }
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyConnection(design, subsys_2_en1_In2, 0, subsys_2_en1_Product, GraphTestHelper::InPortType::STANDARD, 1, real_uint16_width1, arcCheckCount);
    }
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyConnection(design, subsys_2_en1_In2, 0, subsys_2_en1_sub_sub_sys_Product, GraphTestHelper::InPortType::STANDARD, 1, real_uint16_width1, arcCheckCount);
    }
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyConnection(design, subsys_2_en1_Out1, 0, outputs, GraphTestHelper::InPortType::STANDARD, 4, real_uint32_width1, arcCheckCount);
    }
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyConnection(design, subsys_2_en1_Out2, 0, outputs, GraphTestHelper::InPortType::STANDARD, 5, real_ufix48_width1, arcCheckCount);
    }

    //subsys_2_en1/Product
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyConnection(design, subsys_2_en1_Product, 0, subsys_2_en1_Delay1, GraphTestHelper::InPortType::STANDARD, 0, real_uint32_width1, arcCheckCount);
    }

    //subsys_2_en1/Delay1
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyConnection(design, subsys_2_en1_Delay1, 0, subsys_2_en1_Out1, GraphTestHelper::InPortType::STANDARD, 0, real_uint32_width1, arcCheckCount);
    }
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyConnection(design, subsys_2_en1_Delay1, 0, subsys_2_en1_sub_sub_sys_Product, GraphTestHelper::InPortType::STANDARD, 0, real_uint32_width1, arcCheckCount);
    }

    //subsys_2_en1/sub_sub_sys/Product
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyConnection(design, subsys_2_en1_sub_sub_sys_Product, 0, subsys_2_en1_sub_sub_sys_Delay1, GraphTestHelper::InPortType::STANDARD, 0, real_ufix48_width1, arcCheckCount);
    }

    //subsys_2_en1/sub_sub_sys/Delay1
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyConnection(design, subsys_2_en1_sub_sub_sys_Delay1, 0, subsys_2_en1_Out2, GraphTestHelper::InPortType::STANDARD, 0, real_ufix48_width1, arcCheckCount);
    }

    //subsys_2_en2/ENABLE NODES
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyConnection(design, subsys_2_en2_In1, 0, subsys_2_en2_Product, GraphTestHelper::InPortType::STANDARD, 0, real_uint16_width1, arcCheckCount);
    }
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyConnection(design, subsys_2_en2_In2, 0, subsys_2_en2_Product, GraphTestHelper::InPortType::STANDARD, 1, real_uint16_width1, arcCheckCount);
    }
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyConnection(design, subsys_2_en2_In2, 0, subsys_2_en2_sub_sub_sys_In2, GraphTestHelper::InPortType::STANDARD, 0, real_uint16_width1, arcCheckCount);
    }
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyConnection(design, subsys_2_en2_In3, 0, subsys_2_en2_sub_sub_sys_In1, GraphTestHelper::InPortType::ENABLE, 0, real_bool_width1, arcCheckCount);
    }
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyConnection(design, subsys_2_en2_In3, 0, subsys_2_en2_sub_sub_sys_In2, GraphTestHelper::InPortType::ENABLE, 0, real_bool_width1, arcCheckCount);
    }
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyConnection(design, subsys_2_en2_In3, 0, subsys_2_en2_sub_sub_sys_Out1, GraphTestHelper::InPortType::ENABLE, 0, real_bool_width1, arcCheckCount);
    }
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyConnection(design, subsys_2_en2_Out1, 0, outputs, GraphTestHelper::InPortType::STANDARD, 6, real_uint32_width1, arcCheckCount);
    }
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyConnection(design, subsys_2_en2_Out2, 0, outputs, GraphTestHelper::InPortType::STANDARD, 7, real_ufix48_width1, arcCheckCount);
    }

    //subsys_2_en2/Product
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyConnection(design, subsys_2_en2_Product, 0, subsys_2_en2_Delay1, GraphTestHelper::InPortType::STANDARD, 0, real_uint32_width1, arcCheckCount);
    }

    //subsys_2_en2/Delay1
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyConnection(design, subsys_2_en2_Delay1, 0, subsys_2_en2_Out1, GraphTestHelper::InPortType::STANDARD, 0, real_uint32_width1, arcCheckCount);
    }
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyConnection(design, subsys_2_en2_Delay1, 0, subsys_2_en2_sub_sub_sys_In1, GraphTestHelper::InPortType::STANDARD, 0, real_uint32_width1, arcCheckCount);
    }

    //subsys_2_en2/sub_sub_sys/ENABLE NODES
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyConnection(design, subsys_2_en2_sub_sub_sys_In1, 0, subsys_2_en2_sub_sub_sys_Product, GraphTestHelper::InPortType::STANDARD, 0, real_uint32_width1, arcCheckCount);
    }
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyConnection(design, subsys_2_en2_sub_sub_sys_In2, 0, subsys_2_en2_sub_sub_sys_Product, GraphTestHelper::InPortType::STANDARD, 1, real_uint16_width1, arcCheckCount);
    }
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyConnection(design, subsys_2_en2_sub_sub_sys_Out1, 0, subsys_2_en2_Out2, GraphTestHelper::InPortType::STANDARD, 0, real_ufix48_width1, arcCheckCount);
    }

    //subsys_2_en2/sub_sub_sys/Product
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyConnection(design, subsys_2_en2_sub_sub_sys_Product, 0, subsys_2_en2_sub_sub_sys_Delay1, GraphTestHelper::InPortType::STANDARD, 0, real_ufix48_width1, arcCheckCount);
    }

    //subsys_2_en2/sub_sub_sys/Delay1
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyConnection(design, subsys_2_en2_sub_sub_sys_Delay1, 0, subsys_2_en2_sub_sub_sys_Out1, GraphTestHelper::InPortType::STANDARD, 0, real_ufix48_width1, arcCheckCount);
    }

    //subsys_2_en3/Product
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyConnection(design, subsys_2_en3_Product, 0, subsys_2_en3_Delay1, GraphTestHelper::InPortType::STANDARD, 0, real_uint32_width1, arcCheckCount);
    }

    //subsys_2_en3/Delay1
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyConnection(design, subsys_2_en3_Delay1, 0, outputs, GraphTestHelper::InPortType::STANDARD, 8, real_uint32_width1, arcCheckCount);
    }
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyConnection(design, subsys_2_en3_Delay1, 0, subsys_2_en3_sub_sub_sys_In1, GraphTestHelper::InPortType::STANDARD, 0, real_uint32_width1, arcCheckCount);
    }

    //subsys_2_en3/sub_sub_sys/ENABLE NODE
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyConnection(design, subsys_2_en3_sub_sub_sys_In1, 0, subsys_2_en3_sub_sub_sys_Product, GraphTestHelper::InPortType::STANDARD, 0, real_uint32_width1, arcCheckCount);
    }
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyConnection(design, subsys_2_en3_sub_sub_sys_In2, 0, subsys_2_en3_sub_sub_sys_Product, GraphTestHelper::InPortType::STANDARD, 1, real_uint16_width1, arcCheckCount);
    }
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyConnection(design, subsys_2_en3_sub_sub_sys_Out1, 0, outputs, GraphTestHelper::InPortType::STANDARD, 9, real_ufix48_width1, arcCheckCount);
    }

    //subsys_2_en3/sub_sub_sys/Product
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyConnection(design, subsys_2_en3_sub_sub_sys_Product, 0, subsys_2_en3_sub_sub_sys_Delay1, GraphTestHelper::InPortType::STANDARD, 0, real_ufix48_width1, arcCheckCount);
    }

    //subsys_2_en3/sub_sub_sys/Delay1
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyConnection(design, subsys_2_en3_sub_sub_sys_Delay1, 0, subsys_2_en3_sub_sub_sys_Out1, GraphTestHelper::InPortType::STANDARD, 0, real_ufix48_width1, arcCheckCount);
    }

    ASSERT_EQ(arcCheckCount, design.getArcs().size()) << "Did not check all arcs in design or checked more arcs than what is in design";
}


