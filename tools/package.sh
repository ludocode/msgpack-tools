#!/bin/bash
tools/debug.sh || exit $?

MAJOR=`cat CMakeLists.txt | grep "set(PROJECT_VERSION_MAJOR" | grep -o '[0-9][0-9]*'`
MINOR=`cat CMakeLists.txt | grep "set(PROJECT_VERSION_MINOR" | grep -o '[0-9][0-9]*'`
PATCH=`cat CMakeLists.txt | grep "set(PROJECT_VERSION_PATCH" | grep -o '[0-9][0-9]*'`
if [ "$PATCH" == "0" ]; then
    VERSION="$MAJOR.$MINOR"
else
    VERSION="$MAJOR.$MINOR.$PATCH"
fi

echo "Packaging version ${VERSION}"

NAME=msgpack-tools-${VERSION}
FILES="\
    README.md \
    LICENSE \
    AUTHORS \
    CHANGELOG.md \
    CMakeLists.txt \
    configure \
    contrib \
    contrib/*.tar.gz \
    contrib/*.zip \
    docs \
    docs/json2msgpack.1 \
    docs/msgpack2json.1 \
    src \
    src/* \
    tests \
    tests/* \
    tools \
    tools/debug.sh \
    tools/valgrind.sh \
    tools/valgrind-suppressions \
    tools/test-compare.sh \
    tools/test-fail.sh \
    "

cat docs/json2msgpack.md | sed "s/\$version/$VERSION/" | md2man-roff > docs/json2msgpack.1 || exit $?
cat docs/msgpack2json.md | sed "s/\$version/$VERSION/" | md2man-roff > docs/msgpack2json.1 || exit $?

tar -czf ${NAME}.tar.gz --transform "s@^@$NAME/@" --no-recursion ${FILES}

echo "Wrote ${NAME}.tar.gz"
