//
// Created by Christopher Yarp on 7/6/18.
//

#ifndef VITIS_SUM_H
#define VITIS_SUM_H

#include <vector>
#include <memory>
#include <map>
#include <string>

#include "PrimitiveNode.h"
#include "GraphMLTools/GraphMLDialect.h"

/**
 * \addtogroup PrimitiveNodes Primitives
 */
/*@{*/

/**
 * @brief Represents a Sum Block.  Parameters dictate the signs of the inputs.
 */
class Sum : public PrimitiveNode{
    friend NodeFactory;
private:
    std::vector<bool> inputSign; ///<An array of input signs to the sum operation.  True = positive, False = negative.  Or a number indicating the number of + inputs

    //==== Constructors ====
    /**
     * @brief Constructs an empty sum node
     *
     * @note To construct from outside of hierarchy, use factories in @ref NodeFactory
     */
    Sum();

    /**
     * @brief Constructs an empty sum node with a given parent.  This node is not added to the children list of the parent.
     *
     * @note To construct from outside of hierarchy, use factories in @ref NodeFactory
     *
     * @param parent parent node
     */
    explicit Sum(std::shared_ptr<SubSystem> parent);

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
    Sum(std::shared_ptr<SubSystem> parent, Sum* orig);

public:
    //====Getters/Setters====
    std::vector<bool> getInputSign() const;
    void setInputSign(const std::vector<bool> &inputSign);

    //====Factories====
    /**
     * @brief Creates a sum node from a GraphML Description
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
    static std::shared_ptr<Sum> createFromGraphML(int id, std::string name,
                                                  std::map<std::string, std::string> dataKeyValueMap,
                                                  std::shared_ptr<SubSystem> parent, GraphMLDialect dialect);

    //==== Emit Functions ====
    std::set<GraphMLParameter> graphMLParameters() override;

    xercesc::DOMElement* emitGraphML(xercesc::DOMDocument* doc, xercesc::DOMElement* graphNode, bool include_block_node_type = true) override ;

    std::string labelStr() override ;

    void validate() override ;

    std::shared_ptr<Node> shallowClone(std::shared_ptr<SubSystem> parent) override;

    //TODO: Implement More Accumulator Types.  Simulink allows the accumulator type for the sum to be specified.  Import this property?  I need to be very careful of overflow if I take a less conservative approach.
    /**
     * @brief Emits a C expression for the sum
     *
     * For more than 2 inputs, the size of the accumulator can be ambiguous.  It could be the same as the output or some other value.
     * It is only important when overflow is possible
     *
     * For now, the C emit take a semi-conserative approach:
     *   - For floating point, all arguments are cast to the highest resolution floating point type of the inputs.  The result is then cast to the type of the output
     *   - For integer or fixed point, the most conservative common type is calculated.  The log2(number of ports) bits is added to account for the potential bit growth.  All inputs are cast to this type (sifted so that they align and cast up to a larger CPU type if applicable).  The result is then scaled to the output type and trunkated.  If the specified type is less than the CPU type, it is masked & sign extended if signed.
     *
     * For 2 input sums, a different approach is taken for integers:
     *   - If the output type is the same as the first input, then the second input is converted to the type of the first input (but not masked) and is added.  The result is masked (& sign exteded if signed) to the output type if not a perfect CPU type.  No further casting is required.
     *   - If the output type is the most conservative type that fits both inputs + 1 integer bit, then both inputs are cast to that type and added.  No masking or casting is required for the output
     *
     * @note Masking can be (is) omitted (in the case of the output having the same type as the first input) if the total number of bits is a standard CPU type.
     *
     */
    CExpr emitCExpr(std::vector<std::string> &cStatementQueue, int outputPortNum, bool imag = false) override;

};

/*@}*/

#endif //VITIS_SUM_H
