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
};

/*@}*/

#endif //VITIS_PRODUCT_H
