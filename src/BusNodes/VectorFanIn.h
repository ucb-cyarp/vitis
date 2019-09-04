//
// Created by Christopher Yarp on 7/24/18.
//

#ifndef VITIS_VECTORFANIN_H
#define VITIS_VECTORFANIN_H

#include "VectorFan.h"
#include "GraphCore/NodeFactory.h"
#include "GraphMLTools/GraphMLDialect.h"

/**
 * \addtogroup BusNodes Bus Nodes
 * @{
*/

/**
 * @brief A an adapter between standard arcs and bus arcs where multiple standard arcs are grouped into a single bus arc (fan-in)
 *
 * No additional state variable are required beyond what is provided by node.  Each incoming arc is connected to
 * an input port.  The port number specifies the position of the standard arc in the bus.  The bus arc is connected
 * to output port 0.
 */
class VectorFanIn : public VectorFan {
    friend NodeFactory;

private:
    //==== Constructors ====
    /**
     * @brief Constructs an empty VectorFanIn node
     *
     * @note To construct from outside of hierarchy, use factories in @ref NodeFactory
     */
    VectorFanIn();

    /**
     * @brief Constructs an empty VectorFanIn node with a given parent.  This node is not added to the children list of the parent.
     *
     * @note To construct from outside of hierarchy, use factories in @ref NodeFactory
     *
     * @param parent parent node
     */
    explicit VectorFanIn(std::shared_ptr<SubSystem> parent);

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
    VectorFanIn(std::shared_ptr<SubSystem> parent, VectorFanIn* orig);

public:
    //====Factories====
    /**
     * @brief Creates a VectorFanIn node from a GraphML Description
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
    static std::shared_ptr<VectorFanIn> createFromGraphML(int id, std::string name,
                                                      std::map<std::string, std::string> dataKeyValueMap,
                                                      std::shared_ptr<SubSystem> parent, GraphMLDialect dialect);

    //==== Emit Functions ====
    std::set<GraphMLParameter> graphMLParameters() override;

    xercesc::DOMElement* emitGraphML(xercesc::DOMDocument* doc, xercesc::DOMElement* graphNode, bool include_block_node_type = true) override ;

    std::string labelStr() override ;

    void validate() override ;

    std::shared_ptr<Node> shallowClone(std::shared_ptr<SubSystem> parent) override;

};

/*! @} */

#endif //VITIS_VECTORFANIN_H
