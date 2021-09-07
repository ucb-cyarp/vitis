//
// Created by Christopher Yarp on 9/3/21.
//

#ifndef VITIS_INTRAPARTITIONSCHEDULING_H
#define VITIS_INTRAPARTITIONSCHEDULING_H

#include "GraphCore/Design.h"
#include "General/TopologicalSortParameters.h"

/**
 * \addtogroup Scheduling Schedulers
 * @brief Functions for scheduling operations
 *
 * @{
 */

/**
 * @brief Functions for scheduling nodes within a design which are executed on a single CPU.
 *
 * Ensures predecessor nodes are executed before nodes which are dependent on their results.
 */
namespace IntraPartitionScheduling {
    /**
     * @brief Schedule the nodes using topological sort.
     *
     * @param params the parameters used by the scheduler (ex. what heuristic to use, random seed (if applicable))
     * @param prune if true, prune the design before scheduling.  Pruned nodes will not be scheduled but will also not be removed from the origional graph.
     * @param rewireContexts if true, arcs between a node outside a context to a node inside a context are rewired to the context itself (for scheduling, the origional is left untouched).  If false, no rewiring operation is made for scheduling
     *
     * @param designName the name of the design (used when dumping a partially scheduled graph in the event an error is encountered)
     * @param dir the directory to dump any error report files into
     * @param printNodeSched if true, print the node schedule to the console
     * @param schedulePartitions if true, each partition in the design is scheduled seperatly
     *
     * @warning: UpsampleClockDomains currently rely on all of their nodes being scheduled together (ie not being split up).
     * This currently is provided by the hierarchical implementation of the scheduler.  However, if this were to be changed
     * later, a method for having vector intermediates would be required.
     *
     * @return the number of nodes pruned (if prune is true)
     */
    unsigned long scheduleTopologicalStort(Design &design, TopologicalSortParameters params, bool prune, bool rewireContexts, std::string designName, std::string dir, bool printNodeSched, bool schedulePartitions);

    /**
     * @brief Topological sort the current graph.
     *
     * @warning This destroys the graph by removing arcs from the nodes.
     * It is reccomended to run on a copy of the graph and to back propagate the results
     *
     * @param designName name of the design used in the working graph export at part of the file name
     * @param dir location to export working graph in the event of a cycle or error
     * @param params the parameters used by the scheduler (ex. what heuristic to use, random seed (if applicable))
     *
     * @return A vector of nodes arranged in topological order
     */
    std::vector<std::shared_ptr<Node>> topologicalSortDestructive(Design &design, std::string designName, std::string dir, TopologicalSortParameters params);

    /**
     * @brief Topological sort of a given partition in the the current graph.
     *
     * @warning This does not schedule the output node except for partition -2 (I/O)
     *
     * @warning This destroys the graph by removing arcs from the nodes.
     * It is reccomended to run on a copy of the graph and to back propagate the results
     *
     * @param designName name of the design used in the working graph export at part of the file name
     * @param dir location to export working graph in the event of a cycle or error
     * @param params the parameters used by the scheduler (ex. what heuristic to use, random seed (if applicable))
     * @param partition the partitions
     *
     * @return A vector of nodes arranged in topological order
     */
    std::vector<std::shared_ptr<Node>> topologicalSortDestructive(Design &design, std::string designName, std::string dir, TopologicalSortParameters params, int partitionNum);
};

/*! @} */

#endif //VITIS_INTRAPARTITIONSCHEDULING_H
