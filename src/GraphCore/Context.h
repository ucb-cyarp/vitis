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

class Context {
private:
    std::shared_ptr<ContextRoot> contextRoot; ///<The node which is the root of the context
    int subContext; ///<For nodes which can have multiple contexts, specifies which context (default to 0)

public:
    std::shared_ptr<ContextRoot> getContextRoot() const;
    void setContextRoot(const std::shared_ptr<ContextRoot> &contextRoot);
    int getSubContext() const;
    void setSubContext(int subContext);

    bool operator==(const Context &rhs) const;
    bool operator!=(const Context &rhs) const;

};

/*@}*/

#endif //VITIS_CONTEXT_H
