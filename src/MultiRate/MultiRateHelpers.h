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
class MultiRateHelpers {
public:

    /**
     * @brief Finds nodes in a clock domain by searching within subsystems (including Enabled Subsystems)
     *
     * Nested ClockDomains are included in the list but nodes within them are not included in the list
     *
     * @param nodesToSearch
     * @return
     */
    static std::set<std::shared_ptr<Node>> getNodesInClockDomainHelper(const std::set<std::shared_ptr<Node>> nodesToSearch);

    /**
     * @brief Finds nodes of a specified type in a clock domain by searching within subsystems (including Enabled Subsystems)
     *
     * Nested ClockDomains are included in the list but nodes within them are not included in the list
     *
     * @param nodesToSearch
     * @tparam T only adds nodes of type T to the list
     * @return
     */
    template <typename T>
    static std::set<std::shared_ptr<T>> getNodesInClockDomainHelperFilter(const std::set<std::shared_ptr<Node>> nodesToSearch){
        std::set<std::shared_ptr<T>> foundNodes;

        for(auto it = nodesToSearch.begin(); it != nodesToSearch.end(); it++){
            if(GeneralHelper::isType<Node, T>(*it)){
                std::shared_ptr<T> asT = std::static_pointer_cast<T>(*it);
                foundNodes.insert(asT);
            }

            if(GeneralHelper::isType<Node, ClockDomain>(*it)){
                //This is a ClockDomain, add it to the list but do not proceed into it
            }else if(GeneralHelper::isType<Node, SubSystem>(*it)){
                //This is another type of subsystem, add it to the list and proceed to search inside
                std::shared_ptr<SubSystem> asSubSystem = std::static_pointer_cast<SubSystem>(asSubSystem);
                std::set<std::shared_ptr<Node>> innerNodes = getNodesInClockDomainHelper(asSubSystem->getChildren());

                foundNodes.insert(innerNodes.begin(), innerNodes.end());
            }
            //Otherwise, this is a standard node which is just added to the list if it is the correct type
        }

        return foundNodes;
    }

    /**
     * @brief Find the clock domain that this node is a part of.
     *
     * If there are multiple nested clock domains, it finds the most specific clock domain it is a part of (ie. the
     * one it is a leaf node in)
     *
     * @param node
     * @return the ClockDomain the node is under or nullptr if it is in the base domain
     */
    static std::shared_ptr<ClockDomain> findClockDomain(std::shared_ptr<Node> node);

    /**
     * @brief Check if ClockDomain a is outside of ClockDomain b
     *
     * ClockDomain a is outside of ClockDomain b if ClockDomain b is not eqivalent to or nested under ClockDomain a
     *
     * Either ClockDomain a or b can be nullptr
     *
     * @param a
     * @param b
     * @return true if ClockDomain a is outside of ClockDomain b
     */
    static bool isOutsideClkDomain(std::shared_ptr<ClockDomain> a, std::shared_ptr<ClockDomain> b);

    /**
     * @brief Check if ClockDomain a is equal to ClockDomain b or is a ClockDomain directly nested within clock domain b (one level below)
     * @param a
     * @param b
     * @return true if ClockDomain a is equal to ClockDomain b or is a ClockDomain directly nested within clock domain b (one level below)
     */
    static bool isClkDomainOrOneLvlNested(std::shared_ptr<ClockDomain> a, std::shared_ptr<ClockDomain> b);

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
    static bool areWithinOneClockDomainOfEachOther(std::shared_ptr<ClockDomain> a, std::shared_ptr<ClockDomain> b);

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
    static std::pair<std::set<std::shared_ptr<OutputPort>>, std::set<std::shared_ptr<InputPort>>> validateRateChangeInput_SetMasterRates(std::shared_ptr<RateChange> rc, bool setMasterRates);

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
    static std::pair<std::set<std::shared_ptr<OutputPort>>, std::set<std::shared_ptr<InputPort>>> validateRateChangeOutput_SetMasterRates(std::shared_ptr<RateChange> rc, bool setMasterRates);

    /**
     * @brief Clones the clock domain association of MasterNode ports
     *
     * Used in design clone function
     *
     * @param origMaster a pointer to the master in the origional design
     * @param clonedMaster a pointer to a clone of the master
     * @param origToCopyNode a map of
     */
    static void cloneMasterNodePortClockDomains(std::shared_ptr<MasterNode> origMaster, std::shared_ptr<MasterNode> clonedMaster, const std::map<std::shared_ptr<Node>, std::shared_ptr<Node>> &origToCopyNode);
};

/*! @} */

#endif //VITIS_MULTIRATEHELPERS_H
