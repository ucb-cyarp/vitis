//
// Created by Christopher Yarp on 7/24/18.
//

#ifndef VITIS_VECTORFANOUT_H
#define VITIS_VECTORFANOUT_H

#include "VectorFan.h"
#include "GraphCore/NodeFactory.h"

/**
 * \addtogroup BusNodes Bus Nodes
 */
/*@{*/

/**
 * @brief A an adapter between standard arcs and bus arcs where a single bus arc is fanned-out into multiple standard arcs
 *
 * No additional state variable are required beyond what is provided by node.  The bus is connected to input port 0.
 * Individual standard arcs arc connected to output ports.  The port number specifies the position of the standard arc in the bus.
 */
class VectorFanOut : public VectorFan{
    friend NodeFactory;

private:
    //==== Constructors ====
    /**
     * @brief Constructs an empty VectorFanOut node
     *
     * @note To construct from outside of hierarchy, use factories in @ref NodeFactory
     */
    VectorFanOut();

    /**
     * @brief Constructs an empty VectorFanOut node with a given parent.  This node is not added to the children list of the parent.
     *
     * @note To construct from outside of hierarchy, use factories in @ref NodeFactory
     *
     * @param parent parent node
     */
    explicit VectorFanOut(std::shared_ptr<SubSystem> parent);

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
    VectorFanOut(std::shared_ptr<SubSystem> parent, VectorFanOut* orig);

public:
    //====Factories====
    /**
     * @brief Creates a VectorFanOut node from a GraphML Description
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
    static std::shared_ptr<VectorFanOut> createFromGraphML(int id, std::string name,
                                                          std::map<std::string, std::string> dataKeyValueMap,
                                                          std::shared_ptr<SubSystem> parent, GraphMLDialect dialect);

    //==== Emit Functions ====
    std::set<GraphMLParameter> graphMLParameters() override;

    xercesc::DOMElement* emitGraphML(xercesc::DOMDocument* doc, xercesc::DOMElement* graphNode, bool include_block_node_type = true) override ;

    std::string labelStr() override ;

    void validate() override ;

    std::shared_ptr<Node> shallowClone(std::shared_ptr<SubSystem> parent) override;

};

/*@}*/

#endif //VITIS_VECTORFANOUT_H
