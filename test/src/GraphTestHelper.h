//
// Created by Christopher Yarp on 7/10/18.
//

#ifndef VITIS_GRAPHTESTHELPER_H
#define VITIS_GRAPHTESTHELPER_H

#include <memory>
#include <vector>
#include <set>
#include "gtest/gtest.h"
#include "GraphCore/Arc.h"
#include "GraphCore/Node.h"
#include "GraphCore/EnablePort.h"
#include "GraphCore/Port.h"
#include "GraphCore/DataType.h"
#include "GraphCore/Design.h"
#include "PrimitiveNodes/Mux.h"

/**
 * @brief Helper functions for testing graph
 */
class GraphTestHelper {
public:

    /**
     * @brief Check if the arc connected to 2 ports have a reference to the given arc.  Checks that dst has only 1 arc (no multi driver nets)
     * @param arc arc to check
     */
    static void verifyArcLinks(std::shared_ptr<Arc> arc);

    /**
     * @brief Check if the arc connected to 2 ports have a reference to the given arc.  Terminator, Unconnected, and Vis are allowed to have many inputs
     * @param arc arc to check
     */
    static void verifyArcLinksTerminator(std::shared_ptr<Arc> arc);

    /**
     * @brief Check the given DataType object is as expected
     * @param dataType
     * @param floatingPtExpected
     * @param signedTypeExpected
     * @param complexExpected
     * @param totalBitsExpected
     * @param fractionalBitsExpected
     * @param widthExpected
     */
    static void verifyDataType(DataType dataType, bool floatingPtExpected, bool signedTypeExpected, bool complexExpected, int totalBitsExpected, int fractionalBitsExpected, int widthExpected);

    /**
     * @brief Verify that the port numbers for input and output ports are correct
     * @param node node to check
     */
    static void verifyPortNumbers(std::shared_ptr<Node> node);

    /**
     * @brief Compare 2 vectors for equality
     * @tparam T Type of the vectors
     * @param tgt Target vector for check
     * @param expected Vector of expected entries
     */
    template<typename T>
    static void verifyVector(std::vector<T> tgt, std::vector<T> expected){
        ASSERT_EQ(tgt.size(), expected.size()) << "Vector lengths do not match";

        unsigned long vecLen = expected.size();

        for(unsigned long i = 0; i<vecLen; i++){
            ASSERT_EQ(tgt[i], expected[i]) << "Failed at index " << i;
        }

    }

    /**
     * @brief Tests if a pointer can be cast to another pointer type
     * @tparam orig original type
     * @tparam tgt target type
     * @param ptr pointer to cast
     */
    template<typename orig, typename tgt>
    static void assertCast(std::shared_ptr<orig> ptr){
        //Attempt the cast
        std::shared_ptr<tgt> castPtr = std::dynamic_pointer_cast<tgt>(ptr);

        ASSERT_NE(castPtr, nullptr);

        //NOTE: Cannot return cast ptr.  gtest wants the function to have a void return type
    }

    /**
     * @brief Specifies the type of InPort
     */
    enum class InPortType{
        STANDARD, ///<Standard Input Port
        ENABLE, ///<Enable Input Port
        SELECT ///<Select Input Port
    };

    /**
     * @brief Verifies a connection exists between 2 nodes in the graph.  Also validates arc if connection exists and if the arc is in the Design arc list.
     *
     * @note @ref Design::getNodeByNamePath can be used to get a pointer to a node by name

     * @param design Design from which to check the arc vector
     * @param src A pointer to the source node
     * @param outPort The output port number
     * @param dst A pointer to the destination node
     * @param inPort The input port number of the
     * @param foundArc Is populated with a pointer to the arc if the connection exists, nullptr otherwise
     * @param dstIsTerminator true if the destination node is a terminator, false otherwise
     */
    static void verifyConnection(Design& design, std::shared_ptr<Node> src, int outPort, std::shared_ptr<Node> dst, InPortType inPortType, int inPort, std::shared_ptr<Arc> &foundArc, bool dstIsTerminator = false);

    /**
     * @brief Verifies a connection exists between 2 nodes in the graph.  Also validates arc if connection exists, validates the arc datatype, and validates if the arc is in the Design arc list.
     *
     * @note @ref Design::getNodeByNamePath can be used to get a pointer to a node by name
     *
     * checkCount is incremented in each check.  This is used to make sure each arc in the design has been checked
     *
     * @param design Design from which to check the arc vector
     * @param src A pointer to the source node
     * @param outPort The output port number
     * @param dst A pointer to the destination node
     * @param inPort The input port number of the
     * @param expectedType The expected datatype of the arc
     * @param checkCount a counter which is incremented by the check - used to verify all arcs in the design have been checked
     * @param dstIsTerminator true if the destination node is a terminator, false otherwise
     */
    static void verifyConnection(Design& design, std::shared_ptr<Node> src, int outPort, std::shared_ptr<Node> dst, InPortType inPortType, int inPort, DataType expectedType, unsigned long &checkCount, bool dstIsTerminator = false);


    /**
     * @brief Verifies the number of arcs connected to the output ports of a given node
     *
     * Also checks the total number of ports matches the size of the arcCounts vector.
     *
     * @param node The node to check
     * @param arcCounts A vector of arc counts.  The value of arcCounts[i] is the number of arcs that should be connected to output port i
     */
    static void verifyOutputPortArcCounts(std::shared_ptr<Node> node, std::vector<unsigned long> arcCounts);

    /**
     * @brief Verifies the number of arcs connected to the output ports of a given node as well as the total number of input ports
     *
     * Also checks the total number of ports matches the size of the arcCounts vector and that each input port has exactly 1 arc connected.
     * @param node The node to check
     * @param inputPorts number of imput ports the node should have
     * @param arcCounts A vector of arc counts.  The value of arcCounts[i] is the number of arcs that should be connected to output port i
     */
    static void verifyPortArcCounts(std::shared_ptr<Node> node, unsigned long inputPorts,
                                    std::vector<unsigned long> arcCounts);

    /**
     * @brief Checks that nodes with no parent are in the top level node list
     * @param design Design to check
     */
    static void verifyNodesWithoutParentAreInTopLevelList(Design &design);

    /**
     * @brief Check the port numbers match the port indexes for all nodes in the design
     *
     * Includes master nodes and nodes in node vector
     *
     * @param design Design to check
     */
    static void verifyNodePortNumbers(Design &design);

    /**
     * @brief Verifies that all nodes that are accessible from a traversal of the hierarchy are in the node array
     * Also checks that the number of nodes found in the traversal matches the number in the node vector of the design
     *
     * @param design the design to check
     */
    static void verifyNodesInDesignFromHierarchicalTraversal(Design &design);

private:
    //Note that need to use return type of void to play nice with gtest.  Count is therefore passed as a reference.
    static void verifyNodesInDesignFromHierarchicalTraversalHelper(Design &design, std::shared_ptr<Node> node, unsigned long &count);
};


#endif //VITIS_GRAPHTESTHELPER_H
