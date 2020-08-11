//
// Created by Christopher Yarp on 8/9/18.
//

#ifndef VITIS_FIXEDPOINTHELPERS_H
#define VITIS_FIXEDPOINTHELPERS_H

#include <cstdint>

namespace FixedPointHelpers {
    /**
     * @brief Scales a signed integer to a fixed point
     *
     * If the integer will not fit into the fixed point type, an exception is thrown
     *
     * @param orig The original integer to convert to fixed point
     * @param totalBits The total number of bits in the fixed point type
     * @param fractionalBits The number of fractional bits in the fixed point type
     * @return The integer converted to the appropriate fixed point type (via shifting). Returned as an integer since there is no standard C fixed point type
     */
    int64_t toFixedPointSigned(int64_t orig, int totalBits, int fractionalBits);

    /**
     * @brief Scales an unsigned integer to a fixed point
     *
     * If the integer will not fit into the fixed point type, an exception is thrown
     *
     * @param orig The original integer to convert to fixed point
     * @param totalBits The total number of bits in the fixed point type
     * @param fractionalBits The number of fractional bits in the fixed point type
     * @return The integer converted to the appropriate fixed point type (via shifting). Returned as an integer since there is no standard C fixed point type
     */
    uint64_t toFixedPointUnsigned(uint64_t orig, int totalBits, int fractionalBits);

    /**
     * @brief Scales a signed fractional number to a fixed point
     *
     * If the number will not fit into the fixed point type, an exception is thrown
     *
     * @param orig The original integer to convert to fixed point
     * @param totalBits The total number of bits in the fixed point type
     * @param fractionalBits The number of fractional bits in the fixed point type
     * @return The integer converted to the appropriate fixed point type (via shifting). Returned as an integer since there is no standard C fixed point type
     */
    int64_t toFixedPointSigned(double orig, int totalBits, int fractionalBits);

    /**
     * @brief Scales an unsigned fractional number to a fixed point
     *
     * If the number will not fit into the fixed point type, an exception is thrown
     *
     * @param orig The original integer to convert to fixed point
     * @param totalBits The total number of bits in the fixed point type
     * @param fractionalBits The number of fractional bits in the fixed point type
     * @return The integer converted to the appropriate fixed point type (via shifting). Returned as an integer since there is no standard C fixed point type
     */
    uint64_t toFixedPointUnsigned(double orig, int totalBits, int fractionalBits);
};


#endif //VITIS_FIXEDPOINTHELPERS_H
