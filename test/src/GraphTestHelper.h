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
     * @brief Check if the arc connected to 2 ports have a reference to the given arc.
     * @param arc arc to check
     */
    static void verifyArcLinks(std::shared_ptr<Arc> arc);

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
};


#endif //VITIS_GRAPHTESTHELPER_H
