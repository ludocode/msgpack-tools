#!/bin/bash
VERSION=`cat src/common.h | grep "#define VERSION" | grep -o '".*"' | sed 's/"//g'`
NAME=msgpack-tools-${VERSION}
FILES="\
    README.md \
    LICENSE \
    CMakeLists.txt \
    configure \
    contrib \
    contrib/*.tar.gz \
    contrib/*.zip \
    docs \
    docs/json2msgpack.1 \
    docs/msgpack2json.1 \
    src \
    src/common.h \
    src/json2msgpack.c \
    src/msgpack2json.c \
    tests \
    tests/base64-bin-ext.mp \
    tests/base64-bin.mp \
    tests/base64-debug.json \
    tests/base64-detect.json \
    tests/base64-mixed.json \
    tests/base64-partial-ext.mp \
    tests/base64-partial.mp \
    tests/base64-prefix-lax.json \
    tests/base64-prefix.json \
    tests/base64-str-prefix.mp \
    tests/base64-str.mp \
    tests/basic-lax.json \
    tests/basic-min.json \
    tests/basic.json \
    tests/basic.mp \
    tools \
    tools/debug.sh \
    tools/valgrind.sh \
    tools/valgrind-suppressions \
    tools/test-compare.sh \
    tools/test-fail.sh \
    "

tools/fetch.sh || exit $?
tools/man.sh || exit $?
tar -czf ${NAME}.tar.gz --transform "s@^@$NAME/@" --no-recursion ${FILES}
