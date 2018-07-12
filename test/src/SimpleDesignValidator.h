//
// Created by Christopher Yarp on 7/12/18.
//

#ifndef VITIS_SIMPLEDESIGNVALIDATOR_H
#define VITIS_SIMPLEDESIGNVALIDATOR_H

#include "GraphCore/Design.h"

#include <memory>
#include <vector>
#include <set>

#include "gtest/gtest.h"
#include "GraphMLTools/SimulinkGraphMLImporter.h"
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
 * @brief Class containing method to validate the Simple Simulink Design Stimulus (in test/stimulus/simulink/basic/simple.graphml)
 */
class SimpleDesignValidator {
public:
    /**
     * @brief Validate the imported design
     * @param design to validate
     */
    static void validate(Design &design);
};


#endif //VITIS_SIMPLEDESIGNVALIDATOR_H
