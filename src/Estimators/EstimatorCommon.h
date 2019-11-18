//
// Created by Christopher Yarp on 10/9/19.
//

#ifndef VITIS_ESTIMATORCOMMON_H
#define VITIS_ESTIMATORCOMMON_H

#include <map>
#include <set>
#include <typeindex>

/**
 * \addtogroup Estimators Estimators
 * @{
*/

class EstimatorCommon {
public:
    //Operators

    /**
     * @brief Indicates whether or not the operands were floating point or integer.
     */
    enum class OperandType{
        INT, ///< Integer type
        FLOAT ///< Floating point type
    };

    static std::string operandTypeToString(OperandType operandType);

    /**
     * @brief Identifies an operation represented by a node in the graph.
     *
     * It includes the node type itself, the type of operands (int or float),
     * the width of each operand (the maximum across all, in bits), the number of real inputs
     * and the number of complex inputs
     */
    struct NodeOperation{
        std::type_index nodeType; ///<The node type of the operation
        OperandType operandType; ///<The type of operands (int or float)
        int operandBits; ///<The maximum number of bits per operand
        int numRealInputs; ///<The number of real operands to the node
        int numCplxInputs; ///<The number of complex operands to the node

        //Need an explicit constructor for this
        NodeOperation(std::type_index nodeType, OperandType operandType, int operandBits, int numRealInputs, int numCplxInputs);

        bool operator==(const NodeOperation &rhs) const;
        bool operator!=(const NodeOperation &rhs) const;
        bool operator<(const NodeOperation &rhs) const;
        bool operator>(const NodeOperation &rhs) const;
        bool operator<=(const NodeOperation &rhs) const;
        bool operator>=(const NodeOperation &rhs) const;
    };

    class NodeOperationComparatorByName{
        std::map<std::type_index, std::string> nameMapping;
    public:

        explicit NodeOperationComparatorByName(std::map<std::type_index, std::string> nameMapping);

        bool comp(const NodeOperation &a, const NodeOperation &b) const;

        bool operator()(const NodeOperation &a, const NodeOperation &b) const;
    };

    /**
     * @brief Types of primitive operations
     */
    enum class PrimitiveOpType{
        ADD_SUB, ///<Add or Subtract Operation
        MULT, ///<Multiply operation
        DIV, ///<Divide operation
        BITWISE, ///<Bitwise operator
        LOGICAL, ///<Logical operator
        CAST, ///<Cast operator
        LOAD, ///<Memory load operator
        STORE, ///<Memory store operator
        BRANCH ///<Branch operator
    };

    /**
     * @brief Represents a primitive operation
     */
    struct PrimitiveOperation{
        PrimitiveOpType opType; ///<The type of primitive operation
        OperandType operandType; ///<The type of operands (
        int operandBits;
    };

    /**
     * @brief Represents a workload in terms of primitive operations
     */
    struct PrimitiveWorkload{
        std::map<PrimitiveOperation, int> operationCount;
    };

    //Communication

    /**
     * @brief Represents a communication workload between threads
     */
    struct InterThreadCommunicationWorkload{
        int numBytesPerSample; ///<The number of bytes per sample period that must be transfered from one partition to another
        int numBytesPerBlock; ///<The number of bytes per block.  This is typically the number of bytes per sample * the block size.
        int numFIFOs; ///<The number of FIFOs from one partition to another

        InterThreadCommunicationWorkload();
        InterThreadCommunicationWorkload(int numBytesPerSample, int numBytesPerBlock, int numFIFOs);
        //TODO: Modify if unequal block sizes are ever used.
    };

    //Hardware resources

    /**
     * @brief Represents a compute capability for a particular type of primitive operation
     */
    struct ComputeCapability{
        PrimitiveOperation operation; ///<The operation this capability is able to execute
        int pipelineDepth; ///<The depth of the pipeline, if known, when executing this type of operation
        double timeToCompleteNS; ///<The estimated time to complete an operation of this type
    };

    /**
     * @brief Represents the capabilities of an execution unit in terms of what operations it can execute
     */
    struct ExecutionUnit{
        std::set<ComputeCapability> computeCapabilities; ///<The compute capabilities provided by this execution unit
    };

    //TODO: Add communication capabilities for the computer hierarchy

};

/*! @} */

#endif //VITIS_ESTIMATORCOMMON_H
