//
// Created by Christopher Yarp on 8/8/18.
//

#ifndef VITIS_VARIABLE_H
#define VITIS_VARIABLE_H

#include "DataType.h"
#include "NumericValue.h"
#include <string>

/**
 * \addtogroup GraphCore Graph Core
 * @{
*/

#define VITIS_C_VAR_NAME_RE_SUFFIX "_re" ///<The suffix for the real component of C var names
#define VITIS_C_VAR_NAME_IM_SUFFIX "_im" ///<The suffix for the imag component of C var names

/**
 * @brief A class used to contain information about program variables including their name and DataType
 *
 * Used like a tuple or struct
 */
class Variable {
protected:
    std::string name; ///<The name of the variable
    DataType dataType; ///<The DataType of the variable
    std::vector<NumericValue> initValue; ///<The Initial value of the variable.  Stored in C/C++ memory order
    bool atomicVar; ///< Indicates if the variable is a stdatomic type

public:
    Variable();

    Variable(std::string name, DataType dataType);

    Variable(std::string name, DataType dataType, std::vector<NumericValue> initValue);

    Variable(std::string name, DataType dataType, std::vector<NumericValue> initValue, bool volatileVar);

    /**
     * @brief Get the C variable name for the real or imag component of the variable
     *
     * @note If imag is true but the @ref Variable::dataType is not complex, an exception will be thrown
     *
     * @param imag imag if true, generates the imaginary component's name.  if false, generate the real component's name
     * @return C variable name
     */
    std::string getCVarName(bool imag = false);

    /**
     * @brief Get a C variable declaration statement
     *
     * Takes the form of datatype varname_\<re/im\>
     *
     * @note This function does call DataType::getCPUStorageType to get the delaration type
     *
     * @note If imag is true but the @ref Variable::dataType is not complex, an exception will be thrown
     *
     * @param imag if true, generates the imaginary component's declaration.  if false, generate the real component's declaration
     * @param includeDimensions if true, includes dimensions for vector/matrix types in declaration (ex. int name[5]), otherwise does not (ex. int name[]).  includeArray
     * @param includeInit if true, includes an assignment to the initial value.  If not, no assignment is made in the declaration statement
     * @param includeArray if true, includes the array brackets in the declaration (int name[]), if false the array brackets are not included (int name).
     * @param includeRef if true, includes the & after the type declaration
     * @return a C variable declaration statement
     */
    std::string getCVarDecl(bool imag = false, bool includeDimensions = false, bool includeInit = false, bool includeArray = false, bool includeRef = false);

    std::string getCPtrDecl(bool imag = false);

    std::string getName() const;
    void setName(const std::string &name);
    DataType getDataType() const;
    void setDataType(const DataType &dataType);
    std::vector<NumericValue> getInitValue() const;
    void setInitValue(const std::vector<NumericValue> &initValue);
    bool isAtomicVar() const;
    void setAtomicVar(bool volatileVar);

    bool operator==(const Variable &rhs) const;

    bool operator!=(const Variable &rhs) const;
};

/*! @} */

#endif //VITIS_VARIABLE_H
