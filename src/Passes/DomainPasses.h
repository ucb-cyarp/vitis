//
// Created by Christopher Yarp on 9/2/21.
//

#ifndef VITIS_DOMAINPASSES_H
#define VITIS_DOMAINPASSES_H

#include "GraphCore/Design.h"

/**
 * \addtogroup Passes Passes Design Passes
 * @{
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
};

/*! @} */

#endif //VITIS_DOMAINPASSES_H
