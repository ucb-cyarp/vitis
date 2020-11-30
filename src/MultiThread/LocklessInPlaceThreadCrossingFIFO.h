//
// Created by Christopher Yarp on 11/28/20.
//

#ifndef VITIS_LOCKLESSINPLACETHREADCROSSINGFIFO_H
#define VITIS_LOCKLESSINPLACETHREADCROSSINGFIFO_H

#include "LocklessThreadCrossingFIFO.h"

/**
 * \addtogroup MultiThread Multi-Thread Support
 * @{
 */

/**
 * @brief This is a modification to the Lockless Thread Crossing FIFO where data is operated on in place instead of
 * being copied to/from local buffers which are used for computation.
 *
 * One reason to do this is that it should make it easier to overlap communication and computation because, when samples
 * are accessed in the block loop, the referenced value is the value in the shared buffer.  If speculative execution is
 * working, the hope is that the next element will be requested before it is actually needed, hiding the latency if
 * a new cache line needs to be transferred.  On the writer side, it is hoped that speculative execution will allow
 * computation to run ahead even if a store requires acquiring exclusive access to a given cache line.  It should be noted
 * that after bringing in a cache line (on the reader side) or acquiring exclusive access (on the writer side), the
 * data resides in the L1 cache and further read/write actions in that line should be fast.
 *
 * One change that needs to be made for this approach is when checks against the FIFO state are made and when they are
 * updated.  Also, when "reading/writing" the FIFO, only a pointer to the current block is returned.  No other action
 * is performed.  The logic for determining of the FIFO is empty/full and for updating the FIFO state remains the same.
 */
class LocklessInPlaceThreadCrossingFIFO : public LocklessThreadCrossingFIFO {
    friend NodeFactory;

protected:
    //==== Constructors ====
    /**
     * @brief Constructs an empty Lockless In Place ThreadCrossing FIFO node
     *
     * @note To construct from outside of hierarchy, use factories in @ref NodeFactory
     */
    LocklessInPlaceThreadCrossingFIFO();

    /**
     * @brief Constructs an empty Lockless In Place ThreadCrossing FIFO node with a given parent.  This node is not added to the children list of the parent.
     *
     * @note To construct from outside of hierarchy, use factories in @ref NodeFactory
     *
     * @param parent parent node
     */
    explicit LocklessInPlaceThreadCrossingFIFO(std::shared_ptr<SubSystem> parent);

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
    LocklessInPlaceThreadCrossingFIFO(std::shared_ptr<SubSystem> parent, LocklessInPlaceThreadCrossingFIFO *orig);

public:
    bool isInPlace() override;

    std::string emitCWriteToFIFO(std::vector<std::string> &cStatementQueue, std::string src, int numBlocks, Role roll, bool pushStateAfter, bool forceNotInPlace) override;

    std::string emitCReadFromFIFO(std::vector<std::string> &cStatementQueue, std::string dst, int numBlocks, Role roll, bool pushStateAfter, bool forceNotInPlace) override;

};

/*! @} */

#endif //VITIS_LOCKLESSINPLACETHREADCROSSINGFIFO_H
