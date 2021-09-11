#!/bin/sh

if ! [ -e ./msgpack2json ]; then
    echo "Build msgpack2json first."
    exit 1
fi
if ! [ -e ./json2msgpack ]; then
    echo "Build json2msgpack first."
    exit 1
fi

SOURCE_DIR=.
#VALGRIND="$SOURCE_DIR/tools/valgrind.sh"
VALGRIND=
TESTS_DIR="$SOURCE_DIR/tests"

run_test() {
    NAME="$1"
    EXPECTED_OUTPUT="$2"
    EXPECTED_RESULT="$3"
    shift
    shift
    shift

    # run test
    ACTUAL_OUTPUT=.build/test-stdout
    ACTUAL_ERROR=.build/test-stderr
    "$@" 1>"$ACTUAL_OUTPUT" 2>$ACTUAL_ERROR

    ACTUAL_RESULT=$?
    if [ $ACTUAL_RESULT -ne $EXPECTED_RESULT ]; then
        cat $ACTUAL_ERROR
        if [ $EXPECTED_RESULT -eq 0 ]; then
            echo "TEST FAILED: $NAME failed to run"
        else
            echo "TEST FAILED: $NAME return code incorrect"
            echo "    Expected: $EXPECTED_RESULT"
            echo "    Actual:   $ACTUAL_RESULT"
        fi
        exit 1
    fi

    if [ "$EXPECTED_OUTPUT" != "no-compare" ]; then
        diff -u "$EXPECTED_OUTPUT" "$ACTUAL_OUTPUT"
        if [ $? -ne 0 ]; then
            echo "TEST FAILED: $NAME output mismatch"
            echo "    Expected contents of file: $EXPECTED_OUTPUT"
            echo "    Actual contents in file:   $ACTUAL_OUTPUT"
            exit 1
        fi
    fi

    echo "Pass: $NAME"
}

go() {
    run_test "json2msgpack-basic" ${TESTS_DIR}/basic.mp 0 ${VALGRIND} ./json2msgpack -i ${TESTS_DIR}/basic.json
    run_test "json2msgpack-basic-min" ${TESTS_DIR}/basic.mp 0 ${VALGRIND} ./json2msgpack -i ${TESTS_DIR}/basic-min.json
    run_test "json2msgpack-basic-lax" ${TESTS_DIR}/basic.mp 0 ${VALGRIND} ./json2msgpack -li ${TESTS_DIR}/basic-lax.json
    run_test "json2msgpack-basic-base64" ${TESTS_DIR}/basic.mp 0 ${VALGRIND} ./json2msgpack -B 22 -bi ${TESTS_DIR}/basic.json
    run_test "json2msgpack-basic-strict-fail" no-compare 1 ${VALGRIND} ./json2msgpack -i ${TESTS_DIR}/basic-lax.json

    run_test "msgpack2json-basic-min" ${TESTS_DIR}/basic-min.json 0 ${VALGRIND} ./msgpack2json -i ${TESTS_DIR}/basic.mp
    run_test "msgpack2json-basic" ${TESTS_DIR}/basic.json 0 ${VALGRIND} ./msgpack2json -pi ${TESTS_DIR}/basic.mp
    run_test "msgpack2json-basic-debug" ${TESTS_DIR}/basic.json 0 ${VALGRIND} ./msgpack2json -di ${TESTS_DIR}/basic.mp
    run_test "msgpack2json-stdin" ${TESTS_DIR}/basic.json 0 bash -c "cat ${TESTS_DIR}/basic.mp | ${VALGRIND} ./msgpack2json -p"
    run_test "msgpack2json-bin-ext-debug" ${TESTS_DIR}/bin-ext-debug.txt 0 ${VALGRIND} ./msgpack2json -di ${TESTS_DIR}/base64-bin-ext.mp

    run_test "json2msgpack-base64-str-prefix" ${TESTS_DIR}/base64-str-prefix.mp 0 ${VALGRIND} ./json2msgpack -i ${TESTS_DIR}/base64-prefix.json
    run_test "json2msgpack-base64-bin" ${TESTS_DIR}/base64-bin-ext.mp 0 ${VALGRIND} ./json2msgpack -bi ${TESTS_DIR}/base64-prefix.json
    run_test "json2msgpack-base64-bin-lax" ${TESTS_DIR}/base64-bin-ext.mp 0 ${VALGRIND} ./json2msgpack -bli ${TESTS_DIR}/base64-prefix-lax.json

    run_test "json2msgpack-base64-no-detect-str" ${TESTS_DIR}/base64-str.mp 0 ${VALGRIND} ./json2msgpack -i ${TESTS_DIR}/base64-detect.json
    run_test "json2msgpack-base64-detect-str" ${TESTS_DIR}/base64-str.mp 0 ${VALGRIND} ./json2msgpack -B 200 -i ${TESTS_DIR}/base64-detect.json
    run_test "json2msgpack-base64-detect-partial" ${TESTS_DIR}/base64-partial.mp 0 ${VALGRIND} ./json2msgpack -B 50 -i ${TESTS_DIR}/base64-detect.json
    run_test "json2msgpack-base64-detect-most" ${TESTS_DIR}/base64-most.mp 0 ${VALGRIND} ./json2msgpack -B 22 -i ${TESTS_DIR}/base64-detect.json

    run_test "json2msgpack-base64-mixed-partial" ${TESTS_DIR}/base64-mixed-partial.mp 0 ${VALGRIND} ./json2msgpack -bB 50 -i ${TESTS_DIR}/base64-mixed.json
    run_test "json2msgpack-base64-mixed-bin" ${TESTS_DIR}/base64-bin-ext.mp 0 ${VALGRIND} ./json2msgpack -bB 22 -i ${TESTS_DIR}/base64-mixed.json

    run_test "json2msgpack-value-string" ${TESTS_DIR}/value-string.mp 0 ${VALGRIND} ./json2msgpack -i ${TESTS_DIR}/value-string.json
    run_test "json2msgpack-value-int" ${TESTS_DIR}/value-int.mp 0 ${VALGRIND} ./json2msgpack -i ${TESTS_DIR}/value-int.json
    run_test "msgpack2json-value-string" ${TESTS_DIR}/value-string.json 0 ${VALGRIND} ./msgpack2json -i ${TESTS_DIR}/value-string.mp
    run_test "msgpack2json-value-int" ${TESTS_DIR}/value-int.json 0 ${VALGRIND} ./msgpack2json -i ${TESTS_DIR}/value-int.mp

    run_test "msgpack2json-continuous-single" ${TESTS_DIR}/basic-min.json 0 ${VALGRIND} ./msgpack2json -ci ${TESTS_DIR}/basic.mp
    run_test "msgpack2json-continuous" ${TESTS_DIR}/continuous.json 0 ${VALGRIND} ./msgpack2json -cpi ${TESTS_DIR}/continuous.mp
    run_test "msgpack2json-continuous-commas" ${TESTS_DIR}/continuous-commas.json 0 ${VALGRIND} ./msgpack2json -Cpi ${TESTS_DIR}/continuous.mp
    run_test "msgpack2json-continuous-commas-min" ${TESTS_DIR}/continuous-commas-min.json 0 ${VALGRIND} ./msgpack2json -Ci ${TESTS_DIR}/continuous.mp

    echo "All tests passed."
}

go
