//
// Created by Christopher Yarp on 4/29/20.
//

#ifndef VITIS_DOWNSAMPLEINPUT_H
#define VITIS_DOWNSAMPLEINPUT_H

#include "Downsample.h"
#include "GraphCore/NodeFactory.h"

/**
 * \addtogroup MultiRate Multi-Rate Support Nodes
 * @{
*/

/**
 * @brief This node represents a downsample.
 *
 * More specifically, it represents a downsampling at the input of a DownsampleClockDomain.  Because the clock domain
 * context selectivly executes, this node just acts as a pass through.  The downsample is accomplished by the context
 * driver of the DownsampleClockDomain itself.
 *
 */
class DownsampleInput : public Downsample {
friend class NodeFactory;

protected:
    //==== Constructors ====
    /**
     * @brief Construct a DownsampleInput node
     */
    DownsampleInput();

    /**
     * @brief Construct a DownsampleInput node, with a given parent.
     *
     * @note To construct from outside of hierarchy, use factories in @ref NodeFactory
     *
     * @param parent parent node
     */
    explicit DownsampleInput(std::shared_ptr<SubSystem> parent);

    /**
     * @brief Constructs a new node with a shallow copy of parameters from the original node.  Ports are not copied and neither is the parent reference.  This node is not added to the children list of the parent.
     *
     * @note To construct from outside of hierarchy, use factories in @ref NodeFactory
     *
     * @note If copying a graph, the parent should be one of the copies and not from the original graph.
     *
     * @warning Because pointer (this) is passed to ports, nodes must be allocated on the heap and not moved.  All interaction should be via pointers.
     *
     * @param parent parent node
     * @param orig The origional node from which a shallow copy is being made
     */
    DownsampleInput(std::shared_ptr<SubSystem> parent, DownsampleInput* orig);

public:
    //==== Factories ====
    /**
     * @brief Creates a DownsampleInput node from a GraphML Description
     *
     * @note This function does not add the node to the design or to the nodeID/pointer map
     *
     * @param id the ID number of the node
     * @param name the human readable name of a node
     * @param dataKeyValueMap A map of property keys and values extracted from the data nodes in the GraphML
     * @param parent The parent of this node in the hierarchy
     * @param dialect The dialect of the GraphML file being imported
     * @return a pointer to the new delay node
     */
    static std::shared_ptr<DownsampleInput> createFromGraphML(int id, std::string name,
                                                 std::map<std::string, std::string> dataKeyValueMap,
                                                 std::shared_ptr<SubSystem> parent, GraphMLDialect dialect);

    //==== Emit Functions ====
    std::set<GraphMLParameter> graphMLParameters() override;

    xercesc::DOMElement* emitGraphML(xercesc::DOMDocument* doc, xercesc::DOMElement* graphNode, bool include_block_node_type = true) override ;

    std::string typeNameStr() override;

    //==== Validation ====
    void validate() override;

    std::shared_ptr<Node> shallowClone(std::shared_ptr<SubSystem> parent) override;

    /**
     * @brief Emits type conversion logic.
     *
     * If no type conversion is required (input and target types are the same), no type conversion logic is returned.
     */
    CExpr emitCExpr(std::vector<std::string> &cStatementQueue, SchedParams::SchedType schedType, int outputPortNum,
                    bool imag = false) override;

    bool isSpecialized() override;

    bool isInput() override;
};

/*! @} */

#endif //VITIS_DOWNSAMPLEINPUT_H
