//
// Created by Christopher Yarp on 2018-12-17.
//

#ifndef VITIS_CONTEXTVARIABLEUPDATE_H
#define VITIS_CONTEXTVARIABLEUPDATE_H

#include "Node.h"
//#include "SubSystem.h"
//#include "ContextRoot.h"
//#include "NodeFactory.h"
#include <memory>

//Forward Decl
class SubSystem;
class ContextRoot;
class ContextRoot;

/**
* \addtogroup GraphCore Graph Core
* @{
*/

/**
 * @brief A node that represents a Context Variable Update
 *
 * The node is similar to the StateUpdate node in that it is a node that is typically not included in a DSP design.
 * It's presence is primarily to make it easier when emitting contexts.
 *
 * A primary use of this node is to serve as an assignment statement generator for the outputs of contexts.  For
 * example, Muxs have a single output that is assigned in each of the contexts.  Instead of checking for the last
 * statement emitted in each context (and checking that it should modify a context variable), a ContextVariableUpdate
 * node is inserted between the Mux input and the node feeding it.
 *
 * Each ContextVariableUpdate Node contains a reference to the ContextRoot to which it is attached.  When it's cEmit function
 * is called, it queries the ContextRoot for the appropriate variable and instantiates the assignment.
 *
 */
class ContextVariableUpdate : public Node{
    friend NodeFactory;

private:
    std::shared_ptr<ContextRoot> contextRoot; ///<The ContextRoot
    int contextVarIndex; ///<The index of the Context Variable to update (queried from context root)

    /**
     * @brief Constructs a default ContextVariableUpdate node with no contextRoot
     *
     * @note To construct from outside of hierarchy, use factories in @ref NodeFactory
     *
     */
    ContextVariableUpdate();

    /**
     * @brief Constructs a default ContextVariableUpdate node with no contextRoot, with a given parent.
     *
     * @note To construct from outside of hierarchy, use factories in @ref NodeFactory
     *
     * @param parent parent node
     */
    explicit ContextVariableUpdate(std::shared_ptr<SubSystem> parent);

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
    ContextVariableUpdate(std::shared_ptr<SubSystem> parent, ContextVariableUpdate* orig);

public:
    //Getters & Setters
    std::shared_ptr<ContextRoot> getContextRoot() const;
    void setContextRoot(const std::shared_ptr<ContextRoot>&contextRoot);
    int getContextVarIndex() const;
    void setContextVarIndex(int contextVarIndex);

    //Note: Currently is not exported as part of the XML
    //TODO: Implement XML Export/Import
    std::set<GraphMLParameter> graphMLParameters() override;
    xercesc::DOMElement* emitGraphML(xercesc::DOMDocument* doc, xercesc::DOMElement* graphNode, bool include_block_node_type = true) override ;

    bool canExpand() override;
    void validate() override;

    /**
     * @note Clone function does not set the reference for the contextRoot node.  This needs to be done after all nodes
     * have been copied since we must be sure that the primary node was copied
     */
    std::shared_ptr<Node> shallowClone(std::shared_ptr<SubSystem> parent) override;

    /**
     * @brief Emits the C Statement for the ContextVariableUpdate
     *
     * The input is assigned to a temporary variable which is then passed to the output.  Before that, the ContextVariable
     * is assigned to the temporary.
     *
     */
    CExpr emitCExpr(std::vector<std::string> &cStatementQueue, SchedParams::SchedType schedType, int outputPortNum, bool imag) override;

    std::string typeNameStr() override;

    std::string labelStr() override ;
};

/*! @} */

#endif //VITIS_CONTEXTVARIABLEUPDATE_H
