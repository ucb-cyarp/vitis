cmake_minimum_required(VERSION 3.10)
project(vitis)

set(CMAKE_CXX_STANDARD 11)

add_executable(vitis main.cpp Node.cpp Node.h Port.cpp Port.h SubSystem.cpp SubSystem.h Arc.cpp Arc.h GraphMLParameter.cpp GraphMLParameter.h)