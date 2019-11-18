//
// Created by Christopher Yarp on 9/24/19.
//

#ifndef VITIS_PARTITIONPARAMS_H
#define VITIS_PARTITIONPARAMS_H

#include <string>

/**
 * \addtogroup MultiThread Multi-Thread Support
 * @{
 */

/**
 * @brief Class for representing partitioning parameters.
 */
class PartitionParams {
public:
    /**
     * @brief Represents the types of partitioning schemes supported
     */
    enum class PartitionType{
        MANUAL ///<Paritioning is manually accomplished by using VITIS_PARTITION directives
    };

    /**
     * @brief Parse a scheduler type
     * @param str the string to parse
     * @return The equivalent SchedType
     */
    static PartitionType parsePartitionTypeStr(std::string str);

    /**
     * @brief Get a string representation of the SchedType
     * @param schedType the SchedType to get a string of
     * @return string representation of the SchedType
     */
    static std::string partitionTypeToString(PartitionType partitionType);
};

/*! @} */

#endif //VITIS_PARTITIONPARAMS_H
