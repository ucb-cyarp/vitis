#Orig Version Provided by CLion
#cmake_minimum_required(VERSION 3.10)

#Version in Ubuntu 14.04
cmake_minimum_required(VERSION 3.5.1)

project(vitis)

set(CMAKE_CXX_STANDARD 11)

add_compile_options("$<$<CONFIG:DEBUG>:-O0>")
add_compile_options("$<$<CONFIG:RELEASE>:-O3>")

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
        src/HighLevelNodes/HighLevelNode.cpp
        src/HighLevelNodes/HighLevelNode.h
        src/HighLevelNodes/DiscreteFIR.cpp
        src/HighLevelNodes/DiscreteFIR.h
        src/BusNodes/VectorFan.cpp
        src/BusNodes/VectorFan.h
        src/BusNodes/VectorFanIn.cpp
        src/BusNodes/VectorFanIn.h
        src/BusNodes/VectorFanOut.cpp
        src/BusNodes/VectorFanOut.h)

#Add Include Directories for Project Srcs
include_directories(src)

#Create shared static lib for common files
add_library(VitisLib STATIC ${COMMON_SRCS})

#==== Dependencies ====
#Set up external library dependencies
#This was based off the CLion boost import snippit
# see https://cmake.org/cmake/help/latest/module/FindXercesC.html
find_package(XercesC)
include_directories(${XercesC_INCLUDE_DIRS})

#==== Executables ====
add_executable(simulinkGraphMLImporter src/Tools/simulinkGraphMLImporter.cpp)
target_link_libraries(simulinkGraphMLImporter VitisLib ${XercesC_LIBRARIES})

add_executable(simulinkGraphmlDOMPrint src/Tools/simulinkGraphmlDOMPrint.cpp)
target_link_libraries(simulinkGraphmlDOMPrint VitisLib ${XercesC_LIBRARIES})

add_executable(designExpander src/Tools/designExpander.cpp)
target_link_libraries(designExpander VitisLib ${XercesC_LIBRARIES})

add_executable(singleThreadedGenerator src/Tools/singleThreadedGenerator.cpp)
target_link_libraries(singleThreadedGenerator VitisLib ${XercesC_LIBRARIES})

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