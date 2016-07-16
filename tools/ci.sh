#!/bin/bash
# Used to run the test suite under continuous integration. Handles the
# special compiler type "scan-build" which runs static analysis.

if [ "$DEBUG" == 1 ]; then
    CMAKE_BUILD_TYPE=Debug
else
    CMAKE_BUILD_TYPE=Release
fi

mkdir -p build
cd build

if [[ "$CC" == "scan-build" ]]; then
    unset CXX
    CC=`which clang`
    cmake -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} -DCMAKE_INSTALL_PREFIX="./install" .. || exit $?
    unset CC
    scan-build -o analysis --use-cc=`which clang` --status-bugs make all test install || exit $?
else
    cmake -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} -DCMAKE_INSTALL_PREFIX="./install" .. || exit $?
    make all test install || exit $?
fi

find install
