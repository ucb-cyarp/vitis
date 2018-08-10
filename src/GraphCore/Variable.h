//
// Created by Christopher Yarp on 8/8/18.
//

#ifndef VITIS_VARIABLE_H
#define VITIS_VARIABLE_H

#include "DataType.h"
#include <string>

/**
 * \addtogroup GraphCore Graph Core
 */
/*@{*/

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

public:
    Variable();

    Variable(std::string name, DataType dataType);

    /**
     * @brief Get the C variable name for the real or imag component of the variable
     *
     * @note If @ref imag is true but the @ref Variable::dataType is not complex, an exception will be thrown
     *
     * @param imag imag if true, generates the imaginary component's name.  if false, generate the real component's name
     * @return C variable name
     */
    std::string getCVarName(bool imag = false);

    /**
     * @brief Get a C variable declaration statement
     *
     * Takes the form of datatype varname_<re/im>
     *
     * @note This function does not call DataType::getCPUStorageType.  As a result, if the dataType is fixed point, this function will throw an exception
     *
     * @note If @ref imag is true but the @ref Variable::dataType is not complex, an exception will be thrown
     *
     * @param imag if true, generates the imaginary component's declaration.  if false, generate the real component's declaration
     * @param includeWidth if true, includes width for vector types in declaration (ex. int[5]), otherwise does not (ex. int[])
     * @return a C variable declaration statement
     */
    std::string getCVarDecl(bool imag = false, bool includeWidth = false);

    std::string getName() const;
    void setName(const std::string &name);
    DataType getDataType() const;
    void setDataType(const DataType &dataType);

};

/*@}*/

#endif //VITIS_VARIABLE_H
