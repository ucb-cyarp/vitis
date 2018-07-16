//
// Created by Christopher Yarp on 7/16/18.
//

#ifndef VITIS_GAIN_H
#define VITIS_GAIN_H

#include "GraphCore/Node.h"
#include "GraphCore/NodeFactory.h"
#include "GraphCore/SubSystem.h"
#include "GraphCore/NumericValue.h"
#include "GraphMLTools/GraphMLDialect.h"

#include <vector>
#include <map>
#include <string>

/**
 * \addtogroup PrimitiveNodes Primitives
 */
/*@{*/

/**
 * @brief Represents a gain (multiply by constant) block
 */
class Gain : public Node{
    friend NodeFactory;

private:
    std::vector<NumericValue> gain; ///<The gain to multiply the signal by (may be a vector)

    //==== Constructors ====
    /**
     * @brief Constructs a gain node with no value
     */
    Gain();

    /**
     * @brief Constructs a gain node with no value and a given parent.  This node is not added to the children list of the parent.
     *
     * @note To construct from outside of hierarchy, use factories in @ref NodeFactory
     *
     * @param parent parent node
     */
    explicit Gain(std::shared_ptr<SubSystem> parent);

public:
    //==== Getters/Setters ====
    std::vector<NumericValue> getGain() const;
    void setGain(const std::vector<NumericValue> &gain);

    //==== Factories ====
    /**
     * @brief Creates a gain node from a GraphML Description
     *
     * @note This function does not add the node to the design or to the nodeID/pointer map
     *
     * @param id the ID number of the node
     * @param dataKeyValueMap A map of property keys and values extracted from the data nodes in the GraphML
     * @param parent The parent of this node in the hierarchy
     * @param dialect The dialect of the GraphML file being imported
     * @return a pointer to the new delay node
     */
    static std::shared_ptr<Gain> createFromSimulinkGraphML(int id, std::string name, std::map<std::string, std::string> dataKeyValueMap, std::shared_ptr<SubSystem> parent, GraphMLDialect dialect);

    //==== Emit Functions ====
    std::set<GraphMLParameter> graphMLParameters() override;

    xercesc::DOMElement* emitGraphML(xercesc::DOMDocument* doc, xercesc::DOMElement* graphNode, bool include_block_node_type = true) override ;

    std::string labelStr() override ;

};

/*@}*/

#endif //VITIS_GAIN_H
