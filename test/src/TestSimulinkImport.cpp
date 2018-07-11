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

TEST(SimulinkImport, SimpleDesign) {
    //==== Import File ====
    std::string inputFile = "./stimulus/simulink/basic/simple.graphml";
    std::unique_ptr<Design> design = SimulinkGraphMLImporter::importSimulinkGraphML(inputFile);
    ASSERT_EQ(design->getNodes().size(), 4); //4 Nodes besides the master nodes
    ASSERT_EQ(design->getTopLevelNodes().size(), 4); //4 Nodes
    ASSERT_EQ(design->getArcs().size(), 11); //11 arcs

    //==== Traverse Design and Check ====

    //++++ Check Master Node Sizes ++++
    std::shared_ptr<MasterInput> inputs = design->getInputMaster();
    ASSERT_EQ(inputs->getInputPorts().size(), 0);
    ASSERT_EQ(inputs->getOutputPorts().size(), 2);
    ASSERT_EQ(inputs->getOutputPort(0)->getArcs().size(), 1);
    ASSERT_EQ(inputs->getOutputPort(1)->getArcs().size(), 3);

    std::shared_ptr<MasterOutput> outputs = design->getOutputMaster();
    ASSERT_EQ(outputs->getInputPorts().size(), 2);
    ASSERT_EQ(outputs->getOutputPorts().size(), 0);

    std::shared_ptr<MasterOutput> terminator = design->getTerminatorMaster();
    ASSERT_EQ(terminator->getInputPorts().size(), 1);
    ASSERT_EQ(terminator->getOutputPorts().size(), 0);

    std::shared_ptr<MasterOutput> vis = design->getVisMaster();
    ASSERT_EQ(vis->getInputPorts().size(), 1);
    ASSERT_EQ(vis->getOutputPorts().size(), 0);

    std::shared_ptr<MasterUnconnected> unconnected = design->getUnconnectedMaster();
    ASSERT_EQ(unconnected->getInputPorts().size(), 0);
    ASSERT_EQ(unconnected->getOutputPorts().size(), 0);

    //++++ Check Node Port Numbers ++++
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyPortNumbers(inputs);
    }
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyPortNumbers(outputs);
    }
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyPortNumbers(terminator);
    }
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyPortNumbers(vis);
    }
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyPortNumbers(unconnected);
    }

    std::vector<std::shared_ptr<Node>> nodes = design->getNodes();
    for(unsigned long i = 0; i<nodes.size(); i++)
    {
        {
            SCOPED_TRACE("Node: " + std::to_string(i));
            GraphTestHelper::verifyPortNumbers(unconnected);
        }
    }

    //++++ Get 1st Arc Of Input Master++++
    std::shared_ptr<Port> inPort0 = inputs->getOutputPort(0);
    std::set<std::shared_ptr<Arc>> inPort0Arcs = inPort0->getArcs();
    std::shared_ptr<Arc> inPort0Arc = *(inPort0->arcsBegin());

    //Check Arc
    std::shared_ptr<Port> srcPort = inPort0Arc->getSrcPort();
    ASSERT_EQ(inPort0, srcPort); //The src port returned by the arc should be the input master node's port.

    {//Scope this so that SCOPED_TRACE does not accumulate below
        SCOPED_TRACE("");
        GraphTestHelper::verifyArcLinks(inPort0Arc);
    }

    DataType inPort0ArcDataType = inPort0Arc->getDataType();
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyDataType(inPort0ArcDataType, false, false, false, 16, 0, 1);
    }

    //++++ Check Sum Node ++++
    std::shared_ptr<Port> dstPort = inPort0Arc->getDstPort();
    std::shared_ptr<Node> SumNodeSuper = dstPort->getParent();

    //Check Type
    {
        SCOPED_TRACE("");
        GraphTestHelper::assertCast<Node, Sum>(SumNodeSuper); //Check that this node is a Sum (or subclass)
    }
    std::shared_ptr<Sum> SumNode = std::dynamic_pointer_cast<Sum>(SumNodeSuper);

    //Check Input Signs
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyVector(SumNode->getInputSign(), std::vector<bool>{true, true});
    }

    //Verify Number of Ports
    ASSERT_EQ(SumNode->getInputPorts().size(), 2);
    ASSERT_EQ(SumNode->getOutputPorts().size(), 1);

    //----Check 2nd Input Arc----
    std::shared_ptr<Arc> In2 = *(SumNode->getInputPort(1)->arcsBegin());

    //Check Src is Master Input's 2nd port (port 1)
    ASSERT_EQ(In2->getSrcPort(), design->getInputMaster()->getOutputPort(1));

    //Verify Arc Links (& that dst has only 1 incoming arc)
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyArcLinks(In2);
    }

    //Verify Arc DataType
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyDataType(In2->getDataType(), false, false, false, 16, 0, 1);
    }

    //----Check Only 1 Output----
    ASSERT_EQ(SumNode->getOutputPort(0)->getArcs().size(), 1);

    //----Check Output Arc----
    std::shared_ptr<Arc> delayIn = *(SumNode->getOutputPort(0)->arcsBegin());

    //Verify Arc Links
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyArcLinks(delayIn);
    }

    //Verify Arc DataType
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyDataType(delayIn->getDataType(), false, false, false, 16, 0, 1);
    }

    //++++ Check Delay Node ++++
    std::shared_ptr<Node> DelayNodeSuper = delayIn->getDstPort()->getParent();

    //Check Type
    {
        SCOPED_TRACE("");
        GraphTestHelper::assertCast<Node, Delay>(DelayNodeSuper);
    }
    std::shared_ptr<Delay> DelayNode = std::dynamic_pointer_cast<Delay>(DelayNodeSuper);

    //Check Delay Values
    ASSERT_EQ(DelayNode->getDelayValue(), 1);
    //Check Init Cond
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyVector(DelayNode->getInitCondition(), std::vector<NumericValue>{NumericValue((long int) 0)});
    }

    //Verify Number of Ports
    ASSERT_EQ(DelayNode->getInputPorts().size(), 1);
    ASSERT_EQ(DelayNode->getOutputPorts().size(), 1);

    //Check Input Port Number of Delay Input
    ASSERT_EQ(delayIn->getDstPort()->getPortNum(), 0);

    //----Check Outputs of Delay----
    ASSERT_EQ(DelayNode->getOutputPort(0)->getArcs().size(), 3);

    //find which output arc is which
    std::shared_ptr<Arc> OutArc = nullptr;
    std::shared_ptr<Arc> Sum2Arc = nullptr;
    std::shared_ptr<Arc> ProductArc = nullptr;

    for(auto i = DelayNode->getOutputPort(0)->arcsBegin(); i != DelayNode->getOutputPort(0)->arcsEnd(); i++){
        std::shared_ptr<Port> dstPort = (*i)->getDstPort();
        std::shared_ptr<Node> dstNode = dstPort->getParent();

        if(std::dynamic_pointer_cast<MasterOutput>(dstNode) != nullptr){
            OutArc = *i;
        } else if(std::dynamic_pointer_cast<Sum>(dstNode) != nullptr){
            Sum2Arc = *i;
        } else if(std::dynamic_pointer_cast<Product>(dstNode) != nullptr){
            ProductArc = *i;
        }
    }

    ASSERT_NE(OutArc, nullptr);
    ASSERT_NE(Sum2Arc, nullptr);
    ASSERT_NE(ProductArc, nullptr);

    //Verify Output Arcs
    //Verify Arc Links
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyArcLinks(OutArc);
    }

    //Verify Arc DataType
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyDataType(OutArc->getDataType(), false, false, false, 16, 0, 1);
    }

    //Verify Arc Links
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyArcLinks(Sum2Arc);
    }

    //Verify Arc DataType
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyDataType(Sum2Arc->getDataType(), false, false, false, 16, 0, 1);
    }

    //Verify Arc Links
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyArcLinks(ProductArc);
    }

    //Verify Arc DataType
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyDataType(ProductArc->getDataType(), false, false, false, 16, 0, 1);
    }

    //++++ Sum2 Node ++++
    std::shared_ptr<Node> Sum2NodeSuper = Sum2Arc->getDstPort()->getParent();

    //Check Type
    {
        SCOPED_TRACE("");
        GraphTestHelper::assertCast<Node, Sum>(Sum2NodeSuper);
    }
    std::shared_ptr<Sum> Sum2Node = std::dynamic_pointer_cast<Sum>(Sum2NodeSuper);

    //Check Input Signs
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyVector(Sum2Node->getInputSign(), std::vector<bool>{true, true});
    }

    //Verify Number of Ports And Number of Output Arcs
    ASSERT_EQ(Sum2Node->getInputPorts().size(), 2);
    ASSERT_EQ(Sum2Node->getOutputPorts().size(), 1);
    ASSERT_EQ(Sum2Node->getOutputPort(0)->getArcs().size(), 2);

    //Check Input Port From Delay
    ASSERT_EQ(Sum2Arc->getDstPort(), Sum2Node->getInputPort(0));

    //Check Input from Input Master
    std::shared_ptr<Arc> Sum2Input2 = *(Sum2Node->getInputPort(1)->arcsBegin());
    //Verify Arc Links
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyArcLinks(Sum2Input2);
    }
    //Verify Arc DataType
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyDataType(Sum2Input2->getDataType(), false, false, false, 16, 0, 1);
    }
    ASSERT_EQ(Sum2Input2->getSrcPort(), inputs->getOutputPort(1));

    //++++ Delay and Sum2 Connection to Output & Vis ++++
    //Check Delay
    ASSERT_EQ(OutArc->getDstPort(), outputs->getInputPort(0));

    //Check Sum Output
    std::shared_ptr<Arc> Sum2Out = *(outputs->getInputPort(1)->arcsBegin());
    //Verify Arc Links
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyArcLinks(Sum2Out);
    }
    //Verify Arc DataType
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyDataType(Sum2Out->getDataType(), false, false, false, 16, 0, 1);
    }
    //Check that this arc's src is Sum2's Output
    ASSERT_EQ(Sum2Out->getSrcPort(), Sum2Node->getOutputPort(0));

    //Check Sum Vis
    std::shared_ptr<Arc> Sum2Vis = *(vis->getInputPort(0)->arcsBegin());
    //Verify Arc Links
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyArcLinksTerminator(Sum2Vis);
    }
    //Verify Arc DataType
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyDataType(Sum2Vis->getDataType(), false, false, false, 16, 0, 1);
    }
    //Check that this arc's src is Sum2's Output
    ASSERT_EQ(Sum2Vis->getSrcPort(), Sum2Node->getOutputPort(0));

    //++++ Product ++++
    std::shared_ptr<Node> ProductNodeSuper = ProductArc->getDstPort()->getParent();

    //Check Type
    {
        SCOPED_TRACE("");
        GraphTestHelper::assertCast<Node, Product>(ProductNodeSuper);
    }
    std::shared_ptr<Product> ProductNode = std::dynamic_pointer_cast<Product>(ProductNodeSuper);

    //Check Input Ops
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyVector(ProductNode->getInputOp(), std::vector<bool>{true, true});
    }

    //Verify Number of Ports And Number of Output Arcs
    ASSERT_EQ(ProductNode->getInputPorts().size(), 2);
    ASSERT_EQ(ProductNode->getOutputPorts().size(), 1);
    ASSERT_EQ(ProductNode->getOutputPort(0)->getArcs().size(), 1);

    //Check Input Port From Delay
    ASSERT_EQ(ProductArc->getDstPort(), ProductNode->getInputPort(0));

    //Check 2nd Product Input
    std::shared_ptr<Arc> ProductInput2 = *(ProductNode->getInputPort(1)->arcsBegin());
    //Verify Arc Links
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyArcLinks(ProductInput2);
    }
    //Verify Arc DataType
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyDataType(ProductInput2->getDataType(), false, false, false, 16, 0, 1);
    }

    //Check Comes from Input Master Port 1
    ASSERT_EQ(ProductInput2->getSrcPort(), inputs->getOutputPort(1));

    //Check Product Output
    std::shared_ptr<Arc> ProductTerm = *(ProductNode->getOutputPort(0)->arcsBegin());
    //Verify Arc Links
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyArcLinksTerminator(ProductTerm);
    }
    //Verify Arc DataType
    {
        SCOPED_TRACE("");
        GraphTestHelper::verifyDataType(ProductTerm->getDataType(), false, false, false, 32, 0, 1);
    }

    //Check connection to terminator
    ASSERT_EQ(ProductTerm->getDstPort(), terminator->getInputPort(0));

}