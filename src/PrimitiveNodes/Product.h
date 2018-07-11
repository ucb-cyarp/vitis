//
// Created by Christopher Yarp on 7/6/18.
//

#ifndef VITIS_PRODUCT_H
#define VITIS_PRODUCT_H

#include <vector>
#include <memory>
#include <map>
#include <string>

#include "GraphCore/Node.h"

/**
 * \addtogroup PrimitiveNodes Primitives
 */
/*@{*/

/**
 * @brief Represents a Product Block.  Parameters dictate the position of the inputs.
 */
class Product : public Node{
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
     * @brief Creates a product node from a Simulink GraphML Description
     *
     * @note This function does not add the node to the design or to the nodeID/pointer map
     *
     * @param id the ID number of the node
     * @param dataKeyValueMap A map of property keys and values extracted from the data nodes in the GraphML
     * @param parent The parent of this node in the hierarchy
     * @return a pointer to the new delay node
     */
    static std::shared_ptr<Product> createFromSimulinkGraphML(int id, std::map<std::string, std::string> dataKeyValueMap, std::shared_ptr<SubSystem> parent);
};

/*@}*/

#endif //VITIS_PRODUCT_H