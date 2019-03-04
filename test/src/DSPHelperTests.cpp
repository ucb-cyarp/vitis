//
// Created by Christopher Yarp on 2019-02-28.
//

#include "gtest/gtest.h"
#include "General/DSPHelpers.h"

TEST(DSPHelperTests, bin2gray) {
    ASSERT_EQ(DSPHelpers::bin2Gray(0), 0);
    ASSERT_EQ(DSPHelpers::bin2Gray(1), 1);
    ASSERT_EQ(DSPHelpers::bin2Gray(2), 3);
    ASSERT_EQ(DSPHelpers::bin2Gray(3), 2);
    ASSERT_EQ(DSPHelpers::bin2Gray(4), 6);
    ASSERT_EQ(DSPHelpers::bin2Gray(5), 7);
    ASSERT_EQ(DSPHelpers::bin2Gray(6), 5);
    ASSERT_EQ(DSPHelpers::bin2Gray(7), 4);
    ASSERT_EQ(DSPHelpers::bin2Gray(8), 12);
    ASSERT_EQ(DSPHelpers::bin2Gray(9), 13);
    ASSERT_EQ(DSPHelpers::bin2Gray(10), 15);
    ASSERT_EQ(DSPHelpers::bin2Gray(11), 14);
    ASSERT_EQ(DSPHelpers::bin2Gray(12), 10);
    ASSERT_EQ(DSPHelpers::bin2Gray(13), 11);
    ASSERT_EQ(DSPHelpers::bin2Gray(14), 9);
    ASSERT_EQ(DSPHelpers::bin2Gray(15), 8);
}
