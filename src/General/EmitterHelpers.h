//
// Created by Christopher Yarp on 10/4/19.
//

#ifndef VITIS_EMITTERHELPERS_H
#define VITIS_EMITTERHELPERS_H

#include <vector>
#include "GraphCore/SchedParams.h"

//Forward declare
class Node;
class ContextRoot;
class BlackBox;
class MasterOutput;

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

};

/*! @} */


#endif //VITIS_EMITTERHELPERS_H
