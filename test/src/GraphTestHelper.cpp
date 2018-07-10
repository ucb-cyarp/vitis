//
// Created by Christopher Yarp on 7/10/18.
//

#include "GraphTestHelper.h"

void GraphTestHelper::verifyArcLinks(std::shared_ptr<Arc> arc) {
    std::shared_ptr<Port> srcPort = arc->getSrcPort();
    std::shared_ptr<Port> dstPort = arc->getDstPort();

    //Check Link
    std::set<std::shared_ptr<Arc>> srcPortArcs = srcPort->getArcs();
    ASSERT_TRUE(srcPortArcs.find(arc) != srcPortArcs.end());

    std::set<std::shared_ptr<Arc>> dstPortArcs = dstPort->getArcs();
    ASSERT_TRUE(dstPortArcs.find(arc) != dstPortArcs.end());
}

void GraphTestHelper::verifyDataType(DataType dataType, bool floatingPtExpected, bool signedTypeExpected,
                                     bool complexExpected, int totalBitsExpected, int fractionalBitsExpected,
                                     int widthExpected) {
    ASSERT_EQ(dataType.isFloatingPt(), floatingPtExpected);
    ASSERT_EQ(dataType.isSignedType(), signedTypeExpected);
    ASSERT_EQ(dataType.isComplex(), complexExpected);
    ASSERT_EQ(dataType.getTotalBits(), totalBitsExpected);
    ASSERT_EQ(dataType.getFractionalBits(), fractionalBitsExpected);
    ASSERT_EQ(dataType.getWidth(), widthExpected);
}
