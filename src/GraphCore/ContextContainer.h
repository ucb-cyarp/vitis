//
// Created by Christopher Yarp on 10/22/18.
//

#ifndef VITIS_CONTEXTCONTAINER_H
#define VITIS_CONTEXTCONTAINER_H

#include <memory>
#include "SubSystem.h"
#include "Context.h"

/**
 * \addtogroup GraphCore Graph Core
 */
/*@{*/

/**
 * @brief A container class used to contain nodes within a context.
 *
 * This container is typically not conisdered part of the design hierarchy.
 * It is used, however, when performing actions that need to keep
 */
class ContextContainer : public SubSystem {
private:
    Context context; ///<The context corresponding to this context container
    //For nodes that have multiple contexts (ex. muxes), a context container is created for each context
    //The contexts are contained in a ContextFamilyContainer

};

/*@}*/

#endif //VITIS_CONTEXTCONTAINER_H
