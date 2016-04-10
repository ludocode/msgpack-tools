#!/bin/bash

# This script fetches the dependencies. It pulls the versions
# and commits from CMakeLists.txt.

rm -rf contrib
mkdir -p contrib
pushd contrib

getstring() {
    cat ../CMakeLists.txt | grep "set($1" | grep -o '".*"' | sed 's/"//g'
}

# fetch mpack
MPACK_VERSION=`getstring MPACK_VERSION`
MPACK_FILE="mpack-amalgamation-${MPACK_VERSION}.tar.gz"
MPACK_URL="https://github.com/ludocode/mpack/releases/download/v${MPACK_VERSION}/${MPACK_FILE}"
curl -L -o "${MPACK_FILE}" "${MPACK_URL}" || exit $?

# fetch rapidjson
RAPIDJSON_COMMIT=`getstring RAPIDJSON_COMMIT`
RAPIDJSON_FILE="rapidjson-${RAPIDJSON_COMMIT}.tar.gz"
RAPIDJSON_URL="https://github.com/miloyip/rapidjson/archive/${RAPIDJSON_COMMIT}.tar.gz"
curl -L -o "${RAPIDJSON_FILE}" "${RAPIDJSON_URL}" || exit $?

# fetch libb64
LIBB64_VERSION=`getstring LIBB64_VERSION`
LIBB64_FILE="libb64-${LIBB64_VERSION}.zip"
LIBB64_URL="http://downloads.sourceforge.net/project/libb64/libb64/libb64/${LIBB64_FILE}?use_mirror=autoselect"
curl -L -o "${LIBB64_FILE}" "${LIBB64_URL}" || exit $?
