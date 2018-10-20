//
// Created by Christopher Yarp on 10/16/18.
//

#include "GraphAlgs.h"

std::set<std::shared_ptr<Node>>
GraphAlgs::scopedTraceBackAndMark(std::shared_ptr<InputPort> traceFrom, std::map<std::shared_ptr<Arc>, bool> &marks) {
    std::set<std::shared_ptr<Node>> contextNodes;

    //Get the set of arcs connected to this input port
    std::set<std::shared_ptr<Arc>> arcs = traceFrom->getArcs();

    //Look at each incoming arc
    for(auto it = arcs.begin(); it != arcs.end(); it++){

        std::shared_ptr<Node> srcNode = (*it)->getSrcPort()->getParent();

        //Check if the src node is a state element or a enable node (input or output)
        if(!(srcNode->hasState()) && GeneralHelper::isType<Node, EnableNode>(srcNode) == nullptr){
            //The src node is not a state element and is not an enable node, continue

            //Check if arc is already marked (should never occur -> if it does, we have traversed the same node twice which should be impossible.
            // Even with feedback because feedback would cause an output arc to never be marked rather than visited more than once)
            //Will do this as a sanity check for now
            //TODO: remove "already marked" check
            bool alreadyMarked = false;
            if(marks.find(*it) != marks.end()){//Check if arc is already in the map
                    alreadyMarked = marks[*it];
            }
            if(alreadyMarked){
                throw std::runtime_error("Traceback marked an arc twice.  Should never occur.");
            }

            //Mark the arc
            marks[*it]=true;

            //Check if the src node has all of its output ports marked
            bool allMarked = true;
            std::vector<std::shared_ptr<OutputPort>> srcOutputPorts = srcNode->getOutputPorts();
            for(unsigned long i = 0; i<srcOutputPorts.size() && allMarked; i++){
                std::set<std::shared_ptr<Arc>> srcOutputArcs = srcOutputPorts[i]->getArcs();

                for(auto srcOutArc = srcOutputArcs.begin(); srcOutArc != srcOutputArcs.end() && allMarked; srcOutArc++){
                    bool outMarked = false;

                    if(marks.find(*srcOutArc) != marks.end()){//Check if arc is already in the map
                        outMarked = marks[*srcOutArc];
                    }

                    allMarked = outMarked; //If the mark was found, keep set to true, otherwise set to false and break out of for loops
                }
            }

            if(allMarked) {
                //Add the src node to the context
                contextNodes.insert(srcNode);

                //Recursivly call on the input ports of this node (call on the inputs and specials but not on the OrderConstraintPort)

                std::vector<std::shared_ptr<InputPort>> inputPorts = srcNode->getInputPortsIncludingSpecial();
                for(unsigned long i = 0; i<inputPorts.size(); i++){
                    std::set<std::shared_ptr<Node>> moreNodesInContext = scopedTraceBackAndMark(inputPorts[i], marks);

                    //combine the sets of marked nodes
                    contextNodes.insert(moreNodesInContext.begin(), moreNodesInContext.end());
                }

            }
            //else, this node is either not in the context or there is a longer path to the node from within the context.
            //If there is a longer path to the node within the context, the longest path will eventually trigger the node to be inserted

        }//else, stop traversal.  This is a state element or enable node.  Do not mark and do not recurse
    }

    return contextNodes;
}

std::set<std::shared_ptr<Node>>
GraphAlgs::scopedTraceForwardAndMark(std::shared_ptr<OutputPort> traceFrom, std::map<std::shared_ptr<Arc>, bool> &marks){
    std::set<std::shared_ptr<Node>> contextNodes;

    //Get the set of arcs connected to this output port
    std::set<std::shared_ptr<Arc>> arcs = traceFrom->getArcs();

    //Look at each outgoing arc
    for(auto it = arcs.begin(); it != arcs.end(); it++){

        std::shared_ptr<Node> dstNode = (*it)->getDstPort()->getParent();

        //Check if the dst node is a state element or a enable node (input or output)
        if(!(dstNode->hasState()) && GeneralHelper::isType<Node, EnableNode>(dstNode) == nullptr){
            //The dst node is not a state element and is not an enable node, continue

            //Check if arc is already marked (should never occur -> if it does, we have traversed the same node twice which should be impossible.
            // Even with feedback because feedback would cause an output arc to never be marked rather than visited more than once)
            //Will do this as a sanity check for now
            //TODO: remove "already marked" check
            bool alreadyMarked = false;
            if(marks.find(*it) != marks.end()){//Check if arc is already in the map
                alreadyMarked = marks[*it];
            }
            if(alreadyMarked){
                throw std::runtime_error("Traceback marked an arc twice.  Should never occur.");
            }

            //Mark the arc
            marks[*it]=true;

            //Check if the dst node has all of its input ports marked
            bool allMarked = true;
            std::vector<std::shared_ptr<InputPort>> dstInputPorts = dstNode->getInputPortsIncludingSpecial();
            for(unsigned long i = 0; i<dstInputPorts.size() && allMarked; i++){
                std::set<std::shared_ptr<Arc>> dstInputArcs = dstInputPorts[i]->getArcs();

                for(auto dstInArc = dstInputArcs.begin(); dstInArc != dstInputArcs.end() && allMarked; dstInArc++){
                    bool outMarked = false;

                    if(marks.find(*dstInArc) != marks.end()){//Check if arc is already in the map
                        outMarked = marks[*dstInArc];
                    }

                    allMarked = outMarked; //If the mark was found, keep set to true, otherwise set to false and break out of for loops
                }
            }

            if(allMarked) {
                //Add the src node to the context
                contextNodes.insert(dstNode);

                //Recursivly call on the output ports of this node (call on the inputs and specials but not on the OrderConstraintPort)

                std::vector<std::shared_ptr<OutputPort>> outputPorts = dstNode->getOutputPorts();
                for(unsigned long i = 0; i<outputPorts.size(); i++){
                    std::set<std::shared_ptr<Node>> moreNodesInContext = scopedTraceForwardAndMark(outputPorts[i], marks);

                    //combine the sets of marked nodes
                    contextNodes.insert(moreNodesInContext.begin(), moreNodesInContext.end());
                }

            }
            //else, this node is either not in the context or there is a longer path to the node from within the context.
            //If there is a longer path to the node within the context, the longest path will eventually trigger the node to be inserted

        }//else, stop traversal.  This is a state element or enable node.  Do not mark and do not recurse
    }

    return contextNodes;
}

void GraphAlgs::moveNodePreserveHierarchy(std::shared_ptr<Node> nodeToMove, std::shared_ptr<SubSystem> moveUnder,
                                          std::string moveSuffix) {

    std::shared_ptr<SubSystem> moveUnderParent = moveUnder->getParent();

    //Discover hierarchy up to

}
