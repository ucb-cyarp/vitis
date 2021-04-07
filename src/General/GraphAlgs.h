//
// Created by Christopher Yarp on 10/16/18.
//

#ifndef VITIS_GRAPHALGS_H
#define VITIS_GRAPHALGS_H

#include <memory>
#include <set>
#include <map>
#include <string>
#include "GraphCore/InputPort.h"
#include "GraphCore/Node.h"
//#include "GraphCore/SubSystem.h"
#include "GeneralHelper.h"
#include "MasterNodes/MasterOutput.h"
#include "MasterNodes/MasterInput.h"
#include "MasterNodes/MasterUnconnected.h"
#include "TopologicalSortParameters.h"

#define VITIS_CONTEXT_ABSORB_BARRIER_NAME "VITIS_CONTEXT_ABSORB_BARRIER"

//Forward Declare
class EnabledSubSystem;
class EnabledNode;

/**
 * \addtogroup General General Helper Classes
 *
 * @brief A set of general helper classes.
 * @{
*/

/**
 * @brief Helper class to contain graph algorithms
 */
namespace GraphAlgs {
    /**
     * @brief Traces back from a port through the design graph to discover a port's context, stopping at state elements and enabled subsystem boundaries.
     *
     * State elements and enabled nodes (enabled inputs/outputs) are not marked - none of their output arcs are marked by a
     * call to this function.  This is because these nodes cannot be considered part of the contexts that these traces are being used to discover
     *
     * Note that the context will not be properly discovered if there is a combinational loop.  A combinational loop will cause the input of a node in the loopback
     * to never be marked, thus preventing recursion into the loop.
     *
     * @note This function can be re-used for tracing back the inputs of an enabled subsystem.  To do that, call this function on each
     * input port but cary the marks map between calls and combine the returned sets.  This is equivalent to each input port being
     * connected to a single dummy node which is directly connected to our port of interest.
     *
     * @param traceFrom The port to trace from
     * @param marks A map containing the marks for each arc.  A arc is considered marked if the value returned from the map is true.  This will be modified during the call
     * @param stopAtPartitionBoundary if true, context discovery stops at nodes in another partition.  This is useful to avoid communication cycles created during context discovery
     * @param checkForContextBarriers if true, context discovery stops at special nodes whose names indicate that they are context expansion barriers
     * @return The set of nodes in the port's context
     */
    std::set<std::shared_ptr<Node>> scopedTraceBackAndMark(std::shared_ptr<InputPort> traceFrom, std::map<std::shared_ptr<Arc>, bool> &marks, bool stopAtPartitionBoundary, bool checkForContextBarriers);

    /**
     * @brief Traces forward from a port through the design graph to discover a port's context, stopping at state elements and enabled subsystem boundaries.
     *
     * State elements and enabled nodes (enabled inputs/outputs) are not marked - none of their output arcs are marked by a
     * call to this function.  This is because these nodes cannot be considered part of the contexts that these traces are being used to discover
     *
     * Note that the context will not be properly discovered if there is a combinational loop.  A combinational loop will cause the input of a node in the loopback
     * to never be marked, thus preventing recursion into the loop.
     *
     * @note This function can be re-used for tracing forward the output of an enabled subsystem.  To do that, call this function on each
     * output port but cary the marks map between calls and combine the returned sets.  This is equivalent to each input port being
     * connected to a single dummy node which is directly connected to our port of interest.
     *
     * @param traceFrom The port to trace from
     * @param marks A map containing the marks for each arc.  A arc is considered marked if the value returned from the map is true.  This will be modified during the call
     * @param stopAtPartitionBoundary if true, context discovery stops at nodes in another partition.  This is useful to avoid communication cycles created during context discovery
     * @param checkForContextBarriers if true, context discovery stops at special nodes whose names indicate that they are context expansion barriers
     * @return The set of nodes in the port's context
     */
    std::set<std::shared_ptr<Node>> scopedTraceForwardAndMark(std::shared_ptr<OutputPort> traceFrom, std::map<std::shared_ptr<Arc>, bool> &marks, bool stopAtPartitionBoundary, bool checkForContextBarriers);

    /**
     * @brief Moves a node from its current position in the hierarchy to a position under another subsystem.
     *
     * This function attempts to preserve the hierarchy by tracing up from the node's position in the hierarchy to
     * the parent of the subsystem it is being moved under.  In this case, the same series of subsystems are created
     * under the new subsystem with the given suffix.
     *
     * If the node is currently within the moveUnder node's hierarchy, it is simply moved under the moveUnder node.
     *
     * If the root of the hierarchy is reached and the moveUnder node is not one level below the root, the node is simply
     * moved under the moveUnder subsystem.
     *
     * @param nodeToMove the node to move
     * @param moveUnder the subsystem to move the node under
     * @param newNodes a vector of new nodes created during the call to this function
     * @param moveSuffix this suffix is added to new subsystems created during the move process
     * @param overridePartition if not -1, overrides the partition of any created subsystem with the given partition
     */
    void moveNodePreserveHierarchy(std::shared_ptr<Node> nodeToMove, std::shared_ptr<SubSystem> moveUnder, std::vector<std::shared_ptr<Node>> &newNodes, std::string moveSuffix = "_moved", int overridePartition = -1);

    /**
     * @brief Discovers all the nodes at this level in the context hierarchy (including at enabled subsystems, muxes, and ClockDomains).
     *
     * Also finds muxes, enabled subsystems, and clock domains within subsystems (and below).
     *
     * @note Does not recurse into enabled subsystems or ClockDomains
     *
     * @note ClockDomains are not ContextRoots until they are specialized (ie. are Upsample or Downsample ClockDomains)
     * This function will check this before adding it to the list.  Discovering and updating contexts should occur
     * after the specialization of clock domains
     *
     * @param nodesToSearch a set of nodes within this level of the design
     * @param contextStack The context stack at this point.  Nodes at this level will be updated with this context stack
     * @param discoveredMux a vector modified to include discovered muxes
     * @param discoveredEnabledSubSystems a vector modified to include discovered enabled subsystems
     * @param discoveredClockDomains a vector modified to include discovered ClockDomains
     * @param discoveredGeneral a vector modified to include discovered general nodes
     */
    void discoverAndUpdateContexts(std::vector<std::shared_ptr<Node>> nodesToSearch,
                                          std::vector<Context> contextStack,
                                          std::vector<std::shared_ptr<Mux>> &discoveredMux,
                                          std::vector<std::shared_ptr<EnabledSubSystem>> &discoveredEnabledSubSystems,
                                          std::vector<std::shared_ptr<ClockDomain>> &discoveredClockDomains,
                                          std::vector<std::shared_ptr<Node>> &discoveredGeneral);

    /**
     * @brief Performs a destructive topological sort on a provided vector of nodes.
     *
     * This function will recursivly scedule ContextFamilyContainers if encountered.  If the schedule should be context
     * aware, the contexts must first be properly discovered and encapsulated.  The ContextFamilyContainers should also
     * be rewired before calling this function.
     *
     * If the schedule should not be context aware, ContextFamily containers should not be included in the design and
     * the list of nodes to be scheduled is the complete list of node in the design.
     *
     * @warning This destroys the graph by removing arcs from the nodes.
     * It is recomended to run on a copy of the graph and to back propagate the results
     *
     * @param parameters the parameters to use when scheduling (ex. heuristic type and random seed)
     * @param nodesToSort a vector of nodes to schedule, including ContextFamilyContainers to be scheduled together if the schedule is context aware.  Should not contain nodes in lower levels of the context hierarchy.  These will be handled through recursion on ContextFamilyContainers
     * @param arcsToDelete a vector of arcs to delete from the design,
     * @param limitRecursionToPartition if true, when scheduling ContextFamilyContainers, limit to the given partition
     * @param partition partition to limit to if limitRecursionToPartition is true
     * @return A vector of nodes arranged in topological order
     */
    std::vector<std::shared_ptr<Node>> topologicalSortDestructive(TopologicalSortParameters parameters,
                                                                         std::vector<std::shared_ptr<Node>> nodesToSort,
                                                                         bool limitRecursionToPartition,
                                                                         int partition,
                                                                         std::vector<std::shared_ptr<Arc>> &arcsToDelete,
                                                                         std::shared_ptr<MasterOutput> outputMaster,
                                                                         std::shared_ptr<MasterInput> inputMaster,
                                                                         std::shared_ptr<MasterOutput> terminatorMaster,
                                                                         std::shared_ptr<MasterUnconnected> unconnectedMaster,
                                                                         std::shared_ptr<MasterOutput> visMaster
                                                                         );

    /**
     * @brief Recursively finds nodes in a hierarchy starting from a list of provided nodes.  Subsystems are searched.  ContexFamilyNodes & EnabledSubsystems are included but are not recursed into
     * @param nodesToSearch a list of nodes to search
     * @return a list of found nodes
     */
    std::vector<std::shared_ptr<Node>> findNodesStopAtContextFamilyContainers(std::vector<std::shared_ptr<Node>> nodesToSearch);

    /**
     * @brief Recursively finds nodes in a hierarchy starting from a list of provided nodes.  Subsystems, including EnabledSubystems are searched.  ContexFamilyNodes are included but are not recursed into
     *
     * @warning This method expects ContextFamilyContainers to have already been created as enabled subsystems are treated like subsystems
     *
     * This version limits the nodes returned to ones in a specified partition
     *
     * Note that ContextFamilyContainers for a particular partition cannot include other partitions.  This is because multiple ContextFamilyContainers
     * can exist for a given context root
     *
     * @param nodesToSearch a list of nodes to search
     * @return a list of found nodes
     */
    std::vector<std::shared_ptr<Node>> findNodesStopAtContextFamilyContainers(std::vector<std::shared_ptr<Node>> nodesToSearch, int partitionNum);

    /**
     * @brief Creates a the StateUpdate node for the given node (in the style of Delay)
     *
     *
     *
     * Creates the state update for the node.
     *
     * The StateUpdate Node is dependent on:
     *   - Next state calculated (calculated when Delay node scheduled) -> order constraint on delay node
     *      - The next state is stored in a temporary.  This would be redundant (the preceding operation should produce
     *        a single assignmnent to a temporary) except for the case when there is another state element.  In that case
     *        a temporary variable is not
     *   - Each node dependent on the output of the delay (order constraint only)
     *
     *   The partition of the StateUpdate node is set to be the same as the stateful node
     *
     */
    bool createStateUpdateNodeDelayStyle(std::shared_ptr<Node> statefulNode,
                                                std::vector<std::shared_ptr<Node>> &new_nodes,
                                                std::vector<std::shared_ptr<Node>> &deleted_nodes,
                                                std::vector<std::shared_ptr<Arc>> &new_arcs,
                                                std::vector<std::shared_ptr<Arc>> &deleted_arcs,
                                                bool includeContext);

    std::shared_ptr<SubSystem> findMostSpecificCommonAncestor(std::shared_ptr<Node> a, std::shared_ptr<Node> b);

    /**
     * @brief Similar to findMostSpecificCommonAncestor except that parent is considered as a subsystem and is allowd to be
     * the common parent.  Helps when finding the common parent of several different nodes
     * @param parent
     * @param b
     * @return
     */
    std::shared_ptr<SubSystem> findMostSpecificCommonAncestorParent(std::shared_ptr<SubSystem> parent, std::shared_ptr<Node> b);

};

/*! @} */

#endif //VITIS_GRAPHALGS_H
