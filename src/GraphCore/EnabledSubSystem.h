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

/**
 * \addtogroup GraphCore Graph Core
 */
/*@{*/

/**
 * @brief Represents a subsystem whose execution can be enabled or disabled via an enable line
 *
 * The enable line is passed to each of the EnableInput/Output node.  The same source should exist for each EnableInput/Output
 */
class EnabledSubSystem : public virtual SubSystem {
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
     * @brief Expands the enabled system context of theis node, then extends the context of decendent enabled subsystem
     */
    void extendEnabledSubsystemContext(std::vector<std::shared_ptr<Node>> &new_nodes,
                                       std::vector<std::shared_ptr<Node>> &deleted_nodes,
                                       std::vector<std::shared_ptr<Arc>> &new_arcs,
                                       std::vector<std::shared_ptr<Arc>> &deleted_arcs) override;
};

/*@}*/

#endif //VITIS_ENABLEDSUBSYSTEM_H
