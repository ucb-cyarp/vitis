//
// Created by Christopher Yarp on 6/27/18.
//

#ifndef VITIS_DESIGN_H
#define VITIS_DESIGN_H

#include <memory>
#include <vector>

//Forward Declaration of Classes
class MasterInput;
class MasterOutput;
class MasterUnconnected;
class Node;
class Arc;

//This Class

/**
* @brief Represents a Streaming DSP Flow Graph Design
*
* This class acts as a container for a streaming DSP flow graph design.  It contains references to the various master
* nodes which represents the inputs, outputs, visualizers, termininators, and unconnected.  It also contains vectors
* of pointers to the nodes and arcs in the design.  This allows the design to contain nodes which are not reachable
* from the master nodes.  These unreachable nodes may be pruned or may be kept.
*/
class Design {
private:
    std::shared_ptr<MasterInput> inputMaster; ///< A pointer to the input master node of the design, representing the inputs to the design
    std::shared_ptr<MasterOutput> outputMaster; ///< A pointer to the output master node of the design, representing the outputs from the design
    std::shared_ptr<MasterOutput> visMaster; ///< A pointer to the visualization master node of the deign, representing signals to be visualized
    std::shared_ptr<MasterUnconnected> unconnectedMaster; ///< A pointer to the unconnected master node of the design, representing unconnected ports in the design
    std::shared_ptr<MasterOutput> terminatorMaster; ///< A pointer to the unconnected master node of the design, representing terminated ports

    std::vector<std::shared_ptr<Node>> nodes; ///< A vector of nodes in the design
    std::vector<std::shared_ptr<Arc>> arcs; ///< A vector of arcs in the design

};


#endif //VITIS_DESIGN_H
