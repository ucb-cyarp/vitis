//
// Created by Christopher Yarp on 5/11/20.
//

#ifndef VITIS_FILEIOHELPERS_H
#define VITIS_FILEIOHELPERS_H

#include <string>

/**
 * \addtogroup General General Helper Classes
 *
 * @brief A set of general helper classes.
 * @{
*/

/**
 * @brief A class for helping with File IO Tasks
 */
namespace FileIOHelpers {
    /**
     * @brief Creates a directory with the specified path if it did not already exist
     *
     * Throws an error if a file that is not a directory exists with the given path
     *
     * @param path the path of the new directory
     * @param reportCreating if true, a status message is printed if a new directory is created
     * @return true if a new directory was created, false otherwise
     */
    bool createDirectoryIfDoesNotExist(std::string path, bool reportCreating);

    void deleteDirectoryRecursive(std::string path, bool reportDeleting);
};

/*! @} */

#endif //VITIS_FILEIOHELPERS_H
