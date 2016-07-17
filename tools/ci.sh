#!/bin/bash
# Used to run the test suite under continuous integration. Handles the
# special compiler type "scan-build" which runs static analysis.

if [ "$DEBUG" == 1 ]; then
    CMAKE_BUILD_TYPE=Debug
else
    CMAKE_BUILD_TYPE=Release
fi

export CTEST_OUTPUT_ON_FAILURE=1

mkdir -p build
cd build

if [[ "$CC" == "scan-build" ]]; then
    unset CXX
    CC=`which clang`
    cmake -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} -DCMAKE_INSTALL_PREFIX="./prefix" .. || exit $?
    unset CC
    scan-build -o analysis --use-cc=`which clang` --status-bugs make all test || exit $?
else
    cmake -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} -DCMAKE_INSTALL_PREFIX="./prefix" .. || exit $?
    make all test || exit $?
fi

if [ -e /etc/debian_version ]; then
    make package
fi

make install
echo "  Installed files:"
find prefix
