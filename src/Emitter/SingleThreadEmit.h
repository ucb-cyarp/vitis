//
// Created by Christopher Yarp on 9/2/21.
//

#ifndef VITIS_SINGLETHREADEMIT_H
#define VITIS_SINGLETHREADEMIT_H

#include <string>
#include "GraphCore/Design.h"

/**
 * \addtogroup Emitter Emitter Helpers and Functions
 * @brief Helpers and functions for emitting a design
 * @{
 */

/**
 * @brief Contains helper methods for Single-Thread Target Emitters
 */
namespace SingleThreadEmit {
    /**
     * @brief Emits the benchmarking drivers for the design
     *
     * @note Design expansion and validation should be run before calling this function
     *
     * @param path path to where the output files will be generated
     * @param fileName base name of the output files (.h and a .c file will be created)
     * @param designName The name of the design (used as the function name)
     * @param blockSize the size of the block (in samples) that is processed in each call to the generated function
     */
    void emitSingleThreadedCBenchmarkingDrivers(Design &design, std::string path, std::string fileName, std::string designName, int blockSize);

    /**
     * @brief Emits the benchmarking drivers (constant arguments) for the design
     *
     * @note Design expansion and validation should be run before calling this function
     *
     * @param path path to where the output files will be generated
     * @param fileName base name of the output files (.h and a .c file will be created)
     * @param designName The name of the design (used as the function name)
     * @param blockSize the size of the block (in samples) that is processed in each call to the generated function
     */
    void emitSingleThreadedCBenchmarkingDriverConst(Design &design, std::string path, std::string fileName, std::string designName, int blockSize);

    /**
     * @brief Emits the benchmarking drivers (memory arguments) for the design
     *
     * @note Design expansion and validation should be run before calling this function
     *
     * @param path path to where the output files will be generated
     * @param fileName base name of the output files (.h and a .c file will be created)
     * @param designName The name of the design (used as the function name)
     * @param blockSize the size of the block (in samples) that is processed in each call to the generated function
     */
    void emitSingleThreadedCBenchmarkingDriverMem(Design &design, std::string path, std::string fileName, std::string designName, int blockSize);
};

/*! @} */

#endif //VITIS_SINGLETHREADEMIT_H
