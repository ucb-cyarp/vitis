//
// Created by Christopher Yarp on 8/9/18.
//

#include "gtest/gtest.h"
#include "General/FixedPointHelpers.h"

TEST(FixedPointConvert, ConvertSignedInts) {
    ASSERT_EQ(FixedPointHelpers::toFixedPointSigned((int64_t) 0, 12, 5), 0);
    ASSERT_EQ(FixedPointHelpers::toFixedPointSigned((int64_t) 1, 12, 5), 32);
    ASSERT_EQ(FixedPointHelpers::toFixedPointSigned((int64_t) 5, 12, 5), 160);
    ASSERT_EQ(FixedPointHelpers::toFixedPointSigned((int64_t) 63, 12, 5), 2016);
    ASSERT_THROW(FixedPointHelpers::toFixedPointSigned((int64_t) 64, 12, 5), std::runtime_error);
    ASSERT_EQ(FixedPointHelpers::toFixedPointSigned((int64_t) -64, 12, 5), -2048);
    ASSERT_THROW(FixedPointHelpers::toFixedPointSigned((int64_t) -65, 12, 5), std::runtime_error);
}

TEST(FixedPointConvert, ConvertUnsignedInts) {
    ASSERT_EQ(FixedPointHelpers::toFixedPointUnsigned((uint64_t) 0, 12, 5), 0);
    ASSERT_EQ(FixedPointHelpers::toFixedPointUnsigned((uint64_t) 1, 12, 5), 32);
    ASSERT_EQ(FixedPointHelpers::toFixedPointUnsigned((uint64_t) 5, 12, 5), 160);
    ASSERT_EQ(FixedPointHelpers::toFixedPointUnsigned((uint64_t) 127, 12, 5), 4064);
    ASSERT_THROW(FixedPointHelpers::toFixedPointUnsigned((uint64_t) 128, 12, 5), std::runtime_error);
}

TEST(FixedPointConvert, ConvertSignedReal) {
    //Same as Int Test
    ASSERT_EQ(FixedPointHelpers::toFixedPointSigned(0.0, 12, 5), 0);
    ASSERT_EQ(FixedPointHelpers::toFixedPointSigned(1.0, 12, 5), 32);
    ASSERT_EQ(FixedPointHelpers::toFixedPointSigned(5.0, 12, 5), 160);
    ASSERT_EQ(FixedPointHelpers::toFixedPointSigned(63.0, 12, 5), 2016);
    ASSERT_THROW(FixedPointHelpers::toFixedPointSigned(64.0, 12, 5), std::runtime_error);
    ASSERT_EQ(FixedPointHelpers::toFixedPointSigned(-64.0, 12, 5), -2048);
    ASSERT_THROW(FixedPointHelpers::toFixedPointSigned(-65.0, 12, 5), std::runtime_error);

    //Check Fractional
    ASSERT_EQ(FixedPointHelpers::toFixedPointSigned(5.25, 12, 5), 168); //Fits Exactly
    ASSERT_EQ(FixedPointHelpers::toFixedPointSigned(-5.25, 12, 5), -168); //Fits Exactly

    //Check Round
    ASSERT_EQ(FixedPointHelpers::toFixedPointSigned(5.9765625, 12, 5), 191); //Round Down 5.96875 (Break Point is 5.984375)
    ASSERT_EQ(FixedPointHelpers::toFixedPointSigned(5.984375, 12, 5), 192); //Round Up (6)
    ASSERT_EQ(FixedPointHelpers::toFixedPointSigned(-5.9765625, 12, 5), -191); //Round Up -5.96875 (Break Point is -5.984375)
    ASSERT_EQ(FixedPointHelpers::toFixedPointSigned(-5.984375, 12, 5), -192); //Round Down (-6)
}

TEST(FixedPointConvert, ConvertUnsignedReal) {
    //Same as Int Test
    ASSERT_EQ(FixedPointHelpers::toFixedPointUnsigned(0.0, 12, 5), 0);
    ASSERT_EQ(FixedPointHelpers::toFixedPointUnsigned(1.0, 12, 5), 32);
    ASSERT_EQ(FixedPointHelpers::toFixedPointUnsigned(5.0, 12, 5), 160);
    ASSERT_EQ(FixedPointHelpers::toFixedPointUnsigned(127.0, 12, 5), 4064);
    ASSERT_THROW(FixedPointHelpers::toFixedPointUnsigned(128.0, 12, 5), std::runtime_error);
    ASSERT_THROW(FixedPointHelpers::toFixedPointUnsigned(-1.0, 12, 5), std::runtime_error);

    //Check Fractional
    ASSERT_EQ(FixedPointHelpers::toFixedPointUnsigned(5.25, 12, 5), 168); //Fits Exactly

    //Check Round
    ASSERT_EQ(FixedPointHelpers::toFixedPointUnsigned(5.9765625, 12, 5), 191); //Round Down 5.96875 (Break Point is 5.984375)
    ASSERT_EQ(FixedPointHelpers::toFixedPointUnsigned(5.984375, 12, 5), 192); //Round Up (6)
}