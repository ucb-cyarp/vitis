# vitis
An Optimizing DSP Compiler for Data Flow Graphs

## Status:
CI Status (master Branch): [![Build Status](https://travis-ci.com/ucb-cyarp/vitis.svg?token=3DFsVQ6rTxi6J46pKtZ6&branch=master)](https://travis-ci.com/ucb-cyarp/vitis)

## Dependencies:
- [CMake](https://cmake.org): Build Tool
- [Apache Xerces-C](https://xerces.apache.org/xerces-c): XML Parser
- [Google Test (gtest)](https://github.com/google/googletest): Testing Framework
- gcc or clang: C++ Compiler
- lcov (if using GCC): Coverage Report Generator
- llvm-profdata, llvm-cov (if using clang): Coverage Report Generator
- doxygen: Documentation Generator
- graphviz: Diagrams in Documentation (Used by Doxygen)

## Simulink Frontend:
[Simulink Support & Vitis Specific Simulink](src/docs/vitis_simulink.md)

## Optimization Passes:
[Currently Implemented Optimization Passes](src/docs/optimization_passes.md)

## Installation:
- Clone git repository

- Install Dependencies:
    - GCC Version 4.9 and Clang Version 3 Required Due to C++11 Regex Support
        - Tested with GCC 5 and Clang 3.8

### Dependencies:
#### Ubuntu:
  
```
sudo apt-get install build-essential
sudo apt-get install cmake
sudo apt-get install libxerces-c-dev
sudo apt-get install graphviz
sudo apt-get install doxygen
```

#### Mac:

- Install [xcode](https://developer.apple.com/xcode)
- Install [Brew](https://brew.sh)
- Install Dependencies from Brew:

To install the dependencies automatically from the provided ``Brewfile``, run the following command in the vitis directory:

```
brew bundle -v
```

To install the dependencies manually, run the following commands:
  
```
brew install cmake
brew install xerces-c
brew install graphviz
brew install doxygen
```
    
### Build:
To build the suite of vitis tools, run the following commands:

```
cd vitis
mkdir build
cd build
cmake ..
make
```

Executables will be built in the ``vitis/build`` directory
    
## Test:
To test your compiled vitis tools, run the following command inside the ``build`` directory:

```
make test ARGS="-V"
```

A test report will be written to stdout,
    
### Code Coverage:
To generate a code coverage report, run the following command inside the ``build`` directory:
```
make coverage
```
    
Coverage report will be copied to ``vitis/covReport``.
    
## Docs:
To generate documentation, run the following command in the ``build`` directory:

```
make docs
```
    
Alternatively, run doxygen in the ``vitis`` directory

```
cd vitis
doxygen
```
    
Documentation will be copied into ``vitis/docs``
    
## Usage
### Exporting Design from Simulink
There are a set of scripts (simulink_to_graph) which can export Simulink subsystems for use in vitis.  Note that the 
the simulink must use a subset of blocks supported by vitis.  See [Vitis Specific Simulink](src/docs/vitis_simulink.md) 
for details.

To export a subsystem from simulink:
1. Launch Matlab
2. Add vitis/scripts/matlab to the Matlab path
3. Set the Matlab working directory to where your design is located or add it to the Matlab path
4. Open your Simulink design.  If you need to execute a script to setup constants, execute that script at this point.
5. Identify the Subsystem you would like to export
6. Return to the Matlab command promp and run a varient of the following command:

```matlab
simulink_to_graphml('designName', 'pathToSubsystem', 'outputFilename.graphml', verbosity);
```

- ``designName`` should be replaced with the name of your Simulink design (ie. the filename without the ``.slx`` extension)
- ``pathToSubsystem`` should be replaced with the absolute path to the subsystem in your design.  The root of the path is
the name of your Simulink design and subsystems are speerated by ``/`` characters.
- ``outputFilename`` should be replaced with your desired output filename
- ``verbosity`` can be one of the following:
    - 0: Warnings Only
    - 1: Limited Status Updates
    - 2: Additional Status Updates (Including Expansion)
    - 3: All Status Updates 
  
For example, if your design was named "mySystem" and the subsystem you wanted to export was named "design" and was nested
within a subsystem named "container", you could run the following command to export the design to "myDesignExport.graphml"
```
simulink_to_graphml('mySystem', 'mySystem/container/design', 'myDesignExport.graphml', 3);
```

### Importing Simulink Exported Design
The GraphML file exported from Simulink contains some Simulink specific information that is not needed by vitis.  It
also has some options which need to be converted for use in vitis.

To generate a vitis representation of the design, run the following command (executable located in the 
``vitis/build`` directory):

```bash
simulinkGraphMLImporter inputfile.graphml outputfile.graphml
```

- ``inputfile.graphml`` is the file exported from simulink using the ``simulink_to_graphml`` script
- ``outputfile.graphml`` is the name of the new vitis design file that will be created by this program

For the above example, the command would look like this:
```bash
simulinkGraphMLImporter myDesignExport.graphml myDesignExport_vitis.graphml
```

### Generating C Code
There are 2 programs which can generate C code for a vitis design:
- multiThreadedGenerator: Generates a multi-threaded C implementation of the design
- singleThreadedGenerator: Generates a single-threaded C implementation of the design

Both applications have several options.  Documentation for each option can be accessed by running the application with
the ``--help`` argument.

There are only 3 required parameters for the programs
```bash
{multi,single}ThreadedGenerator inputFile.graphml outputDir designName
```

- ``inputFile`` is a vitis design file which was created using ``simulinkGraphMLImporter``
- ``outputDir`` is the name of the directory where the generated results will be placed
- ``designName`` is what the generated design will be named

There are several other important options including:
- ``--blockSize`` which sets the number of samples per executed block
- ``--fifoLength`` the number of blocks allocated in inter-partition FIFOs
- ``--ioFifoSize`` the nu,ber of blocks allocated in I/O FIFOs
- ``--PartitionMap`` which sets the partition number to CPU number mapping (see 
  [Vitis Specific Simulink](src/docs/vitis_simulink.md) on how to define partitions in your design.
- ``--telemDumpPrefix`` set the prefix for (and enables writing of) telemetry dump files used by 
  [vitisTelemetryDash](https://github.com/ucb-cyarp/vitisTelemetryDash)
- ``--SCHED_HEUR`` the scheduling heuristic to use

One possible command to generate a C implementation of our example design would be:
```bash
multiThreadedGenerator myDesignExport_vitis.graphml ./myDesignGen myDesign --emitGraphMLSched --schedHeur DFS --blockSize 64 --fifoLength 7 --ioFifoSize 128 --partitionMap [4,4,5,20,21]
```
