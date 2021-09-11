/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2015-2017 Nicholas Fraser
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef MSGPACK2JSON_COMMON_H
#define MSGPACK2JSON_COMMON_H 1

#define _XOPEN_SOURCE

#include <stdio.h>
#include <unistd.h>

#include "rapidjson/error/en.h"

// We include the source for our dependencies directly, silencing whatever
// warnings are necessary. This greatly simplifies the build process.

#pragma GCC diagnostic push

    // clang and gcc both warn about unrecognized warning options, but use
    // different means to silence them.
    #ifdef __clang__
        #pragma GCC diagnostic ignored "-Wunknown-warning-option"
    #else
        #pragma GCC diagnostic ignored "-Wpragmas"
    #endif

    #pragma GCC diagnostic push
        // mpack uses empty variadic macros.
        #pragma GCC diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"

        // mpack may or may not be amalgamated.
        #define MPACK_INTERNAL 1
        #define MPACK_EMIT_INLINE_DEFS 1
        #include "mpack/mpack.h"
        #if MPACK_AMALGAMATED
            #include "mpack/mpack.c"
        #else
            #include "mpack/mpack-platform.c"
            #include "mpack/mpack-common.c"
            #include "mpack/mpack-expect.c"
            #include "mpack/mpack-node.c"
            #include "mpack/mpack-reader.c"
            #include "mpack/mpack-writer.c"
        #endif
    #pragma GCC diagnostic pop

    #pragma GCC diagnostic push
        // libb64 has switch case fallthroughs.
        #pragma GCC diagnostic ignored "-Wimplicit-fallthrough"
        #include "cdecode.c"
        #include "cencode.c"
    #pragma GCC diagnostic pop

    #include "rapidjson/filewritestream.h"
    #include "rapidjson/prettywriter.h"
    #include "rapidjson/writer.h"
    #pragma GCC diagnostic push
        // rapidjson copies classes with memcpy.
        #pragma GCC diagnostic ignored "-Wclass-memaccess"
        #include "rapidjson/document.h"
    #pragma GCC diagnostic pop

#pragma GCC diagnostic pop

#define BUFFER_SIZE 65536

#endif
