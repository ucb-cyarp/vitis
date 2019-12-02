//
// Created by Christopher Yarp on 9/3/19.
//

#ifndef VITIS_LOCKLESSTHREADCROSSINGFIFO_H
#define VITIS_LOCKLESSTHREADCROSSINGFIFO_H

#include "ThreadCrossingFIFO.h"

/**
 * \addtogroup MultiThread Multi-Thread Support
 * @{
 */

/**
 * @brief Implements a lockless thread-crossing single producer single consumer FIFO which is compatible with x86 based systems
 *
 * The allocated array contains 1 extra block to allow empty FIFOs to be distinguished from full FIFOs without
 * requiring additional shared state
 *
 * For an empty FIFO, it is initialized in the following way
 *
 * At the start, readOffset = 0 and writeOffset = 1 (one more than the read pointer)
 * This indicates that index 0 of the array has been read and index 1 is pending a write.
 * The FIFO is empty (writeOffset - readOffset = 1)
 * The number of elements available to read is (writeOffset - readOffset -1)
 * The number of elements available to write is (fifoLengthNotIncludingExtraEntry - elementsAvailable)
 *
 * x x _ _ _ _ _ (empty)
 * r w
 *
 * FIFO is full the write pointer overlaps the read pointer
 * The value at readOffset is the extra element since the read pointer indicates the position that was read last (and in
 * theory could be written to). By following this convention, the writeOffset will never pass the read pointer.
 *
 * 7 % 2 3 4 5 6 (full)
 *   r (already read %)
 *   w
 *
 * The wrinkle is the circular aspect of the array.  It is possible for the write pointer to have wrapped around the
 * end of the FIFO but for the read pointer to remain
 *
 * 7 8 % % 4 5 6 (5 entries in FIFO)
 *     w r
 *
 * 7 8 9 % 4 5 6 (6 entries in FIFO, full)
 *       r
 *       w
 *
 * Since our scheme guarentees that the write pointer will never pass the read pointer (though can meet it)
 * we can detect the write pointer having looped around and the read pointer not yet having looped around by
 * checking writeOffset <= readOffset
 *
 * Note that the FIFO cannot be in the full state unless the write pointer has
 *
 * ==== Cases for empty ====
 * Std:
 * _ _ _ _ _ _ _ (empty)
 * r w
 *
 * _ _ _ _ _ _ _ (empty)
 *           r w
 *
 * Boarder (only on 1 side):
 * _ _ _ _ _ _ _ (empty)
 * w           r
 * 0 1 2 3 4 5 6 (6 element fifo, array 7 long)
 *
 * Reverse (not possible)
 *
 * Empty = (writeOffset-readOffset == 1) || (writeOffset-readOffset==-(arrayLength-1)) : this is because the maximum negative distance between the pointers occurs when the FIFO is empty on the boarder)
 * Empty = (writeOffset-readOffset == 1) || (writeOffset==0 && readOffset==(arrayLength-1))
 *
 * ==== Cases for full ====
 * % 1 2 3 4 5 6 (full)
 * r
 * w
 *
 * 7 8 % 3 4 5 6 (full)
 *     r
 *     w
 *
 * 7 8 9 a b c % (full)
 *             r
 *             w
 *
 * Full = (readOffset == writeOffset)
 *
 * ==== Entries Available to read ====
 * % 1 2 3 _ _ _ ((readOffset < writeOffset) : writeOffset - readOffset - 1 = 3)
 * r       w
 *
 * 7 _ _ % 4 5 6 ((readOffset >= writeOffset) : arrayLength - (readOffset - writeOffset + 1) = 4) | arrayLength includes the extra entry
 *   w   r
 *
 * 7 8 9 % 4 5 6 ((readOffset >= writeOffset) : arrayLength - (readOffset - writeOffset + 1) = 6)
 *       r
 *       w
 *
 * _ % _ _ _ _ _ ((readOffset < writeOffset) : writeOffset - readOffset - 1 = 0)
 *   r w
 *
 * _ _ _ _ _ _ _ ((readOffset >= writeOffset) : arrayLength - (readOffset - writeOffset + 1) = 0)
 * w           r
 *
 * Avail to read: (readOffset < writeOffset) ? writeOffset - readOffset - 1 : arrayLength - (readOffset - writeOffset + 1)
 * Avail to read: (readOffset < writeOffset) ? writeOffset - readOffset - 1 : arrayLength - readOffset + writeOffset - 1
 *
 * ==== Space Left ====
 * Note, the max space availible (when the array is empty) is arrayLength-1
 *
 * % 1 2 3 _ _ _ ((readOffset < writeOffset) : arrayLength - 1 - (writeOffset - readOffset - 1) = arrayLength - 1 - writeOffset + readOffset + 1 = arrayLength - writeOffset + readOffset = 3) arrayLength includes the extra entry
 * r       w
 *
 * 7 _ _ % 4 5 6 ((readOffset >= writeOffset) : (arrayLength - 1) - (arrayLength - (readOffset - writeOffset + 1)) = (arrayLength - 1) - (arrayLength - readOffset + writeOffset - 1) = (arrayLength - 1) - arrayLength + readOffset - writeOffset + 1) = readOffset - writeOffset = 2)
 *   w   r
 *
 * 7 8 9 % 4 5 6 ((readOffset >= writeOffset) : readOffset - writeOffset = 0)
 *       r
 *       w
 *
 * _ % _ _ _ _ _ ((readOffset < writeOffset) : arrayLength - writeOffset + readOffset = 6)
 *   r w
 *
 * _ _ _ _ _ _ _ ((readOffset >= writeOffset) : readOffset - writeOffset = 6)
 * w           r
 *
 * Space Left: (readOffset < writeOffset) ? arrayLength - writeOffset + readOffset : readOffset - writeOffset
 *
 * This logic is also on pointers to blocks rather than indevidual items
 */
class LocklessThreadCrossingFIFO : public ThreadCrossingFIFO {
    friend NodeFactory;

protected:
    //All the ports are bundled in a structure.  Only one set of underlying shared FIFO variables are needed
    Variable cWriteOffsetPtr; ///<The C variable corresponding to the write pointer offset (in blocks)
    Variable cReadOffsetPtr; ///<The C variable corresponding to the read pointer offset (in blocks)
    Variable cArrayPtr; ///<The C variable corresponding to the FIFO array

    Variable cWriteOffsetCached; ///<The C variable corresponding to the cached value of the write offset (in blocks)
    Variable cReadOffsetCached; ///<The C variable corresponding to the cached value of the read offset (in blocks)

    bool cWriteOffsetPtrInitialized;
    bool cReadOffsetPtrInitialized;
    bool cArrayPtrInitialized;

    bool cWriteOffsetCachedInitialized;
    bool cReadOffsetCachedInitialized;

    //==== Constructors ====
    /**
     * @brief Constructs an empty Lockless ThreadCrossing FIFO node
     *
     * @note To construct from outside of hierarchy, use factories in @ref NodeFactory
     */
    LocklessThreadCrossingFIFO();

    /**
     * @brief Constructs an empty Lockless ThreadCrossing FIFO node with a given parent.  This node is not added to the children list of the parent.
     *
     * @note To construct from outside of hierarchy, use factories in @ref NodeFactory
     *
     * @param parent parent node
     */
    explicit LocklessThreadCrossingFIFO(std::shared_ptr<SubSystem> parent);

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
    LocklessThreadCrossingFIFO(std::shared_ptr<SubSystem> parent, LocklessThreadCrossingFIFO *orig);

    //====Getters/Setters====
public:
    /**
     * @brief Gets the cWriteOffsetPtr for this FIFO.  If it has not yet been initialized, it will be initialized at this point
     *
     * @note This function is used internally whenever accessing the object to ensure it is initialized
     *
     * @return the initialized cWriteOffsetPtr for this FIFO
     */
    Variable getCWriteOffsetPtr();

    /**
     * @brief Gets the cReadOffsetPtr for this FIFO.  If it has not yet been initialized, it will be initialized at this point
     *
     * @note This function is used internally whenever accessing the object to ensure it is initialized
     *
     * @return the initialized cWriteOffsetPtr for this FIFO
     */
    Variable getCReadOffsetPtr();

    /**
     * @brief Gets the cArrayPtr for this FIFO.  If it has not yet been initialized, it will be initialized at this point
     *
     * @note This function is used internally whenever accessing the object to ensure it is initialized
     *
     * @return the initialized cArrayPtr for this FIFO
     */
    Variable getCArrayPtr();

    /**
     * @brief Gets the cWriteOffsetCached for this FIFO.  If it has not yet been initialized, it will be initialized at this point
     *
     * @note This function is used internally whenever accessing the object to ensure it is initialized
     *
     * @return the initialized cWriteOffsetPtr for this FIFO
     */
    Variable getCWriteOffsetCached();

    /**
     * @brief Gets the cReadOffsetCached for this FIFO.  If it has not yet been initialized, it will be initialized at this point
     *
     * @note This function is used internally whenever accessing the object to ensure it is initialized
     *
     * @return the initialized cWriteOffsetPtr for this FIFO
     */
    Variable getCReadOffsetCached();

    //====Factory====
    //This uses a function of the abstract class to
    static std::shared_ptr<LocklessThreadCrossingFIFO> createFromGraphML(int id, std::string name,
                                                    std::map<std::string, std::string> dataKeyValueMap,
                                                    std::shared_ptr<SubSystem> parent, GraphMLDialect dialect);

    //====Emit Functions====
    std::set<GraphMLParameter> graphMLParameters() override;

    //This uses a function in the abstract class to populates state of abstract class
    xercesc::DOMElement* emitGraphML(xercesc::DOMDocument* doc, xercesc::DOMElement* graphNode, bool include_block_node_type) override ;

    std::shared_ptr<Node> shallowClone(std::shared_ptr<SubSystem> parent) override;

    std::string typeNameStr() override;

    std::string labelStr() override;

    void validate() override;

    //==== FIFO Implementation Functions ====

    std::string emitCIsNotEmpty(std::vector<std::string> &cStatementQueue, Roll roll) override;

    std::string emitCIsNotFull(std::vector<std::string> &cStatementQueue, Roll roll) override;

    std::string emitCNumBlocksAvailToRead(std::vector<std::string> &cStatementQueue, Roll roll) override;

    std::string emitCNumBlocksAvailToWrite(std::vector<std::string> &cStatementQueue, Roll roll) override;

    void emitCWriteToFIFO(std::vector<std::string> &cStatementQueue, std::string src, int numBlocks, Roll roll, bool pushStateAfter) override;

    void emitCReadFromFIFO(std::vector<std::string> &cStatementQueue, std::string dst, int numBlocks, Roll roll, bool pushStateAfter) override;

    std::vector<std::pair<Variable, std::string>> getFIFOSharedVariables() override;

    void createSharedVariables(std::vector<std::string> &cStatementQueue, int core) override;

    void cleanupSharedVariables(std::vector<std::string> &cStatementQueue) override ;

    void createLocalVars(std::vector<std::string> &cStatementQueue) override; //Creates local variables to be used

    void initLocalVars(std::vector<std::string> &cStatementQueue, Roll roll) override; //Initializes the local variables declared in createLocalVars

    void pullLocalVars(std::vector<std::string> &cStatementQueue, Roll roll) override; //Loads the local variables based on the roll (producer/consumer)

    void pushLocalVars(std::vector<std::string> &cStatementQueue, Roll roll) override; //Writes the local variables based on the roll (producer/consumer)

    /**
     * @brief Initializes the shared array with the FIFO's initial values and sets the initial read and write indexes
     *
     * Note that the array as implemented is oversized by one element.  Initial elements are placed starting at index [1]
     *
     * The read pointer is initialized to index 0 (indicating element [1] is to be read next).
     * If the FIFO contains no initial conditions, the write pointer is initialized to index 1 (indicating element [1] is to be written next)
     *
     * If the FIFO has n initial conditions, the write pointer is moved forward by n elements (will be placed at index (n+1) mod arrayLength)
     *
     * @note The number of initial elements must not be larger than the size of the FIFO, or equivalently larger than the arrayLength-1.
     *
     * @param cStatementQueue where the C statements will be emitted
     */
    void initializeSharedVariables(std::vector<std::string> &cStatementQueue) override ;
};

/*! @} */


#endif //VITIS_LOCKLESSTHREADCROSSINGFIFO_H
