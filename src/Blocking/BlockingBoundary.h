//
// Created by Christopher Yarp on 8/24/21.
//

#ifndef VITIS_BLOCKINGBOUNDARY_H
#define VITIS_BLOCKINGBOUNDARY_H

#include "GraphCore/Node.h"
#include "GraphCore/NodeFactory.h"

/**
 * \addtogroup Blocking Blocking Support Nodes
 * @brief Nodes to support blocked sample processing
 *
 * @{
 */

/**
 * @brief Node representing data entering/exiting a blocking domain
 */
class BlockingBoundary : public Node {
    friend class NodeFactory;

protected:
    int blockingLen; ///< The number of samples at the boundary of this blocking domain
    int subBlockingLen; ///< The number of samples processed at once inside of the blocking domain (the increment for each loop of the blocking domain)

protected:
    /**
     * @brief Default constructor
     */
    BlockingBoundary();

    /**
     * @brief Construct a node with a given parent
     * @param parent parent of the new node
     */
    explicit BlockingBoundary(std::shared_ptr<SubSystem> parent);

    /**
     * @brief Constructs a new node with a shallow copy of parameters from the origional node.  Ports are not copied and neither is the parent reference.  This node is not added to the children list of the parent.
     *
     * @note To construct from outside of hierarchy, use factories in @ref NodeFactory
     *
     * @note If copying a graph, the parent should be one of the copies and not from the origional graph.
     *
     * @warning Because pointer (this) is passed to ports, nodes must be allocated on the heap and not moved.  All interaction should be via pointers.
     *
     * @param parent parent node
     * @param orig The origional node from which a shallow copy is being made
     */
    BlockingBoundary(std::shared_ptr<SubSystem> parent, BlockingBoundary* orig);

    /**
     * @brief Emit the common parameters for classes derived from BlockingBoundary
     * @param doc
     * @param graphNode Graph node that was created in a subclass of BlockingBoundary by calling emitGraphMLBasics
     */
    void emitBoundaryGraphMLParams(xercesc::DOMDocument* doc, xercesc::DOMElement* graphNode);

public:
    int getBlockingLen() const;
    void setBlockingLen(int blockingLen);
    int getSubBlockingLen() const;
    void setSubBlockingLen(int subBlockingLen);

    void validate() override;

    std::set<GraphMLParameter> graphMLParameters() override;
};

/*! @} */

#endif //VITIS_BLOCKINGBOUNDARY_H
