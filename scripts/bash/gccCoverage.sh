#!/bin/bash

startDir=`pwd`
scriptDir=`dirname $0`

rootDir="$scriptDir/../.."
cd $rootDir

echo "**** Removing Previous Code Coverage Results ****"
rm -rf covReport

echo "**** Generating Code Coverage with GCC/gcov/lcov ****"
mkdir buildCoverage
cd buildCoverage
cmake -DCMAKE_BUILD_TYPE=DEBUG ..
make
make test
mkdir lcov
cd lcov
lcov --capture --directory ../CMakeFiles/ --output-file coverage.info
lcov --remove coverage.info "/usr*" --output-file coverage_trimmed.info
genhtml coverage_trimmed.info --output-directory covReport
cp -r covReport ../../covReport
echo "**** Wrote Coverage Report to covReport ****"

echo "**** Cleanup - Removing Code Coverage Build Directory (buildCoverage) ****"
cd ../..
rm -rf buildCoverage

cd $startDir