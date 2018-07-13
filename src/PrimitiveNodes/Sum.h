//
// Created by Christopher Yarp on 7/6/18.
//

#ifndef VITIS_SUM_H
#define VITIS_SUM_H

#include <vector>
#include <memory>
#include <map>
#include <string>

#include "GraphCore/Node.h"
#include "GraphMLTools/GraphMLDialect.h"

/**
 * \addtogroup PrimitiveNodes Primitives
 */
/*@{*/

/**
 * @brief Represents a Sum Block.  Parameters dictate the signs of the inputs.
 */
class Sum : public Node{
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

public:
    std::set<GraphMLParameter> graphMLParameters() override;

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
     * @param dataKeyValueMap A map of property keys and values extracted from the data nodes in the GraphML
     * @param parent The parent of this node in the hierarchy
     * @param dialect The dialect of the GraphML file being imported
     * @return a pointer to the new delay node
     */
    static std::shared_ptr<Sum> createFromSimulinkGraphML(int id, std::string name, std::map<std::string, std::string> dataKeyValueMap, std::shared_ptr<SubSystem> parent, GraphMLDialect dialect);

    xercesc::DOMElement* emitGraphML(xercesc::DOMDocument* doc, xercesc::DOMElement* graphNode, bool include_block_node_type = true) override ;

};

/*@}*/

#endif //VITIS_SUM_H
