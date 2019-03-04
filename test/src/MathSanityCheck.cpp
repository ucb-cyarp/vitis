//
// Created by Christopher Yarp on 2019-02-27.
//

#include "gtest/gtest.h"

TEST(CMathSanity, Dec2IntPos) {
    double a = 0.1;
    int a_int = (int) a;

    double b = 0.5;
    int b_int = (int) b;

    double c = 0.75;
    int c_int = (int) c;

    double d = 1;
    int d_int = (int) d;

    double e = 1.1;
    int e_int = (int) e;


    ASSERT_EQ(a_int, 0);
    ASSERT_EQ(b_int, 0);
    ASSERT_EQ(c_int, 0);
    ASSERT_EQ(d_int, 1);
    ASSERT_EQ(d_int, 1);
}

TEST(CMathSanity, Dec2IntNeg) {
    double a = -0.1;
    int a_int = (int) a;

    double b = -0.5;
    int b_int = (int) b;

    double c = -0.75;
    int c_int = (int) c;

    double d = -1;
    int d_int = (int) d;

    double e = -1.1;
    int e_int = (int) e;


    ASSERT_EQ(a_int, 0);
    ASSERT_EQ(b_int, 0);
    ASSERT_EQ(c_int, 0);
    ASSERT_EQ(d_int, -1);
    ASSERT_EQ(d_int, -1);
}