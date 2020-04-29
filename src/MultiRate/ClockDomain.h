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

private:
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
     * @param convertToInput if true, the node is converted to an input.  if false, the node is converted to an output
     * @return a pointer to the newly created node
     */

//    virtual std::shared_ptr<ClockDomain> convertToUpsampleDownsampleDomain(bool convertToUpsampleDomain, std::shared_ptr<Design> design = nullptr);
    //TODO Implement clone and children.  Need to clone rate change nodes, set their pointers to the Clock Domain, and add them to the set of

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


    //TODO: Implement discoverAndMarkContexts in subclasses
};

/*! @} */

#endif //VITIS_CLOCKDOMAIN_H
