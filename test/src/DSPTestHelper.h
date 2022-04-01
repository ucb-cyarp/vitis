//
// Created by Christopher Yarp on 10/7/21.
//

#ifndef VITIS_DSPTESTHELPER_H
#define VITIS_DSPTESTHELPER_H

#include "GraphCore/Design.h"

namespace DSPTestHelper {
    void runMultithreadGenForSinglePartitionDefaultSettings(Design &design, std::string outputDir, std::string designName);
};


#endif //VITIS_DSPTESTHELPER_H
