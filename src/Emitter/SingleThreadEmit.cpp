//
// Created by Christopher Yarp on 9/2/21.
//

#include <iostream>
#include <fstream>
#include "SingleThreadEmit.h"
#include "General/GeneralHelper.h"

void SingleThreadEmit::emitSingleThreadedCBenchmarkingDrivers(Design &design, std::string path, std::string fileName, std::string designName, int blockSize) {
    emitSingleThreadedCBenchmarkingDriverConst(design, path, fileName, designName, blockSize);
    emitSingleThreadedCBenchmarkingDriverMem(design, path, fileName, designName, blockSize);
}

void
SingleThreadEmit::emitSingleThreadedCBenchmarkingDriverConst(Design &design, std::string path, std::string fileName, std::string designName, int blockSize) {
    std::string fileNameUpper =  GeneralHelper::toUpper(fileName);

    //#### Emit Kernel File & Header ####
    std::ofstream benchKernel;
    benchKernel.open(path+"/"+fileName+"_benchmark_kernel.cpp", std::ofstream::out | std::ofstream::trunc);

    benchKernel << "#include \"" << fileName << ".h" << "\"" << std::endl;
    benchKernel << "#include \"intrin_bench_default_defines.h\"" << std::endl;
    benchKernel << "#include \"" << VITIS_TYPE_NAME << ".h\"" << std::endl;
    benchKernel << "void bench_"+fileName+"()\n"
                                          "{\n";

    //Reset
    benchKernel << "\t" << designName << "_reset();\n";

    benchKernel << "\tOutputType output;\n"
                   "\tunsigned long outputCount;\n";

    //Generate loop
    std::pair<std::vector<Variable>, std::vector<std::pair<int, int>>> inputVarRatePairs = design.getCInputVariables();
    std::vector<Variable> inputVars = inputVarRatePairs.first;
    unsigned long numInputVars = inputVars.size();

    std::vector<NumericValue> defaultArgs;
    for (unsigned long i = 0; i < numInputVars; i++) {
        DataType type = inputVars[i].getDataType();
        NumericValue val;
        val.setRealInt(1);
        val.setImagInt(1);
        val.setComplex(type.isComplex());
        val.setFractional(false);

        defaultArgs.push_back(val);
    }

    std::string fctnCall = designName + "(";

    //TODO: Update for clock domains
    //For a block size greater than 1, constant arrays need to be created
    for (unsigned long i = 0; i < numInputVars; i++) {
        //Expand the size of the variable to account for the block size;
        Variable blockInputVar = inputVars[i];
        DataType blockDT = blockInputVar.getDataType().expandForBlock(blockSize);

        blockInputVar.setDataType(blockDT);

        std::vector<int> blockedDim = blockDT.getDimensions();

        //Insert comma between variables
        if (i > 0) {
            fctnCall += ", ";
        }

        if(!blockDT.isScalar()) {
            //Create a constant array and pass it as the argument
            benchKernel << "\t" << blockInputVar.getCVarDecl(false, true, false, true)
                        << EmitterHelpers::arrayLiteral(blockedDim, defaultArgs[i].toStringComponent(false, blockDT)) << ";";
            fctnCall += blockInputVar.getCVarName(false);
        }else{
            //Just pass the scalar argument
            fctnCall += defaultArgs[i].toStringComponent(false, blockDT);
        }

        if (inputVars[i].getDataType().isComplex()) {
            fctnCall += ", "; //This is guarenteed to be needed as the real component will come first

            if(!blockDT.isScalar()) {
                //Create a constant array and pass it as the argument
                benchKernel << "\t" << blockInputVar.getCVarDecl(true, true, false, true)
                            << EmitterHelpers::arrayLiteral(blockedDim, defaultArgs[i].toStringComponent(true, blockDT)) << ";";
                fctnCall += blockInputVar.getCVarName(true);
            }else{
                //Just pass the scalar argument
                fctnCall += defaultArgs[i].toStringComponent(true, blockDT);
            }
        }
    }

    if (numInputVars > 0) {
        fctnCall += ", ";
    }
    fctnCall += "&output, &outputCount)";
    if(blockSize > 1){
        benchKernel << "\tint iterLimit = STIM_LEN/" << blockSize << ";" << std::endl;
    }else{
        benchKernel << "\tint iterLimit = STIM_LEN;" << std::endl;
    }
    benchKernel << "\tfor(int i = 0; i<iterLimit; i++)\n"
                   "\t{\n"
                   "\t\t" + fctnCall + ";\n"
                                       "\t}\n"
                                       "}" << std::endl;
    benchKernel.close();

    std::ofstream benchKernelHeader;
    benchKernelHeader.open(path+"/"+fileName+"_benchmark_kernel.h", std::ofstream::out | std::ofstream::trunc);

    benchKernelHeader << "#ifndef " << fileNameUpper << "_BENCHMARK_KERNEL_H" << std::endl;
    benchKernelHeader << "#define " << fileNameUpper << "_BENCHMARK_KERNEL_H" << std::endl;
    benchKernelHeader << "void bench_"+fileName+"();" << std::endl;
    benchKernelHeader << "#endif" << std::endl;
    benchKernelHeader.close();

    //#### Emit Driver File ####
    std::ofstream benchDriver;
    benchDriver.open(path+"/"+fileName+"_benchmark_driver.cpp", std::ofstream::out | std::ofstream::trunc);

    benchDriver << "#include <map>" << std::endl;
    benchDriver << "#include <string>" << std::endl;
    benchDriver << "#include \"intrin_bench_default_defines.h\"" << std::endl;
    benchDriver << "#include \"benchmark_throughput_test.h\"" << std::endl;
    benchDriver << "#include \"kernel_runner.h\"" << std::endl;
    benchDriver << "#include \"" + fileName + "_benchmark_kernel.h\"" << std::endl;

    //If performing a load, exe, store benchmark, define an input structure
    //benchDriver << getCInputPortStructDefn() << std::endl;

    //Driver will define a zero arg kernel that sets reasonable inputs and repeatedly runs the function.
    //The function will be compiled in a seperate object file and should not be in-lined (potentially resulting in eronious
    //optimizations for the purpose of benchmarking since we are feeding dummy data in).  This should be the case so long as
    //the compiler flag -flto is not used durring compile and linkign (https://stackoverflow.com/questions/35922966/lto-with-llvm-and-cmake)
    //(https://llvm.org/docs/LinkTimeOptimization.html), (https://clang.llvm.org/docs/CommandGuide/clang.html),
    //(https://gcc.gnu.org/wiki/LinkTimeOptimization), (https://gcc.gnu.org/onlinedocs/gccint/LTO-Overview.html).

    //Emit name, file, and units string
    benchDriver << "std::string getBenchSuiteName(){\n\treturn \"Generated System: " + designName + "\";\n}" << std::endl;
    benchDriver << "std::string getReportFileName(){\n\treturn \"" + fileName + "_benchmarking_report\";\n}" << std::endl;
    benchDriver << "std::string getReportUnitsName(){\n\treturn \"STIM_LEN: \" + std::to_string(STIM_LEN) + \" (Samples/Vector/Trial), TRIALS: \" + std::to_string(TRIALS);\n}" << std::endl;

    //Emit Benchmark Report Selection
    benchDriver << "void getBenchmarksToReport(std::vector<std::string> &kernels, std::vector<std::string> &vec_ext){\n"
                   "\tkernels.push_back(\"" + designName + "\");\tvec_ext.push_back(\"Single Threaded\");\n}" << std::endl;

    //Emit Benchmark Type Report Selection
    std::string typeStr = "";
    if(numInputVars > 0){
        typeStr = inputVars[0].getDataType().toString(DataType::StringStyle::C);
        typeStr += inputVars[0].getDataType().isComplex() ? " (c)" : " (r)";
    }

    for(unsigned long i = 1; i<numInputVars; i++){
        typeStr += ", " + inputVars[i].getDataType().toString(DataType::StringStyle::C);
        typeStr += inputVars[i].getDataType().isComplex() ? " (c)" : " (r)";
    }

    benchDriver << "std::vector<std::string> getVarientsToReport(){\n"
                   "\tstd::vector<std::string> types;\n"
                   "\ttypes.push_back(\"" + typeStr + "\");\n"
                                                      "\treturn types;\n}" << std::endl;

    //Generate call to loop
    benchDriver << "std::map<std::string, std::map<std::string, Results*>*> runBenchSuite(Profiler* profiler, int* cpu_num_int){\n"
                   "\tstd::map<std::string, std::map<std::string, Results*>*> kernel_results;\n"
                   "\n"
                   "\tstd::map<std::string, Results*>* type_result = new std::map<std::string, Results*>;\n"
                   "\tResults* result = zero_arg_kernel(profiler, &bench_" + fileName + ", *cpu_num_int, \"===== Generated System: " + designName + " =====\");\n"
                                                                                                                                                    "\t(*type_result)[\"" + typeStr + "\"] = result;\n"
                                                                                                                                                                                      "\tkernel_results[\"" + designName + "\"] = type_result;\n"
                                                                                                                                                                                                                           "\tprintf(\"\\n\");\n"
                                                                                                                                                                                                                           "\treturn kernel_results;\n}" << std::endl;

    benchDriver << "void initInput(void* ptr, unsigned long index){}" << std::endl;

    benchDriver.close();

    //#### Emit Makefiles ####
    std::string makefileContent =  "BUILD_DIR=build\n"
                                   "DEPENDS_DIR=./depends\n"
                                   "COMMON_DIR=./common\n"
                                   "SRC_DIR=./intrin\n"
                                   "LIB_DIR=.\n"
                                   "\n"
                                   "#Parameters that can be supplied to this makefile\n"
                                   "USE_PCM=1\n"
                                   "USE_AMDuPROF=1\n"
                                   "\n"
                                   "UNAME:=$(shell uname)\n"
                                   "\n"
                                   "#Compiler Parameters\n"
                                   "#CXX=g++\n"
                                   "#Main Benchmark file is not optomized to avoid timing code being re-organized\n"
                                   "CFLAGS = -O0 -c -g -std=c++11 -march=native -masm=att\n"
                                   "#Generated system should be allowed to optomize - reintepret as c++ file\n"
                                   "SYSTEM_CFLAGS = -Ofast -c -g -x c++ -std=c++11 -march=native -masm=att\n"
                                   "#Most kernels are allowed to be optomized.  Most assembly kernels use asm 'volitile' to force execution\n"
                                   "KERNEL_CFLAGS = -Ofast -c -g -std=c++11 -march=native -masm=att\n"
                                   "#For kernels that should not be optimized, the following is used\n"
                                   "KERNEL_NO_OPT_CFLAGS = -O0 -c -g -std=c++11 -march=native -masm=att\n"
                                   "INC=-I $(COMMON_DIR) -I $(SRC_DIR)\n"
                                   "LIB_DIRS=-L $(COMMON_DIR)\n"
                                   "LIB=-pthread -lProfilerCommon\n"
                                   "\n"
                                   "DEFINES=\n"
                                   "\n"
                                   "DEPENDS=\n"
                                   "DEPENDS_LIB=$(COMMON_DIR)/libProfilerCommon.a\n"
                                   "\n"
                                   "ifeq ($(USE_PCM), 1)\n"
                                   "DEFINES+= -DUSE_PCM=1\n"
                                   "DEPENDS+= $(DEPENDS_DIR)/pcm/Makefile\n"
                                   "DEPENDS_LIB+= $(DEPENDS_DIR)/pcm/libPCM.a\n"
                                   "INC+= -I $(DEPENDS_DIR)/pcm\n"
                                   "LIB_DIRS+= -L $(DEPENDS_DIR)/pcm\n"
                                   "#Need an additional include directory if on MacOS.\n"
                                   "#Using the technique in pcm makefile to detect MacOS\n"
                                   "UNAME:=$(shell uname)\n"
                                   "ifeq ($(UNAME), Darwin)\n"
                                   "INC+= -I $(DEPENDS_DIR)/pcm/MacMSRDriver\n"
                                   "LIB+= -lPCM -lPcmMsr\n"
                                   "APPLE_DRIVER = $(DEPENDS_DIR)/pcm/MacMSRDriver/build/Release/libPcmMsr.dylib\n"
                                   "LIB_DIRS+= -L $(DEPENDS_DIR)/pcm/MacMSRDriver/build/Release\n"
                                   "else\n"
                                   "LIB+= -lrt -lPCM\n"
                                   "APPLE_DRIVER = \n"
                                   "endif\n"
                                   "else\n"
                                   "DEFINES+= -DUSE_PCM=0\n"
                                   "endif\n"
                                   "\n"
                                   "ifeq ($(USE_AMDuPROF), 1)\n"
                                   "DEFINES+= -DUSE_AMDuPROF=1\n"
                                   "LIB+= -lAMDPowerProfileAPI\n"
                                   "else\n"
                                   "DEFINES+= -DUSE_AMDuPROF=0\n"
                                   "endif\n"
                                   "\n"
                                   "MAIN_FILE = benchmark_throughput_test.cpp\n"
                                   "LIB_SRCS = " + fileName + "_benchmark_driver.cpp #These files are not optimized. micro_bench calls the kernel runner (which starts the timers by calling functions in the profilers).  Re-ordering code is not desired\n"
                                   "SYSTEM_SRC = " + fileName + ".c\n"
                                   "KERNEL_SRCS = " + fileName + "_benchmark_kernel.cpp\n"
                                   "KERNEL_NO_OPT_SRCS = \n"
                                   "\n"
                                   "SRCS=$(MAIN_FILE)\n"
                                   "OBJS=$(patsubst %.cpp,$(BUILD_DIR)/%.o,$(SRCS))\n"
                                   "LIB_OBJS=$(patsubst %.cpp,$(BUILD_DIR)/%.o,$(LIB_SRCS))\n"
                                   "SYSTEM_OBJS=$(patsubst %.c,$(BUILD_DIR)/%.o,$(SYSTEM_SRC))\n"
                                   "KERNEL_OBJS=$(patsubst %.cpp,$(BUILD_DIR)/%.o,$(KERNEL_SRCS))\n"
                                   "KERNEL_NO_OPT_OBJS=$(patsubst %.cpp,$(BUILD_DIR)/%.o,$(KERNEL_NO_OPT_SRCS))\n"
                                   "\n"
                                   "#Production\n"
                                   "all: benchmark_" + fileName +"_const\n"
                                   "\n"
                                   "benchmark_" + fileName + "_const: $(OBJS) $(SYSTEM_OBJS) $(LIB_OBJS) $(KERNEL_OBJS) $(KERNEL_NO_OPT_OBJS) $(DEPENDS_LIB) \n"
                                   "\t$(CXX) $(INC) $(LIB_DIRS) -o benchmark_" + fileName + "_const $(OBJS) $(SYSTEM_OBJS) $(LIB_OBJS) $(KERNEL_OBJS) $(KERNEL_NO_OPT_OBJS) $(LIB)\n"
                                   "\n"
                                   "$(KERNEL_NO_OPT_OBJS): $(BUILD_DIR)/%.o : $(SRC_DIR)/%.cpp | $(BUILD_DIR)/ $(DEPENDS)\n"
                                   "\t$(CXX) $(KERNEL_NO_OPT_CFLAGS) $(INC) $(DEFINES) -o $@ $<\n"
                                   "\n"
                                   "$(SYSTEM_OBJS): $(BUILD_DIR)/%.o : $(LIB_DIR)/%.c | $(BUILD_DIR)/ $(DEPENDS)\n"
                                   "\t$(CXX) $(SYSTEM_CFLAGS) $(INC) $(DEFINES) -o $@ $<\n"
                                   "\n"
                                   "$(KERNEL_OBJS): $(BUILD_DIR)/%.o : $(LIB_DIR)/%.cpp | $(BUILD_DIR)/ $(DEPENDS)\n"
                                   "\t$(CXX) $(KERNEL_CFLAGS) $(INC) $(DEFINES) -o $@ $<\n"
                                   "\n"
                                   "$(LIB_OBJS): $(BUILD_DIR)/%.o : $(LIB_DIR)/%.cpp | $(BUILD_DIR)/ $(DEPENDS)\n"
                                   "\t$(CXX) $(CFLAGS) $(INC) $(DEFINES) -o $@ $<\n"
                                   "\n"
                                   "$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp | $(BUILD_DIR)/ $(DEPENDS)\n"
                                   "\t$(CXX) $(CFLAGS) $(INC) $(DEFINES) -o $@ $<\n"
                                   "\n"
                                   "$(BUILD_DIR)/:\n"
                                   "\tmkdir -p $@\n"
                                   "\n"
                                   "$(DEPENDS_DIR)/pcm/Makefile:\n"
                                   "\tgit submodule update --init --recursive $(DEPENDS_DIR)/pcm\n"
                                   "\n"
                                   "$(DEPENDS_DIR)/pcm/libPCM.a: $(DEPENDS_DIR)/pcm/Makefile\n"
                                   "\tcd $(DEPENDS_DIR)/pcm; make libPCM.a\n"
                                   "\n"
                                   "$(COMMON_DIR)/libProfilerCommon.a:\n"
                                   "\tcd $(COMMON_DIR); make USE_PCM=$(USE_PCM) USE_AMDuPROF=$(USE_AMDuPROF)\n"
                                   "\t\n"
                                   "clean:\n"
                                   "\trm -f benchmark_" + fileName + "_const\n"
                                   "\trm -rf build/*\n"
                                   "\n"
                                   ".PHONY: clean\n";

    std::ofstream makefile;
    makefile.open(path+"/Makefile_" + fileName + "_const", std::ofstream::out | std::ofstream::trunc);
    makefile << makefileContent;
    makefile.close();
}

void SingleThreadEmit::emitSingleThreadedCBenchmarkingDriverMem(Design &design, std::string path, std::string fileName, std::string designName, int blockSize) {
    std::string fileNameUpper =  GeneralHelper::toUpper(fileName);

    //#### Emit Kernel File & Header (Memory)
    std::ofstream benchKernelMem;
    benchKernelMem.open(path+"/"+fileName+"_benchmark_kernel_mem.cpp", std::ofstream::out | std::ofstream::trunc);

    benchKernelMem << "#include \"" << fileName << ".h\"" << std::endl;
    benchKernelMem << "#include \"intrin_bench_default_defines.h\"" << std::endl;
    benchKernelMem << "#include \"" << VITIS_TYPE_NAME << ".h\"" << std::endl;
    benchKernelMem << "void bench_"+fileName+"_mem(void** in, void** out)" << std::endl;
    benchKernelMem << "{\n";

    //Reset
    benchKernelMem << "\t" << designName << "_reset();\n";

    //Cast the input and output arrays to the correct type
    std::pair<std::vector<Variable>, std::vector<std::pair<int, int>>> inputVarRatePairs = design.getCInputVariables();
    std::vector<Variable> inputVars = inputVarRatePairs.first;
    unsigned long numInputVars = inputVars.size();
    int inCur = 0; //This is used to track the current index in the input array since it may not allign with i (due to mixed real and imag inputs)
    for(unsigned long i = 0; i<numInputVars; i++){
        std::string inputDTStr = inputVars[i].getDataType().toString(DataType::StringStyle::C, false, false);
        benchKernelMem << "\t" << inputDTStr << "* " << inputVars[i].getCVarName(false) << " = (" << inputDTStr << "*) in[" << inCur << "];" << std::endl;
        inCur++;
        if(inputVars[i].getDataType().isComplex()){
            benchKernelMem << "\t" << inputDTStr << "* " << inputVars[i].getCVarName(true) << " = (" << inputDTStr << "*) in[" << inCur << "];" << std::endl;
            inCur++;
        }
    }

    //Cast output array
    benchKernelMem << "\tOutputType* outArr = (OutputType*) out[0];" << std::endl;

    //TODO: Update for clock domains

    //Generate loop
    benchKernelMem << "\tunsigned long outputCount;\n";
    if(blockSize>1){
        benchKernelMem << "\tint iterLimit = STIM_LEN/" << blockSize << ";\n";
    }else{
        benchKernelMem << "\tint iterLimit = STIM_LEN;\n";
    }
    benchKernelMem << "\tfor(int i = 0; i<iterLimit; i++)\n";
    benchKernelMem << "\t{\n";

    if(blockSize>1){
        benchKernelMem << "\t\tint j = i*" << blockSize << ";\n";
    }

    benchKernelMem << "\t\t" << designName << "(";
    for(unsigned long i = 0; i<numInputVars; i++){
        if(i > 0){
            benchKernelMem << ", ";
        }
        benchKernelMem << inputVars[i].getCVarName(false);
        if(blockSize>1){
            benchKernelMem << "+j";
        }else{
            benchKernelMem << "[i]";
        }
        if(inputVars[i].getDataType().isComplex()){
            benchKernelMem << ", " << inputVars[i].getCVarName(true);
            if(blockSize>1){
                benchKernelMem << "+j";
            }else{
                benchKernelMem << "[i]";
            }
        }
    }
    if(numInputVars>=1){
        benchKernelMem << ", ";
    }
    benchKernelMem << "outArr+i, &outputCount);" << std::endl;

    benchKernelMem << "\t}" << std::endl;
    benchKernelMem << "}" << std::endl;
    benchKernelMem.close();

    std::ofstream benchKernelHeaderMem;
    benchKernelHeaderMem.open(path+"/"+fileName+"_benchmark_kernel_mem.h", std::ofstream::out | std::ofstream::trunc);

    benchKernelHeaderMem << "#ifndef " << fileNameUpper << "_BENCHMARK_KERNEL_H" << std::endl;
    benchKernelHeaderMem << "#define " << fileNameUpper << "_BENCHMARK_KERNEL_H" << std::endl;
    benchKernelHeaderMem << "void bench_"+fileName+"_mem(void** in, void** out);" << std::endl;
    benchKernelHeaderMem << "#endif" << std::endl;
    benchKernelHeaderMem.close();

    //#### Emit Driver File (Mem) ####
    std::ofstream benchMemDriver;
    benchMemDriver.open(path+"/"+fileName+"_benchmark_driver_mem.cpp", std::ofstream::out | std::ofstream::trunc);

    benchMemDriver << "#include <map>" << std::endl;
    benchMemDriver << "#include <string>" << std::endl;
    benchMemDriver << "#include \"intrin_bench_default_defines.h\"" << std::endl;
    benchMemDriver << "#include \"benchmark_throughput_test.h\"" << std::endl;
    benchMemDriver << "#include \"kernel_runner.h\"" << std::endl;
    benchMemDriver << "#include \"" + fileName + "_benchmark_kernel_mem.h\"" << std::endl;
    benchMemDriver << "#include \"" + fileName + ".h\"" << std::endl;

    //If performing a load, exe, store benchmark, define an input structure
    //benchDriver << getCInputPortStructDefn() << std::endl;

    //Driver will define a zero arg kernel that sets reasonable inputs and repeatedly runs the function.
    //The function will be compiled in a seperate object file and should not be in-lined (potentially resulting in eronious
    //optimizations for the purpose of benchmarking since we are feeding dummy data in).  This should be the case so long as
    //the compiler flag -flto is not used durring compile and linkign (https://stackoverflow.com/questions/35922966/lto-with-llvm-and-cmake)
    //(https://llvm.org/docs/LinkTimeOptimization.html), (https://clang.llvm.org/docs/CommandGuide/clang.html),
    //(https://gcc.gnu.org/wiki/LinkTimeOptimization), (https://gcc.gnu.org/onlinedocs/gccint/LTO-Overview.html).

    //Emit name, file, and units string
    benchMemDriver << "std::string getBenchSuiteName(){\n\treturn \"Generated System: " + designName + "\";\n}" << std::endl;
    benchMemDriver << "std::string getReportFileName(){\n\treturn \"" + fileName + "_benchmarking_report\";\n}" << std::endl;
    benchMemDriver << "std::string getReportUnitsName(){\n\treturn \"STIM_LEN: \" + std::to_string(STIM_LEN) + \" (Samples/Vector/Trial), TRIALS: \" + std::to_string(TRIALS);\n}" << std::endl;

    //Emit Benchmark Report Selection
    benchMemDriver << "void getBenchmarksToReport(std::vector<std::string> &kernels, std::vector<std::string> &vec_ext){\n"
                      "\tkernels.push_back(\"" + designName + "\");\tvec_ext.push_back(\"Single Threaded\");\n}" << std::endl;

    //Emit Benchmark Type Report Selection
    std::string typeStr = "";
    if(numInputVars > 0){
        typeStr = inputVars[0].getDataType().toString(DataType::StringStyle::C);
        typeStr += inputVars[0].getDataType().isComplex() ? " (c)" : " (r)";
    }

    for(unsigned long i = 1; i<numInputVars; i++){
        typeStr += ", " + inputVars[i].getDataType().toString(DataType::StringStyle::C);
        typeStr += inputVars[i].getDataType().isComplex() ? " (c)" : " (r)";
    }

    benchMemDriver << "std::vector<std::string> getVarientsToReport(){\n"
                      "\tstd::vector<std::string> types;\n"
                      "\ttypes.push_back(\"" + typeStr + "\");\n" //This was calculated
                                                         "\treturn types;\n}" << std::endl;

    //Generate call to loop
    benchMemDriver << "std::map<std::string, std::map<std::string, Results*>*> runBenchSuite(Profiler* profiler, int* cpu_num_int){\n"
                      "\tstd::map<std::string, std::map<std::string, Results*>*> kernel_results;\n"
                      "\n"
                      "\tstd::map<std::string, Results*>* type_result = new std::map<std::string, Results*>;\n";

    //Create vectors for input, output sizing
    std::string inSizeStr = "";
    std::string inElementsStr = "";
    for(unsigned long i = 0; i<numInputVars; i++){
        if(i > 0){
            inSizeStr += ", ";
            inElementsStr += ", ";
        }
        DataType dt = inputVars[i].getDataType().getCPUStorageType();
        inSizeStr += GeneralHelper::to_string(dt.getTotalBits()/8);
        inElementsStr += "STIM_LEN";

        if(dt.isComplex()){
            inSizeStr += ", " + GeneralHelper::to_string(dt.getTotalBits()/8);
            inElementsStr += ", STIM_LEN";
        }
    }

    benchMemDriver << "\tstd::vector<int> inSizes = {" << inSizeStr << "};" << std::endl;
    benchMemDriver << "\tstd::vector<int> numInElements = {" << inElementsStr << "};" << std::endl;

    std::string outputEntriesStr;
    if(blockSize>1) {
        outputEntriesStr = "STIM_LEN/" + GeneralHelper::to_string(blockSize);
    }else{
        outputEntriesStr = "STIM_LEN";
    }

    benchMemDriver << "\tstd::vector<int> outSizes = {sizeof(OutputType)};" << std::endl;
    benchMemDriver << "\tstd::vector<int> numOutElements = {" << outputEntriesStr << "};" <<std::endl;

    benchMemDriver << "\tResults* result = load_store_arb_init_kernel<__m256>(profiler, &bench_" + fileName + "_mem, "
                                                                                                              "inSizes, outSizes, numInElements, numOutElements, "
                                                                                                              "*cpu_num_int, \"===== Generated System: " + designName + " =====\");\n"
                                                                                                                                                                        "\t(*type_result)[\"" + typeStr + "\"] = result;\n"
                                                                                                                                                                                                          "\tkernel_results[\"" + designName + "\"] = type_result;\n"
                                                                                                                                                                                                                                               "\tprintf(\"\\n\");\n"
                                                                                                                                                                                                                                               "\treturn kernel_results;\n}" << std::endl;

    //==== Generate init ====
    std::vector<NumericValue> defaultArgs;
    for(unsigned long i = 0; i<numInputVars; i++){
        DataType type = inputVars[i].getDataType();
        NumericValue val;
        val.setRealInt(1);
        val.setImagInt(1);
        val.setComplex(type.isComplex());
        val.setFractional(false);

        defaultArgs.push_back(val);
    }

    benchMemDriver << "void initInput(void* in, unsigned long index){" << std::endl;

    //Cast the input and output arrays to the correct type
    benchMemDriver << "\tvoid** in_cast = (void**) in;" << std::endl;
    inCur = 0;
    for(unsigned long i = 0; i<numInputVars; i++){
        std::string inputDTStr = inputVars[i].getDataType().toString(DataType::StringStyle::C, false, false);
        benchMemDriver << "\t" << inputDTStr << "* " << inputVars[i].getCVarName(false) << " = (" << inputDTStr << "*) in_cast[" << inCur << "];" << std::endl;
        inCur++;
        if(inputVars[i].getDataType().isComplex()){
            benchMemDriver << "\t" << inputDTStr << "* " << inputVars[i].getCVarName(true) << " = (" << inputDTStr << "*) in_cast[" << inCur << "];" << std::endl;
            inCur++;
        }
    }

    for(unsigned long i = 0; i<numInputVars; i++){
        benchMemDriver << "\t" << inputVars[i].getCVarName(false) << "[index] = " << defaultArgs[i].toStringComponent(false, inputVars[i].getDataType()) << ";" << std::endl;
        //Check if complex
        if(inputVars[i].getDataType().isComplex()) {
            benchMemDriver << "\t" << inputVars[i].getCVarName(true) << "[index] = " << defaultArgs[i].toStringComponent(true, inputVars[i].getDataType()) << ";" << std::endl;
        }
    }
    benchMemDriver << "}" << std::endl;

    benchMemDriver.close();

    //#### Emit Makefiles ####
    std::string makefileContent =  "BUILD_DIR=build\n"
                                   "DEPENDS_DIR=./depends\n"
                                   "COMMON_DIR=./common\n"
                                   "SRC_DIR=./intrin\n"
                                   "LIB_DIR=.\n"
                                   "\n"
                                   "#Parameters that can be supplied to this makefile\n"
                                   "USE_PCM=1\n"
                                   "USE_AMDuPROF=1\n"
                                   "\n"
                                   "UNAME:=$(shell uname)\n"
                                   "\n"
                                   "#Compiler Parameters\n"
                                   "#CXX=g++\n"
                                   "#Main Benchmark file is not optomized to avoid timing code being re-organized\n"
                                   "CFLAGS = -O0 -c -g -std=c++11 -march=native -masm=att\n"
                                   "#Generated system should be allowed to optomize - reintepret as c++ file\n"
                                   "SYSTEM_CFLAGS = -Ofast -c -g -x c++ -std=c++11 -march=native -masm=att\n"
                                   "#Most kernels are allowed to be optomized.  Most assembly kernels use asm 'volitile' to force execution\n"
                                   "KERNEL_CFLAGS = -Ofast -c -g -std=c++11 -march=native -masm=att\n"
                                   "#For kernels that should not be optimized, the following is used\n"
                                   "KERNEL_NO_OPT_CFLAGS = -O0 -c -g -std=c++11 -march=native -masm=att\n"
                                   "INC=-I $(COMMON_DIR) -I $(SRC_DIR)\n"
                                   "LIB_DIRS=-L $(COMMON_DIR)\n"
                                   "LIB=-pthread -lProfilerCommon\n"
                                   "\n"
                                   "DEFINES=\n"
                                   "\n"
                                   "DEPENDS=\n"
                                   "DEPENDS_LIB=$(COMMON_DIR)/libProfilerCommon.a\n"
                                   "\n"
                                   "ifeq ($(USE_PCM), 1)\n"
                                   "DEFINES+= -DUSE_PCM=1\n"
                                   "DEPENDS+= $(DEPENDS_DIR)/pcm/Makefile\n"
                                   "DEPENDS_LIB+= $(DEPENDS_DIR)/pcm/libPCM.a\n"
                                   "INC+= -I $(DEPENDS_DIR)/pcm\n"
                                   "LIB_DIRS+= -L $(DEPENDS_DIR)/pcm\n"
                                   "#Need an additional include directory if on MacOS.\n"
                                   "#Using the technique in pcm makefile to detect MacOS\n"
                                   "UNAME:=$(shell uname)\n"
                                   "ifeq ($(UNAME), Darwin)\n"
                                   "INC+= -I $(DEPENDS_DIR)/pcm/MacMSRDriver\n"
                                   "LIB+= -lPCM -lPcmMsr\n"
                                   "APPLE_DRIVER = $(DEPENDS_DIR)/pcm/MacMSRDriver/build/Release/libPcmMsr.dylib\n"
                                   "LIB_DIRS+= -L $(DEPENDS_DIR)/pcm/MacMSRDriver/build/Release\n"
                                   "else\n"
                                   "LIB+= -lrt -lPCM\n"
                                   "APPLE_DRIVER = \n"
                                   "endif\n"
                                   "else\n"
                                   "DEFINES+= -DUSE_PCM=0\n"
                                   "endif\n"
                                   "\n"
                                   "ifeq ($(USE_AMDuPROF), 1)\n"
                                   "DEFINES+= -DUSE_AMDuPROF=1\n"
                                   "LIB+= -lAMDPowerProfileAPI\n"
                                   "else\n"
                                   "DEFINES+= -DUSE_AMDuPROF=0\n"
                                   "endif\n"
                                   "\n"
                                   "MAIN_FILE = benchmark_throughput_test.cpp\n"
                                   "LIB_SRCS = " + fileName + "_benchmark_driver_mem.cpp #These files are not optimized. micro_bench calls the kernel runner (which starts the timers by calling functions in the profilers).  Re-ordering code is not desired\n"
                                   "SYSTEM_SRC = " + fileName + ".c\n"
                                   "KERNEL_SRCS = " + fileName + "_benchmark_kernel_mem.cpp\n"
                                   "KERNEL_NO_OPT_SRCS = \n"
                                   "\n"
                                   "SRCS=$(MAIN_FILE)\n"
                                   "OBJS=$(patsubst %.cpp,$(BUILD_DIR)/%.o,$(SRCS))\n"
                                   "LIB_OBJS=$(patsubst %.cpp,$(BUILD_DIR)/%.o,$(LIB_SRCS))\n"
                                   "SYSTEM_OBJS=$(patsubst %.c,$(BUILD_DIR)/%.o,$(SYSTEM_SRC))\n"
                                   "KERNEL_OBJS=$(patsubst %.cpp,$(BUILD_DIR)/%.o,$(KERNEL_SRCS))\n"
                                   "KERNEL_NO_OPT_OBJS=$(patsubst %.cpp,$(BUILD_DIR)/%.o,$(KERNEL_NO_OPT_SRCS))\n"
                                   "\n"
                                   "#Production\n"
                                   "all: benchmark_" + fileName +"_mem\n"
                                   "\n"
                                   "benchmark_" + fileName + "_mem: $(OBJS) $(SYSTEM_OBJS) $(LIB_OBJS) $(KERNEL_OBJS) $(KERNEL_NO_OPT_OBJS) $(DEPENDS_LIB) \n"
                                   "\t$(CXX) $(INC) $(LIB_DIRS) -o benchmark_" + fileName + "_mem $(OBJS) $(SYSTEM_OBJS) $(LIB_OBJS) $(KERNEL_OBJS) $(KERNEL_NO_OPT_OBJS) $(LIB)\n"
                                   "\n"
                                   "$(KERNEL_NO_OPT_OBJS): $(BUILD_DIR)/%.o : $(SRC_DIR)/%.cpp | $(BUILD_DIR)/ $(DEPENDS)\n"
                                   "\t$(CXX) $(KERNEL_NO_OPT_CFLAGS) $(INC) $(DEFINES) -o $@ $<\n"
                                   "\n"
                                   "$(SYSTEM_OBJS): $(BUILD_DIR)/%.o : $(LIB_DIR)/%.c | $(BUILD_DIR)/ $(DEPENDS)\n"
                                   "\t$(CXX) $(SYSTEM_CFLAGS) $(INC) $(DEFINES) -o $@ $<\n"
                                   "\n"
                                   "$(KERNEL_OBJS): $(BUILD_DIR)/%.o : $(LIB_DIR)/%.cpp | $(BUILD_DIR)/ $(DEPENDS)\n"
                                   "\t$(CXX) $(KERNEL_CFLAGS) $(INC) $(DEFINES) -o $@ $<\n"
                                   "\n"
                                   "$(LIB_OBJS): $(BUILD_DIR)/%.o : $(LIB_DIR)/%.cpp | $(BUILD_DIR)/ $(DEPENDS)\n"
                                   "\t$(CXX) $(CFLAGS) $(INC) $(DEFINES) -o $@ $<\n"
                                   "\n"
                                   "$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp | $(BUILD_DIR)/ $(DEPENDS)\n"
                                   "\t$(CXX) $(CFLAGS) $(INC) $(DEFINES) -o $@ $<\n"
                                   "\n"
                                   "$(BUILD_DIR)/:\n"
                                   "\tmkdir -p $@\n"
                                   "\n"
                                   "$(DEPENDS_DIR)/pcm/Makefile:\n"
                                   "\tgit submodule update --init --recursive $(DEPENDS_DIR)/pcm\n"
                                   "\n"
                                   "$(DEPENDS_DIR)/pcm/libPCM.a: $(DEPENDS_DIR)/pcm/Makefile\n"
                                   "\tcd $(DEPENDS_DIR)/pcm; make libPCM.a\n"
                                   "\n"
                                   "$(COMMON_DIR)/libProfilerCommon.a:\n"
                                   "\tcd $(COMMON_DIR); make USE_PCM=$(USE_PCM) USE_AMDuPROF=$(USE_AMDuPROF)\n"
                                   "\t\n"
                                   "clean:\n"
                                   "\trm -f benchmark_" + fileName + "_mem\n"
                                   "\trm -rf build/*\n"
                                   "\n"
                                   ".PHONY: clean\n";

    std::ofstream makefile;
    makefile.open(path+"/Makefile_" + fileName + "_mem", std::ofstream::out | std::ofstream::trunc);
    makefile << makefileContent;
    makefile.close();
}
