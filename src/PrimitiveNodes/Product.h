//
// Created by Christopher Yarp on 7/6/18.
//

#ifndef VITIS_PRODUCT_H
#define VITIS_PRODUCT_H

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
 * @brief Represents a Product Block.  Parameters dictate the position of the inputs.
 */
class Product : public PrimitiveNode{
    friend NodeFactory;
private:
    std::vector<bool> inputOp; ///<An array of *, / to indicate the operation on the input.  * = Multiply (Numerator), / = Divide (Denominator)

    //==== Constructors ====
    /**
     * @brief Constructs an empty product node
     *
     * @note To construct from outside of hierarchy, use factories in @ref NodeFactory
     */
    Product();

    /**
     * @brief Constructs an empty product node with a given parent.  This node is not added to the children list of the parent.
     *
     * @note To construct from outside of hierarchy, use factories in @ref NodeFactory
     *
     * @param parent parent node
     */
    explicit Product(std::shared_ptr<SubSystem> parent);

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
    Product(std::shared_ptr<SubSystem> parent, std::shared_ptr<Product> orig);

public:
    //====Getters/Setters====
    std::vector<bool> getInputOp() const;
    void setInputOp(const std::vector<bool> &inputOp);

    //====Factories====
    /**
     * @brief Creates a product node from a GraphML Description
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
    static std::shared_ptr<Product> createFromGraphML(int id, std::string name,
                                                      std::map<std::string, std::string> dataKeyValueMap,
                                                      std::shared_ptr<SubSystem> parent, GraphMLDialect dialect);

    //==== Emit Functions ====
    std::set<GraphMLParameter> graphMLParameters() override;

    xercesc::DOMElement* emitGraphML(xercesc::DOMDocument* doc, xercesc::DOMElement* graphNode, bool include_block_node_type = true) override ;

    std::string labelStr() override ;

    void validate() override ;

    /**
     * @brief Emits a C expression for the product
     *
     * For more than 2 inputs, the size of the intermediate project can be ambiguous.  It could be the same as the output or some other value.
     * It is only important when overflow is possible
     *
     * For now, the C emit take a semi-conserative approach:
     *   - For floating point, all arguments are cast to the highest resolution floating point type of the inputs.  The result is then cast to the type of the output
     *   - For integer or fixed point, the most conservative type for a product of all inputs is calculated (assuming full bit growth).  The CPU type of this conservative type is calculated and the inputs are expanded to the width of the CPU type type without shifting.  The products are then taken of these new integers.  The position of the binary point is the sum of the fractional bits of all of the inputs (no shifting durring multiplication is required).  The result is then cast to the output type (shifted, cast, masked, and sign extended if signed)
     *
     * @note The computation of complex multiplies of > 2 inputs gets much more complex due to the mixing of real and imagionry components into the calculation.  The number of extra bits added to account for this intermixing in the intermediate is 2*(number of inputs -1). Complex multiply is handled by the recursive emitting a multieply 2 expression.
     *
     * For 2 input sums, a different approach is taken for integers:
     *   - If the output type is the same as the first input, then the second input is converted to the type of the first input (but not masked) and is multiplied.  The result is masked (& sign exteded if signed) to the output type if not a perfect CPU type.  No further casting is required.
     *   - If the output type is the most conservative type that fits the product (sum of total bits and sum of fractional bits), then both inputs are cast to that type (but are not shifted) and are multipled.  The binary point is calculated by taking the sum of their fractional bits.  No masking or casting is required for the output
     *   - For complex*complex multiplication, the output may either be sized to the type that fits the product or the size that fits the product + 1 bit for the internal sum.  The output is inspected to determine which is expected.  If the smaller one is present, then the output is down cast & masked / sign extended if appropriate
     *
     * @note Masking can be (is) omitted (in the case of the output having the same type as the first input) if the total number of bits is a standard CPU type.
     *
     */
    CExpr emitCExpr(std::vector<std::string> &cStatementQueue, int outputPortNum, bool imag = false) override;
};

/*@}*/

#endif //VITIS_PRODUCT_H
