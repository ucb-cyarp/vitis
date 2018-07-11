#Orig Version Provided by CLion
#cmake_minimum_required(VERSION 3.10)

#Version in Ubuntu 14.04
cmake_minimum_required(VERSION 3.9.2)

project(vitis)

set(CMAKE_CXX_STANDARD 11)

#Add DEBUG Option (For Now)
#TODO: Remove
add_compile_options("-DDEBUG")
#SET(CMAKE_CXX_FLAGS "-g -O0")
#SET(CMAKE_C_FLAGS "-g -O0")

#==== Vitis Framework ====
#List of Common Library Files (used by many targets)
set(COMMON_SRCS
        src/GraphCore/Node.cpp
        src/GraphCore/Node.h
        src/GraphCore/Port.cpp
        src/GraphCore/Port.h
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
        src/MasterNodes/MasterInput.cpp
        src/MasterNodes/MasterInput.h
        src/GraphMLTools/SimulinkGraphMLImporter.cpp
        src/GraphMLTools/SimulinkGraphMLImporter.h
        src/GraphCore/Design.cpp
        src/GraphCore/Design.h
        src/MasterNodes/MasterOutput.cpp
        src/MasterNodes/MasterOutput.h
        src/MasterNodes/MasterUnconnected.cpp
        src/MasterNodes/MasterUnconnected.h
        src/PrimitiveNodes/Sum.cpp
        src/PrimitiveNodes/Sum.h
        src/PrimitiveNodes/Product.cpp
        src/PrimitiveNodes/Product.h
        src/PrimitiveNodes/Delay.cpp
        src/PrimitiveNodes/Delay.h)

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

#==== Google Test ====
set(BASE_SRC_DIR "../src") #This is the base src directory from the perspective of the test cmake file
enable_testing()
add_subdirectory(test)