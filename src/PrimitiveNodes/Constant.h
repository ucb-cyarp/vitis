//
// Created by Christopher Yarp on 7/16/18.
//

#ifndef VITIS_CONSTANT_H
#define VITIS_CONSTANT_H

#include "PrimitiveNode.h"
#include "GraphCore/NodeFactory.h"
#include "GraphCore/NumericValue.h"
#include "GraphMLTools/GraphMLDialect.h"

#include <map>
#include <string>
#include <vector>

/**
 * \addtogroup PrimitiveNodes Primitives
 * @{
*/

/**
 * @brief Represents a block storing a constant value
 */
class Constant : public PrimitiveNode{
    friend NodeFactory;

private:
    std::vector<NumericValue> value; ///<The value of this constant block (can be a vector of values).  Uses the dimension of the output port to determine if this is a scalar, vector or matrix.  Constants in the vector should be supplied in C/C++ memory order
    int subBlockingLength; ///<The sub-blocking length

    //==== Constructors ====
    /**
     * @brief Constructs a default constant node with no value (an empty vector)
     *
     * @note To construct from outside of hierarchy, use factories in @ref NodeFactory
     *
     */
    Constant();

    /**
     * @brief Constructs a default constant node with no value (an empty vector) with a given parent.
     *
     * @note To construct from outside of hierarchy, use factories in @ref NodeFactory
     *
     * @param parent parent node
     */
    explicit Constant(std::shared_ptr<SubSystem> parent);

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
    Constant(std::shared_ptr<SubSystem> parent, Constant* orig);

public:
    //==== Getters/Setters ====
    std::vector<NumericValue> getValue() const;
    void setValue(const std::vector<NumericValue> &values);
    int getSubBlockingLength() const;
    void setSubBlockingLength(int subBlockingLength);

    //==== Factories ====
    /**
     * @brief Creates a sum node from a GraphML Description
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
    static std::shared_ptr<Constant> createFromGraphML(int id, std::string name,
                                                       std::map<std::string, std::string> dataKeyValueMap,
                                                       std::shared_ptr<SubSystem> parent, GraphMLDialect dialect);

    //==== Emit Functions ====
    std::set<GraphMLParameter> graphMLParameters() override;

    xercesc::DOMElement* emitGraphML(xercesc::DOMDocument* doc, xercesc::DOMElement* graphNode, bool include_block_node_type = true) override ;

    std::string typeNameStr() override;

    std::string labelStr() override ;

    void validate() override;

    std::shared_ptr<Node> shallowClone(std::shared_ptr<SubSystem> parent) override;

    CExpr emitCExpr(std::vector<std::string> &cStatementQueue, SchedParams::SchedType schedType, int outputPortNum,
                        bool imag = false) override;

    /**
     * @brief Constants do not need to check for fanout.  There is is no savings to storing the constant in a temporary variable if it is used
     * more than once.  Putting the number directly in the code allows instructions with immediate operands to be used.
     */
    CExpr
    emitC(std::vector<std::string> &cStatementQueue, SchedParams::SchedType schedType, int outputPortNum, bool imag,
              bool checkFanout, bool forceFanout) override;

    bool hasGlobalDecl() override;

    std::string getGlobalDecl() override;

    void specializeForBlocking(int localBlockingLength,
                               int localSubBlockingLength,
                               std::vector<std::shared_ptr<Node>> &nodesToAdd,
                               std::vector<std::shared_ptr<Node>> &nodesToRemove,
                               std::vector<std::shared_ptr<Arc>> &arcsToAdd,
                               std::vector<std::shared_ptr<Arc>> &arcsToRemove,
                               std::vector<std::shared_ptr<Node>> &nodesToRemoveFromTopLevel,
                               std::map<std::shared_ptr<Arc>, std::tuple<int, int, bool, bool>>
                                   &arcsWithDeferredBlockingExpansion) override;

    bool specializesForBlocking() override;

};

/*! @} */

#endif //VITIS_CONSTANT_H
