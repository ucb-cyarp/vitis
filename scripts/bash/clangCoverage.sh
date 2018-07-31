#!/bin/bash

startDir=`pwd`
scriptDir=`dirname $0`

launcher=""
if [[ `uname` == 'Darwin' ]]; then
    launcher="xcrun"
fi

rootDir="$scriptDir/../.."
cd $rootDir

echo "**** Removing Previous Code Coverage Results ****"
rm -rf covReport

echo "**** Generating Code Coverage with LLVM/Clang/llvm-cov ****"
mkdir buildCoverage
cd buildCoverage
cmake -DCMAKE_BUILD_TYPE=DEBUG ..
make
make test LLVM_PROFILE_FILE="testCoverage.profraw"
cd test
$launcher llvm-profdata merge -sparse testCoverage.profraw -o testCoverage.profdata
mkdir covReport
$launcher llvm-cov show -instr-profile=testCoverage.profdata -format=html -output-dir=covReport testRunner ../../src
cp -r covReport ../../.
echo "**** Wrote Coverage Report to covReport ****"

echo "**** Cleanup - Removing Code Coverage Build Directory (buildCoverage) ****"
cd ../..
rm -rf buildCoverage

cd $startDir