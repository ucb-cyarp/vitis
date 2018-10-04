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

TEST(DataType, ParseInts) {
    DataType int8Type = DataType("int8", false);
    ASSERT_EQ(int8Type.isFloatingPt(), false);
    ASSERT_EQ(int8Type.isSignedType(), true);
    ASSERT_EQ(int8Type.getTotalBits(), 8);
    ASSERT_EQ(int8Type.getFractionalBits(), 0);
    ASSERT_EQ(int8Type.isComplex(), false);

    DataType int16Type = DataType("int16", false);
    ASSERT_EQ(int16Type.isFloatingPt(), false);
    ASSERT_EQ(int16Type.isSignedType(), true);
    ASSERT_EQ(int16Type.getTotalBits(), 16);
    ASSERT_EQ(int16Type.getFractionalBits(), 0);
    ASSERT_EQ(int16Type.isComplex(), false);

    DataType int32Type = DataType("int32", false);
    ASSERT_EQ(int32Type.isFloatingPt(), false);
    ASSERT_EQ(int32Type.isSignedType(), true);
    ASSERT_EQ(int32Type.getTotalBits(), 32);
    ASSERT_EQ(int32Type.getFractionalBits(), 0);
    ASSERT_EQ(int32Type.isComplex(), false);

    DataType int64Type = DataType("int64", false);
    ASSERT_EQ(int64Type.isFloatingPt(), false);
    ASSERT_EQ(int64Type.isSignedType(), true);
    ASSERT_EQ(int64Type.getTotalBits(), 64);
    ASSERT_EQ(int64Type.getFractionalBits(), 0);
    ASSERT_EQ(int64Type.isComplex(), false);

    DataType uint8Type = DataType("uint8", false);
    ASSERT_EQ(uint8Type.isFloatingPt(), false);
    ASSERT_EQ(uint8Type.isSignedType(), false);
    ASSERT_EQ(uint8Type.getTotalBits(), 8);
    ASSERT_EQ(uint8Type.getFractionalBits(), 0);
    ASSERT_EQ(uint8Type.isComplex(), false);

    DataType uint16Type = DataType("uint16", false);
    ASSERT_EQ(uint16Type.isFloatingPt(), false);
    ASSERT_EQ(uint16Type.isSignedType(), false);
    ASSERT_EQ(uint16Type.getTotalBits(), 16);
    ASSERT_EQ(uint16Type.getFractionalBits(), 0);
    ASSERT_EQ(uint16Type.isComplex(), false);

    DataType uint32Type = DataType("uint32", false);
    ASSERT_EQ(uint32Type.isFloatingPt(), false);
    ASSERT_EQ(uint32Type.isSignedType(), false);
    ASSERT_EQ(uint32Type.getTotalBits(), 32);
    ASSERT_EQ(uint32Type.getFractionalBits(), 0);
    ASSERT_EQ(uint32Type.isComplex(), false);

    DataType uint64Type = DataType("uint64", false);
    ASSERT_EQ(uint64Type.isFloatingPt(), false);
    ASSERT_EQ(uint64Type.isSignedType(), false);
    ASSERT_EQ(uint64Type.getTotalBits(), 64);
    ASSERT_EQ(uint64Type.getFractionalBits(), 0);
    ASSERT_EQ(uint64Type.isComplex(), false);
}

TEST(DataType, ParseBool) {
    DataType boolType = DataType("boolean", false);
    ASSERT_EQ(boolType.isFloatingPt(), false);
    ASSERT_EQ(boolType.isSignedType(), false);
    ASSERT_EQ(boolType.getTotalBits(), 1);
    ASSERT_EQ(boolType.getFractionalBits(), 0);
    ASSERT_EQ(boolType.isComplex(), false);

    DataType logicalType = DataType("logical", false);
    ASSERT_EQ(logicalType.isFloatingPt(), false);
    ASSERT_EQ(logicalType.isSignedType(), false);
    ASSERT_EQ(logicalType.getTotalBits(), 1);
    ASSERT_EQ(logicalType.getFractionalBits(), 0);
    ASSERT_EQ(logicalType.isComplex(), false);
}

TEST(DataType, ParseFixedPt) {
    DataType sfix32_En16Type = DataType("sfix32_En16", false);
    ASSERT_EQ(sfix32_En16Type.isFloatingPt(), false);
    ASSERT_EQ(sfix32_En16Type.isSignedType(), true);
    ASSERT_EQ(sfix32_En16Type.getTotalBits(), 32);
    ASSERT_EQ(sfix32_En16Type.getFractionalBits(), 16);
    ASSERT_EQ(sfix32_En16Type.isComplex(), false);

    DataType ufix32_En11Type = DataType("ufix32_En11", false);
    ASSERT_EQ(ufix32_En11Type.isFloatingPt(), false);
    ASSERT_EQ(ufix32_En11Type.isSignedType(), false);
    ASSERT_EQ(ufix32_En11Type.getTotalBits(), 32);
    ASSERT_EQ(ufix32_En11Type.getFractionalBits(), 11);
    ASSERT_EQ(ufix32_En11Type.isComplex(), false);

    DataType ufix32Type = DataType("ufix42", false);
    ASSERT_EQ(ufix32Type.isFloatingPt(), false);
    ASSERT_EQ(ufix32Type.isSignedType(), false);
    ASSERT_EQ(ufix32Type.getTotalBits(), 42);
    ASSERT_EQ(ufix32Type.getFractionalBits(), 0);
    ASSERT_EQ(ufix32Type.isComplex(), false);

DataType fixdt_sfix32_En16Type = DataType("fixdt(1,32,16)", false);
    ASSERT_EQ(fixdt_sfix32_En16Type.isFloatingPt(), false);
    ASSERT_EQ(fixdt_sfix32_En16Type.isSignedType(), true);
    ASSERT_EQ(fixdt_sfix32_En16Type.getTotalBits(), 32);
    ASSERT_EQ(fixdt_sfix32_En16Type.getFractionalBits(), 16);
    ASSERT_EQ(fixdt_sfix32_En16Type.isComplex(), false);

    DataType fixdt_ufix32_En11Type = DataType("fixdt(0,32,11)", false);
    ASSERT_EQ(fixdt_ufix32_En11Type.isFloatingPt(), false);
    ASSERT_EQ(fixdt_ufix32_En11Type.isSignedType(), false);
    ASSERT_EQ(fixdt_ufix32_En11Type.getTotalBits(), 32);
    ASSERT_EQ(fixdt_ufix32_En11Type.getFractionalBits(), 11);
    ASSERT_EQ(fixdt_ufix32_En11Type.isComplex(), false);

    DataType fixdt_sfix32_En16Type2 = DataType("fixdt(1, 32, 16)", false);
    ASSERT_EQ(fixdt_sfix32_En16Type2.isFloatingPt(), false);
    ASSERT_EQ(fixdt_sfix32_En16Type2.isSignedType(), true);
    ASSERT_EQ(fixdt_sfix32_En16Type2.getTotalBits(), 32);
    ASSERT_EQ(fixdt_sfix32_En16Type2.getFractionalBits(), 16);
    ASSERT_EQ(fixdt_sfix32_En16Type2.isComplex(), false);

    DataType fixdt_ufix32_En11Type2 = DataType("fixdt(0, 32, 11)", false);
    ASSERT_EQ(fixdt_ufix32_En11Type2.isFloatingPt(), false);
    ASSERT_EQ(fixdt_ufix32_En11Type2.isSignedType(), false);
    ASSERT_EQ(fixdt_ufix32_En11Type2.getTotalBits(), 32);
    ASSERT_EQ(fixdt_ufix32_En11Type2.getFractionalBits(), 11);
    ASSERT_EQ(fixdt_ufix32_En11Type2.isComplex(), false);
}

TEST(DataType, toStringFloats) {
    DataType singleType = DataType("single", false);
    ASSERT_EQ(singleType.toString(), "single");

    DataType doubleType = DataType("double", false);
    ASSERT_EQ(doubleType.toString(), "double");
}

TEST(DataType, toStringInts) {
    DataType int8Type = DataType("int8", false);
    ASSERT_EQ(int8Type.toString(), "int8");

    DataType int16Type = DataType("int16", false);
    ASSERT_EQ(int16Type.toString(), "int16");

    DataType int32Type = DataType("int32", false);
    ASSERT_EQ(int32Type.toString(), "int32");

    DataType int64Type = DataType("int64", false);
    ASSERT_EQ(int64Type.toString(), "int64");

    DataType uint8Type = DataType("uint8", false);
    ASSERT_EQ(uint8Type.toString(), "uint8");

    DataType uint16Type = DataType("uint16", false);
    ASSERT_EQ(uint16Type.toString(), "uint16");

    DataType uint32Type = DataType("uint32", false);
    ASSERT_EQ(uint32Type.toString(), "uint32");

    DataType uint64Type = DataType("uint64", false);
    ASSERT_EQ(uint64Type.toString(), "uint64");
}

TEST(DataType, toStringBool) {
    DataType boolType = DataType("boolean", false);
    ASSERT_EQ(boolType.toString(), "boolean");

    DataType logicalType = DataType("logical", false);
    ASSERT_EQ(logicalType.toString(), "boolean");
}

TEST(DataType, toStringFixedPt) {
    DataType sfix32_En16Type = DataType("sfix32_En16", false);
    ASSERT_EQ(sfix32_En16Type.toString(), "sfix32_En16");

    DataType ufix32_En11Type = DataType("ufix32_En11", false);
    ASSERT_EQ(ufix32_En11Type.toString(), "ufix32_En11");

    DataType fixdt_sfix32_En16Type = DataType("fixdt(1,32,16)", false);
    ASSERT_EQ(fixdt_sfix32_En16Type.toString(), "sfix32_En16");

    DataType fixdt_ufix32_En11Type = DataType("fixdt(0,32,11)", false);
    ASSERT_EQ(fixdt_ufix32_En11Type.toString(), "ufix32_En11");

    DataType fixdt_sfix32_En16Type2 = DataType("fixdt(1, 32, 16)", false);
    ASSERT_EQ(fixdt_sfix32_En16Type2.toString(), "sfix32_En16");

    DataType fixdt_ufix32_En11Type2 = DataType("fixdt(0, 32, 11)", false);
    ASSERT_EQ(fixdt_ufix32_En11Type2.toString(), "ufix32_En11");
}

TEST(DataType, toStringC) {
    DataType singleType = DataType("single", false);
    ASSERT_EQ(singleType.toString(DataType::StringStyle::C), "float");

    DataType doubleType = DataType("double", false);
    ASSERT_EQ(doubleType.toString(DataType::StringStyle::C), "double");

    DataType int8Type = DataType("int8", false);
    ASSERT_EQ(int8Type.toString(DataType::StringStyle::C), "int8_t");

    DataType int16Type = DataType("int16", false);
    ASSERT_EQ(int16Type.toString(DataType::StringStyle::C), "int16_t");

    DataType int32Type = DataType("int32", false);
    ASSERT_EQ(int32Type.toString(DataType::StringStyle::C), "int32_t");

    DataType int64Type = DataType("int64", false);
    ASSERT_EQ(int64Type.toString(DataType::StringStyle::C), "int64_t");

    DataType uint8Type = DataType("uint8", false);
    ASSERT_EQ(uint8Type.toString(DataType::StringStyle::C), "uint8_t");

    DataType uint16Type = DataType("uint16", false);
    ASSERT_EQ(uint16Type.toString(DataType::StringStyle::C), "uint16_t");

    DataType uint32Type = DataType("uint32", false);
    ASSERT_EQ(uint32Type.toString(DataType::StringStyle::C), "uint32_t");

    DataType uint64Type = DataType("uint64", false);
    ASSERT_EQ(uint64Type.toString(DataType::StringStyle::C), "uint64_t");

    DataType boolType = DataType("boolean", false);
    ASSERT_EQ(boolType.toString(DataType::StringStyle::C), "bool");
}