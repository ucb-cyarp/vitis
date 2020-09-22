//
// Created by Christopher Yarp on 7/31/20.
//

#ifndef VITIS_CONCATENATE_H
#define VITIS_CONCATENATE_H

#include "PrimitiveNode.h"
#include "GraphCore/NodeFactory.h"

/**
 * \addtogroup PrimitiveNodes Primitives
 * @{
*/

/**
 * @brief Represents a vector/matrix concatenate function.
 *
 * @note: Currently, only vector concatenate is currently supported
 */
class Concatenate : public PrimitiveNode {
    friend NodeFactory;
    //TODO: implement multidimensional matrix concatenate support

private:
    //Parameters only used for multidimensional emit
    // - ConcatenateDimension

    //==== Constructors ====
    /**
     * @brief Constructs an empty Concatenate node
     *
     * @note To construct from outside of hierarchy, use factories in @ref NodeFactory
     */
    Concatenate();

    /**
     * @brief Constructs an empty Concatenate node with a given parent.  This node is not added to the children list of the parent.
     *
     * @note To construct from outside of hierarchy, use factories in @ref NodeFactory
     *
     * @param parent parent node
     */
    explicit Concatenate(std::shared_ptr<SubSystem> parent);

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
    Concatenate(std::shared_ptr<SubSystem> parent, Concatenate* orig);

public:
    //====Factories====
    /**
     * @brief Creates a Concatenate node from a GraphML Description
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
    static std::shared_ptr<Concatenate> createFromGraphML(int id, std::string name,
                                                  std::map<std::string, std::string> dataKeyValueMap,
                                                  std::shared_ptr<SubSystem> parent, GraphMLDialect dialect);

    //==== Emit Functions ====
    std::set<GraphMLParameter> graphMLParameters() override;

    xercesc::DOMElement* emitGraphML(xercesc::DOMDocument* doc, xercesc::DOMElement* graphNode, bool include_block_node_type) override ;

    std::string typeNameStr() override;

    std::string labelStr() override ;

    void validate() override ;

    std::shared_ptr<Node> shallowClone(std::shared_ptr<SubSystem> parent) override;

    /**
     * @brief Emits a C expression for the Concatenate
     */
    CExpr emitCExpr(std::vector<std::string> &cStatementQueue, SchedParams::SchedType schedType, int outputPortNum,
                    bool imag) override;

};

/*! @} */

#endif //VITIS_CONCATENATE_H
