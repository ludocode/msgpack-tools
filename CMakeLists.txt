cmake_minimum_required(VERSION 2.6)

include(GNUInstallDirs)
include(CheckCCompilerFlag)
include(CheckCXXCompilerFlag)

project(msgpack2json)

set(PROJECT_VERSION_MAJOR 0)
set(PROJECT_VERSION_MINOR 6)
set(PROJECT_VERSION_PATCH 0)

if(PROJECT_VERSION_PATCH STREQUAL "0")
    set(PROJECT_VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}")
else()
    set(PROJECT_VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}")
endif()

set(CONTRIB_DIR ${CMAKE_BINARY_DIR}/contrib)
file(MAKE_DIRECTORY ${CONTRIB_DIR})

if(NOT CMAKE_BUILD_TYPE)
    message(STATUS "No build type selected, default to Release")
    set(CMAKE_BUILD_TYPE "Release")
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_VERBOSE_MAKEFILE ON)
endif()

CHECK_C_COMPILER_FLAG("-std=c11" HAS_C11)
if(HAS_C11)
    set(C_FLAGS   "-std=c11")
else()
    set(C_FLAGS   "-std=c99")
endif()

CHECK_CXX_COMPILER_FLAG("-std=c++11" HAS_CXX11)
if(HAS_CXX11)
    set(CXX_FLAGS   "-std=c++11")
else()
    set(CXX_FLAGS   "")
endif()

set(MPACK_COMMIT "df17e83f0fa8571b9cd0d8ccf38144fa90e244d1")
set(RAPIDJSON_COMMIT "99ba17bd66a85ec64a2f322b68c2b9c3b77a4391")
set(LIBB64_VERSION "1.2.1")

if(CMAKE_BUILD_TOOL MATCHES "(msdev|devenv|nmake)")
    # Windows support is not implemented yet
    add_definitions(/W2)
else()
    set(FLAGS "-Wall -Wextra -DVERSION=\\\"${PROJECT_VERSION}\\\" -DLIBB64_VERSION=\\\"${LIBB64_VERSION}\\\"")
    set(CMAKE_C_FLAGS_DEBUG   "${C_FLAGS} ${FLAGS} -g -O0 -DDEBUG")
    set(CMAKE_C_FLAGS_RELEASE "${C_FLAGS} ${FLAGS} -O3 -DNDEBUG -fPIC -DPIC")
    set(CMAKE_CXX_FLAGS_DEBUG   "${CXX_FLAGS} ${FLAGS} -g -O0 -DDEBUG")
    set(CMAKE_CXX_FLAGS_RELEASE "${CXX_FLAGS} ${FLAGS} -O3 -DNDEBUG -fPIC -DPIC")
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -s")
endif()


# mpack

set(MPACK_FILE "mpack-${MPACK_COMMIT}.tar.gz")
set(MPACK_DIR "${CONTRIB_DIR}/mpack-${MPACK_COMMIT}")
set(MPACK_URL "https://github.com/ludocode/mpack/archive/${MPACK_COMMIT}.tar.gz")

if(EXISTS "${CMAKE_SOURCE_DIR}/contrib/${MPACK_FILE}")
    message(STATUS "Found package: ${MPACK_FILE}")
else()
    message(STATUS "Downloading: ${MPACK_FILE}")
    file(DOWNLOAD ${MPACK_URL} "${CMAKE_SOURCE_DIR}/contrib/${MPACK_FILE}")
    if(NOT EXISTS "${CMAKE_SOURCE_DIR}/contrib/${MPACK_FILE}")
        message(FATAL_ERROR "\nFailed to download source file: ${MPACK_FILE}\nFrom: ${MPACK_URL}")
    endif()
endif()

execute_process(COMMAND ${CMAKE_COMMAND} -E tar xzf ${CMAKE_SOURCE_DIR}/contrib/${MPACK_FILE} WORKING_DIRECTORY ${CONTRIB_DIR})

file(GLOB_RECURSE MPACK_SRCS ${MPACK_DIR}/src/*.c)
include_directories(SYSTEM ${CMAKE_BINARY_DIR} ${MPACK_DIR}/src)


# rapidjson

set(RAPIDJSON_FILE "rapidjson-${RAPIDJSON_COMMIT}.tar.gz")
set(RAPIDJSON_DIR "${CONTRIB_DIR}/rapidjson-${RAPIDJSON_COMMIT}")
set(RAPIDJSON_URL "https://github.com/miloyip/rapidjson/archive/${RAPIDJSON_COMMIT}.tar.gz")

if(EXISTS "${CMAKE_SOURCE_DIR}/contrib/${RAPIDJSON_FILE}")
    message(STATUS "Found package: ${RAPIDJSON_FILE}")
else()
    message(STATUS "Downloading: ${RAPIDJSON_FILE}")
    file(DOWNLOAD ${RAPIDJSON_URL} "${CMAKE_SOURCE_DIR}/contrib/${RAPIDJSON_FILE}")
    if(NOT EXISTS "${CMAKE_SOURCE_DIR}/contrib/${RAPIDJSON_FILE}")
        message(FATAL_ERROR "\nFailed to download source file: ${RAPIDJSON_FILE}\nFrom: ${RAPIDJSON_URL}")
    endif()
endif()

execute_process(COMMAND ${CMAKE_COMMAND} -E tar xzf ${CMAKE_SOURCE_DIR}/contrib/${RAPIDJSON_FILE} WORKING_DIRECTORY ${CONTRIB_DIR})

include_directories(SYSTEM ${RAPIDJSON_DIR}/include)


# libb64

set(LIBB64_FILE "libb64-${LIBB64_VERSION}.zip")
set(LIBB64_DIR "${CONTRIB_DIR}/libb64-${LIBB64_VERSION}")
set(LIBB64_URL "http://downloads.sourceforge.net/project/libb64/libb64/libb64/${LIBB64_FILE}?use_mirror=autoselect")

if(EXISTS "${CMAKE_SOURCE_DIR}/contrib/${LIBB64_FILE}")
    message(STATUS "Found package: ${LIBB64_FILE}")
else()
    message(STATUS "Downloading: ${LIBB64_FILE}")
    file(DOWNLOAD ${LIBB64_URL} "${CMAKE_SOURCE_DIR}/contrib/${LIBB64_FILE}")
    if(NOT EXISTS "${CMAKE_SOURCE_DIR}/contrib/${LIBB64_FILE}")
        message(FATAL_ERROR "\nFailed to download source file: ${LIBB64_FILE}\nFrom: ${LIBB64_URL}")
    endif()
endif()

execute_process(COMMAND ${CMAKE_COMMAND} -E tar xf "${CMAKE_SOURCE_DIR}/contrib/${LIBB64_FILE}" WORKING_DIRECTORY "${CONTRIB_DIR}")

# Remove libb64's newlines
set(LIBB64_CENCODE_FILE ${LIBB64_DIR}/src/cencode.c)
file(READ ${LIBB64_CENCODE_FILE} LIBB64_CENCODE)
string(REPLACE "*codechar++ = '\\n';" "/* *codechar++ = '\\n'; */" LIBB64_CENCODE "${LIBB64_CENCODE}")
file(WRITE ${LIBB64_CENCODE_FILE} "${LIBB64_CENCODE}")

file(GLOB_RECURSE LIBB64_SRCS ${LIBB64_DIR}/src/*.c)
include_directories(SYSTEM ${LIBB64_DIR}/include)


# executable targets

add_executable(msgpack2json src/msgpack2json.cpp ${MPACK_SRCS} ${LIBB64_SRCS})
add_executable(json2msgpack src/json2msgpack.cpp ${MPACK_SRCS} ${LIBB64_SRCS})

install(TARGETS msgpack2json json2msgpack DESTINATION bin)


# manpages

if(EXISTS "${CMAKE_SOURCE_DIR}/docs/msgpack2json.1" AND EXISTS "${CMAKE_SOURCE_DIR}/docs/json2msgpack.1")
    # install pre-generated pages
    message(STATUS "Found pre-generated man pages: docs/msgpack2json.1 docs/json2msgpack.1")
    install(FILES ${CMAKE_SOURCE_DIR}/docs/msgpack2json.1 DESTINATION ${CMAKE_INSTALL_PREFIX}/share/man/man1)
    install(FILES ${CMAKE_SOURCE_DIR}/docs/json2msgpack.1 DESTINATION ${CMAKE_INSTALL_PREFIX}/share/man/man1)
else()

    # try to find md2man-roff
    find_program(MD2MAN-ROFF md2man-roff)
    if(MD2MAN-ROFF STREQUAL "MD2MAN-ROFF-NOTFOUND")
        message(WARNING "\nmd2man-roff is not installed. Man pages will not be created!\nSee: https://github.com/sunaku/md2man")
    else()

        # add custom commands to build man pages
        # cmake is insanely verbose for creating custom builders

        message(STATUS "Found md2man-roff: ${MD2MAN-ROFF}")
        add_custom_command(
            OUTPUT ${CMAKE_BINARY_DIR}/msgpack2json.1
            MAIN_DEPENDENCY ${CMAKE_SOURCE_DIR}/docs/msgpack2json.md
            COMMAND cat ${CMAKE_SOURCE_DIR}/docs/msgpack2json.md | sed "s/\$version/${PROJECT_VERSION}/" | ${MD2MAN-ROFF} > ${CMAKE_BINARY_DIR}/msgpack2json.1
            VERBATIM)
        add_custom_target(msgpack2json-man DEPENDS ${CMAKE_BINARY_DIR}/msgpack2json.1)
        add_dependencies(msgpack2json msgpack2json-man)
        install(FILES ${CMAKE_BINARY_DIR}/msgpack2json.1 DESTINATION share/man/man1)

        add_custom_command(
            OUTPUT ${CMAKE_BINARY_DIR}/json2msgpack.1
            MAIN_DEPENDENCY ${CMAKE_SOURCE_DIR}/docs/json2msgpack.md
            COMMAND cat ${CMAKE_SOURCE_DIR}/docs/json2msgpack.md | sed "s/\$version/${PROJECT_VERSION}/" | ${MD2MAN-ROFF} > ${CMAKE_BINARY_DIR}/json2msgpack.1
            VERBATIM)
        add_custom_target(json2msgpack-man DEPENDS ${CMAKE_BINARY_DIR}/json2msgpack.1)
        add_dependencies(json2msgpack json2msgpack-man)
        install(FILES ${CMAKE_BINARY_DIR}/json2msgpack.1 DESTINATION share/man/man1)

    endif()
endif()


# testing

set(VALGRIND "${CMAKE_SOURCE_DIR}/tools/valgrind.sh")
set(TESTS_DIR "${CMAKE_SOURCE_DIR}/tests")
set(COMPARE "${CMAKE_SOURCE_DIR}/tools/test-compare.sh")
set(FAIL "${CMAKE_SOURCE_DIR}/tools/test-fail.sh")

enable_testing()

add_test("json2msgpack-basic"                 ${COMPARE} ${TESTS_DIR}/basic.mp                     ${VALGRIND} ./json2msgpack -i ${TESTS_DIR}/basic.json)
add_test("json2msgpack-basic-min"             ${COMPARE} ${TESTS_DIR}/basic.mp                     ${VALGRIND} ./json2msgpack -i ${TESTS_DIR}/basic-min.json)
add_test("json2msgpack-basic-lax"             ${COMPARE} ${TESTS_DIR}/basic.mp                     ${VALGRIND} ./json2msgpack -li ${TESTS_DIR}/basic-lax.json)
add_test("json2msgpack-basic-base64"          ${COMPARE} ${TESTS_DIR}/basic.mp                     ${VALGRIND} ./json2msgpack -B 22 -bi ${TESTS_DIR}/basic.json)
add_test("json2msgpack-basic-strict-fail"     ${FAIL}                                              ${VALGRIND} ./json2msgpack -i ${TESTS_DIR}/basic-lax.json)

add_test("msgpack2json-basic-min"             ${COMPARE} ${TESTS_DIR}/basic-min.json               ${VALGRIND} ./msgpack2json -i ${TESTS_DIR}/basic.mp)
add_test("msgpack2json-basic"                 ${COMPARE} ${TESTS_DIR}/basic.json                   ${VALGRIND} ./msgpack2json -pi ${TESTS_DIR}/basic.mp)
add_test("msgpack2json-basic-debug"           ${COMPARE} ${TESTS_DIR}/basic.json                   ${VALGRIND} ./msgpack2json -di ${TESTS_DIR}/basic.mp)
add_test("msgpack2json-stdin"                 ${COMPARE} ${TESTS_DIR}/basic.json                   bash -c "cat ${TESTS_DIR}/basic.mp | ${VALGRIND} ./msgpack2json -p")

add_test("json2msgpack-base64-str-prefix"     ${COMPARE} ${TESTS_DIR}/base64-str-prefix.mp         ${VALGRIND} ./json2msgpack -i ${TESTS_DIR}/base64-prefix.json)
add_test("json2msgpack-base64-bin"            ${COMPARE} ${TESTS_DIR}/base64-bin-ext.mp            ${VALGRIND} ./json2msgpack -bi ${TESTS_DIR}/base64-prefix.json)
add_test("json2msgpack-base64-bin-lax"        ${COMPARE} ${TESTS_DIR}/base64-bin-ext.mp            ${VALGRIND} ./json2msgpack -bli ${TESTS_DIR}/base64-prefix-lax.json)

add_test("json2msgpack-base64-detect-str"     ${COMPARE} ${TESTS_DIR}/base64-str.mp                ${VALGRIND} ./json2msgpack -B 200 -i ${TESTS_DIR}/base64-detect.json)
add_test("json2msgpack-base64-detect-partial" ${COMPARE} ${TESTS_DIR}/base64-partial.mp            ${VALGRIND} ./json2msgpack -B 50 -i ${TESTS_DIR}/base64-detect.json)
add_test("json2msgpack-base64-detect-bin"     ${COMPARE} ${TESTS_DIR}/base64-bin.mp                ${VALGRIND} ./json2msgpack -B 22 -i ${TESTS_DIR}/base64-detect.json)
add_test("json2msgpack-base64-detect-bin-one" ${COMPARE} ${TESTS_DIR}/base64-bin.mp                ${VALGRIND} ./json2msgpack -B 1 -i ${TESTS_DIR}/base64-detect.json)

add_test("json2msgpack-base64-mixed-partial"  ${COMPARE} ${TESTS_DIR}/base64-partial-ext.mp        ${VALGRIND} ./json2msgpack -bB 50 -i ${TESTS_DIR}/base64-mixed.json)
add_test("json2msgpack-base64-mixed-bin"      ${COMPARE} ${TESTS_DIR}/base64-bin-ext.mp            ${VALGRIND} ./json2msgpack -bB 22 -i ${TESTS_DIR}/base64-mixed.json)

add_test("json2msgpack-value-string"          ${COMPARE} ${TESTS_DIR}/value-string.mp              ${VALGRIND} ./json2msgpack -i ${TESTS_DIR}/value-string.json)
add_test("json2msgpack-value-int"             ${COMPARE} ${TESTS_DIR}/value-int.mp                 ${VALGRIND} ./json2msgpack -i ${TESTS_DIR}/value-int.json)
add_test("msgpack2json-value-string"          ${COMPARE} ${TESTS_DIR}/value-string.json            ${VALGRIND} ./msgpack2json -i ${TESTS_DIR}/value-string.mp)
add_test("msgpack2json-value-int"             ${COMPARE} ${TESTS_DIR}/value-int.json               ${VALGRIND} ./msgpack2json -i ${TESTS_DIR}/value-int.mp)

add_test("msgpack2json-continuous-single"     ${COMPARE} ${TESTS_DIR}/basic-min.json               ${VALGRIND} ./msgpack2json -ci ${TESTS_DIR}/basic.mp)
add_test("msgpack2json-continuous"            ${COMPARE} ${TESTS_DIR}/continuous.json              ${VALGRIND} ./msgpack2json -cpi ${TESTS_DIR}/continuous.mp)
add_test("msgpack2json-continuous-commas"     ${COMPARE} ${TESTS_DIR}/continuous-commas.json       ${VALGRIND} ./msgpack2json -Cpi ${TESTS_DIR}/continuous.mp)
add_test("msgpack2json-continuous-commas-min" ${COMPARE} ${TESTS_DIR}/continuous-commas-min.json   ${VALGRIND} ./msgpack2json -Ci ${TESTS_DIR}/continuous.mp)


# packaging

if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux" AND EXISTS "/etc/debian_version")
    message(STATUS "Debian-based Linux OS detected")
    set(CPACK_GENERATOR "DEB")
    set(CPACK_PACKAGE_NAME "msgpack-tools")
    set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
    set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${PROJECT_VERSION}-${CMAKE_SYSTEM_PROCESSOR}" )
    set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "https://github.com/ludocode/msgpack-tools")
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Jerry Jacobs")
    set(CPACK_PACKAGE_CONTACT "jerry.jacobs@xor-gate.org")
    set(CPACK_DEBIAN_PACKAGE_DESCRIPTION "Command-line tools for converting between MessagePack and JSON")
    include(CPack)
endif()
