//
// Created by Christopher Yarp on 6/26/18.
//

#ifndef VITIS_DATATYPE_H
#define VITIS_DATATYPE_H

#include <string>

/**
 * @brief Represents data types within the DSP design
 */
class DataType {
private:
    bool floatingPt;///< True if type is floating point, false if integer or fixed point
    bool signedType; ///< True if type is signed, false if unsigned
    bool complex; ///< True if the type is complex, false if it is real
    int totalBits; ///< The total number of bits in this type
    int fractionalBits; ///< The number of fractional bits if this is an integer or fixed point type

public:
    //==== Constructors ====
    /**
     * @brief Construct a default DataType object
     * Constructs an unsigned, real, integer with 0 bits and 0 fractional bits
     */
    DataType();

    /**
     * @brief Construct a DataType object with the specified properties
     *
     * @param floatingPt True if floating point, false if integer or fixed point
     * @param signedType True if signed, false if unsigned
     * @param complex True if complex, false if real
     * @param totalBits Total number of bits in type
     * @param fractionalBits Number of fractional bits if an integer or fixed point type
     */
    DataType(bool floatingPt, bool signedType, bool complex, int totalBits, int fractionalBits);

    /**
     * @brief Constructs a DataType object from a string description of a type.
     *
     * For example if the string is "double" a signed, floating point, type of 64 total bits and 0 fractional bits
     * is created.
     *
     * Can parse Matlab/Simulink style fixed point descriptions such as sfix18_12 which is a signed real fixed
     * point number with 18 total bits and 12 fractional bits.
     *
     * @param str string representation of type
     */
    DataType(std::string str);

    //==== Functions ====
    /**
     * @brief Deep equivalence check of DataType objects
     * @param a
     * @param b
     * @return true if the types are equivalent
     */
    bool operator == (const DataType &a, const DataType &b);

    //==== Getters/Setters ====
    bool isFloatingPt() const;
    void setFloatingPt(bool floatingPt);
    bool isSignedType() const;
    void setSignedType(bool signedType);
    bool isComplex() const;
    void setComplex(bool complex);
    int getTotalBits() const;
    void setTotalBits(int totalBits);
    int getFractionalBits() const;
    void setFractionalBits(int fractionalBits);

};


#endif //VITIS_DATATYPE_H
