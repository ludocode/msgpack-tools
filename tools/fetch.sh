#!/bin/bash

# This script fetches the dependencies. It pulls the versions
# and commits from CMakeLists.txt.

mkdir -p contrib
pushd contrib

getstring() {
    cat ../CMakeLists.txt | grep "set($1" | grep -o '".*"' | sed 's/"//g'
}

# fetch mpack
MPACK_COMMIT=`getstring MPACK_COMMIT`
MPACK_FILE="mpack-${MPACK_COMMIT}.tar.gz"
if ! [ -e "$MPACK_FILE" ]; then
    MPACK_URL="https://github.com/ludocode/mpack/archive/${MPACK_COMMIT}.tar.gz"
    curl -L -o "${MPACK_FILE}.tmp" "${MPACK_URL}" || exit $?
    mv "${MPACK_FILE}.tmp" "${MPACK_FILE}"
fi

# fetch rapidjson
RAPIDJSON_COMMIT=`getstring RAPIDJSON_COMMIT`
RAPIDJSON_FILE="rapidjson-${RAPIDJSON_COMMIT}.tar.gz"
if ! [ -e "$RAPIDJSON_FILE" ]; then
    RAPIDJSON_URL="https://github.com/miloyip/rapidjson/archive/${RAPIDJSON_COMMIT}.tar.gz"
    curl -L -o "${RAPIDJSON_FILE}.tmp" "${RAPIDJSON_URL}" || exit $?
    mv "${RAPIDJSON_FILE}.tmp" "${RAPIDJSON_FILE}"
fi

# fetch libb64
LIBB64_VERSION=`getstring LIBB64_VERSION`
LIBB64_FILE="libb64-${LIBB64_VERSION}.zip"
if ! [ -e "$LIBB64_FILE" ]; then
    LIBB64_URL="http://downloads.sourceforge.net/project/libb64/libb64/libb64/${LIBB64_FILE}?use_mirror=autoselect"
    curl -L -o "${LIBB64_FILE}.tmp" "${LIBB64_URL}" || exit $?
    mv "${LIBB64_FILE}.tmp" "${LIBB64_FILE}"
fi
