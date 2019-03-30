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

//Forward Declare
class EnabledSubSystem;
class EnabledNode;

class GraphAlgs {
public:

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
     * @param marks A map containing the marks for each arc.  A arc is considered marked if the value returned from the map is true.  This will be modified durring the call
     * @return The set of nodes in the port's context
     */
    static std::set<std::shared_ptr<Node>> scopedTraceBackAndMark(std::shared_ptr<InputPort> traceFrom, std::map<std::shared_ptr<Arc>, bool> &marks);

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
     * @return The set of nodes in the port's context
     */
    static std::set<std::shared_ptr<Node>> scopedTraceForwardAndMark(std::shared_ptr<OutputPort> traceFrom, std::map<std::shared_ptr<Arc>, bool> &marks);

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
     */
    static void moveNodePreserveHierarchy(std::shared_ptr<Node> nodeToMove, std::shared_ptr<SubSystem> moveUnder, std::vector<std::shared_ptr<Node>> &newNodes, std::string moveSuffix = "_moved");

    /**
     * @brief Discovers all the nodes at this level in the context hierarchy (including at enabled subsystems and muxes).
     *
     * Also finds muxes and enabled enabled within subsystems (and below).
     *
     * @note Does not recurse into enabled subsystems
     *
     * @param nodesToSearch a set of nodes within this level of the design
     * @param contextStack The context stack at this point.  Nodes at this level will be updated with this context stack
     * @param discoveredMux a vector modified to include discovered muxes
     * @param discoveredEnabledSubSystems a vector modified to include discovered enabled subsystems
     * @param discoveredGeneral a vector modified to include discovered general nodes
     */
    static void discoverAndUpdateContexts(std::set<std::shared_ptr<Node>> nodesToSearch,
                                          std::vector<Context> contextStack,
                                          std::vector<std::shared_ptr<Mux>> &discoveredMux,
                                          std::vector<std::shared_ptr<EnabledSubSystem>> &discoveredEnabledSubSystems,
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
     * @return A vector of nodes arranged in topological order
     */
    static std::vector<std::shared_ptr<Node>> topologicalSortDestructive(TopologicalSortParameters parameters,
                                                                         std::vector<std::shared_ptr<Node>> nodesToSort,
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
    static std::vector<std::shared_ptr<Node>> findNodesStopAtContextFamilyContainers(std::vector<std::shared_ptr<Node>> nodesToSearch);

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
     */
    static bool createStateUpdateNodeDelayStyle(std::shared_ptr<Node> statefulNode,
                                                std::vector<std::shared_ptr<Node>> &new_nodes,
                                                std::vector<std::shared_ptr<Node>> &deleted_nodes,
                                                std::vector<std::shared_ptr<Arc>> &new_arcs,
                                                std::vector<std::shared_ptr<Arc>> &deleted_arcs);
};


#endif //VITIS_GRAPHALGS_H
