//
// Created by Christopher Yarp on 6/26/18.
//

#ifndef VITIS_DATATYPE_H
#define VITIS_DATATYPE_H

#include <string>
#include <vector>

/**
 * \addtogroup GraphCore Graph Core
 * @{
*/

/**
 * @brief Represents data types within the DSP design
 */
class DataType {
private:
    bool floatingPt;///< True if type is floating point, false if integer or fixed point
    bool signedType; ///< True if type is signed, false if unsigned
    bool complex; ///< True if the type is complex, false if it is real
    int totalBits; ///< The total number of bits in this type (per component)
    int fractionalBits; ///< The number of fractional bits if this is an integer or fixed point type
    std::vector<int> dimensions; ///< The dimensions of the datatype.  Each element of the vector is the size of that dimension.  Scalars have 1 dimension of size 1

public:
    /**
     * @brief Enum representing the dialect used when returning a string represetation of the DataType
     */
    enum class StringStyle{
        SIMULINK, ///<The dialect for Simulink (uses "single")
        C ///<The dialect for C (uses "float", appends _t to integer types)
    };


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
     * @param totalBits Total number of bits in type (per component)
     * @param fractionalBits Number of fractional bits if an integer or fixed point type
     * @param The dimensions of the datatype.  Each element of the vector is the size of that dimension.  Scalars have 1 dimension of size 1
     */
    DataType(bool floatingPt, bool signedType, bool complex, int totalBits, int fractionalBits, std::vector<int> dimensions);

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
     * @param complex true if the type is complex, false if it is real
     * @param width The width of the datatype (>1 if a vector)
     */
    DataType(std::string str, bool complex, std::vector<int> width = {1});

    //==== Functions ====

    /**
     * @brief Gets the dimensions as a c style array definition with each dimension having a seperate "[]".  If the datatype is a scalar, an empty string is returned.
     * @param includeDimension if true, the length of each dimension is included in the string (ex. [5][4] instead of [][])
     * @return
     */
    std::string dimensionsToString(bool includeDimension);

    /**
     * Gets the number of elements in the vector/matrix.  If a scalar, returns 1
     */
    int numberOfElements();

    /**
     * @brief Get the string representation of the DataType.  Complexity is not included in the string representation
     * @param stringStyle The stype of the string to return
     * @param includeDimensions if true, includes the dimensions in the type string.  if false, does not include the vector width in the type string. (only used for C style).  incudeArray must be true for this to have effect
     * @param includeArray if true, includes the array brackets [] in the type string.  If false, does not include the array brackets []. (only used for C style)
     * @return a type string
     */
    std::string toString(StringStyle stringStyle = StringStyle::SIMULINK, bool includeDimensions = false, bool includeArray = false);

    /**
     * @brief Get the smallest standard CPU type which can accomodate the given datatype.
     *
     * For integer and floating point types, the type returned is identical to the given type.
     *
     * For fixed point types, the type returned is the smallest standard integer type which is able to accomodate the
     * number of bits in the fixed point number.  The signed-ness of the varaible is preserved.
     *
     * @return A DataType object which specifies the smallest standard CPU type that can accomodate the
     */
    DataType getCPUStorageType();

    /**
     * @brief Checks if the type is a standard C/C++ CPU type.
     *
     * Standard Types include
     *   * bool
     *   * (u)int8_t
     *   * (u)int16_t
     *   * (u)int32_t
     *   * (u)int64_t
     *   * float
     *   * double
     *
     * @return true if a standard CPU type, false otherwise
     */
    bool isCPUType();

    /**
     * @brief Returns the C code required to convert an expression of one datatype to another datatype
     * @param expr expression to convert
     * @param oldType the origional type of the expression
     * @param newType the new type of the expression
     * @return C code required to convert the expression
     */
    static std::string cConvertType(std::string expr, DataType oldType, DataType newType);

    /**
     * @brief Deep equivalence check of DataType objects
     * @param rhs right hand side of the comparison
     * @return true if the types are equivalent
     */
    bool operator==(const DataType &rhs) const;

    /**
     * @brief Deep equivalence check of DataType objects
     * @param rhs right hand side of the comparison
     * @return true if the types are not equivalent
     */
    bool operator!=(const DataType &rhs) const;

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
    std::vector<int> getDimensions() const;
    void setDimensions(const std::vector<int> &dimensions);
    bool isScalar();

    //==== Added Functions ====
    bool isBool();

    /**
     * @brief Returns a new datatype that is expanded for block sizes or Delays
     *
     * For a block size of 1, the same datatype is returned
     * For a block size > 1, scalars are expanded to vectors, matricies have another dimensions prepended to them
     * The dimension is prepended so that each block is stored contiguously in memory (based on the C/C++ multidimensional array semantic)
     *
     * @param blockSize
     * @return
     */
    DataType expandForBlock(int blockSize);

    bool isVector();

};

/*! @} */

#endif //VITIS_DATATYPE_H
