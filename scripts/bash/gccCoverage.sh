#!/bin/bash

startDir=`pwd`
scriptDir=`dirname $0`

rootDir="$scriptDir/../.."
cd $rootDir
mkdir buildCoverage
cd buildCoverage
cmake -DCMAKE_BUILD_TYPE=DEBUG ..
make
make test
mkdir lcov
cd lcov
lcov --capture --directory ../CMakeFiles/ --output-file coverage.info
lcov --remove coverage.info "/usr*" --output-file coverage_trimmed.info
lcov --remove coverage_trimmed.info "*/test/*" --output-file coverage_trimmed.info
genhtml coverage_trimmed.info --output-directory covReport
cp -r covReport ../../covReport

cd $startDir