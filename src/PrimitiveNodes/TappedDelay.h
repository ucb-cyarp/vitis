//
// Created by Christopher Yarp on 2019-03-15.
//

#ifndef VITIS_TAPPEDDELAY_H
#define VITIS_TAPPEDDELAY_H

#include "Delay.h"
#include "GraphMLTools/GraphMLDialect.h"
#include "GraphCore/NodeFactory.h"

/**
 * \addtogroup PrimitiveNodes Primitives
 * @{
*/

/**
 * @brief This represents a TappedDelay line.  It is an extension of the Delay class as it reveals the internal delay buffer
 *
 * @note Note that allocateExtraSpace is the parameter for passing the current value through.  It's name is because the functionality to allocate space for the current value is in the base Delay class but it unused for standard Delays
 *       It is recorded as IncludeCurrent in the exported XML for TappedDelay
 *
 * @note This used to be considered a high level node which expanded to indevidual delays and a concatenate.  It was
 * re-classified as a primitive type because it can also be viewed as an extension of the primitive node, Delay.
 */
class TappedDelay : public Delay {
    friend NodeFactory;

private:
    //Because this is an extension of the delay class (basically just revealing the whole buffer and inserting the current element if requested)
    //most of the logic, and relevant parameters, are actually declared in the delay class and are set to a default value when imported from
    //simulink.  This class sets these additional parameters to defaults when importing the base Delay class (even though the Delay class
    //has logic for dealing with these parameters).  They are saved and imported in the TappedDelay class since its configuration drives
    //these parameters

    //Note that allocateExtraSpace is used to pass the current value

    //If includeUndelayed, increase the allocated state by 1 to reserve a position for the current value
    //If includeUndelayed, specify that there is a combo path
    //Need to keep track if if this has been emitted before, if not, populate the current value (if includeUndelayed)
    //           Note that we will get the input expression twice, once for cEmit and once for nextState
    //           Declare the input as having internalFanout if includeUndelayed
    //Modify Delay class to allow for reserving an extra element in the array (for the current value if includeUndelayed)
    //           Allow changing direction.
    //           Standard delay will have default values.
    //Returned from emit will be the state variable

    //TODO: For now, tapped delay must have a delay value of at least 1

    /**
     * @brief Constructs a TappedDelay line with 0 delay.
     *
     * @note To construct from outside of hierarchy, use factories in @ref NodeFactory
     */
    TappedDelay();

    /**
     * @brief Constructs a TappedDelay line with 0 delay and a specified parent
     *
     * @note To construct from outside of hierarchy, use factories in @ref NodeFactory
     *
     * @param parent parent node
     */
    explicit TappedDelay(std::shared_ptr<SubSystem> parent);

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
    TappedDelay(std::shared_ptr<SubSystem> parent, TappedDelay *orig);

public:
    //==== Factories ====
    /**
     * @brief Creates a DiscreteFIR node from a GraphML Description
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
    static std::shared_ptr<TappedDelay> createFromGraphML(int id, std::string name,
                                                          std::map<std::string, std::string> dataKeyValueMap,
                                                          std::shared_ptr<SubSystem> parent, GraphMLDialect dialect);

    //==== Emit Functions ====
    bool hasCombinationalPath() override;

    CExpr emitCExpr(std::vector<std::string> &cStatementQueue, SchedParams::SchedType schedType, int outputPortNum,
                    bool imag) override;

    //Declars the parameters
    std::set<GraphMLParameter> graphMLParameters() override;

    xercesc::DOMElement *
    emitGraphML(xercesc::DOMDocument *doc, xercesc::DOMElement *graphNode, bool include_block_node_type) override;

    std::string typeNameStr() override;

    std::string labelStr() override;

    void validate() override;

    std::shared_ptr<Node> shallowClone(std::shared_ptr<SubSystem> parent) override;

    bool hasInternalFanout(int inputPort, bool imag) override;

    void propagateProperties() override;

    std::vector<NumericValue> getExportableInitConds() override;

    //The following functions are only overriden for oldest first circular buffer implementions
    //In this case, the cicular buffer pointer becomes loacated to the insertion point in the array.  It is initialized to be in the middle of the array.  Indexing is performed by subtracting from the pointer and does not handle the wraparound well when not double buffered
    //If delayLen-1, it is positioned at delayLen-1
    void emitCStateUpdate(std::vector<std::string> &cStatementQueue, SchedParams::SchedType schedType,
                          std::shared_ptr<StateUpdate> stateUpdateSrc) override;

    void incrementAndWrapCircularBufferOffset(std::vector<std::string> &cStatementQueue) override;

    int getCircBufferInitialIdx() override;
};

/*! @} */

#endif //VITIS_TAPPEDDELAY_H
