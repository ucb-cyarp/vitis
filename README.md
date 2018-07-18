# vitis

## An Optimizing DSP Compiler for Data Flow Graphs

### Status:

CI Status: [![Build Status](https://travis-ci.com/cyarp/vitis.svg?token=3DFsVQ6rTxi6J46pKtZ6&branch=master "CI Build Status")](https://travis-ci.com/cyarp/vitis)

### Dependencies:
- [CMake](https://cmake.org): Build Tool
- [Apache Xerces-C](https://xerces.apache.org/xerces-c): XML Parser
- [Google Test (gtest)](https://github.com/google/googletest): Testing Framework
- gcc or clang: C++ Compiler

### Installation:
- Clone git repository

- Install Dependencies:
    - GCC Version 4.9 and Clang Version 3 Required Due to C++11 Regex Support
        - Tested with GCC 5 and Clang 3.8

    - Ubuntu
    
        ```
        sudo apt-get install build-essential
        sudo apt-get install cmake
        sudo apt-get install libxerces-c-dev
        ```

    - Mac:
        - Install xcode
        - Install Brew
        - Install Dependencies from Brew:

            ```
            brew install cmake
            brew install xerces-c
            ```
    
- Build

    ```
    cd vitis
    mkdir build
    cd build
    cmake ..
    make
    ```
    
- Test
    ```
    make test ARGS="-V"
    ```