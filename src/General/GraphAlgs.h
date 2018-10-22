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
#include "GraphCore/SubSystem.h"
#include "GeneralHelper.h"

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
};


#endif //VITIS_GRAPHALGS_H
