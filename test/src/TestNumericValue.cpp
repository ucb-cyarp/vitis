//
// Created by Christopher Yarp on 7/9/18.
//

#include "gtest/gtest.h"
#include "GraphCore/NumericValue.h"
#include <regex>

TEST(NumericValue, ParseRealInt) {
    std::vector<NumericValue> test = NumericValue::parseXMLString("5");

    ASSERT_EQ(test.size(), 1);
    ASSERT_EQ(test[0].isComplex(), false);
    ASSERT_EQ(test[0].isFractional(), false);

    ASSERT_EQ(test[0].getRealInt(), 5);
    ASSERT_EQ(test[0].getImagInt(), 0);
    ASSERT_EQ(test[0].toString(), "5");
}

TEST(NumericValue, ParseRealDouble) {
    std::vector<NumericValue> test = NumericValue::parseXMLString("9.3");

    ASSERT_EQ(test.size(), 1);
    ASSERT_EQ(test[0].isComplex(), false);
    ASSERT_EQ(test[0].isFractional(), true);

    ASSERT_DOUBLE_EQ(test[0].getComplexDouble().real(), 9.3);
    ASSERT_DOUBLE_EQ(test[0].getComplexDouble().imag(), 0);

    std::regex regexExpr("9[.]3[0]*");
    std::smatch matches;
    std::string str0 = test[0].toString();
    bool regexMatched = std::regex_match(str0, matches, regexExpr);
    ASSERT_TRUE(regexMatched);

}

TEST(NumericValue, ParseImagInt) {
    std::vector<NumericValue> test = NumericValue::parseXMLString("-6i");

    ASSERT_EQ(test.size(), 1);
    ASSERT_EQ(test[0].isComplex(), true);
    ASSERT_EQ(test[0].isFractional(), false);

    ASSERT_EQ(test[0].getRealInt(), 0);
    ASSERT_EQ(test[0].getImagInt(), -6);
    ASSERT_EQ(test[0].toString(), "0 - 6i");
}

TEST(NumericValue, ParseImagDouble) {
    std::vector<NumericValue> test = NumericValue::parseXMLString("-3.25i");

    ASSERT_EQ(test.size(), 1);
    ASSERT_EQ(test[0].isComplex(), true);
    ASSERT_EQ(test[0].isFractional(), true);

    ASSERT_DOUBLE_EQ(test[0].getComplexDouble().real(), 0);
    ASSERT_DOUBLE_EQ(test[0].getComplexDouble().imag(), -3.25);

    std::regex regexExpr("0[.]?[0]* [-] 3[.]25[0]*i");
    std::smatch matches;
    std::string str0 = test[0].toString();
    bool regexMatched = std::regex_match(str0, matches, regexExpr);

    ASSERT_TRUE(regexMatched) << "Did not match " << test[0].toString();
}

TEST(NumericValue, ParseComplexInt) {
    std::vector<NumericValue> test = NumericValue::parseXMLString("8 - 5i");

    ASSERT_EQ(test.size(), 1);
    ASSERT_EQ(test[0].isComplex(), true);
    ASSERT_EQ(test[0].isFractional(), false);

    ASSERT_EQ(test[0].getRealInt(), 8);
    ASSERT_EQ(test[0].getImagInt(), -5);
    ASSERT_EQ(test[0].toString(), "8 - 5i");
}

TEST(NumericValue, ParseComplexDouble) {
    std::vector<NumericValue> test = NumericValue::parseXMLString("-3 + 2.5j");

    ASSERT_EQ(test.size(), 1);
    ASSERT_EQ(test[0].isComplex(), true);
    ASSERT_EQ(test[0].isFractional(), true);

    ASSERT_DOUBLE_EQ(test[0].getComplexDouble().real(), -3);
    ASSERT_DOUBLE_EQ(test[0].getComplexDouble().imag(), 2.5);

    std::regex regexExpr("-3[.]?[0]* [+] 2[.]5[0]*i");
    std::smatch matches;
    std::string str0 = test[0].toString();
    bool regexMatched = std::regex_match(str0, matches, regexExpr);

    ASSERT_TRUE(regexMatched) << "Did not match " << test[0].toString();
}

TEST(NumericValue, ParseRealIntArray) {
    std::vector<NumericValue> test = NumericValue::parseXMLString("[5, 2, 9]");

    ASSERT_EQ(test.size(), 3);

    ASSERT_EQ(test[0].isComplex(), false);
    ASSERT_EQ(test[0].isFractional(), false);
    ASSERT_EQ(test[0].getRealInt(), 5);
    ASSERT_EQ(test[0].getImagInt(), 0);
    ASSERT_EQ(test[0].toString(), "5");

    ASSERT_EQ(test[1].isComplex(), false);
    ASSERT_EQ(test[1].isFractional(), false);
    ASSERT_EQ(test[1].getRealInt(), 2);
    ASSERT_EQ(test[1].getImagInt(), 0);
    ASSERT_EQ(test[1].toString(), "2");

    ASSERT_EQ(test[2].isComplex(), false);
    ASSERT_EQ(test[2].isFractional(), false);
    ASSERT_EQ(test[2].getRealInt(), 9);
    ASSERT_EQ(test[2].getImagInt(), 0);
    ASSERT_EQ(test[2].toString(), "9");
}

TEST(NumericValue, ParseRealDoubleArray) {
    std::vector<NumericValue> test = NumericValue::parseXMLString("[9.3, 8.5, 2.25]");

    ASSERT_EQ(test.size(), 3);

    ASSERT_EQ(test[0].isComplex(), false);
    ASSERT_EQ(test[0].isFractional(), true);
    ASSERT_DOUBLE_EQ(test[0].getComplexDouble().real(), 9.3);
    ASSERT_DOUBLE_EQ(test[0].getComplexDouble().imag(), 0);
    std::regex regexExpr0("9[.]3[0]*");
    std::smatch matches0;
    std::string str0 = test[0].toString();
    bool regexMatched0 = std::regex_match(str0, matches0, regexExpr0);
    ASSERT_TRUE(regexMatched0);

    ASSERT_EQ(test[1].isComplex(), false);
    ASSERT_EQ(test[1].isFractional(), true);
    ASSERT_DOUBLE_EQ(test[1].getComplexDouble().real(), 8.5);
    ASSERT_DOUBLE_EQ(test[1].getComplexDouble().imag(), 0);
    std::regex regexExpr1("8[.]5[0]*");
    std::smatch matches1;
    std::string str1 = test[1].toString();
    bool regexMatched1 = std::regex_match(str1, matches1, regexExpr1);
    ASSERT_TRUE(regexMatched1);

    ASSERT_EQ(test[2].isComplex(), false);
    ASSERT_EQ(test[2].isFractional(), true);
    ASSERT_DOUBLE_EQ(test[2].getComplexDouble().real(), 2.25);
    ASSERT_DOUBLE_EQ(test[2].getComplexDouble().imag(), 0);
    std::regex regexExpr2("2[.]25[0]*");
    std::smatch matches2;
    std::string str2 = test[2].toString();
    bool regexMatched2 = std::regex_match(str2, matches2, regexExpr2);
    ASSERT_TRUE(regexMatched2);
}

TEST(NumericValue, ParseComplexIntArray) {
    std::vector<NumericValue> test = NumericValue::parseXMLString("[5+6i, 2 -8j, -9-2j]");

    ASSERT_EQ(test.size(), 3);

    ASSERT_EQ(test[0].isComplex(), true);
    ASSERT_EQ(test[0].isFractional(), false);
    ASSERT_EQ(test[0].getRealInt(), 5);
    ASSERT_EQ(test[0].getImagInt(), 6);
    ASSERT_EQ(test[0].toString(), "5 + 6i");

    ASSERT_EQ(test[1].isComplex(), true);
    ASSERT_EQ(test[1].isFractional(), false);
    ASSERT_EQ(test[1].getRealInt(), 2);
    ASSERT_EQ(test[1].getImagInt(), -8);
    ASSERT_EQ(test[1].toString(), "2 - 8i");

    ASSERT_EQ(test[2].isComplex(), true);
    ASSERT_EQ(test[2].isFractional(), false);
    ASSERT_EQ(test[2].getRealInt(), -9);
    ASSERT_EQ(test[2].getImagInt(), -2);
    ASSERT_EQ(test[2].toString(), "-9 - 2i");
}

TEST(NumericValue, ParseComplexDoubleArray) {
    std::vector<NumericValue> test = NumericValue::parseXMLString("[-9.3+8.25i, 8.5-2.5i, 2.25+3.5i]");

    ASSERT_EQ(test.size(), 3);

    ASSERT_EQ(test[0].isComplex(), true);
    ASSERT_EQ(test[0].isFractional(), true);
    ASSERT_DOUBLE_EQ(test[0].getComplexDouble().real(), -9.3);
    ASSERT_DOUBLE_EQ(test[0].getComplexDouble().imag(), 8.25);
    std::regex regexExpr0("[-]9[.]3[0]* [+] 8[.]25[0]*i");
    std::smatch matches0;
    std::string str0 = test[0].toString();
    bool regexMatched0 = std::regex_match(str0, matches0, regexExpr0);
    ASSERT_TRUE(regexMatched0);

    ASSERT_EQ(test[1].isComplex(), true);
    ASSERT_EQ(test[1].isFractional(), true);
    ASSERT_DOUBLE_EQ(test[1].getComplexDouble().real(), 8.5);
    ASSERT_DOUBLE_EQ(test[1].getComplexDouble().imag(), -2.5);
    std::regex regexExpr1("8[.]5[0]* [-] 2[.]5[0]*i");
    std::smatch matches1;
    std::string str1 = test[1].toString();
    bool regexMatched1 = std::regex_match(str1, matches1, regexExpr1);
    ASSERT_TRUE(regexMatched1);

    ASSERT_EQ(test[2].isComplex(), true);
    ASSERT_EQ(test[2].isFractional(), true);
    ASSERT_DOUBLE_EQ(test[2].getComplexDouble().real(), 2.25);
    ASSERT_DOUBLE_EQ(test[2].getComplexDouble().imag(), 3.5);
    std::regex regexExpr2("2[.]25[0]* [+] 3[.]5[0]*i");
    std::smatch matches2;
    std::string str2 = test[2].toString();
    bool regexMatched2 = std::regex_match(str2, matches2, regexExpr2);
    ASSERT_TRUE(regexMatched2);
}

TEST(NumericValue, ParseRealMixedArray) {
    std::vector<NumericValue> test = NumericValue::parseXMLString("[9, 8, 2.25]");

    ASSERT_EQ(test.size(), 3);

    ASSERT_EQ(test[0].isComplex(), false);
    ASSERT_EQ(test[0].isFractional(), true);
    ASSERT_DOUBLE_EQ(test[0].getComplexDouble().real(), 9);
    ASSERT_DOUBLE_EQ(test[0].getComplexDouble().imag(), 0);
    std::regex regexExpr0("9([.][0]*)?");
    std::smatch matches0;
    std::string str0 = test[0].toString();
    bool regexMatched0 = std::regex_match(str0, matches0, regexExpr0);
    ASSERT_TRUE(regexMatched0) << str0 << " does not match " << "9([.][0]*)?";

    ASSERT_EQ(test[1].isComplex(), false);
    ASSERT_EQ(test[1].isFractional(), true);
    ASSERT_DOUBLE_EQ(test[1].getComplexDouble().real(), 8);
    ASSERT_DOUBLE_EQ(test[1].getComplexDouble().imag(), 0);
    std::regex regexExpr1("8([.][0]*)?");
    std::smatch matches1;
    std::string str1 = test[1].toString();
    bool regexMatched1 = std::regex_match(str1, matches1, regexExpr1);
    ASSERT_TRUE(regexMatched1) << str0 << " does not match " << "8([.][0]*)?";

    ASSERT_EQ(test[2].isComplex(), false);
    ASSERT_EQ(test[2].isFractional(), true);
    ASSERT_DOUBLE_EQ(test[2].getComplexDouble().real(), 2.25);
    ASSERT_DOUBLE_EQ(test[2].getComplexDouble().imag(), 0);
    std::regex regexExpr2("2[.]25[0]*");
    std::smatch matches2;
    std::string str2 = test[2].toString();
    bool regexMatched2 = std::regex_match(str2, matches2, regexExpr2);
    ASSERT_TRUE(regexMatched2);
}

TEST(NumericValue, ParseComplexMixedArray) {
    std::vector<NumericValue> test = NumericValue::parseXMLString("[5+6i, 2 -8.5j, -9-2j]");

    ASSERT_EQ(test.size(), 3);

    ASSERT_EQ(test[0].isComplex(), true);
    ASSERT_EQ(test[0].isFractional(), true);
    ASSERT_DOUBLE_EQ(test[0].getComplexDouble().real(), 5);
    ASSERT_DOUBLE_EQ(test[0].getComplexDouble().imag(), 6);
    std::regex regexExpr0("5([.][0]*)? [+] 6([.][0]*)?i");
    std::smatch matches0;
    std::string str0 = test[0].toString();
    bool regexMatched0 = std::regex_match(str0, matches0, regexExpr0);
    ASSERT_TRUE(regexMatched0) << str0 << " does not match " << "5([.][0]*)? [+] 6([.][0]*)?i";

    ASSERT_EQ(test[1].isComplex(), true);
    ASSERT_EQ(test[1].isFractional(), true);
    ASSERT_DOUBLE_EQ(test[1].getComplexDouble().real(), 2);
    ASSERT_DOUBLE_EQ(test[1].getComplexDouble().imag(), -8.5);
    std::regex regexExpr1("2([.][0]*)? [-] 8[.]5[0]*i");
    std::smatch matches1;
    std::string str1 = test[1].toString();
    bool regexMatched1 = std::regex_match(str1, matches1, regexExpr1);
    ASSERT_TRUE(regexMatched1) << str0 << " does not match " << "2([.][0]*)? [-] 8[.]5[0]*i";

    ASSERT_EQ(test[2].isComplex(), true);
    ASSERT_EQ(test[2].isFractional(), true);
    ASSERT_DOUBLE_EQ(test[2].getComplexDouble().real(), -9);
    ASSERT_DOUBLE_EQ(test[2].getComplexDouble().imag(), -2);
    std::regex regexExpr2("[-]9([.][0]*)? [-] 2([.][0]*)?i");
    std::smatch matches2;
    std::string str2 = test[2].toString();
    bool regexMatched2 = std::regex_match(str2, matches2, regexExpr2);
    ASSERT_TRUE(regexMatched2) << str0 << " does not match " << "[-]9([.][0]*)? [-] 2([.][0]*)?i";
}

TEST(NumericValue, ParseRealMixedArrayWithExponents) {
    std::vector<NumericValue> test = NumericValue::parseXMLString("[9e1, 8e1, 2.25e1]");

    ASSERT_EQ(test.size(), 3);

    ASSERT_EQ(test[0].isComplex(), false);
    ASSERT_EQ(test[0].isFractional(), true);
    ASSERT_DOUBLE_EQ(test[0].getComplexDouble().real(), 90);
    ASSERT_DOUBLE_EQ(test[0].getComplexDouble().imag(), 0);

    ASSERT_EQ(test[1].isComplex(), false);
    ASSERT_EQ(test[1].isFractional(), true);
    ASSERT_DOUBLE_EQ(test[1].getComplexDouble().real(), 80);
    ASSERT_DOUBLE_EQ(test[1].getComplexDouble().imag(), 0);

    ASSERT_EQ(test[2].isComplex(), false);
    ASSERT_EQ(test[2].isFractional(), true);
    ASSERT_DOUBLE_EQ(test[2].getComplexDouble().real(), 22.5);
    ASSERT_DOUBLE_EQ(test[2].getComplexDouble().imag(), 0);
}

TEST(NumericValue, ParseComplexMixedArrayWithExponents) {
    std::vector<NumericValue> test = NumericValue::parseXMLString("[5e1+6e1i, 2e1 -8.5e1j, -9e1-2e1j]");

    ASSERT_EQ(test.size(), 3);

    ASSERT_EQ(test[0].isComplex(), true);
    ASSERT_EQ(test[0].isFractional(), true);
    ASSERT_DOUBLE_EQ(test[0].getComplexDouble().real(), 50);
    ASSERT_DOUBLE_EQ(test[0].getComplexDouble().imag(), 60);

    ASSERT_EQ(test[1].isComplex(), true);
    ASSERT_EQ(test[1].isFractional(), true);
    ASSERT_DOUBLE_EQ(test[1].getComplexDouble().real(), 20);
    ASSERT_DOUBLE_EQ(test[1].getComplexDouble().imag(), -85);

    ASSERT_EQ(test[2].isComplex(), true);
    ASSERT_EQ(test[2].isFractional(), true);
    ASSERT_DOUBLE_EQ(test[2].getComplexDouble().real(), -90);
    ASSERT_DOUBLE_EQ(test[2].getComplexDouble().imag(), -20);
}

TEST(NumericValue, ParseRealMixedArrayWithNegativeExponents) {
    std::vector<NumericValue> test = NumericValue::parseXMLString("[9e-1, -8e-1, 2.25e-1]");

    ASSERT_EQ(test.size(), 3);

    ASSERT_EQ(test[0].isComplex(), false);
    ASSERT_EQ(test[0].isFractional(), true);
    ASSERT_DOUBLE_EQ(test[0].getComplexDouble().real(), 0.9);
    ASSERT_DOUBLE_EQ(test[0].getComplexDouble().imag(), 0);

    ASSERT_EQ(test[1].isComplex(), false);
    ASSERT_EQ(test[1].isFractional(), true);
    ASSERT_DOUBLE_EQ(test[1].getComplexDouble().real(), -0.8);
    ASSERT_DOUBLE_EQ(test[1].getComplexDouble().imag(), 0);

    ASSERT_EQ(test[2].isComplex(), false);
    ASSERT_EQ(test[2].isFractional(), true);
    ASSERT_DOUBLE_EQ(test[2].getComplexDouble().real(), 0.225);
    ASSERT_DOUBLE_EQ(test[2].getComplexDouble().imag(), 0);
}

TEST(NumericValue, ParseRealFractionalArrayWithNegativeExponentsIntegerMantissa) {
    std::vector<NumericValue> test = NumericValue::parseXMLString("[9e-1, -8e-1]");

    ASSERT_EQ(test.size(), 2);

    ASSERT_EQ(test[0].isComplex(), false);
    ASSERT_EQ(test[0].isFractional(), true);
    ASSERT_DOUBLE_EQ(test[0].getComplexDouble().real(), 0.9);
    ASSERT_DOUBLE_EQ(test[0].getComplexDouble().imag(), 0);

    ASSERT_EQ(test[1].isComplex(), false);
    ASSERT_EQ(test[1].isFractional(), true);
    ASSERT_DOUBLE_EQ(test[1].getComplexDouble().real(), -0.8);
    ASSERT_DOUBLE_EQ(test[1].getComplexDouble().imag(), 0);
}

TEST(NumericValue, ParseLargeNegativeExponents) {
    std::vector<NumericValue> test = NumericValue::parseXMLString("[-5e-7]");

    ASSERT_EQ(test.size(), 1);

    ASSERT_EQ(test[0].isComplex(), false);
    ASSERT_EQ(test[0].isFractional(), true);
    ASSERT_DOUBLE_EQ(test[0].getComplexDouble().real(), -5E-7);
    ASSERT_DOUBLE_EQ(test[0].getComplexDouble().imag(), 0);
}