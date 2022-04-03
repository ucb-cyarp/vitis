//
// Created by Christopher Yarp on 9/2/21.
//

#ifndef VITIS_DOMAINPASSES_H
#define VITIS_DOMAINPASSES_H

#include "GraphCore/Design.h"
#include "PrimitiveNodes/Delay.h"

/**
 * \addtogroup Passes Design Passes/Transforms
 *
 * @brief A group of compiler passes which operate over a design.
 * @{
 */

/**
 * @brief A collection of passes/transforms focused on domains (ex. ClockDomains and BlockingDomains) within the design
 */
namespace DomainPasses {
    /**
     * @brief Specialize ClockDomains into UpsampleClockDomains or DownsampleClockDomains
     *
     * @returns a new vector of ClockDomain nodes since new nodes are created to replace each clock domain durring specialization.  Already specialized ClockDomains are returned unchanged
     */
    std::vector<std::shared_ptr<ClockDomain>> specializeClockDomains(Design &design, std::vector<std::shared_ptr<ClockDomain>> clockDomains);

    /**
     * @brief Find ClockDomains in design
     */
    std::vector<std::shared_ptr<ClockDomain>> findClockDomains(Design &design);

    /**
     * @brief Creates support nodes for ClockDomains
     */
    void createClockDomainSupportNodes(Design &design, std::vector<std::shared_ptr<ClockDomain>> clockDomains, bool includeContext, bool includeOutputBridgingNodes);

    /**
     * @brief Resets the clock domain links in the design master nodes
     */
    void resetMasterNodeClockDomainLinks(Design &design);

    /**
     * @brief Finds the clock domain rates for each partition in the design
     * @return
     */
    std::map<int, std::set<std::pair<int, int>>> findPartitionClockDomainRates(Design &design);

    /**
     * @brief Determines the effective sub-blocking length present at a node taking into account clock domains
     *
     * This is primarily used to determine if a delay can exist outside of a sub-blocking domain
     *
     * @warning This function should be used before the blocking domain and sub-blocking domain insertion & encapsulation occurs
     *
     * @param node the node for which the effective sub-blocking length is being found
     * @param baseSubBlockingLength the base sub-blocking length (outside of all clock domains)
     * @return a pair indicating if the effective sub-blocking length is an integer and the sub-blocking length (if an integer)
     */
    std::pair<bool, int> findEffectiveSubBlockingLengthForNode(std::shared_ptr<Node> node, int baseSubBlockingLength);

    //Version which uses included baseSubBlockingLength in node
    std::pair<bool, int> findEffectiveSubBlockingLengthForNode(std::shared_ptr<Node> node);

    /**
     * @brief Determines the effective sub-blocking length present for nodes directly under a given clock domain
     *
     * This is used to determine if a clock domain needs to be encased in a blocking domain when sub-blocking or if blocking domains
     * can potentially be pushed into the clock domain
     *
     * @warning This function should be used before the blocking domain and sub-blocking domain insertion & encapsulation occurs
     *
     * @param node the node for which the effective sub-blocking length is being found
     * @param baseSubBlockingLength the base sub-blocking length (outside of all clock domains)
     * @return a pair indicating if the effective sub-blocking length is an integer and the sub-blocking length (if an integer)
     */
    std::pair<bool, int> findEffectiveSubBlockingLengthForNodesUnderClockDomain(std::shared_ptr<ClockDomain> clockDomain, int baseSubBlockingLength);

    /**
     * @brief Find node that can exist outside of sub-blocking domains and can effectively break dependency chains after sub-blocking
     *
     * These nodes can allow cycles in DSP designs to be split by providing a dependency break.  These nodes play the same
     * roll that simple delays play in the unblocked design.
     *
     * @param design
     * @return
     */
    std::set<std::shared_ptr<Node>> discoverNodesThatCanBreakDependenciesWhenSubBlocking(Design &design);

    /**
     * @brief Wraps design in a global blocking domain.  Sub-blocks the remainder of the design
     *
     * @warning requires context discover to occure before this function is called (so that contexts can be properly blocked)
     * @warning clears the context discovery information as part of the process and re-calls ContextPasses::discoverAndMarkContexts(design)
     * @warning Delays have specialization deferred for FIFO delay absorption
     *
     * @param design
     * @param baseBlockingLength
     * @param baseSubBlockingLength
     */
    void blockAndSubBlockDesign(Design &design, int baseBlockingLength);

    /**
     * @brief Sets the per-port clock domain of master nodes based on their assigned clock domains and the base
     *        block length.
     * @param design
     * @param baseBlockingLength
     */
    void setMasterBlockSizesBasedOnPortClockDomain(Design &design, int baseBlockingLength);

    /**
     * @brief Sets the original DataTypes (pre-blocking) for I/O Master Nodes
     *
     * @warning This should be done before the design is blocked
     *
     * @param design
     */
    void setMasterBlockOrigDataTypes(Design &design);

    void specializeDeferredDelays(Design &design);

    /**
     * @brief Creates Blocking nodes for I/O arcs not operating in the base clock domain and are not destined for a clock domain not in vector mode
     *
     * Similar to some of the logic in BlockingHelpers::createBlockingDomainHelper
     *
     * @warning Must be done after blocking domains are created
     * @warning Must be done after clock domains have been specialized for blocking
     * @warning Master node origional datatype must be set
     * @warning Do before arcsWithDeferredBlockingExpansion expanded
     *
     * @param design
     */
    void createBlockingNodesForIONotAtBaseDomain(Design &design,
                                                 std::map<std::shared_ptr<Arc>, std::tuple<int, int, bool, bool>>
                                                     &arcsWithDeferredBlockingExpansion,
                                                 int baseBlockLength);

    void expandArcsDeferredAndInsertBlockingBridges(Design &design, std::map<std::shared_ptr<Arc>, std::tuple<int, int, bool, bool>> &arcsWithDeferredBlockingExpansion);

    void createBlockingInputNodesForIONotAtBaseDomain(std::shared_ptr<MasterInput> masterInput,
                                                      std::set<std::shared_ptr<Node>> &nodesToAdd,
                                                      std::set<std::shared_ptr<Arc>> &arcsToAdd,
                                                      std::map<std::shared_ptr<Arc>, std::tuple<int, int, bool, bool>>
                                                          &arcsWithDeferredBlockingExpansion,
                                                      int baseBlockLength);

    void createBlockingOutputNodesForIONotAtBaseDomain(std::shared_ptr<MasterOutput> masterOutput,
                                                       std::set<std::shared_ptr<Node>> &nodesToAdd,
                                                       std::set<std::shared_ptr<Arc>> &arcsToAdd,
                                                       std::map<std::shared_ptr<Arc>, std::tuple<int, int, bool, bool>>
                                                           &arcsWithDeferredBlockingExpansion,
                                                       int baseBlockLength);

    /**
     * @brief Groups node under contexts together with the possible exception of clock domains where blocking domains can be placed inside
     *
     * @param nodesAtLevel Nodes at the current level in the design from a context perspective.  Includes nodes in subsystems, including context roots, just not nodes under contexts
     */
    void blockingNodeSetDiscoveryTraverse(std::set<std::shared_ptr<Node>> nodesAtLevel,
                                          std::set<std::shared_ptr<std::set<std::shared_ptr<Node>>>> &blockingGroups,
                                          std::map<std::shared_ptr<Node>, std::shared_ptr<std::set<std::shared_ptr<Node>>>> &nodeToBlockingGroup,
                                          std::map<std::shared_ptr<std::set<std::shared_ptr<Node>>>,
                                                   std::shared_ptr<std::set<std::shared_ptr<Node>>>> &blockingEnvelopGroups,
                                          std::set<std::shared_ptr<ClockDomain>> &clockDomainsOutsideSubBlocking);

    /**
     * @brief Merges 2 blocking groups together by transfering the nodes from mergeFrom into mergeInto
     *
     * If mergeFrom and mergeInto are the same, no action is taken as they are already the same group
     *
     * Also merges nodes from associated blockingEnvelopGroups
     *
     * Also check if the resulting merged group has a single base sub-blocking length.
     *
     * @param mergeFrom
     * @param mergeInto
     * @param blockingGroups
     * @param nodeToBlockingGroup
     * @param blockingEnvelopGroups
     */
    void mergeBlockingGroups(std::shared_ptr<std::set<std::shared_ptr<Node>>> mergeFrom,
                             std::shared_ptr<std::set<std::shared_ptr<Node>>> mergeInto,
                             std::set<std::shared_ptr<std::set<std::shared_ptr<Node>>>> &blockingGroups,
                             std::map<std::shared_ptr<Node>, std::shared_ptr<std::set<std::shared_ptr<Node>>>> &nodeToBlockingGroup,
                             std::map<std::shared_ptr<std::set<std::shared_ptr<Node>>>,
                                      std::shared_ptr<std::set<std::shared_ptr<Node>>>> &blockingEnvelopGroups);

    /**
     * @brief Handles discovering blocking sets for contexts that need all of their nodes to be in the same blocking set
     *
     * @warning Context Discovery should be completed before using this function
     *
     * @param contextRoot
     * @param blockingGroups
     * @param nodeToBlockingGroup
     */
    void blockingNodeSetDiscoveryContextNeedsEncapsulation(std::shared_ptr<ContextRoot> contextRoot,
                                                           std::set<std::shared_ptr<std::set<std::shared_ptr<Node>>>> &blockingGroups,
                                                           std::map<std::shared_ptr<Node>, std::shared_ptr<std::set<std::shared_ptr<Node>>>> &nodeToBlockingGroup,
                                                           std::map<std::shared_ptr<std::set<std::shared_ptr<Node>>>,
                                                           std::shared_ptr<std::set<std::shared_ptr<Node>>>> &blockingEnvelopGroups);

    /**
     * @brief Create a blocking domain around a set of nodes.  These nodes will be moved under the blocking sub-domain
     *
     * @param nodesToMove the set of nodes to move.  Contexts that are also subdomains should only have their top level node included.  This way, the nodes under the context will be moved in such a way that the contect is preserved
     */
    void createSubBlockingDomain(Design &design,
                                 std::set<std::shared_ptr<Node>> nodesToMove,
                                 std::set<std::shared_ptr<Node>> nodesInSubBlockingDomain,
                                 int baseSubBlockingLength,
                                 std::map<std::shared_ptr<Arc>, std::tuple<int, int, bool, bool>>
                                     &arcsWithDeferredBlockingExpansion);

    /**
     * @brief Creates a global blocking domain encircling the whole design.
     *
     *        Also creates BlockingInput and BlockingOutput nodes on I/O
     *        Also expands the dimensions of I/O by the blocking factor
     *
     * @param design
     * @param baseBlockingLength
     * @param baseSubBlockingLength
     */
    void createGlobalBlockingDomain(Design &design,
                                    int baseBlockingLength,
                                    std::map<std::shared_ptr<Arc>, std::tuple<int, int, bool, bool>>
                                        &arcsWithDeferredBlockingExpansion);

    /**
     * @brief Identifies Constant Nodes that are their own Blocking Groups and copies/moves them to be within the blocking
     *        groups of their destinations.
     *
     *        @note One exception is that the Constant node is not moved into a blocking domain where the destination
     *        formed its down blocking domain and specified its own blocking specialization.
     *
     *        Why do this: Constant nodes in their own blocking groups are often unesssiarily copied into arrays by
     *        blocking outputs.  This can create unnecessary state/copies and can obscure matters from the compiler.
     * @param blockingGroups
     * @param nodeToBlockingGroup
     * @param blockingEnvelopGroups
     * @param clockDomainsOutsideSubBlocking
     */
    void moveAndOrCopyLoneConstantNodes(std::set<std::shared_ptr<std::set<std::shared_ptr<Node>>>> &blockingGroups,
                                          std::map<std::shared_ptr<Node>, std::shared_ptr<std::set<std::shared_ptr<Node>>>> &nodeToBlockingGroup,
                                          std::map<std::shared_ptr<std::set<std::shared_ptr<Node>>>,
                                                  std::shared_ptr<std::set<std::shared_ptr<Node>>>> &blockingEnvelopGroups,
                                                  std::set<std::shared_ptr<Node>> &nodesToAdd);

    void propagateSubBlockingFromSubsystemsToChildren(Design &design);
};

/*! @} */

#endif //VITIS_DOMAINPASSES_H
