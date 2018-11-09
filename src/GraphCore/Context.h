//
// Created by Christopher Yarp on 10/22/18.
//

#ifndef VITIS_CONTEXT_H
#define VITIS_CONTEXT_H


#include <memory>
#include <vector>
//#include "ContextRoot.h"

class ContextRoot;

/**
 * \addtogroup GraphCore Graph Core
 */
/*@{*/

/**
 * @brief Represents a context identifier.  Contains the node which is the root of the context and the subContext number.
 */
class Context {
private:
    std::shared_ptr<ContextRoot> contextRoot; ///<The node which is the root of the context
    int subContext; ///<For nodes which can have multiple contexts, specifies which context (default to 0).  Should be within the range [0, nContexts-1]

public:
    /**
     * @brief Default constructor for Contex.  contextRoot is set as nullptr and subContext is set to 0
     */
    Context();

    /**
     * @brief Initialize a context with a given context root pointer and sub-context number
     * @param contextRoot
     * @param subContext
     */
    Context(std::shared_ptr<ContextRoot> contextRoot, int subContext);

    std::shared_ptr<ContextRoot> getContextRoot() const;
    void setContextRoot(const std::shared_ptr<ContextRoot> &contextRoot);
    int getSubContext() const;
    void setSubContext(int subContext);

    bool operator==(const Context &rhs) const;
    bool operator!=(const Context &rhs) const;

    /**
     * @brief Check if context stack a is equal to or a subcontext of context stack b
     *
     * This function validates that context stack b (of depth n) is identitical to the top n contexts of context stack a (depth m>=n)
     *
     * Stack A   | Stack B
     * --------- | ---------
     * node a, 0 | node a, 0
     * node b, 0 | node b, 0
     * returns true
     *
     * Stack A   | Stack B
     * --------- | ---------
     * node a, 0 | node a, 0
     * node b, 0 | node b, 0
     * node c, 1 | < Empty >
     * returns true
     *
     * Stack A   | Stack B
     * --------- | ---------
     * node a, 0 | node a, 0
     * node b, 0 | node d, 0
     * node c, 1 | < Empty >
     * returns false
     *
     * Stack A   | Stack B
     * --------- | ---------
     * node a, 0 | node a, 0
     * < Empty > | node b, 0
     * returns false
     *
     * @param a
     * @param b
     * @return true if a is equal to or a subcontext of b
     */
    static bool isEqOrSubContext(std::vector<Context> a, std::vector<Context> b);

};

/*@}*/

#endif //VITIS_CONTEXT_H