#!/bin/bash
# https://github.com/sunaku/md2man
cd "`dirname $0`/.."
md2man-roff docs/json2msgpack.md > docs/json2msgpack.1 || exit $?
md2man-roff docs/msgpack2json.md > docs/msgpack2json.1 || exit $?
