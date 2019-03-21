//
// Created by Christopher Yarp on 2019-03-15.
//

#ifndef VITIS_ERRORHELPERS_H
#define VITIS_ERRORHELPERS_H

#include "GraphCore/Node.h"
#include <memory>

#define VITIS_STD_ERROR_PREAMBLE "vitis"

/**
 * @brief Helpers for handling and reporting errors
 */
class ErrorHelpers {
public:

    //Full version
    static std::string genErrorStr(std::string errorText, std::string errorPreamble, std::string errorContext);

    //Version with default preamble
    static std::string genErrorStr(std::string errorText, std::string errorContext);

    //Version with default preamble and no context
    static std::string genErrorStr(std::string errorText);

    //Version from node
    static std::string genErrorStr(std::string errorText, std::shared_ptr<Node> node, std::string errorPreamble);

    //Version from node
    static std::string genErrorStr(std::string errorText, std::shared_ptr<Node> node);

};


#endif //VITIS_ERRORHELPERS_H
