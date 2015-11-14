#!/bin/bash
rm -rf contrib
mkdir -p contrib
pushd contrib

# fetch mpack
MPACK_VERSION="0.7"
MPACK_FILE="mpack-amalgamation-${MPACK_VERSION}.tar.gz"
MPACK_URL="https://github.com/ludocode/mpack/releases/download/v${MPACK_VERSION}/${MPACK_FILE}"
curl -L -o "${MPACK_FILE}" "${MPACK_URL}" || exit $?

# fetch yajl
# (we use our fork of yajl that adds lots of lax parsing and generation options)
# https://github.com/ludocode/yajl
YAJL_COMMIT="78764146789a0aa263f2c10316cab1b651166cd7"
YAJL_FILE="yajl-${YAJL_COMMIT}.tar.gz"
YAJL_URL="https://github.com/ludocode/yajl/archive/${YAJL_COMMIT}.tar.gz"
curl -L -o "${YAJL_FILE}" "${YAJL_URL}" || exit $?

# fetch libb64
LIBB64_VERSION="1.2.1"
LIBB64_FILE="libb64-${LIBB64_VERSION}.zip"
LIBB64_URL="http://downloads.sourceforge.net/project/libb64/libb64/libb64/${LIBB64_FILE}?use_mirror=autoselect"
curl -L -o "${LIBB64_FILE}" "${LIBB64_URL}" || exit $?
