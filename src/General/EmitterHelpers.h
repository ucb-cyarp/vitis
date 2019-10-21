//
// Created by Christopher Yarp on 10/4/19.
//

#ifndef VITIS_EMITTERHELPERS_H
#define VITIS_EMITTERHELPERS_H

#include <vector>
#include <memory>
#include "GraphCore/SchedParams.h"
#include "GraphCore/Variable.h"

#define VITIS_TYPE_NAME "vitisTypes"

//Forward declare
class Node;
class ContextRoot;
class BlackBox;
class MasterOutput;
class MasterInput;

/**
 * \addtogroup General General Helper Classes
 *
 * @brief A set of general helper classes.
 * @{
*/

/**
 * @brief Contains helper methods for Emitters
 */
class EmitterHelpers {
public:
    static std::vector<std::shared_ptr<Node>> findNodesWithState(std::vector<std::shared_ptr<Node>> &nodesToSearch);

    static std::vector<std::shared_ptr<Node>> findNodesWithGlobalDecl(std::vector<std::shared_ptr<Node>> &nodesToSearch);

    static std::vector<std::shared_ptr<ContextRoot>> findContextRoots(std::vector<std::shared_ptr<Node>> &nodesToSearch);

    static std::vector<std::shared_ptr<BlackBox>> findBlackBoxes(std::vector<std::shared_ptr<Node>> &nodesToSearch);

    /**
     * @brief Emits operators for the given nodes (in the order given).  Does not emit function prototypes.  This emitter is context aware
     * @param cFile the cFile to emit to
     * @param schedType the scheduler being used.  Is passed to downstream context emit functions
     * @param orderedNodes the nodes to emit, given in the order they should be emitted
     * @param outputMaster a pointer to the output master of the design being emitted
     * @param blockSize the size of the block (in samples) that are processed in each call to the function
     * @param indVarName the variable that specifies the index in the block that is being computed
     * @param checkForPartitionChange if true, checks if the partition changes while emitting and throws an error if it does
     */
    static void emitOpsStateUpdateContext(std::ofstream &cFile, SchedParams::SchedType schedType, std::vector<std::shared_ptr<Node>> orderedNodes, std::shared_ptr<MasterOutput> outputMaster, int blockSize = 1, std::string indVarName = "", bool checkForPartitionChange = true);

    /**
     * @brief Get the input variables for this design
     *
     * The input names take the form: portName_portNum
     *
     * @warning Assumes the design has already been validated (ie. has at least one arc per port).
     *
     * @return a vector of input variables ordered by the input port number.  The ith element of the array is the ith input port.
     */
    static std::vector<Variable> getCInputVariables(std::shared_ptr<MasterInput> inputMaster);

    /**
     * @brief Get the output variables for this design
     *
     * The input names take the form: portName_portNum
     *
     * @warning Assumes the design has already been validated (ie. has at exactly one arc per port).
     *
     * @return a vector of output variables ordered by the output port number.  The ith element of the array is the ith output port.
     */
    static std::vector<Variable> getCOutputVariables(std::shared_ptr<MasterOutput> outputMaster);

    /**
     * @brief Get the structure definition for the Input/Output ports
     *
     * The struture definition takes the form of
     *
     * struct {
     *     type1 var1;
     *     type2 var2;
     *     ...
     * } structTypename;
     *
     * This is used by the driver generator.
     *
     * @param portVariables the port variables for the design (either input or output) (in accending port order)
     * @param blockSize the block size (in samples).  The width is multiplied by this number
     * @param the type name of the struct being created
     *
     * @return
     */
    static std::string getCIOPortStructDefn(std::vector<Variable> portVariables, std::string structTypeName, int blockSize = 1);

    /**
     * @brief Outputs the type header
     * @param path
     * @return the name of the type header
     */
    static std::string stringEmitTypeHeader(std::string path);

    /**
     * @brief Emits a parameters file
     * @param path
     * @param filenamePrefix
     * @param blockSize
     * @returns the filename of the header file
     */
    static std::string emitParametersHeader(std::string path, std::string fileNamePrefix, int blockSize);
};

/*! @} */


#endif //VITIS_EMITTERHELPERS_H
