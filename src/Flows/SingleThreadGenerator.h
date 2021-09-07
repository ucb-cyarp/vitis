//
// Created by Christopher Yarp on 9/3/21.
//

#ifndef VITIS_SINGLETHREADGENERATOR_H
#define VITIS_SINGLETHREADGENERATOR_H

#include <string>
#include "GraphCore/Design.h"
#include "GraphCore/SchedParams.h"
#include "General/TopologicalSortParameters.h"

/**
 * \addtogroup Flows Compiler Flows
 * @brief Compiler flows for translating a design to the desired output
 * @{
 */

/**
 * @brief Flow for generating a single-threaded C implementation of a given design
 */
namespace SingleThreadGenerator {
    /**
     * @brief Generates a Single Threaded Version of the Design as a C program
     *
     * Depending on the scheduler specified, this function will prune, schedule, and emit
     *
     * @param outputDir the output directory for the C files
     * @param designName the design name to be used as the C file names
     * @param schedType the type of scheduler to use when generating the C program
     * @param topoSortParams the parameters used by the topological scheduler if applicable (ex. what heuristic to use, random seed)
     * @param blockSize the size of the blocks (in samples) processed in each call to the generated C function
     * @param emitGraphMLSched if true, emit a GraphML file with the computed schedule as a node parameter
     * @param printNodeSched if true, print the node schedule to the console
     */
    void generateSingleThreadedC(Design &design, std::string outputDir, std::string designName, SchedParams::SchedType schedType, TopologicalSortParameters topoSortParams, unsigned long blockSize, bool emitGraphMLSched, bool printNodeSched);

    /**
     * @brief Emits operators using the bottom up emitter
     * @param cFile the cFile to emit tp
     * @param nodesWithState nodes with state in the design
     * @param schedType the specific scheduler used
     */
    void emitSingleThreadedOpsBottomUp(Design &design, std::ofstream &cFile, std::vector<std::shared_ptr<Node>> &nodesWithState, SchedParams::SchedType schedType);

    /**
     * @brief Emits operators using the schedule emitter
     * @param cFile the cFile to emit to
     * @param nodesWithState nodes with state in the design
     */
    void emitSingleThreadedOpsSched(Design &design, std::ofstream &cFile, SchedParams::SchedType schedType);

    /**
     * @brief Emits operators using the schedule emitter.  This emitter is context aware and supports emitting scheduled state updates
     * @param cFile the cFile to emit to
     * @param blockSize the size of the block (in samples) that are processed in each call to the function
     * @param indVarName the variable that specifies the index in the block that is being computed
     */
    void emitSingleThreadedOpsSchedStateUpdateContext(Design &design, std::ofstream &cFile, SchedParams::SchedType schedType, int blockSize = 1, std::string indVarName = "");

    /**
     * @brief Emits the design as a single threaded C function
     *
     * @note Design expansion and validation should be run before calling this function
     *
     * @note To avoid dead code being emitted, prune the design before calling this function
     *
     * @param path path to where the output files will be generated
     * @param fileName name of the output files (.h and a .c file will be created)
     * @param designName The name of the design (used as the function name)
     * @param schedType Schedule type
     * @param blockSize the size of the block (in samples) that is processed in each call to the emitted C function
     */
    void emitSingleThreadedC(Design &design, std::string path, std::string fileName, std::string designName, SchedParams::SchedType schedType, unsigned long blockSize);

};

/*! @} */

#endif //VITIS_SINGLETHREADGENERATOR_H
