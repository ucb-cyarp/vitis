cmake_minimum_required(VERSION 3.10)
project(vitis)

set(CMAKE_CXX_STANDARD 11)

include_directories(src)
add_executable(vitis src/main.cpp src/GraphCore/Node.cpp src/GraphCore/Node.h src/GraphCore/Port.cpp src/GraphCore/Port.h src/GraphCore/SubSystem.cpp src/GraphCore/SubSystem.h src/GraphCore/Arc.cpp src/GraphCore/Arc.h src/GraphCore/GraphMLParameter.cpp src/GraphCore/GraphMLParameter.h src/GraphCore/DataType.cpp src/GraphCore/DataType.h src/GraphCore/EnabledSubSystem.cpp src/GraphCore/EnabledSubSystem.h src/GraphCore/EnableNode.cpp src/GraphCore/EnableNode.h src/GraphCore/EnableInput.cpp src/GraphCore/EnableInput.h src/GraphCore/EnableOutput.cpp src/GraphCore/EnableOutput.h src/GraphCore/EnabledExpandedNode.cpp src/GraphCore/EnabledExpandedNode.h src/GraphCore/ExpandedNode.cpp src/GraphCore/ExpandedNode.h src/MasterNodes/MasterInput.cpp src/MasterNodes/MasterInput.h)