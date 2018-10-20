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
     * @param moveSuffix this suffix is added to new subsystems created durring the move process
     */
    static void moveNodePreserveHierarchy(std::shared_ptr<Node> nodeToMove, std::shared_ptr<SubSystem> moveUnder, std::string moveSuffix = "_moved");

//    /*
//     * @brief Traces back from a port through the design graph, stopping at state elements and enabled subsystem boundaries. Arcs that are traversed are marked with the specified mark value.
//     *
//     * State elements and enabled nodes (enabled inputs/outputs) are not marked - none of their output arcs are marked by a call to this function.  This is because these nodes cannot be considered
//     * part of the contexts that these traces are being used to discover
//     *
//     * If an arc is encountered which has already been traversed by a call to this function, it will stop as this constitutes a combinational loop (* with an exception, see below)
//     *     Identification on whether an arc has been traversed previously is accomplished by checking the markSet for that arc.  If the mark value is found in the set, traversal stops.
//     *
//     *     Note that it is possible for this condition to be true even if the arc was not traversed in a call to this function but in a call to another instance of this function using
//     *     the same mark value.  One case where this will occur is tracing back from enabled subsystem inputs.  In this case, it is OK that the trace stops at an already traversed arc because
//     *     all the arcs upstream of this arc which would have been traversed in this call to the function will have already been traversed and any further traversal will simply do redundant work.
//     *     For the case of tracking back from Mux ports, different mark values are used for each port.  Finding the mark value in any arc will constitute a combinational loop.
//     *
//     *
//     * This method can be used for either mux traceback or enabled subsystem traceback from inputs.
//     *     - When used with muxes, the mark value corresponds to the input port.
//     *     - When used with enabled subsystems, the mark value corresponds to the enabled subsystem.
//     *
//     * @tparam MarkType The datatype used for marking arcs
//     * @param traceFrom The port to trace from
//     * @param mark The mark value.  This value is used to mark traversed arcs during the trace
//     * @param markSet A map containing the mark set for each arc.  This is updated during the call to the function
//     * @return The set of nodes which had output arcs marked during the call to this function
//     */
//    template<typename MarkType>
//    std::set<std::shared_ptr<Node>> scopedTraceBackAndMark(std::shared_ptr<InputPort> traceFrom, MarkType mark, std::map<std::shared_ptr<Arc>, std::set<MarkType>> &markSet){
//        std::set<std::shared_ptr<Node>> markedNodes;
//
//        //Get the set of arcs connected to this input port
//        std::set<std::shared_ptr<Arc>> arcs = traceFrom->getArcs();
//
//        //Look at each arc
//        for(auto it = arcs.begin(); it != arcs.end(); it++){
//
//            std::shared_ptr<Node> srcNode = (*it)->getSrcPort()->getParent();
//
//            //Check if the src node is a state element or a enable node (input or output)
//            if(!(srcNode->hasState()) && GeneralHelper::isType<Node, EnableNode>(srcNode) == nullptr){
//                //The src node is not a state element and is not an enable node, continue
//
//                //Check if arc is already marked
//                bool alreadyMarked = false;
//                if(markSet.find(*it) != markSet.end()){//Check if arc is already in the map
//                    std::set<MarkType> arcMarks = markSet[*it]; //This is technically a copy of the arc set
//                    if(arcMarks.find(mark) != arcMarks.end()){
//                        alreadyMarked = true;
//                    }
//                }
//
//                if(!alreadyMarked){
//                    //Mark the arc
//                    markSet[*it].insert(mark); //map will insert an element if one does not exist
//
//                    //Add the src node to the set of marked nodes
//                    markedNodes.insert(srcNode);
//
//                    //Recursivly call on the input ports of this node (call on the inputs and specials but not on the OrderConstraintPort)
//                    //combine the sets of marked nodes
//                    std::vector<InputPort> inputPorts = srcNode->getInputPortsIncludingSpecial();
//
//
//                }//else, stop traversal
//
//            }//else, stop traversal
//        }
//
//        return markedNodes
//    }
//
//    template<typename MarkType>
//    std::set<Node> invalidateMixedMarkedNodes(std::set<std::shared_ptr<Node>> markedNodes, MarkType mark); //Invalidate nodes with mixed marks or with an unmarked output arc.
//                                                                                                           // Invalidation propagates up to nodes connected to the inputs of the invaludated node.
//                                                                                                           // Repeated for each node in the set of marked node.  Note that some invalidations will
//                                                                                                           // change the node set (ie. remove nodes from the set of nodes to check)
//                                                                                                           //
//                                                                                                           //Note that invalidation does not propagate to outputs.
//                                                                                                           //A node being depended on by other contexts does not preclude one of its downstream nodes from
//                                                                                                           //being in a context.
//                                                                                                           //
//                                                                                                           // If outputs should be invalidated, they will be invalidated because their own arcs (directly or
//                                                                                                           // via another node's invalidation)
//
//                                                                                                           //MarkType must provide a == operator
//                                                                                                           //The set of nodes that remain marked after invalidation
//
//    void invalidateBack(std::shared_ptr<Node> node); //Invalidate the nodes from this node to the inputs.  Stops at state, enabled subsystem boundaries, and at unmarked noded.
//                                                     //Stops at unmarked nodes because unmarked nodes cannot have been traversed.  Their input arcs will be unmarked and thus would cause upstream nodes to have unmarked arcs if they were marked by another path.

};


#endif //VITIS_GRAPHALGS_H
