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
#include "GraphCore/Port.h"
#include "GraphCore/DataType.h"

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
};


#endif //VITIS_GRAPHTESTHELPER_H
