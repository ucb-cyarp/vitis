//
// Created by Christopher Yarp on 3/31/21.
//

#ifndef VITIS_FIFO_H
#define VITIS_FIFO_H

#include "PrimitiveNode.h"
#include "GraphCore/NodeFactory.h"

/**
 * \addtogroup PrimitiveNodes Primitives
 * @{
*/

/**
 * @brief Represents an explicit FIFO in the design.  This FIFO may be converted to another node type during the
 * synthesis process, such as to a ThreadCrossingFIFO if the designed FIFO crosses a partition boundary.  Designed
 * FIFOs do not necessarily need to exist between partitions and can be wholly contained within a single partition.
 * FIFOs can act to decouple
 */
class FIFO : public PrimitiveNode{
    friend NodeFactory;

private:
    bool rebufferAllowed;
    bool flexibleSize;
    int fifoLen;


    //==== Constructors ====
    /**
     * @brief Constructs an empty FIFO node
     *
     * @note To construct from outside of hierarchy, use factories in @ref NodeFactory
     */
    FIFO();

    /**
     * @brief Constructs an empty FIFO node with a given parent.  This node is not added to the children list of the parent.
     *
     * @note To construct from outside of hierarchy, use factories in @ref NodeFactory
     *
     * @param parent parent node
     */
    explicit FIFO(std::shared_ptr<SubSystem> parent);

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
    FIFO(std::shared_ptr<SubSystem> parent, FIFO* orig);

    //====Getters/Setters====


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
    static std::shared_ptr<FIFO> createFromGraphML(int id, std::string name,
                                                   std::map<std::string, std::string> dataKeyValueMap,
                                                   std::shared_ptr<SubSystem> parent, GraphMLDialect dialect);

};


/*! @} */

#endif //VITIS_FIFO_H
