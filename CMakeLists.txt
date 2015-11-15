cmake_minimum_required(VERSION 2.6)

include(GNUInstallDirs)

project(msgpack2json)

set(CONTRIB_DIR ${CMAKE_BINARY_DIR}/contrib)
file(MAKE_DIRECTORY ${CONTRIB_DIR})

if(NOT CMAKE_BUILD_TYPE)
    message(STATUS "No build type selected, default to Release")
    set(CMAKE_BUILD_TYPE "Release")
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_VERBOSE_MAKEFILE ON)
endif()

if(CMAKE_BUILD_TOOL MATCHES "(msdev|devenv|nmake)")
    # Windows support is not implemented yet
    add_definitions(/W2)
else()
    set(FLAGS "-std=c99 -Wall -Wextra")
    set(CMAKE_C_FLAGS_DEBUG   "${FLAGS} -g -O0 -DDEBUG")
    set(CMAKE_C_FLAGS_RELEASE "${FLAGS} -O3 -DNDEBUG -fPIC -DPIC")
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -s")
endif()


# mpack

set(MPACK_VERSION "0.7")
set(MPACK_FILE "mpack-amalgamation-${MPACK_VERSION}.tar.gz")
set(MPACK_DIR "${CONTRIB_DIR}/mpack-amalgamation-${MPACK_VERSION}")

execute_process(COMMAND ${CMAKE_COMMAND} -E tar xzf ${CMAKE_SOURCE_DIR}/contrib/${MPACK_FILE} WORKING_DIRECTORY ${CONTRIB_DIR})

file(COPY ${MPACK_DIR}/src/mpack-config.h.sample DESTINATION ${CMAKE_BINARY_DIR})
file(RENAME ${CMAKE_BINARY_DIR}/mpack-config.h.sample ${CMAKE_BINARY_DIR}/mpack-config.h)

file(GLOB_RECURSE MPACK_SRCS ${MPACK_DIR}/src/*.c)
include_directories(SYSTEM ${CMAKE_BINARY_DIR} ${MPACK_DIR}/src)


# yajl

set(YAJL_COMMIT "78764146789a0aa263f2c10316cab1b651166cd7")
set(YAJL_VERSION "2.1.1")
set(YAJL_FILE "yajl-${YAJL_COMMIT}.tar.gz")
set(YAJL_DIR "${CONTRIB_DIR}/yajl-${YAJL_COMMIT}")

execute_process(COMMAND ${CMAKE_COMMAND} -E tar xzf "${CMAKE_SOURCE_DIR}/contrib/${YAJL_FILE}" WORKING_DIRECTORY "${CONTRIB_DIR}")

execute_process(COMMAND ./configure WORKING_DIRECTORY ${YAJL_DIR})

file(GLOB_RECURSE YAJL_SRCS ${YAJL_DIR}/src/*.c)
include_directories(SYSTEM ${YAJL_DIR}/build/yajl-${YAJL_VERSION}/include)


# libb64

set(LIBB64_VERSION "1.2.1")
set(LIBB64_FILE "libb64-${LIBB64_VERSION}.zip")
set(LIBB64_DIR "${CONTRIB_DIR}/libb64-${LIBB64_VERSION}")

execute_process(COMMAND ${CMAKE_COMMAND} -E tar xf "${CMAKE_SOURCE_DIR}/contrib/${LIBB64_FILE}" WORKING_DIRECTORY "${CONTRIB_DIR}")

# Remove libb64's newlines
set(LIBB64_CENCODE_FILE ${LIBB64_DIR}/src/cencode.c)
file(READ ${LIBB64_CENCODE_FILE} LIBB64_CENCODE)
string(REPLACE "*codechar++ = '\\n';" "/* *codechar++ = '\\n'; */" LIBB64_CENCODE "${LIBB64_CENCODE}")
file(WRITE ${LIBB64_CENCODE_FILE} "${LIBB64_CENCODE}")

file(GLOB_RECURSE LIBB64_SRCS ${LIBB64_DIR}/src/*.c)
include_directories(SYSTEM ${LIBB64_DIR}/include)


# targets

add_executable(msgpack2json src/msgpack2json.c ${MPACK_SRCS} ${YAJL_SRCS} ${LIBB64_SRCS})
add_executable(json2msgpack src/json2msgpack.c ${MPACK_SRCS} ${YAJL_SRCS} ${LIBB64_SRCS})

install(TARGETS msgpack2json json2msgpack DESTINATION bin)

install(FILES ${CMAKE_SOURCE_DIR}/docs/msgpack2json.1 DESTINATION ${CMAKE_INSTALL_PREFIX}/man/man1)
install(FILES ${CMAKE_SOURCE_DIR}/docs/json2msgpack.1 DESTINATION ${CMAKE_INSTALL_PREFIX}/man/man1)


# testing

set(TESTS_DIR "${CMAKE_SOURCE_DIR}/tests")
set(TEST_COMPARE_SCRIPT "${CMAKE_SOURCE_DIR}/tools/test-compare.sh")
set(TEST_FAIL_SCRIPT "${CMAKE_SOURCE_DIR}/tools/test-fail.sh")

enable_testing()

add_test("json2msgpack-basic" ${TEST_COMPARE_SCRIPT} ${TESTS_DIR}/basic.mp ./json2msgpack -i ${TESTS_DIR}/basic.json)
add_test("json2msgpack-basic-min" ${TEST_COMPARE_SCRIPT} ${TESTS_DIR}/basic.mp ./json2msgpack -i ${TESTS_DIR}/basic-min.json)
add_test("json2msgpack-basic-lax" ${TEST_COMPARE_SCRIPT} ${TESTS_DIR}/basic.mp ./json2msgpack -li ${TESTS_DIR}/basic-lax.json)
add_test("json2msgpack-basic-strict-fail" ${TEST_FAIL_SCRIPT} ./json2msgpack -i ${TESTS_DIR}/basic-lax.json)
add_test("json2msgpack-basic-base64" ${TEST_COMPARE_SCRIPT} ${TESTS_DIR}/basic.mp ./json2msgpack -B 22 -bi ${TESTS_DIR}/basic.json)

add_test("msgpack2json-basic-min" ${TEST_COMPARE_SCRIPT} ${TESTS_DIR}/basic-min.json ./msgpack2json -i ${TESTS_DIR}/basic.mp)
add_test("msgpack2json-basic" ${TEST_COMPARE_SCRIPT} ${TESTS_DIR}/basic.json ./msgpack2json -pi ${TESTS_DIR}/basic.mp)
add_test("msgpack2json-basic-debug" ${TEST_COMPARE_SCRIPT} ${TESTS_DIR}/basic.json ./msgpack2json -di ${TESTS_DIR}/basic.mp)

add_test("json2msgpack-base64-str-prefix" ${TEST_COMPARE_SCRIPT} ${TESTS_DIR}/base64-str-prefix.mp ./json2msgpack -i ${TESTS_DIR}/base64-prefix.json)
add_test("json2msgpack-base64-bin" ${TEST_COMPARE_SCRIPT} ${TESTS_DIR}/base64-bin-ext.mp ./json2msgpack -bi ${TESTS_DIR}/base64-prefix.json)
add_test("json2msgpack-base64-bin-lax" ${TEST_COMPARE_SCRIPT} ${TESTS_DIR}/base64-bin-ext.mp ./json2msgpack -bli ${TESTS_DIR}/base64-prefix-lax.json)

add_test("json2msgpack-base64-detect-str" ${TEST_COMPARE_SCRIPT} ${TESTS_DIR}/base64-str.mp ./json2msgpack -B 200 -i ${TESTS_DIR}/base64-detect.json)
add_test("json2msgpack-base64-detect-partial" ${TEST_COMPARE_SCRIPT} ${TESTS_DIR}/base64-partial.mp ./json2msgpack -B 50 -i ${TESTS_DIR}/base64-detect.json)
add_test("json2msgpack-base64-detect-bin" ${TEST_COMPARE_SCRIPT} ${TESTS_DIR}/base64-bin.mp ./json2msgpack -B 22 -i ${TESTS_DIR}/base64-detect.json)
add_test("json2msgpack-base64-detect-bin-one" ${TEST_COMPARE_SCRIPT} ${TESTS_DIR}/base64-bin.mp ./json2msgpack -B 1 -i ${TESTS_DIR}/base64-detect.json)

add_test("json2msgpack-base64-mixed-partial" ${TEST_COMPARE_SCRIPT} ${TESTS_DIR}/base64-partial-ext.mp ./json2msgpack -bB 50 -i ${TESTS_DIR}/base64-mixed.json)
add_test("json2msgpack-base64-mixed-bin" ${TEST_COMPARE_SCRIPT} ${TESTS_DIR}/base64-bin-ext.mp ./json2msgpack -bB 22 -i ${TESTS_DIR}/base64-mixed.json)

#add_test("json2msgpack-base64-bin" ${TEST_COMPARE_SCRIPT} ${TESTS_DIR}/base64-bin.mp ./json2msgpack -bi ${TESTS_DIR}/base64-prefix.json)
#add_test("json2msgpack-base64-bin-lax" ${TEST_COMPARE_SCRIPT} ${TESTS_DIR}/base64-bin.mp ./json2msgpack -lbi ${TESTS_DIR}/base64-prefix-lax.json)
