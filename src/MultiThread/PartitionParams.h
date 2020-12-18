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

    enum class FIFOIndexCachingBehavior{
        NONE, ///<the FIFO indexes are fetched for each block
        PRODUCER_CONSUMER_CACHE, ///<The producer and consumer do not fetch indexes unless it cannot be determined if the FIFO is available based on prior information
        PRODUCER_CACHE, ///<The producer does not fetch indexes unless it cannot be determined if the FIFO is available based on prior information, the consumer fetches the index information for each block
        CONSUMER_CACHE ///The consumer does not fetch indexes unless it cannot be determined if the FIFO is available based on prior information, the producer fetches the index information for each block
    };

    static FIFOIndexCachingBehavior parseFIFOIndexCachingBehavior(std::string str);
    static std::string fifoIndexCachingBehaviorToString(FIFOIndexCachingBehavior fifoIndexCachingBehavior);
};

/*! @} */

#endif //VITIS_PARTITIONPARAMS_H
