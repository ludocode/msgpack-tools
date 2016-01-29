#!/bin/bash
rm -rf contrib
mkdir -p contrib
pushd contrib

# fetch mpack
MPACK_VERSION="0.7"
MPACK_FILE="mpack-amalgamation-${MPACK_VERSION}.tar.gz"
MPACK_URL="https://github.com/ludocode/mpack/releases/download/v${MPACK_VERSION}/${MPACK_FILE}"
curl -L -o "${MPACK_FILE}" "${MPACK_URL}" || exit $?

# fetch rapidjson
# (temporarily using our fork of rapidjson for trailing comma support)
# https://github.com/ludocode/rapidjson
RAPIDJSON_COMMIT="9fce30c06648519f3d44a2599ae91015f4dc311c"
RAPIDJSON_FILE="rapidjson-${RAPIDJSON_COMMIT}.tar.gz"
RAPIDJSON_URL="https://github.com/ludocode/rapidjson/archive/${RAPIDJSON_COMMIT}.tar.gz"
curl -L -o "${RAPIDJSON_FILE}" "${RAPIDJSON_URL}" || exit $?

# fetch yajl
# (we use our fork of yajl that fixes bugs and adds lax
# parsing and generation options)
# https://github.com/ludocode/yajl
YAJL_COMMIT="2c42d2a07e68bfdfb1d16c0ed7efd31f327aae49"
YAJL_FILE="yajl-${YAJL_COMMIT}.tar.gz"
YAJL_URL="https://github.com/ludocode/yajl/archive/${YAJL_COMMIT}.tar.gz"
curl -L -o "${YAJL_FILE}" "${YAJL_URL}" || exit $?

# fetch libb64
LIBB64_VERSION="1.2.1"
LIBB64_FILE="libb64-${LIBB64_VERSION}.zip"
LIBB64_URL="http://downloads.sourceforge.net/project/libb64/libb64/libb64/${LIBB64_FILE}?use_mirror=autoselect"
curl -L -o "${LIBB64_FILE}" "${LIBB64_URL}" || exit $?
