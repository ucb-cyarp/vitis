//
// Created by Christopher Yarp on 9/2/21.
//

#ifndef VITIS_CONTEXTPASSES_H
#define VITIS_CONTEXTPASSES_H

#include "GraphCore/ContextFamilyContainer.h"
#include "GraphCore/ContextRoot.h"
#include "GraphCore/Design.h"

/**
 * \addtogroup Passes Design Passes/Transforms
 *
 * @brief A group of compiler passes which operate over a design.
 * @{
 */

/**
 * @brief A collection of passes/transforms focused on contextual execution within the design
 */
namespace ContextPasses {
    /**
     * @brief Discover and mark contexts for nodes in the design (ie. sets the context stack of nodes).
     *
     * Propogates enabled subsystem contexts to its nodes (and recursively to nested enabled subsystems.
     *
     * Finds mux contexts under the top level and under each enabled subsystem
     *     Muxes are searched within the top level and within any subsystem.
     *
     *     Once all the Muxes within the top level (or within the level of the enabled subsystem are discovered)
     *     their contexts are found.  For each mux, we sum up the number of contexts which contain that given mux.
     *     Hierarchy is discovered by tracing decending layers of the hierarchy tree based on the number of contexts
     *     a mux is in.
     *
     * Since contexts stop at enabled subsystems, the process begins again with any enabled subsystem within (after muxes
     * have been handled)
     *
     * Also updates the topLevelContextRoots for discovered context nodes
     *
     */
    void discoverAndMarkContexts(Design &design);

    /**
     * @brief Expands the enabled subsystems in the design to include combinational logic at the inputs and outputs of enabled subsystems
     *
     * This is an *optimization pass* that can be conducted before code emit.
     *
     * Note that expansion occurs hierarchically starting with the highest level and expanding any enabled subsystem within
     */
    void expandEnabledSubsystemContexts(Design &design);

    /**
     * @brief It is possible that a design may want to visualize nodes inside of an enabled subsystem.  These nodes
     * do not typically have EnableOutput ports associated with them.  This function creates the appropriate EnableOutput
     * nodes for these visualizations.
     *
     * @note Only rewires standard arcs (not order constraint in arcs)
     *
     * @warning This function should be called after EnabledSubsystems are expanded but before state update nodes
     * are created as it does not check for the case where the visualization node becomes an ordered predecessor of a
     * state update node
     */
    void createEnabledOutputsForEnabledSubsysVisualization(Design &design);

    /**
     * @brief Encapsulate contexts inside ContextContainers and ContextFamilyContainers for the purpose of scheduling
     * and inter-thread dependency handling for contexts
     *
     * Nodes should exist in the lowest level context they are a member of.
     *
     * Also moves the context roots for the ContextFamilyContainers inside the ContextFamilyContainer
     */
    void encapsulateContexts(Design &design);

    /**
     * @brief Discovers contextRoots in the design and creates ContextVariableUpdate update nodes for them.
     *
     * The state update nodes have a ordering dependency with all of the nodes that are connected via out arcs from the
     * nodes with state elements.  They are also dependent on the primary node being scheduled first (this is typically
     * when the next state update variable is assigned).
     *
     * @param includeContext if true, inserts the new contect varaible update nodes into the subcontext they reside in, otherwise does not (handled in a later step)
     */
    void createContextVariableUpdateNodes(Design &design, bool includeContext = false);

    /**
     * @brief "Rewires" arcs that go between nodes in different contexts to be between contexts themselves (used primarily for scheduling)
     *
     * Goes through the array of the arcs in a design and rewires arcs that are between a nodes in different contexts.
     *
     * If the src node is at or above the dst node in the context hierarchy, then the src is not rewired. (Equiv to: dst node is at or below the src node -> src not rewired)
     * If the dst node is at or above the src node in the context hierarchy, then the dst is not rewired. (Equiv to: src node is at or below the dst node -> dst not rewired)
     *
     * Putting it another way
     * If the src node is either below the dst node node in the hierarchy or outside the dst node's hierarchy stack, then the src is rewired.
     * If the dst node is either below the src node node in the hierarchy or outside the dst node's hierarchy stack, then the dst is rewired.
     *
     * When rewiring the src, it is set to 1 context below the lowest common context of the src and dst on the src side.
     * When rewiring the dst, it is set to 1 context below the lowest common context of the src and dst on the dst side.
     *
     * @note: During encapsulation, which should occure before this is called, the context root driver arcs are
     * added as order constraint arcs for each ContextFamilyContainer created for a given ContextRoot.
     * For scheduling purposes, these order constraint arcs are what should be considered when scheduling rather
     * than the original context driver.  So, the origional context driver is disconnected by this function.
     *
     * @note This function does not actually rewire existing arcs but creates a new arcs representing how the given
     * arcs should be rewired.  The two returned vectors have a 1-many relationship between an arc to be rewired and the arcs
     * representing the result of that rewiring.  To rewire, simply disconnect the arcs from the origArcs vector.  The
     * arcs returned in contextArcs are already added to the design.
     *
     * Alternatively, the origArcs can be kept and used in the scheduler.  This may be helpful when deciding on whether
     * or not a context should be scheduled in the next iteration of the scheduler
     *
     */
    void rewireArcsToContexts(Design &design, std::vector<std::shared_ptr<Arc>> &origArcs, std::vector<std::vector<std::shared_ptr<Arc>>> &contextArcs);

    /**
     * @brief Gets the ContextFamilyContainer for the provided context root if one exists (is in the ContextRoot's map of ContextFamilyContainers).
     *
     * If the ContextFamilyContainer does not exist for the given partition, one is created and is added to the ContextRoot's map of ContextFamilyContainers.
     * The existing ContextFamilyContainers for the given ContextRoot have their sibling maps updated.  The new ContextFamilyContainer has its sibling map set
     * to include all other ContextFamilyContainers belonging to the given ContextRoot
     *
     * When the ContextFamilyContainer is created, an order constraint arc from the context driver is created.  This ensures that the context driver
     * will be available in each partition that the context resides in (so long as FIFO insertion occurs after this point).
     *
     * @note In cases where a ContexRoot has multiple driver arcs that all come from the same source port (ex. EnabledSubsystems
     * where each EnableInput and EnableOutput has a driver arc), only a single order constraint port is created.
     *
     * @warning The created ContextFamilyContainer does not have its parent set.  This needs to be performed outside of this helper function
     *
     * @param contextRoot
     * @param partition
     * @return
     */
    std::shared_ptr<ContextFamilyContainer> getContextFamilyContainerCreateIfNotNoParent(Design &design, std::shared_ptr<ContextRoot> contextRoot, int partition);

    /**
     * @brief Replicates context root drivers if requested by the ContextRoot
     *
     * New driver arcs will be created as OrderConstraint arcs to the ContextRoot.
     *
     * The driver will have a duplicate copy created for the pariton it is in.  However,
     * a new OrderConstraint arc will be created for that same partition.
     *
     * @warning: This currently only support replicating a single node with no inputs
     * and a single output arc to the context root
     *
     * @warning: This should be done after context discovery but before encapsulation
     */
    void replicateContextRootDriversIfRequested(Design &design);

    /**
     * @brief Places EnableNodes that are not in any partition into a partition.
     * It will attempt to place EnableInput and EnableOutput nodes to be in the same partitions as the nodes they are
     * connected to within the enabled subsystem.  If EnableInputs go to multiple partitions, replicas will be created
     * for each destination partition
     *
     * This will potentially help avoid issues where partition cycles could
     * occur if the EnableInput and EnableOutput nodes took the partitions of the nodes outside of the enabled subsystem.
     * A case where this can occur is where partition A is used to compute the enable signal in context B and another
     * value from context A is passed to the EnabledSubsystem C.  In this case, there would be a cyclic dependency between
     * Partition A->B->A since the input port of the EnabledSubsystem would be placed in partition A.
     *
     * It is possible to be more intelligent solution with placement by checking for cycles when assigning EnableNodes to
     * partitions.
     * TODO: Implement a more intelligent scheme.
     *
     * The EnabledSubsystem node itself is checked to see if it is in a partition.  If not, it is placed in the same partition
     * as its first enable input.  If no enable inputs exist, it is placed in the partition of the first enable output.
     * Even though the EnabledSubsystem node itself is not used post encapsulation, it is not deleted because that node
     * contains the logic for generating the various context checks.  Placing it in a partition avoids a ContextContainer
     * being created for partition -1 (unassigned)
     */
    void placeEnableNodesInPartitions(Design &design);
};

/*! @} */

#endif //VITIS_CONTEXTPASSES_H
