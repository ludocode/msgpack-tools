/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2015 Nicholas Fraser
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

#define _XOPEN_SOURCE

#include <stdio.h>
#include <unistd.h>

extern "C" {
#include "b64/cencode.h"
}

#include "mpack/mpack.h"
#include "yajl/yajl_gen.h"
#include "yajl/yajl_version.h"

#define VERSION "0.2"

typedef struct options_t {
    const char* command;
    const char* out_filename;
    const char* in_filename;
    bool debug;
    bool pretty;
    bool base64;
    bool base64_prefix;
} options_t;

static size_t fill(mpack_reader_t* reader, char* buffer, size_t count) {
    return fread((void*)buffer, 1, count, (FILE*)reader->context);
}

static bool skip_quotes = false;
static FILE* out_file = NULL;

static void* print(void* ctx, const char* str, unsigned int len) {

    // This is used to ignore the output of quotes in order to print
    // our <bin> and <ext> types in debug mode.
    if (skip_quotes && len == 1 && str[0] == '"')
        return NULL;

    fwrite(str, 1, len, (FILE*)ctx);
    // TODO error
    return NULL;
}

// Reads MessagePack string bytes and outputs a JSON string
static bool string(mpack_reader_t* reader, yajl_gen gen, options_t* options, uint32_t len) {
    char* str = (char*)malloc(len);

    mpack_read_bytes(reader, str, len);
    if (mpack_reader_error(reader) != mpack_ok) {
        fprintf(stderr, "%s: error reading string bytes\n", options->command);
        free(str);
        return false;
    }
    mpack_done_str(reader);

    yajl_gen_status status = yajl_gen_string(gen, (const unsigned char*)str, len);
    free(str);
    return status == yajl_gen_status_ok;
}

static const char* ext_str = "ext:";
static const char* b64_str = "base64:";

static uint32_t base64_len(uint32_t len) {
    return ((len + 3) * 4) / 3; // TODO check this
}

// Converts MessagePack bin/ext bytes to JSON base64 string
static bool base64(mpack_reader_t* reader, yajl_gen gen, options_t* options, uint32_t len, char* output, char* p, bool prefix) {
    if (prefix) {
        memcpy(p, b64_str, strlen(b64_str));
        p += strlen(b64_str);
    }

    base64_encodestate state;
    base64_init_encodestate(&state);

    while (len > 0) {
        char buf[4096];
        uint32_t count = (len < sizeof(buf)) ? len : sizeof(buf);
        len -= count;
        mpack_read_bytes(reader, buf, count);
        if (mpack_reader_error(reader) != mpack_ok) {
            fprintf(stderr, "%s: error reading base64 bytes\n", options->command);
            return false;
        }
        p += base64_encode_block(buf, (int)count, p, &state);
    }
    p += base64_encode_blockend(p, &state);

    bool ret = yajl_gen_string(gen, (const unsigned char*)output, p - output) == yajl_gen_status_ok;
    return ret;
}

// Reads MessagePack bin bytes and outputs a JSON base64 string
static bool base64_bin(mpack_reader_t* reader, yajl_gen gen, options_t* options, uint32_t len, bool prefix) {
    uint32_t new_len = base64_len(len) + (prefix ? strlen(b64_str) : 0);
    char* output = (char*)malloc(new_len);

    bool ret = base64(reader, gen, options, len, output, output, prefix);

    mpack_done_bin(reader);
    free(output);
    return ret;
}

// Reads MessagePack ext bytes and outputs a JSON base64 string
static bool base64_ext(mpack_reader_t* reader, yajl_gen gen, options_t* options, int8_t exttype, uint32_t len) {
    uint32_t new_len = base64_len(len) + strlen(ext_str) + 5 + strlen(b64_str);
    char* output = (char*)malloc(new_len);

    char* p = output;
    strcpy(p, ext_str);
    p += strlen(ext_str);
    sprintf(p, "%i", exttype);
    p += strlen(p);
    *p++ = ':';

    bool ret = base64(reader, gen, options, len, output, p, true);

    mpack_done_ext(reader);
    free(output);
    return ret;
}

static bool element(mpack_reader_t* reader, yajl_gen gen, options_t* options, int depth) {
    const mpack_tag_t tag = mpack_read_tag(reader);
    if (mpack_reader_error(reader) != mpack_ok)
        return false;

    if (!options->debug && depth == 0 && (tag.type != mpack_type_map && tag.type != mpack_type_array)) {
        fprintf(stderr, "%s: Top-level object must be a map or array. Try debug viewing mode (-d)\n", options->command);
        return false;
    }

    // TODO check not depth zero
    switch (tag.type) {
        case mpack_type_bool:   return yajl_gen_bool(gen, tag.v.b) == yajl_gen_status_ok;
        case mpack_type_nil:    return yajl_gen_null(gen) == yajl_gen_status_ok;
        case mpack_type_int:    return yajl_gen_integer(gen, tag.v.i) == yajl_gen_status_ok;
        case mpack_type_float:  return yajl_gen_double(gen, tag.v.f) == yajl_gen_status_ok;
        case mpack_type_double: return yajl_gen_double(gen, tag.v.d) == yajl_gen_status_ok;

        case mpack_type_uint:
            if (tag.v.u > (uint64_t)INT64_MAX) {
                char buf[32];
                snprintf(buf, sizeof(buf), "%" PRIu64, tag.v.u);
                return yajl_gen_string(gen, (const unsigned char*)buf, strlen(buf)) == yajl_gen_status_ok;
            }
            return yajl_gen_integer(gen, (int64_t)tag.v.u) == yajl_gen_status_ok;

        case mpack_type_str:
            return string(reader, gen, options, tag.v.l);

        case mpack_type_bin:
            if (options->base64) {
                return base64_bin(reader, gen, options, tag.v.l, options->base64_prefix);
            } else if (options->debug) {
                mpack_skip_bytes(reader, tag.v.l);
                mpack_done_bin(reader);

                // output nothing to allow us to print our debug string
                skip_quotes = true;
                if (yajl_gen_string(gen, (const unsigned char*)"", 0) != yajl_gen_status_ok)
                    return false;
                skip_quotes = false;

                char buf[64];
                snprintf(buf, sizeof(buf), "<bin of size %u>", tag.v.l);
                print(out_file, buf, strlen(buf));
                return true;
            } else {
                fprintf(stderr, "%s: bin unencodable in JSON. Try debug viewing mode (-d)\n", options->command);
                return false;
            }

        case mpack_type_ext:
            if (options->base64) {
                return base64_ext(reader, gen, options, tag.exttype, tag.v.l);
            } else if (options->debug) {
                mpack_skip_bytes(reader, tag.v.l);
                mpack_done_ext(reader);

                // output nothing to allow us to print our debug string
                skip_quotes = true;
                if (yajl_gen_string(gen, (const unsigned char*)"", 0) != yajl_gen_status_ok)
                    return false;
                skip_quotes = false;

                char buf[64];
                snprintf(buf, sizeof(buf), "<ext of type %i size %u>", tag.exttype, tag.v.l);
                print(out_file, buf, strlen(buf));
                return true;
            } else {
                fprintf(stderr, "%s: ext type %i unencodable in JSON. Try debug viewing mode (-d)\n", options->command, tag.exttype);
                return false;
            }

        case mpack_type_array:
            if (yajl_gen_array_open(gen) != yajl_gen_status_ok)
                return false;
            for (size_t i = 0; i < tag.v.l; ++i)
                if (!element(reader, gen, options, depth + 1))
                    return false;
            mpack_done_array(reader);
            return yajl_gen_array_close(gen) == yajl_gen_status_ok;

        case mpack_type_map:
            if (yajl_gen_map_open(gen) != yajl_gen_status_ok)
                return false;
            for (size_t i = 0; i < tag.v.l; ++i) {

                if (options->debug) {
                    element(reader, gen, options, depth + 1);
                } else {
                    uint32_t len = mpack_expect_str(reader);
                    if (mpack_reader_error(reader) != mpack_ok) {
                        fprintf(stderr, "%s: map key is not a string. Try debug viewing mode (-d)\n", options->command);
                        return false;
                    }
                    if (!string(reader, gen, options, len))
                        return false;
                }

                if (!element(reader, gen, options, depth + 1))
                    return false;
            }
            mpack_done_map(reader);
            return yajl_gen_map_close(gen) == yajl_gen_status_ok;
    }

    return true;
}

static bool convert(options_t* options) {
    FILE* in_file;
    if (options->in_filename) {
        in_file = fopen(options->in_filename, "rb");
        if (in_file == NULL) {
            fprintf(stderr, "%s: could not open \"%s\" for reading.\n", options->command, options->in_filename);
            return false;
        }
    } else {
        in_file = stdin;
    }

    if (options->out_filename) {
        out_file = fopen(options->out_filename, "wb");
        if (out_file == NULL) {
            fprintf(stderr, "%s: could not open \"%s\" for writing.\n", options->command, options->out_filename);
            if (in_file != stdin)
                fclose(in_file);
            return false;
        }
    } else {
        out_file = stdout;
    }

    yajl_gen gen = yajl_gen_alloc(NULL);
    if (options->pretty)
        yajl_gen_config(gen, yajl_gen_beautify);
    if (options->debug)
        yajl_gen_config(gen, yajl_gen_allow_non_string_keys);
    yajl_gen_config(gen, yajl_gen_print_callback, print, out_file);

    mpack_reader_t reader;
    mpack_reader_init_stack(&reader);
    mpack_reader_set_fill(&reader, fill);
    mpack_reader_set_context(&reader, in_file);

    bool ret = element(&reader, gen, options, 0);

    mpack_error_t error = mpack_reader_destroy(&reader);
    yajl_gen_free(gen);

    if (out_file != stdout)
        fclose(out_file);
    if (in_file != stdin)
        fclose(in_file);

    if (!ret || error != mpack_ok)
        fprintf(stderr, "%s: parse error %i\n", options->command, (int)error);
    return ret;
}

static void usage(const char* command) {
    fprintf(stderr, "Usage: %s [-dpbB] [-i <infile>] [-o <outfile>]\n", command);
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "    -i <infile>  Input filename (default stdin)\n");
    fprintf(stderr, "    -o <outfile>  Output filename (default stdout)\n");
    fprintf(stderr, "    -d  Debug viewing mode, output pseudo-JSON instead of aborting with error\n");
    fprintf(stderr, "    -p  Output pretty-printed JSON\n");
    fprintf(stderr, "    -b  Convert bin to base64 string with \"base64:\" prefix\n");
    fprintf(stderr, "    -B  Convert bin to base64 string with no prefix\n");
    fprintf(stderr, "    -h  Print this help\n");
    fprintf(stderr, "    -v  Print version information\n");
    fprintf(stderr, "For viewing MessagePack, you probably want -d or -di <filename>.\n");
}

static void version(const char* command) {
    fprintf(stderr, "%s version %s\n", command, VERSION);
    fprintf(stderr, "MPack version %s -- %s\n", MPACK_VERSION_STRING, "https://github.com/ludocode/mpack");

    static char buf[16];
    snprintf(buf, sizeof(buf), "%u.%u.%u", YAJL_MAJOR, YAJL_MINOR, YAJL_MICRO);
    fprintf(stderr, "YAJL version %s -- %s\n", buf, "http://lloyd.github.io/yajl/");

    fprintf(stderr, "libb64 version %s -- %s\n", "1.2.1", "http://libb64.sourceforge.net/");
}

int main(int argc, char** argv) {
    options_t options;
    memset(&options, 0, sizeof(options));
    options.command = argv[0];

    opterr = 0;
    int opt;
    while ((opt = getopt(argc, argv, "i:o:dpbBhv?")) != -1) {
        switch (opt) {
            case 'i':
                options.in_filename = optarg;
                break;
            case 'o':
                options.out_filename = optarg;
                break;
            case 'd':
                options.debug = true;
                options.pretty = true;
                break;
            case 'p':
                options.pretty = true;
                break;
            case 'b':
                options.base64 = true;
                options.base64_prefix = true;
                break;
            case 'B':
                options.base64 = true;
                options.base64_prefix = false;
                break;
            case 'h':
                usage(options.command);
                return EXIT_SUCCESS;
            case 'v':
                version(options.command);
                return EXIT_SUCCESS;
            default: /* ? */
                if (optopt == 0) {
                    // we allow both -h and -? as help
                    usage(options.command);
                    return EXIT_SUCCESS;
                }
                if (optopt == 'i' || optopt == 'o')
                    fprintf(stderr, "%s: option '%c' requires an argument\n", options.command, optopt);
                else
                    fprintf(stderr, "%s: invalid option -- '%c'\n", options.command, optopt);
                usage(options.command);
                return EXIT_FAILURE;
        }
    }

    if (optind < argc) {
        fprintf(stderr, "%s: not an option -- \"%s\"\n", options.command, argv[optind]);
        usage(options.command);
        return EXIT_FAILURE;
    }

    return convert(&options) ? EXIT_SUCCESS : EXIT_FAILURE;
}
