//
// Created by Christopher Yarp on 5/7/20.
//

#ifndef VITIS_DUMMYREPLICA_H
#define VITIS_DUMMYREPLICA_H

#include "SubSystem.h"
#include "GraphMLTools/GraphMLDialect.h"
#include "NodeFactory.h"

class ContextRoot;

/**
* \addtogroup GraphCore Graph Core
* @{
*/

/**
 * @brief This node serves as a dummy node.  It is primarily used for context driver replication.
 *
 * The point of context driver replication is to re-compute the logic driving Contexts across different partitions.
 * However, if the replicated driver arcs are all wired to the original ContectRoot, FIFOs will be automatically inserted
 * between the partitions.  To combat this, a dummy node representing the ContextRoot in another partition is inserted.
 * The dummy node is encapsulated like the context root durring encapsulation and is scheduled, just like the context root
 */

//TODO: Add field to context root about replicas <done>
//TODO: Add dummys to import logic <done>
//TODO: Add dummys to clone logic including re-populating the ContextRoot dummy field <done>
//TODO: Add dummys to contextDriver replication.  Can add the arcs to it and have <done>
//TODO: Add dummys to ContextFamilyContainer <done>
//TODO: Add to encapsulation to encapsulate replicas <done>
//TODO: Add to scheduler to schedule replicas like ContextRoots <done>
//TODO: Change scheduler to schedule ClockDomain context roots because driver is to the ClockDomain and not to the rate change node (like in EnabledSubsystem) <done>

class DummyReplica : public Node {
friend NodeFactory;

    std::shared_ptr<ContextRoot> dummyOf;///<The node that this is a dummy of

    /**
     * @brief Constructs a default dummy node
     */
    DummyReplica();

    /**
     * @brief Constructs a default dummy node with a given parent.
     *
     * @param parent parent node
     */
    explicit DummyReplica(std::shared_ptr<SubSystem> parent);

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
    DummyReplica(std::shared_ptr<SubSystem> parent, Node* orig);

public:
    std::shared_ptr<ContextRoot> getDummyOf() const;
    void setDummyOf(const std::shared_ptr<ContextRoot> &dummyOf);

    bool canExpand() override;

    //==== Factories ====
    /**
     * @brief Creates a dummy node from a GraphML Description
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
    static std::shared_ptr<DummyReplica> createFromGraphML(int id, std::string name,
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

};

/*! @} */

#endif //VITIS_DUMMYREPLICA_H
