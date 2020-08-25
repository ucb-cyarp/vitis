//
// Created by Christopher Yarp on 10/9/19.
//

#ifndef VITIS_ESTIMATORCOMMON_H
#define VITIS_ESTIMATORCOMMON_H

#include <map>
#include <set>
#include <typeindex>

class Node;
class DataType;

/**
 * \addtogroup Estimators Estimators
 * @{
*/

namespace EstimatorCommon {
    //++++ Operators ++++

    /**
     * @brief Indicates whether or not the operands were floating point or integer.
     */
    enum class OperandType{
        INT, ///< Integer type
        FLOAT ///< Floating point type
    };

    std::string operandTypeToString(OperandType operandType);

    /**
     * @brief The complexity of operands
     */
    enum class OperandComplexity{
        REAL, ///<All operands are real
        COMPLEX, ///<All operands are complex
        MIXED ///<There is a mix of real and complex
    };

    std::string operandComplexityToString(OperandComplexity operandComplexity);

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
        int maxNumElements; ///<If a vector or matrix operation, the maximum number of elements in an input
        int scalarInputs; ///<Number of scalar inputs
        int vectorInputs; ///<Number of vector inputs
        int matrixInputs; ///<Number of matrix inputs (>1 dimension)

        //Need an explicit constructor for this
        NodeOperation(std::type_index nodeType, OperandType operandType, int operandBits, int numRealInputs,
                      int numCplxInputs, int maxNumElements, int scalarInputs, int vectorInputs, int matrixInputs);

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
     * @brief Types of operations
     *
     * Includes primitive ops (ex. mult, load, ...) and higher level ops (ex. exp, ln, sin, ...).
     * The function, opTypeToStr reports if the operator is primitive or higher level
     */
    enum class OpType{
        //Primitive Ops
        ADD_SUB, ///<Add or Subtract Operation
        MULT, ///<Multiply operation
        DIV, ///<Divide operation
        BITWISE, ///<Bitwise operator
        LOGICAL, ///<Logical operator
        CAST, ///<Cast operator
        LOAD, ///<Memory load operator
        STORE, ///<Memory store operator
        BRANCH, ///<Branch operator
        //Complex Ops (ex. Trig, Exp, Ln)
        EXP, ///<Exponent operator
        LN, ///<Log operator
        SIN, ///<Sin operator
        COS, ///<Cos operator
        TAN, ///<Tan operator
        ATAN, ///<ATan operator
        ATAN2, ///<ATan2 operator
        //Special
        REDUCE_ADD,
        //Unimplemented TODO: Remove when all operators implemented
        EST_UNIMPLEMENTED, ///<Estimation Unimplemented
        //Unknown
        EST_UNKNOWN ///<Estimation is unknown
    };

    std::string opTypeToStr(OpType opType);

    bool isPrimitiveOpType(OpType opType);

    /**
     * @brief Represents a compute operation
     *
     * Can be a real, primitive, operation, or a more higher level operation (such as complex mult, or
     */
    struct ComputeOperation{
        OpType opType; ///<The type of primitive operation
        OperandType operandType; ///<The type of operands
        OperandComplexity operandComplexity; ///<The complexity of the operands
        int operandBits; ///<The number of bits in the largest operand
        int vecLength; ///<For vector ops, the size of the vector.  If scalar, vector length is 1

        ComputeOperation();
        ComputeOperation(OpType opType, OperandType operandType, OperandComplexity operandComplexity,
                         int operandBits, int vecLength);

        //A version of the constructor which assumes operands are either int/floating point and real/complex
        ComputeOperation(OpType opType, bool isOperandFloatingPoint, bool isOperandComplex,
                         int operandBits, int vecLength);

        bool operator==(const ComputeOperation &rhs) const;
        bool operator!=(const ComputeOperation &rhs) const;
        bool operator<(const ComputeOperation &rhs) const;
        bool operator>(const ComputeOperation &rhs) const;
        bool operator<=(const ComputeOperation &rhs) const;
        bool operator>=(const ComputeOperation &rhs) const;
    };

    /**
     * @brief Represents a workload in terms of primitive operations
     */
    struct ComputeWorkload{
        std::map<ComputeOperation, int> operationCount;

        /**
         * @brief Adds an operation to the workload.  If it exists the count is incremented.  If not, it is added to the operation set
         * @param operation the operation to add to the workload
         */
        void addOperation(ComputeOperation operation);

        /**
         * @brief Add additional operations to this workload
         * @param moreOperations operations to add
         */
        void addOperations(std::map<ComputeOperation, int> &moreOperations);

        /**
         * @brief Add additional operations to this workload
         * @param moreOperations workload from which to add operations
         */
        void addOperations(ComputeWorkload &moreOperations);
    };

    /**
     * @brief Returns true if the given node has any input or output which is non-scalar
     * @param node
     * @return
     */
    bool containsNonScalarInputOrOutput(std::shared_ptr<Node> node);

    /**
     * @brief Returns the number of elements in the input in the
     * @param node
     * @return
     */
    int getLargestInputNumElements(std::shared_ptr<Node> node);

    /**
     * @brief Adds cast operations to the workload if the base types differ (does not compare the complexity of the data types)
     *
     * If the from datatype is complex, a complex cast is inserted.  If expandComplex, 2 casts are added.
     *
     * The type parameters are taken from the type being cast to
     *
     * @param workload
     * @param from
     * @param to
     * @param expandComplex
     * @param vecLen
     */
    void addCastsIfBaseTypesDifferent(ComputeWorkload &workload, DataType from, DataType to, bool expandComplex, int vecLen);

    //++++ Communication ++++

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

    //++++ Hardware resources ++++

    /**
     * @brief Represents a compute capability for a particular type of primitive operation
     */
    struct ComputeCapability{
        OpType operation; ///<The operation this capability is able to execute
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
