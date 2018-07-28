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
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GCC")
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
        src/GraphCore/Node.cpp
        src/GraphCore/Node.h
        src/GraphCore/Port.cpp
        src/GraphCore/Port.h
        src/GraphCore/InputPort.cpp
        src/GraphCore/InputPort.h
        src/GraphCore/OutputPort.cpp
        src/GraphCore/OutputPort.h
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
add_executable(vitis src/main.cpp)
target_link_libraries(vitis VitisLib ${XercesC_LIBRARIES})

add_executable(simulinkGraphMLImporter src/Tools/simulinkGraphMLImporter.cpp)
target_link_libraries(simulinkGraphMLImporter VitisLib ${XercesC_LIBRARIES})

add_executable(simulinkGraphmlDOMPrint src/Tools/simulinkGraphmlDOMPrint.cpp)
target_link_libraries(simulinkGraphmlDOMPrint VitisLib ${XercesC_LIBRARIES})

add_executable(designExpander src/Tools/designExpander.cpp)
target_link_libraries(designExpander VitisLib ${XercesC_LIBRARIES})

#==== Google Test ====
set(BASE_SRC_DIR "../src") #This is the base src directory from the perspective of the test cmake file
enable_testing()
add_subdirectory(test)