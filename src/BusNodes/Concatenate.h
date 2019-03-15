//
// Created by Christopher Yarp on 2019-03-15.
//

#ifndef VITIS_CONCATENATE_H
#define VITIS_CONCATENATE_H

#include "GraphCore/Node.h"
#include "GraphCore/SubSystem.h"
#include "GraphMLTools/GraphMLDialect.h"
#include "GraphCore/NodeFactory.h"

/**
 * \addtogroup BusNodes Bus Nodes
 *
 * @brief Nodes which act on buses and convert to/from buses and standard arcs
 */
/*@{*/

/**
 * @brief An abstract class for Vector Concatenate objects
 *
 * Provides a factory function which parses Simulink Concatenate objects
 */
class Concatenate : public Node {
    friend NodeFactory;

private:
    /**
     * @brief Constructs an empty Concatenate node
     */
    Concatenate();

    /**
     * @brief Constructs an empty Concatenate node with a given parent.  This node is not added to the children list of the parent.
     *
     * @param parent parent node
     */
    explicit Concatenate(std::shared_ptr<SubSystem> parent);

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
    Concatenate(std::shared_ptr<SubSystem> parent, Concatenate* orig);

public:

    //====Factories====
    /**
     * @brief @brief Creates a VectorFan subclass node from a GraphML Description
     *
     * This factory exists to parse SimulinkExport VectorFan objects and to produce either a VectorFanIn or VectorFanOut object.
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
    static std::shared_ptr<Concatenate> createFromGraphML(int id, std::string name,
                                                        std::map<std::string, std::string> dataKeyValueMap,
                                                        std::shared_ptr<SubSystem> parent, GraphMLDialect dialect);

    //====Methods====
    bool canExpand() override;

    std::set<GraphMLParameter> graphMLParameters() override;

    xercesc::DOMElement* emitGraphML(xercesc::DOMDocument* doc, xercesc::DOMElement* graphNode, bool include_block_node_type = true) override ;

    std::string labelStr() override ;

    void validate() override ;

    std::shared_ptr<Node> shallowClone(std::shared_ptr<SubSystem> parent) override;

};


#endif //VITIS_CONCATENATE_H
