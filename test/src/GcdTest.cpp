//
// Created by Christopher Yarp on 4/27/20.
//

#include "gtest/gtest.h"
#include "General/GeneralHelper.h"

TEST(GeneralHelperTest, GCD) {
    ASSERT_EQ(GeneralHelper::gcd(2, 0), 2);
    ASSERT_EQ(GeneralHelper::gcd(0, 2), 2);
    ASSERT_EQ(GeneralHelper::gcd(1, 1), 1);
    ASSERT_EQ(GeneralHelper::gcd(35, 15), 5);
    ASSERT_EQ(GeneralHelper::gcd(15, 35), 5);
    ASSERT_EQ(GeneralHelper::gcd(30, 15), 15);
    ASSERT_EQ(GeneralHelper::gcd(15, 30), 15);
    ASSERT_EQ(GeneralHelper::gcd(17, 30), 1);
    ASSERT_EQ(GeneralHelper::gcd(30, 17), 1);
    ASSERT_EQ(GeneralHelper::gcd(17, 17), 17);
}