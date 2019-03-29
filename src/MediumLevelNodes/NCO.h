//
// Created by Christopher Yarp on 2019-02-18.
//

#ifndef VITIS_NCO_H
#define VITIS_NCO_H

#include "MediumLevelNode.h"
#include "GraphCore/NodeFactory.h"
#include "MasterNodes/MasterUnconnected.h"

/**
 * \addtogroup MediumLevelNodes Medium Level Nodes
 */
/*@{*/

/**
 * @brief Represents a Numerically Controlled Oscillator (NCO)
 *
 * This NCO is constructed from a quarter wave lookup table, a phase accomulator, and a selective sign inverter.
 *
 * Dithering is currently not supported
 *
 */
class NCO : public MediumLevelNode{
friend NodeFactory;

private:
    int lutAddrBits; ///<The number of bits in the Lookup Table Address (note that this is for a full wave lookup table, the actual table is 1/4+1 the size)
    int accumulatorBits; ///<The number of bits in the accumulator.  This is quantized the number of addr bits
    int ditherBits; ///<The number of dither bits
    bool complexOut; ///<If true, a complex exponential is calculated, otherwise a cosine is calculated

    /**
     * @brief Constructs a NCO node
     *
     * @note To construct from outside of hierarchy, use factories in @ref NodeFactory
     */
    NCO();

    /**
     * @brief Constructs a NCO node with a given parent.  This node is not added to the children list of the parent.
     *
     * @note To construct from outside of hierarchy, use factories in @ref NodeFactory
     *
     * @param parent parent node
     */
    explicit NCO(std::shared_ptr<SubSystem> parent);

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
    NCO(std::shared_ptr<SubSystem> parent, NCO* orig);

public:
    int getLutAddrBits() const;
    void setLutAddrBits(int lutAddrBits);

    int getAccumulatorBits() const;
    void setAccumulatorBits(int accumulatorBits);

    int getDitherBits() const;

    void setDitherBits(int ditherBits);

    bool isComplexOut() const;
    void setComplexOut(bool complexOut);

    //==== Factories ====
    /**
     * @brief Creates a NCO node from a GraphML Description
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
    static std::shared_ptr<NCO> createFromGraphML(int id, std::string name,
                                                   std::map<std::string, std::string> dataKeyValueMap,
                                                   std::shared_ptr<SubSystem> parent, GraphMLDialect dialect);

    //==== Expand ====
    /**
     * @brief Expands the gain block into a multiply block and a constant block.
     *
     * Validates before expansion to check assumptions are fulfilled.
     *
     * The type of the constant is determined in the following manner:
     *   - If output type is a floating point type, the constant takes on the same type as the output
     *   - If the output is an integer type, the constant takes the smallest integer type which accommodates the constant.
     *   - If the output is a fixed point type, the constant takes on a fixed point type with the integer portion is the smallest that supports the given constant
     *     the number of fractional bits are the max(fractional bits in output, fractional bits in 1st input)
     *
     *   Complexity and width are taken from the gain NumericValue
     */
    std::shared_ptr<ExpandedNode> expand(std::vector<std::shared_ptr<Node>> &new_nodes, std::vector<std::shared_ptr<Node>> &deleted_nodes,
                                             std::vector<std::shared_ptr<Arc>> &new_arcs, std::vector<std::shared_ptr<Arc>> &deleted_arcs,
                                             std::shared_ptr<MasterUnconnected> &unconnected_master) override ;

    //==== Emit Functions ====
    std::set<GraphMLParameter> graphMLParameters() override;

    xercesc::DOMElement* emitGraphML(xercesc::DOMDocument* doc, xercesc::DOMElement* graphNode, bool include_block_node_type) override ;

    std::string labelStr() override ;

    void validate() override ;

    std::shared_ptr<Node> shallowClone(std::shared_ptr<SubSystem> parent) override;

};


#endif //VITIS_NCO_H
