//
// Created by Christopher Yarp on 2019-03-29.
//

#ifndef VITIS_TOPOLOGICALSORTPARAMETERS_H
#define VITIS_TOPOLOGICALSORTPARAMETERS_H

#include <string>

/**
 * \addtogroup General General Helper Classes
 *
 * @brief A set of general helper classes.
 * @{
*/

/**
 * @brief A container for parameters that can be passed to the topological ordering scheduler
 */
class TopologicalSortParameters {
public:
    enum class Heuristic{
        BFS, ///< Breadth First Search Style
        DFS, ///< Depth First Search Style
        RANDOM ///< Randomly Select Node to Schedule From List of Ready Nodes
    };

    /**
     * @brief Parse a heuristic type
     * @param str the string to parse
     * @return The equivalent Heuristic
     */
    static Heuristic parseHeuristicStr(std::string str);

    /**
     * @brief Get a string representation of the Heuristic
     * @param heuristic the Heuristic to get a string of
     * @return string representation of the Heuristic
     */
    static std::string heuristicToString(Heuristic heuristic);

private:
    Heuristic heuristic; ///<The heuristic to be used by the scheduler
    unsigned long randSeed; ///<A random seed to be used by the scheduler (if applicable)

public:
    /**
     * @brief Create scheduling parameters with defaults: BFS, randSeed 0
     */
    TopologicalSortParameters();

    /**
     * @brief Create scheduling parameters with given heuristic, randSeed 0
     * @param heuristic heuristic to use when scheduling
     */
    explicit TopologicalSortParameters(Heuristic heuristic);

    /**
     * @brief Create scheduling parameters with given heuristic and randSeed
     * @param heuristic heuristic to use when scheduling
     * @param randSeed random seed to use when scheduling
     */
    TopologicalSortParameters(Heuristic heuristic, unsigned long randSeed);

    Heuristic getHeuristic() const;
    void setHeuristic(Heuristic heuristic);

    int getRandSeed() const;
    void setRandSeed(int randSeed);

};

/*! @} */

#endif //VITIS_TOPOLOGICALSORTPARAMETERS_H
