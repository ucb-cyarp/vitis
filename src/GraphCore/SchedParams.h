//
// Created by Christopher Yarp on 2018-12-19.
//

#ifndef VITIS_SCHEDPARAMS_H
#define VITIS_SCHEDPARAMS_H

#include <string>

/**
* \addtogroup GraphCore Graph Core
*
* @brief Core Classes for Representing a Data Flow Graph
* @{
*/

/**
 * @brief Class for representing scheduling parameters.
 */
class SchedParams {
public:
    /**
    * @brief Represents the types of schedulers supported
    */
    enum class SchedType{
        BOTTOM_UP, ///<The original bottom up emit.  Emits from outputs backwards
        TOPOLOGICAL, ///<A generic topological sort based scheduler
        TOPOLOGICAL_CONTEXT ///<A generic topological sort based scheduler with context discovery
    };

    /**
     * @brief Parse a scheduler type
     * @param str the string to parse
     * @return The equivalent SchedType
     */
    static SchedType parseSchedTypeStr(std::string str);

    /**
     * @brief Get a string representation of the SchedType
     * @param schedType the SchedType to get a string of
     * @return string representation of the SchedType
     */
    static std::string schedTypeToString(SchedType schedType);

    /**
     * @brief Determine if the given scheduling scheme is context aware
     * @param schedType the schedule type to check
     * @return true if the schedType is context aware, false if otherwise
     */
    static bool isContextAware(SchedType schedType);
};

/*! @} */

#endif //VITIS_SCHEDPARAMS_H
