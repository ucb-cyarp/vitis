//
// Created by Christopher Yarp on 4/20/20.
//

#ifndef VITIS_CLOCKDOMAIN_H
#define VITIS_CLOCKDOMAIN_H

#include "GraphCore/SubSystem.h"
#include "GraphCore/Design.h"
#include <set>
#include <memory>

//Forward Decls
class RateChange;

/**
 * \addtogroup MultiRate Multi-Rate Support Nodes
 * @brief Nodes for multi-rate support in Vitis designs
 *
 * @{
*/

/**
 * @brief Represents a Clock Domain
 */
class ClockDomain : public SubSystem{
friend class NodeFactory;

protected:
    int upsampleRatio; ///<The upsample ratio compared to the parent clock domain
    int downsampleRatio; ///<The downsample ratio compared to the parent clock domain
    std::set<std::shared_ptr<RateChange>> rateChangeIn; ///<The set of rate change nodes coming into this clock domain
    std::set<std::shared_ptr<RateChange>> rateChangeOut; ///<The set of rate change nodes coming out of this clock domain
    std::set<std::shared_ptr<OutputPort>> ioInput; ///<The set of I/O ports that directly connect to the inputs of nodes in this clock domain.  These should be MasterInput Ports
    std::set<std::shared_ptr<InputPort>> ioOutput; ///<The set of I/O ports that directly connect to the outputs of nodes in this clock domain.  These should be MasterOutput Ports

    std::set<int> suppressClockDomainLogicForPartitions; ///<A set of partitions where it was determined that the only nodes are in this clock domain and no rate change nodes are included.  The clock domain counter logic is suppressed for these partitions and is handled by adjusting the compute outer loop
    bool useVectorSamplingMode; ///<If true, uses vector sampling instead of a counter and conditional to implement the clock domain.  Set by inspecting rate change nodes

    /**
     * @brief Default constructor
     */
    ClockDomain();

    /**
     * @brief Construct a node with a given parent.
     * @param parent parent of the new node
     */
    explicit ClockDomain(std::shared_ptr<SubSystem> parent);

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
    ClockDomain(std::shared_ptr<SubSystem> parent, ClockDomain* orig);

    /**
     * @brief This is a work function for shallowCloneWithChildren functionality in clockDomains.
     *
     * This does the bulk of the work for shallowCloneWithChildren and is seperated because the clone method for
     * the subclasses of clock domain is the same except for
     *
     * @param parent
     * @param nodeCopies
     * @param origToCopyNode
     * @param copyToOrigNode
     */
    void shallowCloneWithChildrenWork(std::shared_ptr<ClockDomain> clonedNode, std::vector<std::shared_ptr<Node>> &nodeCopies, std::map<std::shared_ptr<Node>, std::shared_ptr<Node>> &origToCopyNode, std::map<std::shared_ptr<Node>, std::shared_ptr<Node>> &copyToOrigNode);

public:
    //==== Getters & Setters ====
    int getUpsampleRatio() const;
    void setUpsampleRatio(int upsampleRatio);
    int getDownsampleRatio() const;
    void setDownsampleRatio(int downsampleRatio);
    std::set<std::shared_ptr<RateChange>> getRateChangeIn() const;
    void setRateChangeIn(const std::set<std::shared_ptr<RateChange>> &rateChangeIn);
    std::set<std::shared_ptr<RateChange>> getRateChangeOut() const;
    void setRateChangeOut(const std::set<std::shared_ptr<RateChange>> &rateChangeOut);
    std::set<std::shared_ptr<OutputPort>> getIoInput() const;
    void setIoInput(const std::set<std::shared_ptr<OutputPort>> &ioInput);
    std::set<std::shared_ptr<InputPort>> getIoOutput() const;
    void setIoOutput(const std::set<std::shared_ptr<InputPort>> &ioOutput);

    void addRateChangeIn(std::shared_ptr<RateChange> rateChange);
    void addRateChangeOut(std::shared_ptr<RateChange> rateChange);
    void addIOInput(std::shared_ptr<OutputPort> input);
    void addIOOutput(std::shared_ptr<InputPort> output);

    void removeRateChangeIn(std::shared_ptr<RateChange> rateChange);
    void removeRateChangeOut(std::shared_ptr<RateChange> rateChange);
    //Remove from both Input and Output lists if type is unknown
    void removeRateChange(std::shared_ptr<RateChange> rateChange);
    void removeIOInput(std::shared_ptr<OutputPort> input);
    void removeIOOutput(std::shared_ptr<InputPort> output);

    std::set<int> getSuppressClockDomainLogicForPartitions() const;
    void setSuppressClockDomainLogicForPartitions(const std::set<int> &suppressClockDomainLogicForPartitions);
    void addClockDomainLogicSuppressedPartition(int partitionNum);

    bool isUsingVectorSamplingMode() const;
    void setUseVectorSamplingMode(bool useVectorSamplingMode);

    /**
     * @brief Indicates if a variable which tracks how many times the given clock domain has been executed
     *        in the given function call (processing a given block) is required.
     *
     *        This is typically needed for clock domains that cannot operate in vector mode but have
     *        a FIFO within them.  Ie. the clock domain may be a factor of the global block size but
     *        not of the sub-block size.  In this case, the index into the FIFO needs to be determined
     *        by how many times this domain has executed in the
     *
     *        The variable in question will be reset to 0 during each function call invocation
     * @return
     */
    bool requiresDeclaringExecutionCount();

    Variable getExecutionCountVariable(int blockSizeBase);

    /**
     * @brief Sets the use vector sampling mode and propagates to clock domain inputs and outputs
     *
     * If setting to false, also removes clock domain driver node
     *
     * @param useVectorSamplingMode
     */
    void setUseVectorSamplingModeAndPropagateToRateChangeNodes(bool useVectorSamplingMode,
                                                               std::set<std::shared_ptr<Node>> &nodesToRemove,
                                                               std::set<std::shared_ptr<Arc>> &arcsToRemove);

    /**
     * @brief Copy parameters from another clock domain except for the lists of RateChange nodes
     *
     * This is used when specializing a clock domain
     *
     * @param orig the clock domain to copy from
     */
    virtual void populateParametersExceptRateChangeNodes(std::shared_ptr<ClockDomain> orig);

    /**
     * @brief Check if the clock domain has been specialized (ie. has been converted to an Updample
     * @return
     */
    virtual bool isSpecialized();

    /**
     * @brief Discover and mark contexts for nodes at and within this ClockDomain
     *
     * This is a very similar operation to EnabledSubsystem::discoverAndMarkContexts
     *
     * @note This function will error out if called on the general ClockDomain as it is not a ContextRoot itself
     *
     * @param contextStack the context stack up to this node
     * @return nodes in the context
     */
    virtual std::vector<std::shared_ptr<Node>> discoverAndMarkContexts(std::vector<Context> contextStack);

    /**
     * @brief Get the rate of this clock domain relative to the base rate
     * @return The rate of this clock domain vs. the base rate in the form <numerator, denominator>
     */
    std::pair<int, int> getRateRelativeToBase();

    /**
     * @brief Converts the ClockDomain into either an UpsampleClockDomain or a DownsampleClockDomain.  These are
     * specialized clock domains with different implementations.  The UpsampleClockDomain is implemented as an inner
     * loop while the DownsampleClockDomain is implemented like an EnabledSubsystem driven by a counter.
     *
     * Because the DifferentDomains are different classes, a new node of the appropriate type is created, and all sub
     * nodes are moved within this new node.  As part of this process, all rateChange blocks will be converted to
     * their implementation specific versions.  Their references to the clock domain will be updated.
     *
     * @warning: After conversion, any references to the old node should be converted to the new node.  If provided as
     * a reference, the node will be removed from the design.
     *
     * @warning: This method does not change the references in the context stacks of child nodes.  This function should be run before
     * context discovery and marking
     *
     * @param convertToInput if true, the node is converted to an input.  if false, the node is converted to an output
     * @return a pointer to the newly created node
     */
    virtual std::shared_ptr<ClockDomain> convertToUpsampleDownsampleDomain(bool convertToUpsampleDomain,
                                                                           std::vector<std::shared_ptr<Node>> &nodesToAdd,
                                                                           std::vector<std::shared_ptr<Node>> &nodesToRemove,
                                                                           std::vector<std::shared_ptr<Arc>> &arcsToAdd,
                                                                           std::vector<std::shared_ptr<Arc>> &arcToRemove);

    /**
     * @brief Specializes the ClockDomain to a DownsampleClockDomain or an UpsampleClockDomain
     *
     * @note this should be called after discoverClockDomainParameters
     *
     * @param nodesToAdd
     * @param nodesToRemove
     * @param arcsToAdd
     * @param arcToRemove
     * @return
     */
    std::shared_ptr<ClockDomain> specializeClockDomain(std::vector<std::shared_ptr<Node>> &nodesToAdd,
                                                       std::vector<std::shared_ptr<Node>> &nodesToRemove,
                                                       std::vector<std::shared_ptr<Arc>> &arcsToAdd,
                                                       std::vector<std::shared_ptr<Arc>> &arcToRemove);

    /**
     * @brief Searches for rate change nodes and IO input/output ports connected to this clock domain.  Also sets the
     * clock domain of any ports connected to the clock domain.
     *
     * This includes discovering which rate change blocks are inputs and which are outputs.  It also sets the upsample/
     * downsample ratio based on the rate change blocks
     */
    void discoverClockDomainParameters();


    void validate() override;
    std::string typeNameStr() override;

    xercesc::DOMElement* emitGraphML(xercesc::DOMDocument* doc, xercesc::DOMElement* graphNode, bool include_block_node_type) override ;
    std::shared_ptr<Node> shallowClone(std::shared_ptr<SubSystem> parent) override;

    /**
     * @brief Clones the Clock Domain and its children.  Sets the RateChange node references.
     *
     * @note Does not set IO ports or MasterPort rates - these are handled in the design clone function.
     *
     * @param parent
     * @param nodeCopies
     * @param origToCopyNode
     * @param copyToOrigNode
     */
    void shallowCloneWithChildren(std::shared_ptr<SubSystem> parent, std::vector<std::shared_ptr<Node>> &nodeCopies, std::map<std::shared_ptr<Node>, std::shared_ptr<Node>> &origToCopyNode, std::map<std::shared_ptr<Node>, std::shared_ptr<Node>> &copyToOrigNode) override;

    //Do not need to override emitGramphMLSubgraphAndChildren.  Use the same function as a standard subsystem

    /**
     * @brief Creates any required supporting nodes for the clock domain.
     *
     * This is similar to expansion except that clockdomain cannot use the standard expansion mechanism because
     * it would force the creation of an ExpandedSubsystem with the clock domain in essence removed from the design.
     *
     * Bridging nodes are not required for multirate designs because the FIFO node serves that purpose
     * These nodes are required for single threaded
     *
     * @param nodesToAdd
     * @param nodesToRemove
     * @param arcsToAdd
     * @param arcToRemove
     * @param includeContext if true, contexts are set for the added nodes.  This should be set to true if contexts have already been discovered and marked.  It should be set to false if contexts have not been discovered/marked yet
     * @param includeOutputBridgeNodes if true, bridging nodes are inserted for outputs.  This is necessary for singleThreaded emits that do not use FIFOs
     */
    virtual void createSupportNodes(std::vector<std::shared_ptr<Node>> &nodesToAdd,
                            std::vector<std::shared_ptr<Node>> &nodesToRemove,
                            std::vector<std::shared_ptr<Arc>> &arcsToAdd,
                            std::vector<std::shared_ptr<Arc>> &arcToRemove,
                            bool includeContext, bool includeOutputBridgeNodes);

    /**
     * @brief Sets the clock domain driver
     *
     * @note createSupportNodes should be used in place of this.  This is used when setting some partitions to not emit clock domain logic
     */
    virtual void setClockDomainDriver(std::shared_ptr<Arc> newDriver);

    virtual std::shared_ptr<Arc> getClockDomainDriver();

    /**
     * @brief Resets the sets of discovered Master I/O ports connected to nodes in this clock domain
     */
    void resetIOPorts();

};

/*! @} */

#endif //VITIS_CLOCKDOMAIN_H
