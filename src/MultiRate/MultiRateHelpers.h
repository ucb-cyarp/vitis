//
// Created by Christopher Yarp on 4/22/20.
//

#ifndef VITIS_MULTIRATEHELPERS_H
#define VITIS_MULTIRATEHELPERS_H

#include <set>
#include "ClockDomain.h"

class MasterNode;

/**
 * \addtogroup MultiRate Multi-Rate Support Nodes
 * @{
*/

/**
 * @brief A class containing helper functions for multirate processing.
 */
namespace MultiRateHelpers {
    /**
     * @brief Finds nodes in a clock domain by searching within subsystems (including Enabled Subsystems)
     *
     * Nested ClockDomains are included in the list but nodes within them are not included in the list
     *
     * @param nodesToSearch
     * @return
     */
    std::set<std::shared_ptr<Node>> getNodesInClockDomainHelper(const std::set<std::shared_ptr<Node>> nodesToSearch);

    /**
     * @brief Find the clock domain that this node is a part of.
     *
     * If there are multiple nested clock domains, it finds the most specific clock domain it is a part of (ie. the
     * one it is a leaf node in)
     *
     * @param node
     * @return the ClockDomain the node is under or nullptr if it is in the base domain
     */
    std::shared_ptr<ClockDomain> findClockDomain(std::shared_ptr<Node> node);

    /**
     * @brief Check if ClockDomain a is outside of ClockDomain b
     *
     * ClockDomain a is outside of ClockDomain b if ClockDomain b is not equivalent to or nested under ClockDomain a
     *
     * Either ClockDomain a or b can be nullptr
     *
     * @param a
     * @param b
     * @return true if ClockDomain a is outside of ClockDomain b
     */
    bool isOutsideClkDomain(std::shared_ptr<ClockDomain> a, std::shared_ptr<ClockDomain> b);

    /**
     * @brief Check if ClockDomain a is equal to ClockDomain b or is a ClockDomain directly nested within clock domain b (one level below)
     * @param a
     * @param b
     * @return true if ClockDomain a is equal to ClockDomain b or is a ClockDomain directly nested within clock domain b (one level below)
     */
    bool isClkDomainOrOneLvlNested(std::shared_ptr<ClockDomain> a, std::shared_ptr<ClockDomain> b);

    /**
     * @brief Checks if the clock domains are within one level of each other
     *
     * Ie. if ClockDomain a and ClockDomain b are the same.
     *     if ClockDomain a's outer domain is the outer domain of b
     *     if ClockDomain a's outer domain is b
     *     if ClockDomain b's outer domain is a
     *
     * @param a
     * @param b
     * @return
     */
    bool areWithinOneClockDomainOfEachOther(std::shared_ptr<ClockDomain> a, std::shared_ptr<ClockDomain> b);

    /**
     * @brief Perform the validation for a rate change input block (and optionally set the rates of connected MasterNodes)
     *
     * Note that it does not add the MasterNode ports to the port list of the clock domain
     *
     * - Checks that the input arc comes from outside the domain:
     *   - From a MasterInput node
     *   - From a node in the outer domain
     *   - From a RateChange node in a domain one level below the outer domain (a RateChangeOutput) (that is not the current domain)
     *   - From a RateChange node in the outer domain (a RateChangeInput)
     * - Check that the output arcs are going either to:
     *   - Nodes within the clock domain
     *   - RateChange nodes in a clock domain one level down
     *   - The OutputMaster
     *
     * @param rc The rate change node to validate
     * @returns a pair of sets.  The first are the input master ports connected to rate change nodes, the second are the output master ports connected to the rate change nodes
     */
    std::pair<std::set<std::shared_ptr<OutputPort>>, std::set<std::shared_ptr<InputPort>>> validateRateChangeInput_SetMasterRates(std::shared_ptr<RateChange> rc, bool setMasterRates);

    /**
     * @brief Perform the validation for a rate change input block (and optionally set the rates of connected MasterNodes)
     *
     * Note that it does not add the MasterNode ports to the port list of the clock domain
     *
     * - Checks that the input arc comes from:
     *   - Nodes within the clock domain
     *   - RateChange nodes in a clock domain one level down
     *   - The InputMaster
     * - Check that the output arcs are going nodes outside the domain:
     *   - MasterOutput node
     *   - A node at in the outer domain
     *   - A RateChange node in a domain one level below the outer domain (a RateChangeOutput) (that is not the current domain)
     *   - A RateChange node in the outer domain (a RateChangeInput)
     *
     * @param rc The rate change node to validate
     * @returns a pair of sets.  The first are the input master ports connected to rate change nodes, the second are the output master ports connected to the rate change nodes
     */
    std::pair<std::set<std::shared_ptr<OutputPort>>, std::set<std::shared_ptr<InputPort>>> validateRateChangeOutput_SetMasterRates(std::shared_ptr<RateChange> rc, bool setMasterRates);

    /**
     * @brief Clones the clock domain association of MasterNode ports
     *
     * Used in design clone function
     *
     * @param origMaster a pointer to the master in the origional design
     * @param clonedMaster a pointer to a clone of the master
     * @param origToCopyNode a map of
     */
    void cloneMasterNodePortClockDomains(std::shared_ptr<MasterNode> origMaster, std::shared_ptr<MasterNode> clonedMaster, const std::map<std::shared_ptr<Node>, std::shared_ptr<Node>> &origToCopyNode);

    void validateSpecialiedClockDomain(std::shared_ptr<ClockDomain> clkDomain);

    /**
     * @brief Validates that the ClockDomains rates are acceptable
     *
     * For now, it validates that the clock domains are either interger upsamples or integer downsamples from the base rate
     *
     * @param clockDomainsInDesign a list of clock domains in the design
     */
    void validateClockDomainRates(std::vector<std::shared_ptr<ClockDomain>> clockDomainsInDesign);

    /**
     * @brief Sets the clock domain of a thread crossing FIFOs either based on the context it is in or the master I/O
     * they are connected to
     *
     * @warning Is currently only implemented for FIFOs with a single input and output port
     *
     * @param threadCrossingFIFOs
     */
    void setFIFOClockDomains(std::vector<std::shared_ptr<ThreadCrossingFIFO>> threadCrossingFIFOs);

    /**
     * @brief Checks the IO block sizes are integers (after accounting for clock domains)
     *
     * @note: This is redundant for MultithreadedEmit because IO is communicated to the compute threads via FIFOs and this check is also performed there
     *
     * @param masterNodes
     * @param blockSize
     */
    void checkIOBlockSizes(std::set<std::shared_ptr<MasterNode>> masterNodes, int blockSize);

    /**
     * @brief Sets the block sizes of FIFOs and validates that they are integer length
     *
     * Note that since the IO thread communicates with the compute threads via FIFOs, this sets
     *
     * @warning Clock Domains should be set for each FIFO port before calling this function.
     *
     * @param threadCrossingFIFOs
     * @param blockSize
     * @param setFIFOBlockSize
     */
    void setAndValidateFIFOBlockSizes(std::vector<std::shared_ptr<ThreadCrossingFIFO>> threadCrossingFIFOs, int blockSize, bool setFIFOBlockSize);

    void rediscoverClockDomainParameters(std::vector<std::shared_ptr<ClockDomain>> clockDomainsInDesign);

    /**
     * @brief Find which partitions only have nodes in a single clock domain rate and do not contain any rate transition nodes
     *
     * This function does not consider clock domain drivers or subsystem nodes when determining if all the nodes in a partition run at the same rate
     *
     * @param nodes the nodes in the design
     * @return a map of partitions with only one clock domain and the associated clock domain
     */
    std::map<int, std::set<std::shared_ptr<ClockDomain>>> findPartitionsWithSingleClockDomain(const std::vector<std::shared_ptr<Node>> &nodes);

    /**
     * @brief Finds the clock domains which contain the set of given nodes.  It does not just include the clock domain
     * the nodes are directly in but the clock domains up the hierarchy
     *
     * @param node
     * @return
     */
    template<typename NodeContainerType>
    std::set<std::shared_ptr<ClockDomain>> findClockDomainsOfNodes(NodeContainerType &nodes){
        std::set<std::shared_ptr<ClockDomain>> clockDomains;

        for(const std::shared_ptr<Node> &node : nodes){
            std::shared_ptr<ClockDomain> cursor = MultiRateHelpers::findClockDomain(node); //This function works pre- or post- encapsulation
            //Don't just want the lowest level clock domain the node is in, want the full hierarchy
            while(cursor != nullptr){
                clockDomains.insert(cursor);
                cursor = MultiRateHelpers::findClockDomain(cursor);
            }
        }

        return clockDomains;
    }

};

/*! @} */

#endif //VITIS_MULTIRATEHELPERS_H
