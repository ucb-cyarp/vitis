//
// Created by Christopher Yarp on 7/29/20.
//

#ifndef VITIS_INNERPRODUCT_H
#define VITIS_INNERPRODUCT_H

#include "PrimitiveNode.h"
#include "GraphMLTools/GraphMLDialect.h"
#include "GraphCore/NodeFactory.h"

/**
 * \addtogroup PrimitiveNodes Primitives
 * @{
*/

/**
 * @brief This represents the Inner Product (or Dot Product for real inputs) operation on 2 vectors.
 * For complex vectors, the complex conjugate of the first input is taken by default.  This is in line with
 * the convention followed by Matlab and "Statistical Digital Signal Processing and Modeling" by Monson H. Hayes.
 *
 * If different behavior is desired, it can be set via the
 */
class InnerProduct : public PrimitiveNode {
    friend NodeFactory;
public:
    enum class ComplexConjBehavior{
        FIRST, ///< Take the complex conjugate of the first term (port 0)
        SECOND, ///< Take the complex conjugate of the second term (port 1)
        NONE ///< Do not take the complex conjugate of either term
    };

    static std::string complexConjBehaviorToString(ComplexConjBehavior complexConjBehavior);
    static ComplexConjBehavior parseComplexConjBehavior(std::string string);

private:
    ComplexConjBehavior complexConjBehavior; ///<Controls how complex conjugates are taken.  The default is to take the complex conjugate of the first which follows the Matlab convention.  None is helpful for convolution where neither term has the complex conjugate taken.  Correlation does use the complex conjugation
    bool emittedBefore; ///<Tracks if this InnerProduct has been emitted before (used in emit - not a parameter to save to XML)

    //==== Constructors ====
    /**
     * @brief Constructs an empty InnerProduct node
     *
     * @note To construct from outside of hierarchy, use factories in @ref NodeFactory
     */
    InnerProduct();

    /**
     * @brief Constructs an empty InnerProduct node with a given parent.  This node is not added to the children list of the parent.
     *
     * @note To construct from outside of hierarchy, use factories in @ref NodeFactory
     *
     * @param parent parent node
     */
    explicit InnerProduct(std::shared_ptr<SubSystem> parent);

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
    InnerProduct(std::shared_ptr<SubSystem> parent, InnerProduct* orig);

public:
    //====Getters/Setters====
    ComplexConjBehavior getComplexConjBehavior() const;
    void setComplexConjBehavior(ComplexConjBehavior complexConjBehavior);

    //====Factories====
    /**
     * @brief Creates a InnerProduct node from a GraphML Description
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
    static std::shared_ptr<InnerProduct> createFromGraphML(int id, std::string name,
                                                  std::map<std::string, std::string> dataKeyValueMap,
                                                  std::shared_ptr<SubSystem> parent, GraphMLDialect dialect);

    //==== Emit Functions ====
    std::set<GraphMLParameter> graphMLParameters() override;

    xercesc::DOMElement* emitGraphML(xercesc::DOMDocument* doc, xercesc::DOMElement* graphNode, bool include_block_node_type = true) override ;

    std::string typeNameStr() override;

    std::string labelStr() override ;

    void validate() override ;

    std::shared_ptr<Node> shallowClone(std::shared_ptr<SubSystem> parent) override;

    bool hasInternalFanout(int inputPort, bool imag) override;

    /**
     * @brief Emits a C expression for the InnerProduct
     *
     * @note if the first input is complex, it's complex conjugate is taken
     *
     */
    CExpr emitCExpr(std::vector<std::string> &cStatementQueue, SchedParams::SchedType schedType, int outputPortNum,
                    bool imag = false) override;
};

/*! @} */


#endif //VITIS_INNERPRODUCT_H
