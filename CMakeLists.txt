#Orig Version Provided by CLion
#cmake_minimum_required(VERSION 3.10)

#Version in Ubuntu 14.04
cmake_minimum_required(VERSION 3.5.1)

project(vitis)

#Using technique from Marcus D. Hanwell on https://blog.kitware.com/cmake-and-the-default-build-type/ to set default build type
set(default_build_type "Release")
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "Setting build type to '${default_build_type}' as none was specified.")
    set(CMAKE_BUILD_TYPE "${default_build_type}" CACHE
            STRING "Choose the type of build." FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
            "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

set(CMAKE_CXX_STANDARD 11)

if ("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
    # using Clang
    add_compile_options("$<$<CONFIG:DEBUG>:-fprofile-instr-generate>")
    add_compile_options("$<$<CONFIG:DEBUG>:-fcoverage-mapping>")
    set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} -fprofile-instr-generate -fcoverage-mapping")
elseif("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU")
    # using GCC
    add_compile_options("$<$<CONFIG:DEBUG>:-ftest-coverage>")
    add_compile_options("$<$<CONFIG:DEBUG>:-fprofile-arcs>")
    set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} -ftest-coverage -fprofile-arcs")
endif()

#Add DEBUG Option (For Now)
#TODO: Remove
add_compile_options("-DDEBUG")
#SET(CMAKE_CXX_FLAGS "-g -O0")
#SET(CMAKE_C_FLAGS "-g -O0")

#==== Vitis Framework ====
#List of Common Library Files (used by many targets)
set(COMMON_SRCS
        src/General/GeneralHelper.cpp
        src/General/GeneralHelper.h
        src/General/FixedPointHelpers.cpp
        src/General/FixedPointHelpers.h
        src/General/GraphAlgs.cpp
        src/General/GraphAlgs.h
        src/General/TopologicalSortParameters.cpp
        src/General/TopologicalSortParameters.h
        src/General/DSPHelpers.cpp
        src/General/DSPHelpers.h
        src/General/EmitterHelpers.cpp
        src/General/EmitterHelpers.h
        src/General/ErrorHelpers.cpp
        src/General/ErrorHelpers.h
        src/General/FileIOHelpers.cpp
        src/General/FileIOHelpers.h
        src/GraphCore/SchedParams.cpp
        src/GraphCore/SchedParams.h
        src/GraphCore/Node.cpp
        src/GraphCore/Node.h
        src/GraphCore/Port.cpp
        src/GraphCore/Port.h
        src/GraphCore/InputPort.cpp
        src/GraphCore/InputPort.h
        src/GraphCore/OutputPort.cpp
        src/GraphCore/OutputPort.h
        src/GraphCore/OrderConstraintInputPort.cpp
        src/GraphCore/OrderConstraintInputPort.h
        src/GraphCore/OrderConstraintOutputPort.cpp
        src/GraphCore/OrderConstraintOutputPort.h
        src/GraphCore/EnablePort.cpp
        src/GraphCore/EnablePort.h
        src/GraphCore/SelectPort.cpp
        src/GraphCore/SelectPort.h
        src/GraphCore/SubSystem.cpp
        src/GraphCore/SubSystem.h
        src/GraphCore/Arc.cpp
        src/GraphCore/Arc.h
        src/GraphCore/GraphMLParameter.cpp
        src/GraphCore/GraphMLParameter.h
        src/GraphCore/DataType.cpp
        src/GraphCore/DataType.h
        src/GraphCore/NumericValue.cpp
        src/GraphCore/NumericValue.h
        src/GraphCore/EnabledSubSystem.cpp
        src/GraphCore/EnabledSubSystem.h
        src/GraphCore/EnableNode.cpp
        src/GraphCore/EnableNode.h
        src/GraphCore/EnableInput.cpp
        src/GraphCore/EnableInput.h
        src/GraphCore/EnableOutput.cpp
        src/GraphCore/EnableOutput.h
        src/GraphCore/EnabledExpandedNode.cpp
        src/GraphCore/EnabledExpandedNode.h
        src/GraphCore/ExpandedNode.cpp
        src/GraphCore/ExpandedNode.h
        src/GraphCore/NodeFactory.h
        src/GraphCore/Variable.cpp
        src/GraphCore/Variable.h
        src/GraphCore/CExpr.cpp
        src/GraphCore/CExpr.h
        src/GraphCore/Context.cpp
        src/GraphCore/Context.h
        src/GraphCore/ContextHelper.cpp
        src/GraphCore/ContextHelper.h
        src/GraphCore/ContextContainer.cpp
        src/GraphCore/ContextContainer.h
        src/GraphCore/ContextFamilyContainer.cpp
        src/GraphCore/ContextFamilyContainer.h
        src/GraphCore/ContextRoot.cpp
        src/GraphCore/ContextRoot.h
        src/GraphCore/ContextVariableUpdate.cpp
        src/GraphCore/ContextVariableUpdate.h
        src/GraphCore/StateUpdate.cpp
        src/GraphCore/StateUpdate.h
        src/GraphCore/DummyReplica.cpp
        src/GraphCore/DummyReplica.h
        src/MasterNodes/MasterNode.cpp
        src/MasterNodes/MasterNode.h
        src/MasterNodes/MasterInput.cpp
        src/MasterNodes/MasterInput.h
        src/GraphMLTools/GraphMLImporter.cpp
        src/GraphMLTools/GraphMLImporter.h
        src/GraphMLTools/GraphMLExporter.cpp
        src/GraphMLTools/GraphMLExporter.h
        src/GraphMLTools/XMLTranscoder.cpp
        src/GraphMLTools/XMLTranscoder.h
        src/GraphMLTools/GraphMLHelper.cpp
        src/GraphMLTools/GraphMLHelper.h
        src/GraphMLTools/GraphMLDialect.h
        src/GraphCore/Design.cpp
        src/GraphCore/Design.h
        src/MasterNodes/MasterOutput.cpp
        src/MasterNodes/MasterOutput.h
        src/MasterNodes/MasterUnconnected.cpp
        src/MasterNodes/MasterUnconnected.h
        src/PrimitiveNodes/PrimitiveNode.cpp
        src/PrimitiveNodes/PrimitiveNode.h
        src/PrimitiveNodes/Sum.cpp
        src/PrimitiveNodes/Sum.h
        src/PrimitiveNodes/Product.cpp
        src/PrimitiveNodes/Product.h
        src/PrimitiveNodes/Delay.cpp
        src/PrimitiveNodes/Delay.h
        src/PrimitiveNodes/Constant.cpp
        src/PrimitiveNodes/Constant.h
        src/PrimitiveNodes/Mux.cpp
        src/PrimitiveNodes/Mux.h
        src/PrimitiveNodes/DataTypeConversion.cpp
        src/PrimitiveNodes/DataTypeConversion.h
        src/PrimitiveNodes/Compare.cpp
        src/PrimitiveNodes/Compare.h
        src/PrimitiveNodes/LUT.cpp
        src/PrimitiveNodes/LUT.h
        src/PrimitiveNodes/ComplexToRealImag.cpp
        src/PrimitiveNodes/ComplexToRealImag.h
        src/PrimitiveNodes/RealImagToComplex.cpp
        src/PrimitiveNodes/RealImagToComplex.h
        src/PrimitiveNodes/DataTypeDuplicate.cpp
        src/PrimitiveNodes/DataTypeDuplicate.h
        src/PrimitiveNodes/LogicalOperator.cpp
        src/PrimitiveNodes/LogicalOperator.h
        src/PrimitiveNodes/UnsupportedSink.cpp
        src/PrimitiveNodes/UnsupportedSink.h
        src/PrimitiveNodes/BlackBox.cpp
        src/PrimitiveNodes/BlackBox.h
        src/PrimitiveNodes/SimulinkCoderFSM.cpp
        src/PrimitiveNodes/SimulinkCoderFSM.h
        src/PrimitiveNodes/ReinterpretCast.cpp
        src/PrimitiveNodes/ReinterpretCast.h
        src/PrimitiveNodes/BitwiseOperator.cpp
        src/PrimitiveNodes/BitwiseOperator.h
        src/PrimitiveNodes/Exp.cpp
        src/PrimitiveNodes/Exp.h
        src/PrimitiveNodes/Ln.cpp
        src/PrimitiveNodes/Ln.h
        src/PrimitiveNodes/Trigonometry/Sin.cpp
        src/PrimitiveNodes/Trigonometry/Sin.h
        src/PrimitiveNodes/Trigonometry/Cos.cpp
        src/PrimitiveNodes/Trigonometry/Cos.h
        src/PrimitiveNodes/Trigonometry/Atan.cpp
        src/PrimitiveNodes/Trigonometry/Atan.h
        src/PrimitiveNodes/Trigonometry/Atan2.cpp
        src/PrimitiveNodes/Trigonometry/Atan2.h
        src/PrimitiveNodes/WrappingCounter.cpp
        src/PrimitiveNodes/WrappingCounter.h
        src/PrimitiveNodes/InnerProduct.cpp
        src/PrimitiveNodes/InnerProduct.h
        src/PrimitiveNodes/Concatenate.cpp
        src/PrimitiveNodes/Concatenate.h
        src/PrimitiveNodes/Select.cpp
        src/PrimitiveNodes/Select.h
        src/PrimitiveNodes/Reshape.cpp
        src/PrimitiveNodes/Reshape.h
        src/MediumLevelNodes/MediumLevelNode.cpp
        src/MediumLevelNodes/MediumLevelNode.h
        src/MediumLevelNodes/Gain.cpp
        src/MediumLevelNodes/Gain.h
        src/MediumLevelNodes/CompareToConstant.cpp
        src/MediumLevelNodes/CompareToConstant.h
        src/MediumLevelNodes/ThresholdSwitch.cpp
        src/MediumLevelNodes/ThresholdSwitch.h
        src/MediumLevelNodes/SimulinkMultiPortSwitch.cpp
        src/MediumLevelNodes/SimulinkMultiPortSwitch.h
        src/MediumLevelNodes/Saturate.cpp
        src/MediumLevelNodes/Saturate.h
        src/MediumLevelNodes/NCO.cpp
        src/MediumLevelNodes/NCO.h
        src/MediumLevelNodes/DigitalModulator.cpp
        src/MediumLevelNodes/DigitalModulator.h
        src/MediumLevelNodes/DigitalDemodulator.cpp
        src/MediumLevelNodes/DigitalDemodulator.h
        src/MediumLevelNodes/SimulinkSelect.cpp
        src/MediumLevelNodes/SimulinkSelect.h
        src/MediumLevelNodes/SimulinkBitwiseOperator.cpp
        src/MediumLevelNodes/SimulinkBitwiseOperator.h
        src/MediumLevelNodes/SimulinkBitShift.cpp
        src/MediumLevelNodes/SimulinkBitShift.h
        src/HighLevelNodes/HighLevelNode.cpp
        src/HighLevelNodes/HighLevelNode.h
        src/HighLevelNodes/DiscreteFIR.cpp
        src/HighLevelNodes/DiscreteFIR.h
        src/PrimitiveNodes/TappedDelay.cpp
        src/PrimitiveNodes/TappedDelay.h
        src/MultiThread/MultiThreadEmitterHelpers.cpp
        src/MultiThread/MultiThreadEmitterHelpers.h
        src/MultiThread/MultiThreadTransformHelpers.cpp
        src/MultiThread/MultiThreadTransformHelpers.h
        src/MultiThread/ThreadCrossingFIFO.cpp
        src/MultiThread/ThreadCrossingFIFO.h
        src/MultiThread/StreamIOThread.cpp
        src/MultiThread/StreamIOThread.h
        src/MultiThread/LocklessThreadCrossingFIFO.cpp
        src/MultiThread/LocklessThreadCrossingFIFO.h
        src/MultiThread/PartitionParams.cpp
        src/MultiThread/PartitionParams.h
        src/MultiThread/ThreadCrossingFIFOParameters.cpp
        src/MultiThread/ThreadCrossingFIFOParameters.h
        src/MultiThread/ConstIOThread.cpp
        src/MultiThread/ConstIOThread.h
        src/MultiRate/MultiRateHelpers.cpp
        src/MultiRate/MultiRateHelpers.h
        src/MultiRate/ClockDomain.cpp
        src/MultiRate/ClockDomain.h
        src/MultiRate/RateChange.cpp
        src/MultiRate/RateChange.h
        src/MultiRate/Upsample.cpp
        src/MultiRate/Upsample.h
        src/MultiRate/Downsample.cpp
        src/MultiRate/Downsample.h
        src/MultiRate/Repeat.cpp
        src/MultiRate/Repeat.h
        src/MultiRate/UpsampleClockDomain.cpp
        src/MultiRate/UpsampleClockDomain.h
        src/MultiRate/DownsampleClockDomain.cpp
        src/MultiRate/DownsampleClockDomain.h
        src/MultiRate/UpsampleOutput.cpp
        src/MultiRate/UpsampleOutput.h
        src/MultiRate/RepeatOutput.cpp
        src/MultiRate/RepeatOutput.h
        src/MultiRate/DownsampleInput.cpp
        src/MultiRate/DownsampleInput.h
        src/BusNodes/VectorFan.cpp
        src/BusNodes/VectorFan.h
        src/BusNodes/VectorFanIn.cpp
        src/BusNodes/VectorFanIn.h
        src/BusNodes/VectorFanOut.cpp
        src/BusNodes/VectorFanOut.h
        src/Estimators/EstimatorCommon.cpp
        src/Estimators/EstimatorCommon.h
        src/Estimators/ComputationEstimator.cpp
        src/Estimators/ComputationEstimator.h
        src/Estimators/CommunicationEstimator.cpp
        src/Estimators/CommunicationEstimator.h
        src/Estimators/PartitionNode.cpp
        src/Estimators/PartitionNode.h
        src/Estimators/PartitionCrossing.cpp
        src/Estimators/PartitionCrossing.h
        src/MultiThread/LocklessInPlaceThreadCrossingFIFO.cpp
        src/MultiThread/LocklessInPlaceThreadCrossingFIFO.h
        src/Blocking/BlockingDomain.cpp
        src/Blocking/BlockingDomain.h
        src/Blocking/BlockingBoundary.cpp
        src/Blocking/BlockingBoundary.h
        src/Blocking/BlockingInput.cpp
        src/Blocking/BlockingInput.h
        src/Blocking/BlockingOutput.cpp
        src/Blocking/BlockingOutput.h
        src/Blocking/BlockingHelpers.cpp
        src/Blocking/BlockingHelpers.h
        )

#Add Include Directories for Project Srcs
include_directories(src)

#Create shared static lib for common files
add_library(VitisLib STATIC ${COMMON_SRCS})

#==== Dependencies ====
#Set up external library dependencies
#This was based off the CLion boost import snippit
# see https://cmake.org/cmake/help/latest/module/FindXercesC.html
find_package(XercesC)
find_package(Boost REQUIRED filesystem)
include_directories(${XercesC_INCLUDE_DIRS} ${Boost_INCLUDE_DIRS})

#==== Executables ====
add_executable(simulinkGraphMLImporter src/Tools/simulinkGraphMLImporter.cpp)
target_link_libraries(simulinkGraphMLImporter VitisLib ${XercesC_LIBRARIES} ${Boost_LIBRARIES})

add_executable(simulinkGraphmlDOMPrint src/Tools/simulinkGraphmlDOMPrint.cpp)
target_link_libraries(simulinkGraphmlDOMPrint VitisLib ${XercesC_LIBRARIES} ${Boost_LIBRARIES})

add_executable(designExpander src/Tools/designExpander.cpp)
target_link_libraries(designExpander VitisLib ${XercesC_LIBRARIES} ${Boost_LIBRARIES})

add_executable(singleThreadedGenerator src/Tools/singleThreadedGenerator.cpp)
target_link_libraries(singleThreadedGenerator VitisLib ${XercesC_LIBRARIES} ${Boost_LIBRARIES})

add_executable(multiThreadedGenerator src/Tools/multiThreadedGenerator.cpp)
target_link_libraries(multiThreadedGenerator VitisLib ${XercesC_LIBRARIES} ${Boost_LIBRARIES})

add_executable(checkPlatformDataTypes src/Tools/checkPlatformDataTypes.cpp)

#==== Doxygen ====
#See https://cmake.org/cmake/help/v3.9/module/FindDoxygen.html
find_package(Doxygen
        REQUIRED dot)

#Not using doxygen_add_docs since we already have a Doxyfile
add_custom_target(docs
        ${DOXYGEN_EXECUTABLE}
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})

#==== Google Test ====
set(BASE_SRC_DIR "../src") #This is the base src directory from the perspective of the test cmake file
enable_testing()
add_subdirectory(test)

#==== Coverage ====
if ("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
    # using Clang
    add_custom_target(coverage ${CMAKE_SOURCE_DIR}/scripts/bash/clangCoverage.sh)
elseif("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU")
    # using GCC
    add_custom_target(coverage ${CMAKE_SOURCE_DIR}/scripts/bash/gccCoverage.sh)
endif()