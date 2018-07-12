//
// Created by Christopher Yarp on 6/27/18.
//

#ifndef VITIS_DESIGN_H
#define VITIS_DESIGN_H

#include <memory>
#include <vector>
#include <set>

#include "GraphMLParameter.h"

#include <xercesc/dom/DOM.hpp>
#include <xercesc/util/PlatformUtils.hpp>

//Forward Declaration of Classes
class MasterInput;
class MasterOutput;
class MasterUnconnected;
class Node;
class Arc;

//This Class

/**
* \addtogroup GraphCore Graph Core
*/
/*@{*/

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

    std::vector<std::shared_ptr<Node>> topLevelNodes;///< A vector of nodes at the top level of the design (ie. not under a subsystem)
    std::vector<std::shared_ptr<Node>> nodes; ///< A vector of nodes in the design
    std::vector<std::shared_ptr<Arc>> arcs; ///< A vector of arcs in the design

public:
    /**
     * @brief Constructs a design object, creating master nodes in the process.
     *
     * The vectors are initialized using the default method (not explicitly initialized by the constructor).
     */
    Design();

    void addNode(std::shared_ptr<Node> node);
    void addTopLevelNode(std::shared_ptr<Node> node);
    void addArc(std::shared_ptr<Arc> arc);

    /**
     * @brief Re-number node IDs
     *
     * @note The node IDs may change in node that previously had valid IDs
     */
    void reNumberNodeIDs();

    /**
     * @brief Assigns node IDs to nodes which have an ID < 0
     *
     * Finds the maximum ID of all nodes in the design and begins numbering from there
     */
    void assignNodeIDs();

    /**
     * @brief Re-number arc IDs
     *
     * @note The arc IDs may change in node that previously had valid IDs
     */
    void reNumberArcIDs();

    /**
     * @brief Assigns arc IDs to nodes which have an ID < 0
     *
     * Finds the maximum ID of all arcs in the design and begins numbering from there
     */
    void assignArcIDs();

    /**
     * @brief Emits the given design as GraphML XML
     *
     * @Note: @ref assignNodeIDs and @ref assignArcIDs should be called before this is called.  Otherwise, there may be name colissions.
     * This function does not call them to allow for either assignment or complete re-numbering to occur based on the situation.
     *
     * @param doc the XML document for the design to emit into
     * @param root root XML element of document
     */
    void emitGraphML(xercesc::DOMDocument* doc, xercesc::DOMElement* root);

    /**
     * @brief Get a set of GraphML parameters for this design by querying the nodes within it
     * @return set of GraphML parameters for this design
     */
    std::set<GraphMLParameter> graphMLParameters();

    //==== Getters/Setters ====
    std::shared_ptr<MasterInput> getInputMaster() const;
    void setInputMaster(const std::shared_ptr<MasterInput> inputMaster);
    std::shared_ptr<MasterOutput> getOutputMaster() const;
    void setOutputMaster(const std::shared_ptr<MasterOutput> outputMaster);
    std::shared_ptr<MasterOutput> getVisMaster() const;
    void setVisMaster(const std::shared_ptr<MasterOutput> visMaster);
    std::shared_ptr<MasterUnconnected> getUnconnectedMaster() const;
    void setUnconnectedMaster(const std::shared_ptr<MasterUnconnected> unconnectedMaster);
    std::shared_ptr<MasterOutput> getTerminatorMaster() const;
    void setTerminatorMaster(const std::shared_ptr<MasterOutput> terminatorMaster);
    std::vector<std::shared_ptr<Node>> getTopLevelNodes() const;
    void setTopLevelNodes(const std::vector<std::shared_ptr<Node>> topLevelNodes);
    std::vector<std::shared_ptr<Node>> getNodes() const;
    void setNodes(const std::vector<std::shared_ptr<Node>> nodes);
    std::vector<std::shared_ptr<Arc>> getArcs() const;
    void setArcs(const std::vector<std::shared_ptr<Arc>> arcs);

};

/*@}*/

#endif //VITIS_DESIGN_H
