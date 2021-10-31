//
// Created by Christopher Yarp on 10/18/21.
//

#ifndef VITIS_BLOCKINGDOMAINBRIDGE_H
#define VITIS_BLOCKINGDOMAINBRIDGE_H

#include "GraphCore/Node.h"
#include "GraphCore/NodeFactory.h"

/**
 * @brief Provides a similar function as a rate adapting FIFO between nodes in different base blocking domains.
 *
 * Has a lot of shared logic with ThreadCrossingFIFOs.  Is simpler in that it only contains temporary state for a single
 * block and no capacity checking is performed.
 *
 * Allows signals to enter/exit BlockingDomains and ClockDomains without the typical boundary nodes (in the same way
 * a ThreadCrossingFIFO does.
 *
 */
class BlockingDomainBridge : public Node {
    friend class NodeFactory;

protected:
    int blockSize; ///<The block sizes (in items) of transactions to the bridge.
    //Note, input and output should have the same block size.  However, they may (will) have different sub-block sizes
    int subBlockSizeIn; ///<The sub-block size (in items) of transactions.  This is how many elements are accessed at a time.  This should be the size of the outer dimension of the input and output type.  If the subBlockSizes are 1, the I/O dimension will have 1 fewer dimension
    int baseSubBlockSizeIn; ///<The sub-block size of the nodes at the input of the bridge

    int subBlockSizeOut; ///<The sub-block size (in items) of transactions.  This is how many elements are accessed at a time.  This should be the size of the outer dimension of the input and output type.  If the subBlockSizes are 1, the I/O dimension will have 1 fewer dimension
    int baseSubBlockSizeOut; ///<The sub-block sizes of the nodes at the output of the bridge.

    //==== Constructors ====
    /**
     * @brief Constructs an empty BlockingDomainBridge FIFO node
     *
     * @note To construct from outside of hierarchy, use factories in @ref NodeFactory
     */
    BlockingDomainBridge();

    /**
     * @brief Constructs an empty BlockingDomainBridge FIFO node with a given parent.  This node is not added to the children list of the parent.
     *
     * @note To construct from outside of hierarchy, use factories in @ref NodeFactory
     *
     * @param parent parent node
     */
    explicit BlockingDomainBridge(std::shared_ptr<SubSystem> parent);

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
     * @param orig The original node from which a shallow copy is being made
     */
    BlockingDomainBridge(std::shared_ptr<SubSystem> parent, BlockingDomainBridge *orig);

    //====Getters/Setters====
public:
    int getBlockSize() const;
    void setBlockSize(int blockSize);
    int getSubBlockSizeIn() const;
    void setSubBlockSizeIn(int subBlockSizeIn);
    int getBaseSubBlockSizeIn() const;
    void setBaseSubBlockSizeIn(int baseSubBlockSizeIn);
    int getSubBlockSizeOut() const;
    void setSubBlockSizeOut(int subBlockSizeOut);
    int getBaseSubBlockSizeOut() const;
    void setBaseSubBlockSizeOut(int baseSubBlockSizeOut);

    //==== Emit Functions ====
    void validate() override;
    std::set<GraphMLParameter> graphMLParameters() override;

    bool canExpand() override;

    xercesc::DOMElement* emitGraphML(xercesc::DOMDocument* doc, xercesc::DOMElement* graphNode, bool include_block_node_type = true) override ;

    std::shared_ptr<Node> shallowClone(std::shared_ptr<SubSystem> parent) override;

    std::string typeNameStr() override;



};


#endif //VITIS_BLOCKINGDOMAINBRIDGE_H
