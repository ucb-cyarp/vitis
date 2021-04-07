//
// Created by Christopher Yarp on 2019-03-15.
//

#ifndef VITIS_ERRORHELPERS_H
#define VITIS_ERRORHELPERS_H

#include "GraphCore/Node.h"
#include <memory>

#define VITIS_STD_ERROR_PREAMBLE "vitis - Error"
#define VITIS_STD_WARNING_PREAMBLE "vitis - Warning"
#define VITIS_STD_INFO_PREAMBLE "vitis - Info"

/**
 * \addtogroup General General Helper Classes
 *
 * @brief A set of general helper classes.
 * @{
*/

/**
 * @brief Helpers for handling and reporting errors
 */
namespace ErrorHelpers {
    //Full version
    std::string genErrorStr(std::string errorText, std::string errorPreamble, std::string errorContext);

    //Version with default preamble
    std::string genErrorStr(std::string errorText, std::string errorContext);

    //Version with default preamble and no context
    std::string genErrorStr(std::string errorText);

    //Version from node
    std::string genErrorStr(std::string errorText, std::shared_ptr<Node> node, std::string errorPreamble);

    //Version from node
    std::string genErrorStr(std::string errorText, std::shared_ptr<Node> node);

    //Version with default preamble
    std::string genWarningStr(std::string errorText, std::string errorContext);

    //Version with default preamble and no context
    std::string genWarningStr(std::string errorText);

    //Version from node
    std::string genWarningStr(std::string errorText, std::shared_ptr<Node> node);

};

/*! @} */

#endif //VITIS_ERRORHELPERS_H
