//
// Created by Christopher Yarp on 7/12/18.
//

#ifndef VITIS_GRAPHMLDIALECT_H
#define VITIS_GRAPHMLDIALECT_H

/**
 * \addtogroup GraphMLTools GraphML Import/Export Tools
 * @{
*/

/**
 * @brief Describes the different dialects of GraphML Documment that can be handled.
 */
enum class GraphMLDialect{
    VITIS, ///<The standard dialect for this program.  Can Read & Write This Dialect
    SIMULINK_EXPORT ///<The dialect from the Simulink Export Scripts which contains many Simulink specific parameters.  Can Import This Dialect.
};

/*! @} */

#endif //VITIS_GRAPHMLDIALECT_H
