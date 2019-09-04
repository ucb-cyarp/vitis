//
// Created by Christopher Yarp on 2019-02-21.
//

#ifndef VITIS_BITWISEOPERATOR_H
#define VITIS_BITWISEOPERATOR_H

#include "GraphCore/NodeFactory.h"
#include "PrimitiveNode.h"

/**
 * \addtogroup PrimitiveNodes Primitives
 * @{
*/

/**
 * @brief Represents a bitwise operator block.
 *
 * This block represents a set of bitwise operators such as NOT, AND, OR, XOR, Shift Left, & Shift Right
 *
 * The NOT operator has a single input
 * All other operators have 2 inputs
 *
 * All operators have a single output.
 *
 * The Logical Operations Supported in are generally the same ones supported in C:
 * Operator    | Description          | C Equivalent
 * ------------|----------------------|-------------
 * AND         | Bitwise AND          | &
 * OR          | Bitwise OR           | \|
 * XOR         | Bitwise Exclusive Or | `^`
 * NOT         | Bitwise NOT          | ~
 * SHIFT_LEFT  | Bitwise Shift Left   | <<
 * SHIFT_Right | Bitwise Shift Left   | >>
 */
class BitwiseOperator : public PrimitiveNode {
friend NodeFactory;

public:
    /**
     * @brief An enum representing the bitwise operator used
     */
    enum class BitwiseOp{
        NOT, ///<Bitwise NOT
        AND, ///<Bitwise AND
        OR, ///<Bitwise OR
        XOR, ///<Bitwise Exclusive OR
        SHIFT_LEFT, ///<Bitwise Shift Left
        SHIFT_RIGHT ///<Bitwise Shift Right
    };

    /**
     * @brief Parse a string to a BitwiseOp
     * @param str string to parse
     * @return corresponding BitwiseOp
     */
    static BitwiseOp parseBitwiseOpString(std::string str);

    /**
     * @brief Get a string representation of the BitwiseOp
     * @param op BitwiseOp to get a string representation of
     * @return string representation of the Bitwise
     */
    static std::string bitwiseOpToString(BitwiseOp op);

    /**
     * @brief Get a C style string representation of the basic BitwiseOp
     *
     *
     * @param op BitwiseOp to get a string representation of
     * @return string representation of the BitwiseOp
     */
    static std::string bitwiseOpToCString(BitwiseOp op);

private:
    BitwiseOp op; ///<The bitwise operation this node represents

    //==== Constructors ====
    /**
     * @brief Constructs an empty BitwiseOperator node with operator NOT
     *
     * @note To construct from outside of hierarchy, use factories in @ref NodeFactory
     */
    BitwiseOperator();

    /**
     * @brief Constructs an empty BitwiseOperator node with operator NOT with a given parent.  This node is not added to the children list of the parent.
     *
     * @note To construct from outside of hierarchy, use factories in @ref NodeFactory
     *
     * @param parent parent node
     */
    explicit BitwiseOperator(std::shared_ptr<SubSystem> parent);

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
    BitwiseOperator(std::shared_ptr<SubSystem> parent, BitwiseOperator* orig);

public:
    BitwiseOp getOp() const;
    void setOp(BitwiseOp op);

    //====Factories====
    /**
     * @brief Creates a bitwise operator node from a GraphML Description
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
    static std::shared_ptr<BitwiseOperator> createFromGraphML(int id, std::string name,
                                                              std::map<std::string, std::string> dataKeyValueMap,
                                                              std::shared_ptr<SubSystem> parent, GraphMLDialect dialect);

    //==== Emit Functions ====
    std::set<GraphMLParameter> graphMLParameters() override;

    xercesc::DOMElement* emitGraphML(xercesc::DOMDocument* doc, xercesc::DOMElement* graphNode, bool include_block_node_type = true) override ;

    std::string labelStr() override ;

    void validate() override ;

    std::shared_ptr<Node> shallowClone(std::shared_ptr<SubSystem> parent) override;

    /**
     * @brief Emits a C expression for the logical operator
     *
     */
    CExpr emitCExpr(std::vector<std::string> &cStatementQueue, SchedParams::SchedType schedType, int outputPortNum,
                    bool imag = false) override;

};

/*! @} */

#endif //VITIS_BITWISEOPERATOR_H
