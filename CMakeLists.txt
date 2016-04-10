cmake_minimum_required(VERSION 2.6)

include(GNUInstallDirs)
include(CheckCCompilerFlag)
include(CheckCXXCompilerFlag)

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

CHECK_CXX_COMPILER_FLAG("-std=c++11" HAS_CXX11)
if(HAS_CXX11)
    set(CXX_FLAGS   "-std=c++11")
else()
    set(CXX_FLAGS   "")
endif()

if(CMAKE_BUILD_TOOL MATCHES "(msdev|devenv|nmake)")
    # Windows support is not implemented yet
    add_definitions(/W2)
else()
    set(FLAGS "-Wall -Wextra")
    set(CMAKE_C_FLAGS_DEBUG   "${C_FLAGS} ${FLAGS} -g -O0 -DDEBUG")
    set(CMAKE_C_FLAGS_RELEASE "${C_FLAGS} ${FLAGS} -O3 -DNDEBUG -fPIC -DPIC")
    set(CMAKE_CXX_FLAGS_DEBUG   "${CXX_FLAGS} ${FLAGS} -g -O0 -DDEBUG")
    set(CMAKE_CXX_FLAGS_RELEASE "${CXX_FLAGS} ${FLAGS} -O3 -DNDEBUG -fPIC -DPIC")
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -s")
endif()


# mpack

set(MPACK_VERSION "0.8.2")
set(MPACK_FILE "mpack-amalgamation-${MPACK_VERSION}.tar.gz")
set(MPACK_DIR "${CONTRIB_DIR}/mpack-amalgamation-${MPACK_VERSION}")

if(NOT EXISTS "${CMAKE_SOURCE_DIR}/contrib/${MPACK_FILE}")
    message(FATAL_ERROR "\nMPack source is missing: contrib/${MPACK_FILE}\nTo build from the repository, you need to fetch dependencies with tools/fetch.sh")
endif()

execute_process(COMMAND ${CMAKE_COMMAND} -E tar xzf ${CMAKE_SOURCE_DIR}/contrib/${MPACK_FILE} WORKING_DIRECTORY ${CONTRIB_DIR})

file(COPY ${MPACK_DIR}/src/mpack-config.h.sample DESTINATION ${CMAKE_BINARY_DIR})
file(RENAME ${CMAKE_BINARY_DIR}/mpack-config.h.sample ${CMAKE_BINARY_DIR}/mpack-config.h)

file(GLOB_RECURSE MPACK_SRCS ${MPACK_DIR}/src/*.c)
include_directories(SYSTEM ${CMAKE_BINARY_DIR} ${MPACK_DIR}/src)


# rapidjson

set(RAPIDJSON_COMMIT "75d0e4ff652769309052bbbb3745da12a572af9a")
set(RAPIDJSON_FILE "rapidjson-${RAPIDJSON_COMMIT}.tar.gz")
set(RAPIDJSON_DIR "${CONTRIB_DIR}/rapidjson-${RAPIDJSON_COMMIT}")

if(NOT EXISTS "${CMAKE_SOURCE_DIR}/contrib/${RAPIDJSON_FILE}")
    message(FATAL_ERROR "\nRapidJSON source is missing: contrib/${RAPIDJSON_FILE}\nTo build from the repository, you need to fetch dependencies with tools/fetch.sh")
endif()

execute_process(COMMAND ${CMAKE_COMMAND} -E tar xzf ${CMAKE_SOURCE_DIR}/contrib/${RAPIDJSON_FILE} WORKING_DIRECTORY ${CONTRIB_DIR})

include_directories(SYSTEM ${RAPIDJSON_DIR}/include)


# libb64

set(LIBB64_VERSION "1.2.1")
set(LIBB64_FILE "libb64-${LIBB64_VERSION}.zip")
set(LIBB64_DIR "${CONTRIB_DIR}/libb64-${LIBB64_VERSION}")

if(NOT EXISTS "${CMAKE_SOURCE_DIR}/contrib/${LIBB64_FILE}")
    message(FATAL_ERROR "\nlibb64 source is missing: contrib/${LIBB64_FILE}\nTo build from the repository, you need to fetch dependencies with tools/fetch.sh")
endif()

execute_process(COMMAND ${CMAKE_COMMAND} -E tar xf "${CMAKE_SOURCE_DIR}/contrib/${LIBB64_FILE}" WORKING_DIRECTORY "${CONTRIB_DIR}")

# Remove libb64's newlines
set(LIBB64_CENCODE_FILE ${LIBB64_DIR}/src/cencode.c)
file(READ ${LIBB64_CENCODE_FILE} LIBB64_CENCODE)
string(REPLACE "*codechar++ = '\\n';" "/* *codechar++ = '\\n'; */" LIBB64_CENCODE "${LIBB64_CENCODE}")
file(WRITE ${LIBB64_CENCODE_FILE} "${LIBB64_CENCODE}")

file(GLOB_RECURSE LIBB64_SRCS ${LIBB64_DIR}/src/*.c)
include_directories(SYSTEM ${LIBB64_DIR}/include)


# targets

add_executable(msgpack2json src/msgpack2json.cpp ${MPACK_SRCS} ${LIBB64_SRCS})
add_executable(json2msgpack src/json2msgpack.cpp ${MPACK_SRCS} ${LIBB64_SRCS})

install(TARGETS msgpack2json json2msgpack DESTINATION bin)

install(FILES ${CMAKE_SOURCE_DIR}/docs/msgpack2json.1 DESTINATION ${CMAKE_INSTALL_PREFIX}/man/man1)
install(FILES ${CMAKE_SOURCE_DIR}/docs/json2msgpack.1 DESTINATION ${CMAKE_INSTALL_PREFIX}/man/man1)

if(NOT EXISTS "${CMAKE_SOURCE_DIR}/docs/msgpack2json.1")
    message(FATAL_ERROR "\nmsgpack2json man page is missing: docs/msgpack2json.1\nTo build from the repository, you need to generate manpages with tools/man.sh")
endif()
if(NOT EXISTS "${CMAKE_SOURCE_DIR}/docs/json2msgpack.1")
    message(FATAL_ERROR "\njson2msgpack man page is missing: docs/json2msgpack.1\nTo build from the repository, you need to generate manpages with tools/man.sh")
endif()


# testing

set(VALGRIND "${CMAKE_SOURCE_DIR}/tools/valgrind.sh")
set(TESTS_DIR "${CMAKE_SOURCE_DIR}/tests")
set(COMPARE "${CMAKE_SOURCE_DIR}/tools/test-compare.sh")
set(FAIL "${CMAKE_SOURCE_DIR}/tools/test-fail.sh")

enable_testing()

add_test("json2msgpack-basic"                 ${COMPARE} ${TESTS_DIR}/basic.mp              ${VALGRIND} ./json2msgpack -i ${TESTS_DIR}/basic.json)
add_test("json2msgpack-basic-min"             ${COMPARE} ${TESTS_DIR}/basic.mp              ${VALGRIND} ./json2msgpack -i ${TESTS_DIR}/basic-min.json)
add_test("json2msgpack-basic-lax"             ${COMPARE} ${TESTS_DIR}/basic.mp              ${VALGRIND} ./json2msgpack -li ${TESTS_DIR}/basic-lax.json)
add_test("json2msgpack-basic-base64"          ${COMPARE} ${TESTS_DIR}/basic.mp              ${VALGRIND} ./json2msgpack -B 22 -bi ${TESTS_DIR}/basic.json)
add_test("json2msgpack-basic-strict-fail"     ${FAIL}                                       ${VALGRIND} ./json2msgpack -i ${TESTS_DIR}/basic-lax.json)

add_test("msgpack2json-basic-min"             ${COMPARE} ${TESTS_DIR}/basic-min.json        ${VALGRIND} ./msgpack2json -i ${TESTS_DIR}/basic.mp)
add_test("msgpack2json-basic"                 ${COMPARE} ${TESTS_DIR}/basic.json            ${VALGRIND} ./msgpack2json -pi ${TESTS_DIR}/basic.mp)
add_test("msgpack2json-basic-debug"           ${COMPARE} ${TESTS_DIR}/basic.json            ${VALGRIND} ./msgpack2json -di ${TESTS_DIR}/basic.mp)

add_test("json2msgpack-base64-str-prefix"     ${COMPARE} ${TESTS_DIR}/base64-str-prefix.mp  ${VALGRIND} ./json2msgpack -i ${TESTS_DIR}/base64-prefix.json)
add_test("json2msgpack-base64-bin"            ${COMPARE} ${TESTS_DIR}/base64-bin-ext.mp     ${VALGRIND} ./json2msgpack -bi ${TESTS_DIR}/base64-prefix.json)
add_test("json2msgpack-base64-bin-lax"        ${COMPARE} ${TESTS_DIR}/base64-bin-ext.mp     ${VALGRIND} ./json2msgpack -bli ${TESTS_DIR}/base64-prefix-lax.json)

add_test("json2msgpack-base64-detect-str"     ${COMPARE} ${TESTS_DIR}/base64-str.mp         ${VALGRIND} ./json2msgpack -B 200 -i ${TESTS_DIR}/base64-detect.json)
add_test("json2msgpack-base64-detect-partial" ${COMPARE} ${TESTS_DIR}/base64-partial.mp     ${VALGRIND} ./json2msgpack -B 50 -i ${TESTS_DIR}/base64-detect.json)
add_test("json2msgpack-base64-detect-bin"     ${COMPARE} ${TESTS_DIR}/base64-bin.mp         ${VALGRIND} ./json2msgpack -B 22 -i ${TESTS_DIR}/base64-detect.json)
add_test("json2msgpack-base64-detect-bin-one" ${COMPARE} ${TESTS_DIR}/base64-bin.mp         ${VALGRIND} ./json2msgpack -B 1 -i ${TESTS_DIR}/base64-detect.json)

add_test("json2msgpack-base64-mixed-partial"  ${COMPARE} ${TESTS_DIR}/base64-partial-ext.mp ${VALGRIND} ./json2msgpack -bB 50 -i ${TESTS_DIR}/base64-mixed.json)
add_test("json2msgpack-base64-mixed-bin"      ${COMPARE} ${TESTS_DIR}/base64-bin-ext.mp     ${VALGRIND} ./json2msgpack -bB 22 -i ${TESTS_DIR}/base64-mixed.json)

#add_test("json2msgpack-base64-bin" ${COMPARE_SCRIPT} ${TESTS_DIR}/base64-bin.mp valgrind ./json2msgpack -bi ${TESTS_DIR}/base64-prefix.json)
#add_test("json2msgpack-base64-bin-lax" ${COMPARE_SCRIPT} ${TESTS_DIR}/base64-bin.mp valgrind ./json2msgpack -lbi ${TESTS_DIR}/base64-prefix-lax.json)
