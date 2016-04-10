#!/bin/bash
# Requires md2man-roff: https://github.com/sunaku/md2man
cd "`dirname $0`/.."
VERSION=`cat src/common.h | grep "#define VERSION" | grep -o '".*"' | sed 's/"//g'`
cd docs
cat json2msgpack.md | sed "s/\$version/$VERSION/" | md2man-roff > json2msgpack.1 || exit $?
cat msgpack2json.md | sed "s/\$version/$VERSION/" | md2man-roff > msgpack2json.1 || exit $?
