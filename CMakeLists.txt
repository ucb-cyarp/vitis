cmake_minimum_required(VERSION 3.10)
project(vitis)

set(CMAKE_CXX_STANDARD 11)

add_executable(vitis main.cpp Node.cpp Node.h Port.cpp Port.h SubSystem.cpp SubSystem.h Arc.cpp Arc.h GraphMLParameter.cpp GraphMLParameter.h DataType.cpp DataType.h EnabledSubSystem.cpp EnabledSubSystem.h EnableNode.cpp EnableNode.h EnableInput.cpp EnableInput.h EnableOutput.cpp EnableOutput.h EnabledExpandedNode.cpp EnabledExpandedNode.h ExpandedNode.cpp ExpandedNode.h)