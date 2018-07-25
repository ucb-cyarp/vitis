//
// Created by Christopher Yarp on 7/24/18.
//

#ifndef VITIS_NESTEDSUBSYSTEMVALIDATOR_H
#define VITIS_NESTEDSUBSYSTEMVALIDATOR_H

#include "GraphCore/Design.h"

#include <memory>
#include <vector>
#include <set>

#include "gtest/gtest.h"
#include "GraphMLTools/GraphMLImporter.h"
#include "GraphCore/Design.h"
#include "GraphCore/Port.h"
#include "MasterNodes/MasterInput.h"
#include "MasterNodes/MasterOutput.h"
#include "MasterNodes/MasterUnconnected.h"
#include "PrimitiveNodes/Sum.h"
#include "PrimitiveNodes/Product.h"
#include "PrimitiveNodes/Delay.h"

#include "GraphTestHelper.h"

/**
 * @brief Class containing method to validate the nested_subsystem Simulink Design Stimulus (in test/stimulus/simulink/basic/nested.graphml)
 */
class NestedSubSystemValidator {
public:
    /**
     * @brief Validate the imported design
     * @param design to validate
     */
    static void validate(Design &design);
};


#endif //VITIS_NESTEDSUBSYSTEMVALIDATOR_H
