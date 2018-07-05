//
// Created by Christopher Yarp on 7/5/18.
//

#include "gtest/gtest.h"
#include "GraphCore/DataType.h"

TEST(DataType, ParseFloats) {
    DataType singleType = DataType("single", false);

    ASSERT_EQ(singleType.isFloatingPt(), true);
    ASSERT_EQ(singleType.isSignedType(), true);
    ASSERT_EQ(singleType.getTotalBits(), 32);
    ASSERT_EQ(singleType.getFractionalBits(), 0);
    ASSERT_EQ(singleType.isComplex(), false);

    DataType doubleType = DataType("double", false);

    ASSERT_EQ(doubleType.isFloatingPt(), true);
    ASSERT_EQ(doubleType.isSignedType(), true);
    ASSERT_EQ(doubleType.getTotalBits(), 64);
    ASSERT_EQ(doubleType.getFractionalBits(), 0);
    ASSERT_EQ(doubleType.isComplex(), false);
}