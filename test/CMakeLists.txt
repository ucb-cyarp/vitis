#Following instructions from https://github.com/google/googletest/blob/master/googletest/README.md
#on how to include Google Test in an existing CMake project

# Download and unpack googletest at configure time
configure_file(CMakeLists.txt.in googletest-download/CMakeLists.txt)
execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
        RESULT_VARIABLE result
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/test/googletest-download )
if(result)
    message(FATAL_ERROR "CMake step for googletest failed: ${result}")
endif()
execute_process(COMMAND ${CMAKE_COMMAND} --build .
        RESULT_VARIABLE result
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/test/googletest-download )
if(result)
    message(FATAL_ERROR "Build step for googletest failed: ${result}")
endif()

# Prevent overriding the parent project's compiler/linker
# settings on Windows
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

# Add googletest directly to our build. This defines
# the gtest and gtest_main targets.
add_subdirectory(${CMAKE_BINARY_DIR}/googletest-src
        ${CMAKE_BINARY_DIR}/googletest-build
        EXCLUDE_FROM_ALL)

# The gtest/gtest_main targets carry header search path
# dependencies automatically when using CMake 2.8.11 or
# later. Otherwise we have to add them here ourselves.
if (CMAKE_VERSION VERSION_LESS 2.8.11)
    include_directories("${gtest_SOURCE_DIR}/include")
endif()

find_package(Boost REQUIRED filesystem)
include_directories(${Boost_INCLUDE_DIRS})

# Now simply link against gtest or gtest_main as needed.
set(TEST_WORKING_DIR, ${CMAKE_BINARY_DIR}/test) #Issue with using the current binary DIR for some reason with the add_test method
#TODO: look at this later, for now use above var

#Tests
add_executable(testRunner
        src/MathSanityCheck.cpp
        src/GcdTest.cpp
        src/DSPHelperTests.cpp
        src/TestDataType.cpp
        src/TestNumericValue.cpp
        src/TestFixedPointConvert.cpp
        src/TestSimulinkImport.cpp src/GraphTestHelper.h
        src/GraphTestHelper.cpp src/SimpleDesignValidator.h
        src/SimpleDesignValidator.cpp src/TestSimulinkExportReImport.cpp
        src/NestedSubSystemValidator.cpp src/NestedSubSystemValidator.h
        src/TestNestedSubsystems.cpp src/LUTDesignValidator.cpp
        src/LUTDesignValidator.h src/TestLutSubsystem.cpp
        src/EnabledSubsystemDesignValidator.cpp
        src/EnabledSubsystemDesignValidator.h
        src/DSP_TestAGC.cpp
        src/TestEnabledSubsys.cpp
        src/DSP_TestFastRxBaseband.cpp
        src/DSP_Stateflow_CoarseCFO_est.cpp
        src/StronglyConnectedComponents.cpp
        src/DSPTestHelper.cpp
        src/DSPTestHelper.h)

target_link_libraries(testRunner gtest_main VitisLib ${XercesC_LIBRARIES} ${Boost_LIBRARIES})
add_test(testRunner testRunner WORKING_DIRECTORY ${TEST_WORKING_DIR})

#Copy Stimulus Files (https://cmake.org/cmake/help/v3.0/command/file.html)
file(COPY "stimulus" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")