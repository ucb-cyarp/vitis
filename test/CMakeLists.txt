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


# Now simply link against gtest or gtest_main as needed.
set(TEST_WORKING_DIR, ${CMAKE_BINARY_DIR}/test) #Issue with using the current binary DIR for some reason with the add_test method
#TODO: look at this later, for now use above var

#Tests
add_executable(coreTester src/TestDataType.cpp src/TestNumericValue.cpp)
target_link_libraries(coreTester gtest_main VitisLib)
add_test(coreTester coreTester WORKING_DIRECTORY ${TEST_WORKING_DIR})

add_executable(xmlTester src/TestSimulinkImport.cpp)
target_link_libraries(xmlTester gtest_main VitisLib ${XercesC_LIBRARIES})
add_test(xmlTester xmlTester WORKING_DIRECTORY ${TEST_WORKING_DIR})

#Copy Stimulus Files (https://cmake.org/cmake/help/v3.0/command/file.html)
file(COPY "stimulus" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")