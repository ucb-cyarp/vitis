//
// Created by Christopher Yarp on 6/26/18.
//

#ifndef VITIS_ENABLEDSUBSYSTEM_H
#define VITIS_ENABLEDSUBSYSTEM_H

#include <memory>
#include <vector>
#include "SubSystem.h"
#include "EnablePort.h"
#include "EnableInput.h"
#include "EnableOutput.h"
#include "ContextRoot.h"

/**
 * \addtogroup GraphCore Graph Core
 */
/*@{*/

/**
 * @brief Represents a subsystem whose execution can be enabled or disabled via an enable line
 *
 * The enable line is passed to each of the EnableInput/Output node.  The same source should exist for each EnableInput/Output
 */
class EnabledSubSystem : public virtual SubSystem, public ContextRoot {
friend class NodeFactory;

protected:
    std::vector<std::shared_ptr<EnableInput>> enabledInputs; ///< A vector of pointers to the @ref EnableInput nodes for this @ref EnabledSubSystem
    std::vector<std::shared_ptr<EnableOutput>> enabledOutputs; ///< A vector of pointers to the @ref EnableOutput nodes for this @ref EnabledSubSystem

    /**
     * @brief Default constructor
     */
    EnabledSubSystem();

    /**
     * @brief Construct a node with a given parent.
     * @param parent parent of the new node
     */
    explicit EnabledSubSystem(std::shared_ptr<SubSystem> parent);

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
    EnabledSubSystem(std::shared_ptr<SubSystem> parent, EnabledSubSystem* orig);

    /**
     * @brief Performs check of @ref Node in addition to checking the enable port
     */
    void validate() override;

    std::string labelStr() override ;

public:
    void addEnableInput(std::shared_ptr<EnableInput> &input);
    void addEnableOutput(std::shared_ptr<EnableOutput> &output);

    void removeEnableInput(std::shared_ptr<EnableInput> &input);
    void removeEnableOutput(std::shared_ptr<EnableOutput> &output);

    std::vector<std::shared_ptr<EnableInput>> getEnableInputs() const;
    std::vector<std::shared_ptr<EnableOutput>> getEnableOutputs() const;

    /**
     * @brief Gets the port which is the source of the enable input to the EnableNodes directly contained within this enabled subsystem
     *
     * @note EnableSubsystem::validate should generally be run before calling this function.  It ensures that all EnableNodes have the same enable source.
     *
     * This method first checks the first EnableInput node to get the enable source, if that does not exist it checks the
     * first EnableOutput.
     *
     * @return the source port for the enable lines to the EnableNodes
     */
    std::shared_ptr<OutputPort> getEnableSrc();

    /**
     * @brief Gets an example arc which is the source of the enable input to the EnableNodes directly contained within this enabled subsystem
     *
     * @note EnableSubsystem::validate should generally be run before calling this function.  It ensures that all EnableNodes have the same enable source.
     *
     * This method first checks the first EnableInput node to get the enable source, if that does not exist it checks the
     * first EnableOutput.
     *
     * @return an example arc for the enable lines to the EnableNodes
     */
    std::shared_ptr<Arc> getEnableDriverArc();

    xercesc::DOMElement* emitGraphML(xercesc::DOMDocument* doc, xercesc::DOMElement* graphNode, bool include_block_node_type = true) override ;

    std::shared_ptr<Node> shallowClone(std::shared_ptr<SubSystem> parent) override;

    void shallowCloneWithChildren(std::shared_ptr<SubSystem> parent, std::vector<std::shared_ptr<Node>> &nodeCopies, std::map<std::shared_ptr<Node>, std::shared_ptr<Node>> &origToCopyNode, std::map<std::shared_ptr<Node>, std::shared_ptr<Node>> &copyToOrigNode) override;

    /**
     * @brief Extends the context of the enabled subsystem by pulling combinational nodes from outside the enabled subsystem (inputs) inside.
     *
     * The nodes pulled in are not depended upon outside of the enabled subsystem (on the input side) and do not depend on signals from outside of the enabled subsystem (on the output side)
     *
     * @param new_nodes A vector which will be filled with the new nodes created during expansion
     * @param deleted_nodes A vector which will be filled with the nodes deleted during expansion
     * @param new_arcs A vector which will be filled with the new arcs created during expansion
     * @param deleted_arcs A vector which will be filled with the arcs deleted during expansion
     */
    void extendContextInputs(std::vector<std::shared_ptr<Node>> &new_nodes,
                       std::vector<std::shared_ptr<Node>> &deleted_nodes,
                       std::vector<std::shared_ptr<Arc>> &new_arcs,
                       std::vector<std::shared_ptr<Arc>> &deleted_arcs);

    /**
     * @brief Extends the context of the enabled subsystem by pulling combinational nodes from outside the enabled subsystem (outputs) inside.
     *
     * The nodes pulled in are not depended upon outside of the enabled subsystem (on the input side) and do not depend on signals from outside of the enabled subsystem (on the output side)
     *
     * @param new_nodes A vector which will be filled with the new nodes created during expansion
     * @param deleted_nodes A vector which will be filled with the nodes deleted during expansion
     * @param new_arcs A vector which will be filled with the new arcs created during expansion
     * @param deleted_arcs A vector which will be filled with the arcs deleted during expansion
     */
    void extendContextOutputs(std::vector<std::shared_ptr<Node>> &new_nodes,
                             std::vector<std::shared_ptr<Node>> &deleted_nodes,
                             std::vector<std::shared_ptr<Arc>> &new_arcs,
                             std::vector<std::shared_ptr<Arc>> &deleted_arcs);

    /**
     * @brief Extends the context of the enabled subsystem by pulling combinational nodes from outside the enabled subsystem inside.
     *
     * The nodes pulled in are not depended upon outside of the enabled subsystem (on the input side) and do not depend on signals from outside of the enabled subsystem (on the output side)
     *
     * @param new_nodes A vector which will be filled with the new nodes created during expansion
     * @param deleted_nodes A vector which will be filled with the nodes deleted during expansion
     * @param new_arcs A vector which will be filled with the new arcs created during expansion
     * @param deleted_arcs A vector which will be filled with the arcs deleted during expansion
     */
    void extendContext(std::vector<std::shared_ptr<Node>> &new_nodes,
                       std::vector<std::shared_ptr<Node>> &deleted_nodes,
                       std::vector<std::shared_ptr<Arc>> &new_arcs,
                       std::vector<std::shared_ptr<Arc>> &deleted_arcs);


    /**
     * @brief Expands the enabled system context of this node, then extends the context of decendent enabled subsystem
     */
    void extendEnabledSubsystemContext(std::vector<std::shared_ptr<Node>> &new_nodes,
                                       std::vector<std::shared_ptr<Node>> &deleted_nodes,
                                       std::vector<std::shared_ptr<Arc>> &new_arcs,
                                       std::vector<std::shared_ptr<Arc>> &deleted_arcs) override;

    /**
     * @brief Discover and mark contexts for nodes at and within this enabled subsystem.
     *
     * Propogates enabled subsystem contexts to its nodes (and recursively to nested enabled subsystems).
     *
     * Finds mux contexts under the top level and under each enabled subsystem
     *     Muxes are searched within the top level and within any subsystem.
     *
     *     Once all the Muxes within the top level (or within the level of the enabled subsystem are discovered)
     *     their contexts are found.  For each mux, we sum up the number of contexts which contain that given mux.
     *     Hierarchy is discovered by tracing decending layers of the hierarchy tree based on the number of contexts
     *     a mux is in.
     *
     * Since contexts stop at enabled subsystems, the process begins again with any enabled subsystem within (after muxess
     * have been handled)
     *
     * @param contextStack the context stack up to this node
     * @return nodes in the context
     */
    std::vector<std::shared_ptr<Node>> discoverAndMarkContexts(std::vector<Context> contextStack);

    void orderConstrainZeroInputNodes(std::vector<std::shared_ptr<Node>> predecessorNodes,
                                      std::vector<std::shared_ptr<Node>> &new_nodes,
                                      std::vector<std::shared_ptr<Node>> &deleted_nodes,
                                      std::vector<std::shared_ptr<Arc>> &new_arcs,
                                      std::vector<std::shared_ptr<Arc>> &deleted_arcs) override;

    //==== Implement Context Root Functions ====

    //No additional variables need to be declared outside of the scope since the EnableOutputs are the gateway for all
    //outputs of the enable subsystem.

    std::vector<Variable> getCContextVars() override;
    Variable getCContextVar(int contextVarIndex) override;

    bool requiresContiguousContextEmits() override;

    void emitCContextOpenFirst(std::vector<std::string> &cStatementQueue, int subContextNumber) override;
    void emitCContextOpenMid(std::vector<std::string> &cStatementQueue, int subContextNumber) override;
    void emitCContextOpenLast(std::vector<std::string> &cStatementQueue, int subContextNumber) override;

    void emitCContextCloseFirst(std::vector<std::string> &cStatementQueue, int subContextNumber) override;
    void emitCContextCloseMid(std::vector<std::string> &cStatementQueue, int subContextNumber) override;
    void emitCContextCloseLast(std::vector<std::string> &cStatementQueue, int subContextNumber) override;

    //However, the EnableOutputs variables must be declared before the context is emitted.  It is important to recall
    // that enabled outputs are transparent latched and therefore contain a state variables that must be declared such
    // that their value persists between itterations of the design.  These declarations will be made when the getCStateVars
    // function is called.




    //No else method is nessisary for the enable subsystem since the EnableOutputs update instantaniously when enabled and pass the state variable directly.  The state can be updated instantly because this is a latch, not a delay - nothing depends on the previous value before it is updated - all nodes depend on the calculated value (immediatly).
        //This should be thought of more like a fanout variable that stores its value rather than the state found in a delay.
    //When not enabled, the state variable is not updated, and value is held for dependent nodes.  No 'else' is required to pass the previous value when the susbsytem is disabled.





};

/*@}*/

#endif //VITIS_ENABLEDSUBSYSTEM_H
