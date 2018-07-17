//
// Created by Christopher Yarp on 7/16/18.
//

#ifndef VITIS_CONSTANT_H
#define VITIS_CONSTANT_H

#include "GraphCore/Node.h"
#include "GraphCore/NodeFactory.h"
#include "GraphCore/NumericValue.h"
#include "GraphMLTools/GraphMLDialect.h"

#include <map>
#include <string>
#include <vector>

/**
 * \addtogroup PrimitiveNodes Primitives
 */
/*@{*/

/**
 * @brief Represents a block storing a constant value
 */
class Constant : public Node{
    friend NodeFactory;

private:
    std::vector<NumericValue> value; ///<The value of this constant block (can be a vector of values)

    //==== Constructors ====
    /**
     * @brief Constructs a default constant node with no value (an empty vector)
     *
     * @note To construct from outside of hierarchy, use factories in @ref NodeFactory
     *
     */
    Constant();

    /**
     * @brief Constructs a default constant node with no value (an empty vector) with a given parent.
     *
     * @note To construct from outside of hierarchy, use factories in @ref NodeFactory
     *
     * @param parent parent node
     */
    explicit Constant(std::shared_ptr<SubSystem> parent);

public:
    //==== Getters/Setters ====
    std::vector<NumericValue> getValue() const;
    void setValue(const std::vector<NumericValue> &values);

    //==== Factories ====
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
    static std::shared_ptr<Constant> createFromGraphML(int id, std::string name,
                                                       std::map<std::string, std::string> dataKeyValueMap,
                                                       std::shared_ptr<SubSystem> parent, GraphMLDialect dialect);

    //==== Emit Functions ====
    std::set<GraphMLParameter> graphMLParameters() override;

    xercesc::DOMElement* emitGraphML(xercesc::DOMDocument* doc, xercesc::DOMElement* graphNode, bool include_block_node_type = true) override ;

    std::string labelStr() override ;

};

/*@}*/

#endif //VITIS_CONSTANT_H
