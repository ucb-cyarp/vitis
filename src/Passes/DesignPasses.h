//
// Created by Christopher Yarp on 9/2/21.
//

#ifndef VITIS_DESIGNPASSES_H
#define VITIS_DESIGNPASSES_H

#include <string>

#include "GraphCore/Design.h"

/**
 * \addtogroup Passes Design Passes/Transforms
 *
 * @brief A group of compiler passes which operate over a design.
 * @{
 */

/**
 * @brief A collection of more general passes/transforms over a design
 */
namespace DesignPasses {
    /**
     * @brief Discovers nodes with state in the design and creates state update nodes for them.
     *
     * The state update nodes have a ordering dependency with all of the nodes that are connected via out arcs from the
     * nodes with state elements.  They are also dependent on the primary node being scheduled first (this is typically
     * when the next state update variable is assigned).
     *
     * @note This function updates the context of the the state update to mirror the context of the primary node.
     *
     * @note It appears that inserting these nodes before context discovery could result in some nodes being erroniously
     * left out of discoverd contexts.  For example, in mux context, nodes that are directly dependent on state will have
     * an order constraint arc to the StateUpdate node for the particular state element.  This StateUpdate is not reachable
     * from the mux and will therefore cause the node to not be included in the mux context even if it should be.  It
     * is therefore reccomended to insert the state update nodes after context discovery but before scheduling
     *
     * @param includeContext if true, the state update node is included in the same context (and partition) as the root node
     */
    void createStateUpdateNodes(Design &design, bool includeContext);

    /**
     * @brief OrderConstraint Nodes with Zero Inputs in Contexts
     *
     * This is to prevent nodes with 0 inputs in a context from being scheduled before the context drivers are computed
     *
     * @note Apply this after partitioning, replicating context drivers, and the creation of partition specific context drivers
     *
     * @note This is only needed if ContextFamilyNodes are not scheduled as a single unit.
     *
     */
    void orderConstrainZeroInputNodes(Design &design); //TODO: Use if scheduler changed to no longer schedule contexts as a single unit

    /**
     * @brief This searches the graph for subsystems which have no children (either because they were empty or all of their children had been moved) and can be removed.  Upon removal, it parent subsystem is also checked
     *
     * Note, subsystems that are context roots are not removed
     *
     * @return
     */
    void cleanupEmptyHierarchy(Design &design, std::string reason);

    /**
     * @brief Disconnect arcs to the unconnected, terminated, or vis masters
     * @param removeVisArcs if true, removes arcs connected to the vis masters.  If false, leaves these arcs alone
     */
    void pruneUnconnectedArcs(Design &design, bool removeVisArcs);

    /**
     * @brief Prunes the design
     *
     * Removes unused nodes from the graph.
     *
     * Tracks nodes with 0 out degree (when connections to the Unconnected and Terminated masters are not counted)
     *
     * The unused and terminated masters are not removed during pruning.  However, nodes connected to them may be removed.
     *
     * Ports that are left unused are connected to the Unconnected master node
     *
     * @param includeVisMaster If true, will add the Vis master to the set of nodes that are not considered when calculating output degree.
     *
     * @return number of nodes removed from the graph
     */
    unsigned long prune(Design &design, bool includeVisMaster = true);

};

/*! @} */

#endif //VITIS_DESIGNPASSES_H
