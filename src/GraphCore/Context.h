//
// Created by Christopher Yarp on 10/22/18.
//

#ifndef VITIS_CONTEXT_H
#define VITIS_CONTEXT_H


#include <memory>
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
    int subContext; ///<For nodes which can have multiple contexts, specifies which context (default to 0)

public:
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

};

/*@}*/

#endif //VITIS_CONTEXT_H
