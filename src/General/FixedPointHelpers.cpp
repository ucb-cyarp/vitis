//
// Created by Christopher Yarp on 8/9/18.
//

#include <stdexcept>
#include <string>
#include <cmath>
#include "FixedPointHelpers.h"
#include "GeneralHelper.h"
#include "ErrorHelpers.h"

int64_t FixedPointHelpers::toFixedPointSigned(int64_t orig, int totalBits, int fractionalBits) {
    //The original integer must be able to fit in the number of integer bits in the fixed point number (totalBits-fractionalBits)
    int integerBits = totalBits-fractionalBits;

    //The max range for signed integers is [-2^(integerBits-1), 2^(integerBits-1)-1]
    int64_t minInt = ((int64_t)(-1)) << (integerBits-1); // (-1) << n is equiv to -2^n
    int64_t maxInt = (((int64_t)1) << (integerBits-1))-1; // 1 << n is equiv to 2^n

    if(orig < minInt || orig > maxInt){
        throw std::runtime_error(ErrorHelpers::genErrorStr("The signed integer " + GeneralHelper::to_string(orig) + " does not fit into a fixed point number with " + GeneralHelper::to_string(totalBits) + " total bits and " + GeneralHelper::to_string(fractionalBits) + " fractional bits"));
    }

    //The number is shifted left by fractional bits since the orig integer has a binary point at 0
    //(basically, when introducing fractional bits, we need to shift the integer portion by the number of fractional
    //bits being inserted to maintain the proper scaling of the integer).

    int64_t fixedPoint = orig << fractionalBits;

    return fixedPoint;
}

uint64_t FixedPointHelpers::toFixedPointUnsigned(uint64_t orig, int totalBits, int fractionalBits) {
    //The original integer must be able to fit in the number of integer bits in the fixed point number (totalBits-fractionalBits)
    int integerBits = totalBits-fractionalBits;

    //The max range for unsigned integers is [0, 2^(integerBits)-1]
    int64_t maxInt = (((uint64_t)1) << integerBits)-1; // 1 << n is equiv to 2^n

    if(orig > maxInt){ //Don't need to check min bound for unsigned
        throw std::runtime_error(ErrorHelpers::genErrorStr("The unsigned integer " + GeneralHelper::to_string(orig) + " does not fit into a fixed point number with " + GeneralHelper::to_string(totalBits) + " total bits and " + GeneralHelper::to_string(fractionalBits) + " fractional bits"));
    }

    //The number is shifted left by fractional bits since the orig integer has a binary point at 0
    //(basically, when introducing fractional bits, we need to shift the integer portion by the number of fractional
    //bits being inserted to maintain the proper scaling of the integer).

    uint64_t fixedPoint = orig << fractionalBits;

    return fixedPoint;
}

int64_t FixedPointHelpers::toFixedPointSigned(double orig, int totalBits, int fractionalBits) {
    //The floating point type is multiplied by 2^(fractionalBits) to scale it by the fixed point binary point offset).
    //This is the same thing we do when shifting integers by the number of fractional bits.
    //The fractional portion of the floating point number after scaling will not be included in the fixed point number.

    //If directly cast to an integer, the reamaining fractional numbers will be trunkated.
    //Alternativly, the number can be rounded to the nearest integer.  This may be desierable to avoid the unintentinal
    //intoduction of a bias that the trunkation can cause.

    //When rounding, the check for whether or not the number can fit in the fixed point variable must be performed after
    //the round.

    double scaled = orig * pow(2, fractionalBits);

    double scaledRounded = std::round(scaled);

    //Perform range checks for the scaled number.  We use the total number of bits since the number has already been scaled
    int64_t minInt = (-1) << (totalBits-1); // (-1) << n is equiv to -2^n
    int64_t maxInt = (1 << (totalBits-1))-1; // 1 << n is equiv to 2^n

    if(scaledRounded < minInt || scaledRounded > maxInt){
        throw std::runtime_error(ErrorHelpers::genErrorStr("The signed real number " + GeneralHelper::to_string(orig) + " does not fit into a fixed point number with " + GeneralHelper::to_string(totalBits) + " total bits and " + GeneralHelper::to_string(fractionalBits) + " fractional bits"));
    }

    int64_t fixedPt = static_cast<int64_t>(scaledRounded);

    return fixedPt;
}

uint64_t FixedPointHelpers::toFixedPointUnsigned(double orig, int totalBits, int fractionalBits) {
    //The floating point type is multiplied by 2^(fractionalBits) to scale it by the fixed point binary point offset).
    //This is the same thing we do when shifting integers by the number of fractional bits.
    //The fractional portion of the floating point number after scaling will not be included in the fixed point number.

    //If directly cast to an integer, the reamaining fractional numbers will be trunkated.
    //Alternativly, the number can be rounded to the nearest integer.  This may be desierable to avoid the unintentinal
    //intoduction of a bias that the trunkation can cause.

    //When rounding, the check for whether or not the number can fit in the fixed point variable must be performed after
    //the round.

    double scaled = orig * pow(2, fractionalBits);

    double scaledRounded = std::round(scaled);

    //Perform range checks for the scaled number.  We use the total number of bits since the number has already been scaled
    int64_t minInt = 0;
    int64_t maxInt = (1 << totalBits)-1; // 1 << n is equiv to 2^n

    if(scaledRounded < minInt || scaledRounded > maxInt){ //Check for min since double can hold a negative value
        throw std::runtime_error(ErrorHelpers::genErrorStr("The unsigned real number " + GeneralHelper::to_string(orig) + " does not fit into a fixed point number with " + GeneralHelper::to_string(totalBits) + " total bits and " + GeneralHelper::to_string(fractionalBits) + " fractional bits"));
    }

    uint64_t fixedPt = static_cast<uint64_t>(scaledRounded);

    return fixedPt;
}