//
// Created by Christopher Yarp on 4/29/20.
//

#ifndef VITIS_CONTEXTHELPER_H
#define VITIS_CONTEXTHELPER_H

#include <vector>
#include "Node.h"
#include "Context.h"
#include "PrimitiveNodes/Mux.h"
#include "EnabledSubSystem.h"
#include "MultiRate/ClockDomain.h"

/**
 * \addtogroup GraphCore Graph Core
 * @{
*/

/**
 * @brief A helper class for context routines
 */
class ContextHelper {
public:
    /**
     * @brief Discover and mark contexts for nodes at and within an EnabledSubsystem or ClockDomain
     *
     * Propagates EnabledSubsystem/ClockDomain contexts to its nodes (and recursively to nested EnabledSubsystems/ClockDomains).
     *
     * Finds mux contexts under the top level and under each EnabledSubsystem/ClockDomain
     *     Muxes are searched within the top level and within any subsystem.
     *
     *     Once all the Muxes within the top level (or within the level of the EnabledSubsystem/ClockDomain are discovered)
     *     their contexts are found.  For each mux, we sum up the number of contexts which contain that given mux.
     *     Hierarchy is discovered by tracing decending layers of the hierarchy tree based on the number of contexts
     *     a mux is in.
     *
     * Since contexts stop at EnabledSubsystems and RateChange nodes, the process begins again with any
     * EnabledSubsystems/ClockDomain within (after muxes have been handled)
     *
     * @note: This was re-factored into a helper function because
     *
     * @tparam T EnabledSubsystem or ClockDomain
     * @param curContextStack the context stack up to the rootNode
     * @param rootNode the EnabledSubsystem or ClockDomain under which contexts are marked/discovered
     * @return nodes in the context (including ones from recursion)
     */
    template<typename T>
    static std::vector<std::shared_ptr<Node>> discoverAndMarkContexts_SubsystemContextRoots(std::vector<Context> curContextStack, std::shared_ptr<T> rootNode){

        //Return all nodes in context (including ones from recursion)
        //Add this to the context stack
        std::vector<Context> newContextStack = curContextStack;
        newContextStack.push_back(Context(std::dynamic_pointer_cast<T>(rootNode->getSharedPointer()), 0)); //The sub-context for enabled subsystems is 0

        //Update each of the nodes in this system with the input context + the context for this enabled subsystem
        //Also, Find the multiplexers and enabled subsystems at this level
        std::vector<std::shared_ptr<Mux>> discoveredMuxes;
        std::vector<std::shared_ptr<EnabledSubSystem>> discoveredEnabledSubsystems;
        std::vector<std::shared_ptr<ClockDomain>> discoveredClockDomains;
        std::vector<std::shared_ptr<Node>> discoveredGeneral;

        rootNode->discoverAndUpdateContexts(newContextStack, discoveredMuxes, discoveredEnabledSubsystems, discoveredClockDomains, discoveredGeneral);

        std::vector<std::shared_ptr<Node>> discoveredNodes;
        discoveredNodes.insert(discoveredNodes.end(), discoveredMuxes.begin(), discoveredMuxes.end());
        discoveredNodes.insert(discoveredNodes.end(), discoveredEnabledSubsystems.begin(), discoveredEnabledSubsystems.end());
        discoveredNodes.insert(discoveredNodes.end(), discoveredClockDomains.begin(), discoveredClockDomains.end());
        discoveredNodes.insert(discoveredNodes.end(), discoveredGeneral.begin(), discoveredGeneral.end());

        //Set the nodes in the context for this enabled subsystem
        for(unsigned long i = 0; i<discoveredMuxes.size(); i++){
            rootNode->addSubContextNode(0, discoveredMuxes[i]);
        }
        for(unsigned long i = 0; i<discoveredEnabledSubsystems.size(); i++){
            rootNode->addSubContextNode(0, discoveredEnabledSubsystems[i]);
        }
        for(unsigned long i = 0; i<discoveredClockDomains.size(); i++){
            rootNode->addSubContextNode(0, discoveredClockDomains[i]);
        }
        for(unsigned long i = 0; i<discoveredGeneral.size(); i++){
            rootNode->addSubContextNode(0, discoveredGeneral[i]);
        }

        //Get and mark the Mux contexts
        //Note that context should be set for the nodes within this subsystem before this is called (done above)
        Mux::discoverAndMarkMuxContextsAtLevel(discoveredMuxes);

        //Recursivly call on the discovered enabled subsystems
        for(unsigned long i = 0; i<discoveredEnabledSubsystems.size(); i++){
            std::vector<std::shared_ptr<Node>> nestedDiscoveries = discoveredEnabledSubsystems[i]->discoverAndMarkContexts(newContextStack);

            //Add nodes returned by recursive call
            discoveredNodes.insert(discoveredNodes.end(), nestedDiscoveries.begin(), nestedDiscoveries.end());

            //Add the nodes returned by the recursive call to the list of nodes within this context
            for(unsigned long j = 0; j<nestedDiscoveries.size(); j++){
                rootNode->addSubContextNode(0, nestedDiscoveries[j]);
            }
        }

        //Recursivly call on the discovered ClockDomains
        for(unsigned long i = 0; i<discoveredClockDomains.size(); i++){
            std::vector<std::shared_ptr<Node>> nestedDiscoveries = discoveredClockDomains[i]->discoverAndMarkContexts(newContextStack);

            //Add nodes returned by recursive call
            discoveredNodes.insert(discoveredNodes.end(), nestedDiscoveries.begin(), nestedDiscoveries.end());

            //Add the nodes returned by the recursive call to the list of nodes within this context
            for(unsigned long j = 0; j<nestedDiscoveries.size(); j++){
                rootNode->addSubContextNode(0, nestedDiscoveries[j]);
            }
        }

        //Return all nodes in context (including ones from recursion)

        return discoveredNodes;
    }
};

/*! @} */

#endif //VITIS_CONTEXTHELPER_H
