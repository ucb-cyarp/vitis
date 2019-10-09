//
// Created by Christopher Yarp on 9/11/18.
//

#ifndef VITIS_LOGICOPERATOR_H
#define VITIS_LOGICOPERATOR_H

#include "PrimitiveNode.h"
#include "GraphCore/NodeFactory.h"
#include "GraphMLTools/GraphMLDialect.h"

/**
 * \addtogroup PrimitiveNodes Primitives
 * @{
*/

/**
 * @brief Represents a logical operator block.
 *
 * This block represents a set of logical operators such as NOT, AND, OR, XOR, ...
 *
 * NOT has a single input but all other operators must have more than 1 input.
 * Operators other than NOT can actually have more than 2 inputs.
 *
 * All operators have a single output.
 *
 * Note: Simulink specifies the number of inputs for all operators other than NOT.  This is used
 * to create more ports.  This number is not imported by vitis as it is redundant with the number of input ports
 * and serves no other purpose (does not define the sign of the input port, for example)
 *
 * The Logical Operations Supported in Simulink are:
 * AND - Logical AND (True iff all inputs are true) - Can be composed of cascaded 2 input AND operations
 * OR - Logical OR (True if any input is true, false only if all inputs are false)
 * NAND - Not AND (Negating the result of an AND)
 * NOR - Not OR (Negating the result of an OR)
 * XOR - Exclusive OR - For 2 inputs is well defined (true iff only 1 input is true) - Is less well defined for >2 inputs.  Matlab takes the view of: true iff an odd number of inputs are true
 *     - Even though there is no logical XOR operator in C, != works (is one of the recomended approached when forum splunking)
 *     - XOR with more than 2 is treated as cascaded 2XORs.
 *     - See (https://en.wikipedia.org/wiki/XOR_gate) and (https://stackoverflow.com/questions/1596668/logical-xor-operator-in-c)
 *       for some explanations.  The truth table for != and XOR are the same for 2 bools.  The cascade of XORs determines
 *       if the number of inputs is odd.  In the base case (2 inputs) XOR is true iff a single input is true. Let is assume
 *       the previous stage is true iff an odd number of inputs were true.  In the nextnstage, the output is true iff
 *       the first input is true (odd number of previous inputs are true) and the current input is not true, or
 *       the previous input is false (an even number of inputs were true) and the current input is true.  These cases encompas
 *       all possibilities for a total odd number of inputs being true up to this state.  Otherwise, the output
 *       is false (ie. for even inputs).  We have thus shown the base and itterative case.
 * NXOR - Not XOR - Matlab takes the view: True iff an even number of inputs are true
 *      - I take this to mean that 0 true inputs would result in NXOR being true.
 *      - Not applied to a cascade of 2XORs
 * NOT - Logical NOT
 */
class LogicalOperator : public PrimitiveNode {
    friend NodeFactory;
public:
    /**
     * @brief An enum representing the comparison operator used when comparing the signals
     */
    enum class LogicalOp{
        NOT, ///<Logical NOT
        AND, ///<Logical AND
        OR, ///<Logical OR
        NAND, ///<Not AND
        NOR, ///<Not OR
        XOR, ///<Logical Exclusive OR (Cascade of 2XORs if > 2 Inputs) - True iff an odd number of inputs are true
        NXOR ///<Not XOR (Not of a Casecade of 2XORs if > 2 Inputs) - True iff an even number of inputs (or 0) are true
    };

    /**
     * @brief Parse a string to a LogicalOp
     * @param str string to parse
     * @return corresponding LogicalOp
     */
    static LogicalOp parseLogicalOpString(std::string str);

    /**
     * @brief Get a string representation of the LogicalOp
     * @param op LogicalOp to get a string representation of
     * @return string representation of the CompareOp
     */
    static std::string logicalOpToString(LogicalOp op);

    /**
     * @brief Get a C style string representation of the basic LogicalOp
     *
     * @note NAND, NOR, NXOR return the operators used by AND, OR, XOR respectivly.
     * @note XOR returns ==.  Operands should be cast to bool before using
     *
     * @param op LogicalOp to get a string representation of
     * @return string representation of the CompareOp
     */
    static std::string logicalOpToCString(LogicalOp op);

    /**
     * @brief Determines if an outer NOT is a component of this operation (ie. NOT, NAND, NOR, NXOR)
     *
     * @param op LogicalOp to check
     * @return true if an outer NOT is a component of the operation, false otherwise
     */
    static bool logicalOpIsNegating(LogicalOp op);

private:
    LogicalOp logicalOp; ///< The LogicalOp this LogicOperator Represents

    //==== Constructors ====
    /**
     * @brief Constructs an empty LogicalOperator node with operator NOT
     *
     * @note To construct from outside of hierarchy, use factories in @ref NodeFactory
     */
    LogicalOperator();

    /**
     * @brief Constructs an empty LogicalOperator node with operator NOT with a given parent.  This node is not added to the children list of the parent.
     *
     * @note To construct from outside of hierarchy, use factories in @ref NodeFactory
     *
     * @param parent parent node
     */
    explicit LogicalOperator(std::shared_ptr<SubSystem> parent);

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
    LogicalOperator(std::shared_ptr<SubSystem> parent, LogicalOperator* orig);

public:
    LogicalOp getLogicalOp() const;
    void setLogicalOp(LogicalOp logicalOp);

    //====Factories====
    /**
     * @brief Creates a compare (relational operator) node from a GraphML Description
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
    static std::shared_ptr<LogicalOperator> createFromGraphML(int id, std::string name,
                                                      std::map<std::string, std::string> dataKeyValueMap,
                                                      std::shared_ptr<SubSystem> parent, GraphMLDialect dialect);

    //==== Emit Functions ====
    std::set<GraphMLParameter> graphMLParameters() override;

    xercesc::DOMElement* emitGraphML(xercesc::DOMDocument* doc, xercesc::DOMElement* graphNode, bool include_block_node_type = true) override ;

    std::string typeNameStr() override;

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

#endif //VITIS_LOGICOPERATOR_H
