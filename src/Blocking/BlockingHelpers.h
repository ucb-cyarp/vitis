//
// Created by Christopher Yarp on 8/24/21.
//

#ifndef VITIS_BLOCKINGHELPERS_H
#define VITIS_BLOCKINGHELPERS_H

#include "BlockingDomain.h"
#include "BlockingInput.h"
#include "BlockingOutput.h"

/**
 * \addtogroup Blocking Blocking Support Nodes
 * @brief Nodes to support blocked sample processing
 *
 * @{
 */

/**
 * @brief Helpers for Blocking
 */
namespace BlockingHelpers {
    /**
     * @brief Finds nodes in a Blocking domain by searching within subsystems (including Enabled Subsystems and Clock Domains)
     *
     * Nested Blocking domains are included in the list but nodes within them are not included in the list
     *
     * @param nodesToSearch
     * @return
     */
    std::set<std::shared_ptr<Node>> getNodesInBlockingDomainHelper(const std::set<std::shared_ptr<Node>> nodesToSearch);

    std::set<std::shared_ptr<BlockingInput>> getBlockingInputsInBlockingDomainHelper(const std::set<std::shared_ptr<Node>> nodesToSearch);
    std::set<std::shared_ptr<BlockingOutput>> getBlockingOutputsInBlockingDomainHelper(const std::set<std::shared_ptr<Node>> nodesToSearch);

    /**
     * @brief Find the blocking domain that this node is a part of.
     *
     * If there are multiple nested blocking domains, it finds the most specific blocking domain it is a part of
     * (ie. the one it is a leaf node in)
     *
     * @param node
     * @return the BlockingDomain the node is under or nullptr if it is not in a BlockingDomain
     */
    std::shared_ptr<BlockingDomain> findBlockingDomain(std::shared_ptr<Node> node);

    /**
     * @brief Check if BlockingDomain a is outside of BlockingDomain b
     *
     * BlockingDomain a is outside of BlockingDomain b if BlockingDomain b is not equivalent to or nested under BlockingDomain a
     *
     * Either BlockingDomain a or b can be nullptr
     *
     * @param a
     * @param b
     * @return true if BlockingDomain a is outside of BlockingDomain b
     */
    bool isOutsideBlockingDomain(std::shared_ptr<BlockingDomain> a, std::shared_ptr<BlockingDomain> b);

    /**
     * @brief Check if BlockingDomain a is equal to BlockingDomain b or is a BlockingDomain directly nested within clock domain b (one level below)
     * @param a
     * @param b
     * @return true if BlockingDomain a is equal to BlockingDomain b or is a BlockingDomain directly nested within clock domain b (one level below)
     */
    bool isBlockingDomainOrOneLvlNested(std::shared_ptr<BlockingDomain> a, std::shared_ptr<BlockingDomain> b);

    /**
     * @brief Checks if the clock domains are within one level of each other
     *
     * Ie. if BlockingDomain a and BlockingDomain b are the same.
     *     if BlockingDomain a's outer domain is the outer domain of b
     *     if BlockingDomain a's outer domain is b
     *     if BlockingDomain b's outer domain is a
     *
     * @param a
     * @param b
     * @return
     */
    bool areWithinOneBlockingDomainOfEachOther(std::shared_ptr<BlockingDomain> a, std::shared_ptr<BlockingDomain> b);

    /**
     * @brief Find the blocking domain hierarchy that this node is a part of.
     *
     * The first entry is the outer blocking domain and the last entry is the most specific blocking domain this node is a part of
     *
     * @param node
     * @return
     */
    std::vector<std::shared_ptr<BlockingDomain>> findBlockingDomainStack(std::shared_ptr<Node> node);

    /**
     * @brief Determine the dimensions to be used inside a blocking domain given the dimension(s) outside the domain
     * @param outerDimensions
     * @param subBlockingLength
     * @return
     */
    std::vector<int> blockingDomainDimensionReduce(std::vector<int> outerDimensions, int subBlockingLength);

    void createBlockingDomainHelper(std::set<std::shared_ptr<Node>> nodesToMove,
                                    std::set<std::shared_ptr<Arc>> arcsIntoDomain,
                                    std::set<std::shared_ptr<Arc>> arcsOutOfDomain,
                                    std::shared_ptr<SubSystem> blockingDomainParent,
                                    int blockingLength,
                                    int subBlockingLength,
                                    std::string blockingName,
                                    std::vector<std::shared_ptr<Node>> &nodesToAdd,
                                    std::vector<std::shared_ptr<Arc>> &arcsToAdd,
                                    std::vector<std::shared_ptr<Node>> &nodesToRemoveFromTopLevel,
                                    std::map<std::shared_ptr<Arc>, int> &arcsWithDeferredBlockingExpansion);
};

/*! @} */

#endif //VITIS_BLOCKINGHELPERS_H
