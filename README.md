# vitis

## An Optimizing DSP Compiler for Data Flow Graphs

### Status:
CI Status (master Branch): [![Build Status](https://travis-ci.com/ucb-cyarp/vitis.svg?token=3DFsVQ6rTxi6J46pKtZ6&branch=master)](https://travis-ci.com/ucb-cyarp/vitis)

### Simulink Frontend:
[Simulink Support & Vitis Specific Simulink](src/docs/vitis_simulink.md)

### Optimization Passes:
[Currently Implemented Optimization Passes](src/docs/optimization_passes.md)

### Dependencies:
- [CMake](https://cmake.org): Build Tool
- [Apache Xerces-C](https://xerces.apache.org/xerces-c): XML Parser
- [Google Test (gtest)](https://github.com/google/googletest): Testing Framework
- gcc or clang: C++ Compiler
- lcov (if using GCC): Coverage Report Generator
- llvm-profdata, llvm-cov (if using clang): Coverage Report Generator
- doxygen: Documentation Generator
- graphviz: Diagrams in Documentation (Used by Doxygen)

### Installation:
- Clone git repository

- Install Dependencies:
    - GCC Version 4.9 and Clang Version 3 Required Due to C++11 Regex Support
        - Tested with GCC 5 and Clang 3.8

#### Dependencies:
##### Ubuntu
    
```
sudo apt-get install build-essential
sudo apt-get install cmake
sudo apt-get install libxerces-c-dev
sudo apt-get install graphviz
sudo apt-get install doxygen
```

##### Mac:
- Install [xcode](https://developer.apple.com/xcode)
- Install [Brew](https://brew.sh)
- Install Dependencies from Brew:

```
brew install cmake
brew install xerces-c
brew install graphviz
brew install doxygen
```
    
#### Build:

```
cd vitis
mkdir build
cd build
cmake ..
make
```

Executables will be built in the vitis/build directory
    
### Test:

```
make test ARGS="-V"
```

A test report will be written to stdout,
    
### Code Coverage:

```
make coverage
```
    
Coverage report will be copied to vitis/covReport.
    
### Docs:

```
make docs
```
    
Alternatively, run doxygen in the vitis directory

```
cd vitis
doxygen
```
    
Documentation will be copied into vitis/docs
    
