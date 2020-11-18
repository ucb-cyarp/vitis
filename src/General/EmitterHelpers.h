//
// Created by Christopher Yarp on 10/4/19.
//

#ifndef VITIS_EMITTERHELPERS_H
#define VITIS_EMITTERHELPERS_H

#include <vector>
#include <set>
#include <memory>
#include <map>
#include "GraphCore/SchedParams.h"
#include "GraphCore/Variable.h"
#include "GraphCore/Context.h"
#include "GeneralHelper.h"

#define VITIS_TYPE_NAME "vitisTypes"

//Forward declare
class Node;
class Arc;
class ContextRoot;
class BlackBox;
class MasterOutput;
class MasterInput;

/**
 * \addtogroup General General Helper Classes
 *
 * @brief A set of general helper classes.
 * @{
*/

/**
 * @brief Contains helper methods for Emitters
 */
namespace EmitterHelpers {
    std::vector<std::shared_ptr<Node>> findNodesWithState(std::vector<std::shared_ptr<Node>> &nodesToSearch);

    std::vector<std::shared_ptr<Node>> findNodesWithGlobalDecl(std::vector<std::shared_ptr<Node>> &nodesToSearch);

    std::vector<std::shared_ptr<ContextRoot>> findContextRoots(std::vector<std::shared_ptr<Node>> &nodesToSearch);

    std::vector<std::shared_ptr<BlackBox>> findBlackBoxes(std::vector<std::shared_ptr<Node>> &nodesToSearch);

    /**
     * @brief Emits operators for the given nodes (in the order given).  Does not emit function prototypes.  This emitter is context aware
     * @param cFile the cFile to emit to
     * @param schedType the scheduler being used.  Is passed to downstream context emit functions
     * @param orderedNodes the nodes to emit, given in the order they should be emitted
     * @param outputMaster a pointer to the output master of the design being emitted
     * @param blockSize the size of the block (in samples) that are processed in each call to the function
     * @param indVarName the variable that specifies the index in the block that is being computed
     * @param checkForPartitionChange if true, checks if the partition changes while emitting and throws an error if it does
     */
    void emitOpsStateUpdateContext(std::ofstream &cFile, SchedParams::SchedType schedType, std::vector<std::shared_ptr<Node>> orderedNodes, std::shared_ptr<MasterOutput> outputMaster, int blockSize = 1, std::string indVarName = "", bool checkForPartitionChange = true);

    /**
     * @brief A helper function for emitting a single node
     *
     * Outputs are checked to see if they are connected and only the connected outputs are emitted
     *
     * @param nodeToEmit
     * @param cFile
     * @param schedType
     */
    void emitNode(std::shared_ptr<Node> nodeToEmit, std::ofstream &cFile, SchedParams::SchedType schedType);

    /**
     * @brief Get the input variables for this design
     *
     * The input names take the form: portName_portNum
     *
     * @warning Assumes the design has already been validated (ie. has at least one arc per port).
     *
     * @return a vector of input variables ordered by the input port number paired with their rate relative to the baseRate.  The ith element of the array is the ith input port.
     */
    std::pair<std::vector<Variable>, std::vector<std::pair<int, int>>> getCInputVariables(std::shared_ptr<MasterInput> inputMaster);

    /**
     * @brief Get the output variables for this design
     *
     * The input names take the form: portName_portNum
     *
     * @warning Assumes the design has already been validated (ie. has at exactly one arc per port).
     *
     * @return a vector of output variables ordered by the output port number paired with their rate relative to the baseRate.  The ith element of the array is the ith output port.
     */
    std::pair<std::vector<Variable>, std::vector<std::pair<int, int>>> getCOutputVariables(std::shared_ptr<MasterOutput> outputMaster);

    /**
     * @brief Get the structure definition for the Input/Output ports
     *
     * The struture definition takes the form of
     *
     * struct {
     *     type1 var1;
     *     type2 var2;
     *     ...
     * } structTypename;
     *
     * This is used by the driver generator.
     *
     * @param portVariables the port variables for the design (either input or output) (in accending port order)
     * @param portBlockSizes the block size (in samples) each port.  If the port is a scalar, it converted to a vector of this length.  If the port is a vector or matrix, a new dimension is added to the front of the dimensions array (so that blocks are stored contiguously in memory according to the C/C++ array semantics)
     * @param the type name of the struct being created
     *
     * @return
     */
    std::string getCIOPortStructDefn(std::vector<Variable> portVariables, std::vector<int> portBlockSizes, std::string structTypeName);

    /**
     * @brief Outputs the type header
     * @param path
     * @return the name of the type header
     */
    std::string stringEmitTypeHeader(std::string path);

    /**
     * @brief Emits a parameters file
     * @param path
     * @param filenamePrefix
     * @param blockSize
     * @returns the filename of the header file
     */
    std::string emitParametersHeader(std::string path, std::string fileNamePrefix, int blockSize);

    /**
     * @brief Emits a parameters file
     * @param path
     * @param filenamePrefix
     * @returns the filename of the header file
     */
    std::string emitTelemetryHelper(std::string path, std::string fileNamePrefix);

    /**
     * @brief Emits helper files for using the PAPI library for reading performance counters
     * @param path
     * @param filenamePrefix
     * @returns the filename of the header file
     */
    std::string emitPAPIHelper(std::string path, std::string fileNamePrefix);

    /**
     * @brief Transfers arcs from one node to another.
     *
     * The function re-wires existing arcs rather than creating new arcs.
     *
     * @warning This function only copies direct input/output and OrderConstraint input/output arcs
     *
     * A check is performed to make sure all arcs are moved.  If not all arcs are moved, it is likley that
     * other ports exist for the node
     *
     * @param from the node to transfer arcs from
     * @param to the node to transfer arcs to
     */
    void transferArcs(std::shared_ptr<Node> from, std::shared_ptr<Node> to);

    /**
     * @brief Finds any arcs connecting the given node to the MasterInput Node
     * @param node
     * @param directOnly if true, only the direct input arcs are searched
     * @return
     */
    std::set<std::shared_ptr<Arc>> getConnectionsToMasterInputNode(std::shared_ptr<Node> node, bool directOnly);

    /**
     * @brief Finds any arcs connecting the given node to MasterOutput Nodes
     * @param node
     * @param directOnly if true, only the direct input arcs are searched
     * @return
     */
    std::set<std::shared_ptr<Arc>> getConnectionsToMasterOutputNodes(std::shared_ptr<Node> node, bool directOnly);

    /**
     * @brief Get block sizes from rates
     * @param rates
     * @param blockSizeBase
     * @return
     */
    std::vector<int> getBlockSizesFromRates(const std::vector<std::pair<int, int>> &rates, int blockSizeBase);

    template<typename T>
    std::string arrayLiteralWorker(std::vector<int> &dimensions, int dimIndex, T val){
        std::string str = "";

        if(dimIndex >= dimensions.size()){
            //Base case, just emit the value, with no {}
            str = GeneralHelper::to_string(val);
        }else{
            str += "{";

            for(int i = 0; i<dimensions[dimIndex]; i++){
                if(i > 0){
                    str += ", ";
                }

                str += arrayLiteralWorker(dimensions, dimIndex+1, val);
            }

            str += "}";
        }

        return str;
    }

    template<typename T>
    std::string arrayLiteral(std::vector<int> &dimensions, T val){
        return arrayLiteralWorker(dimensions, 0, val);
    }

    template<typename T>
    //Val index is a reference so that it can be incremented in the base case
    std::string arrayLiteralWorker(std::vector<int> &dimensions, int dimIndex, std::vector<T> &val, int &valIndex){
        std::string str = "";

        if(dimIndex >= dimensions.size()){
            //Base case, just emit the value, with no {}
            str = GeneralHelper::to_string(val[valIndex]);
            valIndex++;
        }else{
            str += "{";

            for(int i = 0; i<dimensions[dimIndex]; i++){
                if(i > 0){
                    str += ", ";
                }

                str += arrayLiteralWorker(dimensions, dimIndex+1, val, valIndex);
            }

            str += "}";
        }

        return str;
    }

    template<typename T>
    std::string arrayLiteral(std::vector<int> &dimensions, std::vector<T> val){
        int idx = 0;
        return arrayLiteralWorker(dimensions, 0, val, idx);
    }

    //Val index is a reference so that it can be incremented in the base case
    std::string arrayLiteralWorker(std::vector<int> &dimensions, int dimIndex, std::vector<NumericValue> &val, int &valIndex, bool imag, DataType valType, DataType storageType);

    std::string arrayLiteral(std::vector<int> &dimensions, std::vector<NumericValue> val, bool imag, DataType valType, DataType storageType);

    /**
     * @brief Generates nested for loops to be used when emitting element wise vector/matrix operations
     * @param dimensions
     * @return a tuple containing, the nested for loop declarations in the order of declaration (outer to inner), the index variables used for each successive for loop - starting with the outer most first, and finally the closings for the nested for loops
     */
    std::tuple<std::vector<std::string>, std::vector<std::string>, std::vector<std::string>> generateVectorMatrixForLoops(const std::vector<int>& dimensions);

    /**
     * @brief Generates a C/C++ style array indexing operation
     * @param index
     * @return a string with the index variables sandwidtched between square brackets
     */
    template<typename T>
    std::string generateIndexOperation(const std::vector<T>& index){
        std::string str = "";

        for(T i : index){
            str += "[" + GeneralHelper::to_string(i) + "]";
        }

        return str;
    }

    /**
     * @brief Generates a C/C++ style array indexing operation without the final dereferencing
     * @param index
     * @return a string with the index variables sandwidtched between square brackets except for the last one where the "+" operator is used
     */
    template<typename T>
    std::string generateIndexOperationWODereference(const std::vector<T>& index){
        std::string str = "";

        for(int i = 0; i<(((int) index.size())-1); i++){
            str += "[" + GeneralHelper::to_string(index[i]) + "]";
        }
        if(!index.empty()){
            str += "+" + GeneralHelper::to_string(index[index.size()-1]);
        }

        return str;
    }

    /**
     * @brief Converts a memory index (like the index of a linear vector) to the index of a C/C++ array occupying the same memory space
     * @param idx
     * @param dimensions the dimensions of the array
     * @return
     */
    std::vector<int> memIdx2ArrayIdx(int idx, std::vector<int> dimensions);

    /**
     * @brief Emits the logic for opening or closing a context.  This is done during partition emit and uses the current
     * context as well as the previously emitted context to determine if contexts should be closed and/or opened before
     * the node nodeContext
     *
     * @param schedType
     * @param contextFirst
     * @param alreadyEmittedSubContexts
     * @param subContextEmittedCount
     * @param partition the partition being emitted
     * @param nodeContext the context of the node to be emitted next
     * @param lastEmittedContext the context of the last emitted node
     * @param contextStatements the context open/close statements to emit
     */
    void emitCloseOpenContext(const SchedParams::SchedType &schedType, std::vector<bool> &contextFirst,
                              std::set<Context> &alreadyEmittedSubContexts,
                              std::map<std::shared_ptr<ContextRoot>, int> &subContextEmittedCount, int partition,
                              const std::vector<Context> &nodeContext, std::vector<Context> &lastEmittedContext,
                              std::vector<std::string> &contextStatements);
};

/*! @} */


#endif //VITIS_EMITTERHELPERS_H
