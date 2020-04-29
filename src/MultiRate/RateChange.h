//
// Created by Christopher Yarp on 4/20/20.
//

#ifndef VITIS_RATECHANGE_H
#define VITIS_RATECHANGE_H

#include "GraphCore/Node.h"
#include "ClockDomain.h"
#include <memory>

/**
 * \addtogroup MultiRate Multi-Rate Support Nodes
 * @{
*/

/**
 * @brief Represents rate change blocks
 *
 * This includes Upsample, Downsample, and Repeat blocks
 */
class RateChange : public Node {
friend class NodeFactory;
protected:
    std::shared_ptr<ClockDomain> clockDomain; ///<The clock domain that this rate change is associated with

    /**
     * @brief Default constructor for RateChange
     */
    RateChange();

    /**
     * @brief Construct a rate change node with a given parent
     * @param parent parent of the new node
     */
    explicit RateChange(std::shared_ptr<SubSystem> parent);

    /**
     * @brief Constructs a new node with a shallow copy of parameters from the original node.  Ports are not copied and neither is the parent reference.  This node is not added to the children list of the parent.
     *
     * @note To construct from outside of hierarchy, use factories in @ref NodeFactory
     *
     * @note If copying a graph, the parent should be one of the copies and not from the original graph.
     *
     * @warning Because pointer (this) is passed to ports, nodes must be allocated on the heap and not moved.  All interaction should be via pointers.
     *
     * @warning This function does not clone the reference to the ClockDomain.  This should be re-assigned durring the ClockDomain clone as
     *          RateChanges should exist under clock domains
     *
     * @param parent parent node
     * @param orig The original node from which a shallow copy is being made
     */
    RateChange(std::shared_ptr<SubSystem> parent, RateChange* orig);

public:
    /**
     * @brief Gets the rate change at the boundary of this block as a ratio of upsampling to downsampling
     * @return A pair with (upsample, downsample) representing a rate change of upsample/downsample
     */
    virtual std::pair<int, int> getRateChangeRatio() = 0;

    /**
     * @brief Converts the RateChange node into either a RateChange Input or a RateChange Output.  This is performed
     * after import where generic rate change nodes need to be converted to Inputs or Outputs to clock domains.  This
     * specialization is performed because the different rate change blocks have different implementations if they are
     * at the input or the output of a clock domain.
     *
     * Because the Input and Output types are different classes, a new node of the appropriate type is created, and all
     * arcs are moved to this new node.
     *
     * @warning: After conversion, any references to the old node should be converted to the new node.  If provided as
     * a reference, the node will be removed from the design.
     *
     * @param convertToInput if true, the node is converted to an input.  if false, the node is converted to an output
     * @return a pointer to the newly created node
     */
//    virtual std::shared_ptr<RateChange> convertToRateChangeInputOutput(bool convertToInput, std::shared_ptr<Design> design = nullptr) = 0;

    /**
     * @brief Also removes this node from the list of RateChange nodes in the assocted clock domain if the reference to the
     * containing clock domain is set
     */
    void removeKnownReferences() override;

    void validate() override;

    bool canExpand() override;
};

/*! @} */


#endif //VITIS_RATECHANGE_H
