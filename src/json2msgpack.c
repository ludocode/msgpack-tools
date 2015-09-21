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
#include <errno.h>

#include "b64/cdecode.h"
#include "mpack/mpack.h"
#include "yajl/yajl_tree.h"
#include "yajl/yajl_version.h"

#define VERSION "0.1"

typedef struct options_t {
    const char* command;
    const char* out_filename;
    const char* in_filename;
    bool lax;
    bool use_float;
    bool base64_prefix;
    size_t base64_min_bytes;
} options_t;

static void flush(mpack_writer_t* writer, const char* buffer, size_t count) {
    if (fwrite(buffer, 1, count, (FILE*)writer->context) != count)
        mpack_writer_flag_error(writer, mpack_error_io);
}

static const char* prefix_ext    = "ext:";
static const char* prefix_base64 = "base64:";

// libb64 doesn't have the means to return errors on invalid characters, so
// for base64 detection, we manually scan the string for invalid characters.
// For detection we allow CR/LF but we don't allow spaces (otherwise pretty
// much any normal string would be detected as base64.)
static bool is_base64(const char* string, size_t length) {
    for (size_t i = 0; i < length; ++i) {
        char c = string[i];
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')
                || c == '+' || c == '/' || c == '=' || c == '\r' || c == '\n')) {
            return false;
        }
    }
    return true;
}

static char* convert_base64(options_t* options, const char* p, size_t len, size_t* out_bytes) {
    size_t bytes = len * 3 / 4 + 4; // TODO check this
    char* data = (char*)malloc(bytes);
    if (!data) {
        fprintf(stderr, "%s: allocation failure\n", options->command);
        return NULL;
    }
    base64_decodestate state;
    base64_init_decodestate(&state);
    bytes = base64_decode_block(p, len, data, &state);
    *out_bytes = bytes;
    return data;
}

static bool write_string(options_t* options, mpack_writer_t* writer, const char* string, bool allow_detection) {
    size_t length = strlen(string);

    if (options->base64_prefix) {

        // check for base64 prefix
        if (length >= strlen(prefix_base64) && memcmp(string, prefix_base64, strlen(prefix_base64)) == 0) {
            const char* base64_data = string + strlen(prefix_base64);
            size_t base64_len = length - strlen(prefix_base64);
            if (!is_base64(base64_data, base64_len)) {
                fprintf(stderr, "%s: string prefixed with \"base64:\" contains invalid base64\n", options->command);
                return false;
            }

            // write base64
            size_t count;
            char* bytes = convert_base64(options, base64_data, base64_len, &count);
            if (bytes) {
                mpack_write_bin(writer, bytes, count);
                free(bytes);
                return mpack_writer_error(writer) == mpack_ok;
            }
            return false;
        }

        // check for ext prefix
        if (length >= strlen(prefix_ext) && memcmp(string, prefix_ext, strlen(prefix_ext)) == 0) {

            // parse exttype
            const char* exttype_str = string + strlen(prefix_ext);
            char* remainder;
            errno = 0;
            int64_t exttype = strtol(exttype_str, &remainder, 10);
            if (errno != 0 || *(remainder++) != ':' || strlen(remainder) < strlen(prefix_base64)
                    || memcmp(remainder, prefix_base64, strlen(prefix_base64)) != 0) {
                fprintf(stderr, "\"%s\"\n", remainder);
                fprintf(stderr, "%s: string prefixed with \"ext:\" contains invalid prefix\n", options->command);
                return false;
            }
            if (exttype < INT8_MIN || exttype > INT8_MAX) {
                fprintf(stderr, "%s: string prefixed with \"ext:\" has out-of-bounds ext type: %" PRIi64 "\n", options->command, exttype);
                return false;
            }

            // check base64
            const char* base64_data = remainder + strlen(prefix_base64);
            size_t base64_len = strlen(base64_data);
            if (!is_base64(base64_data, base64_len)) {
                fprintf(stderr, "\"%s\"\n", base64_data);
                fprintf(stderr, "%s: string prefixed with \"ext:\" contains invalid base64\n", options->command);
                return false;
            }

            // write ext
            size_t count;
            char* bytes = convert_base64(options, base64_data, base64_len, &count);
            if (bytes) {
                mpack_write_ext(writer, (int8_t)exttype, bytes, count);
                free(bytes);
                return mpack_writer_error(writer) == mpack_ok;
            }
            return false;
        }

    }

    // try to parse as base64
    if (allow_detection && options->base64_min_bytes != 0 && length >= options->base64_min_bytes && is_base64(string, length)) {
        size_t count;
        char* bytes = convert_base64(options, string, length, &count);
        if (bytes) {
            mpack_write_bin(writer, bytes, count);
            free(bytes);
            return mpack_writer_error(writer) == mpack_ok;
        }
        return false;
    }

    mpack_write_str(writer, string, length);
    return mpack_writer_error(writer) == mpack_ok;
}

static bool write_node(options_t* options, yajl_val node, mpack_writer_t* writer) {
    switch (node->type) {
        case yajl_t_null:   mpack_write_nil(writer);    break;
        case yajl_t_true:   mpack_write_true(writer);   break;
        case yajl_t_false:  mpack_write_false(writer);  break;

        case yajl_t_number:
            if (YAJL_IS_INTEGER(node)) {
                mpack_write_i64(writer, YAJL_GET_INTEGER(node));
            } else if (YAJL_IS_DOUBLE(node)) {
                if (options->use_float)
                    mpack_write_float(writer, (float)YAJL_GET_DOUBLE(node));
                else
                    mpack_write_double(writer, YAJL_GET_DOUBLE(node));
            } else {
                const char* str = YAJL_GET_NUMBER(node);
                char* end;
                errno = 0;
                uint64_t val = strtoull(str, &end, 10);
                if (errno != 0 || *end != '\0' || val == 0) {
                    fprintf(stderr, "%s: JSON number cannot be encoded in MessagePack: \"%s\"\n",
                            options->command, str);
                    return false;
                }
                mpack_write_u64(writer, val);
            }
            break;

        case yajl_t_string:
            if (!write_string(options, writer, YAJL_GET_STRING(node), true))
                return false;
            break;

        case yajl_t_array: {
            size_t count = YAJL_GET_ARRAY(node)->len;
            mpack_start_array(writer, count);
            for (size_t i = 0; i < count; ++i) {
                write_node(options, YAJL_GET_ARRAY(node)->values[i], writer);
                if (mpack_writer_error(writer) != mpack_ok)
                    return false;
            }
            mpack_finish_array(writer);
            break;
        }

        case yajl_t_object: {
            size_t count = YAJL_GET_OBJECT(node)->len;
            mpack_start_map(writer, count);
            for (uint32_t i = 0; i < count; ++i) {
                if (!write_string(options, writer, YAJL_GET_OBJECT(node)->keys[i], false))
                    return false;
                write_node(options, YAJL_GET_OBJECT(node)->values[i], writer);
                if (mpack_writer_error(writer) != mpack_ok)
                    return false;
            }
            mpack_finish_map(writer);
            break;
        }

        default:
            return false;
    }

    return mpack_writer_error(writer) == mpack_ok;
}

static bool output(options_t* options, yajl_val node) {
    FILE* out_file;
    if (options->out_filename) {
        out_file = fopen(options->out_filename, "wb");
        if (out_file == NULL) {
            fprintf(stderr, "%s: could not open \"%s\" for writing.\n", options->command, options->out_filename);
            return false;
        }
    } else {
        out_file = stdout;
    }

    mpack_writer_t writer;
    mpack_writer_init_stack(&writer);
    mpack_writer_set_context(&writer, out_file);
    mpack_writer_set_flush(&writer, flush);

    write_node(options, node, &writer);

    mpack_error_t error = mpack_writer_destroy(&writer);
    if (out_file != stdout)
        fclose(out_file);
    return error == mpack_ok;
}

static bool load_file(options_t* options, char** out_data, size_t* out_size) {
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

    size_t capacity = 4096;
    size_t size = 0;
    char* data = (char*)malloc(capacity);

    while (1) {
        size_t n = fread(data + size, 1, capacity - size, in_file);

        // YAJL parses a null-terminated string, so we need to scan the data
        // to make sure it has no null bytes. They are not legal JSON anyway.
        for (size_t i = size; i < size + n; ++i) {
            if (data[i] == '\0') {
                fprintf(stderr, "%s: JSON cannot contain null bytes\n", options->command);
                if (in_file != stdin)
                    fclose(in_file);
                free(data);
                return false;
            }
        }

        size += n;

        if (ferror(in_file)) {
            fprintf(stderr, "%s: error reading data\n", options->command);
            if (in_file != stdin)
                fclose(in_file);
            free(data);
            return false;
        }

        // We always need enough space to store the null-terminator
        if (size == capacity) {
            capacity *= 2;
            data = (char*)realloc(data, capacity);
        }

        if (feof(in_file))
            break;

        // This shouldn't happen; no bytes should mean error or EOF. We
        // check and throw an error anyway to avoid an infinite loop.
        if (n == 0) {
            fprintf(stderr, "%s: error reading data\n", options->command);
            if (in_file != stdin)
                fclose(in_file);
            free(data);
            return false;
        }
    }

    data[size] = '\0';

    if (in_file != stdin)
        fclose(in_file);
    *out_data = data;
    *out_size = size;
    return true;
}

static bool convert(options_t* options) {
    char* data = NULL;
    size_t size = 0;
    if (!load_file(options, &data, &size))
        return false;

    // The data has been null-terminated by load_file()
    char errbuf[1024];
    yajl_val node = yajl_tree_parse_options(data, errbuf, sizeof(errbuf),
            options->lax ? yajl_tree_option_allow_trailing_separator : yajl_tree_option_dont_allow_comments);
    if (node == NULL) {
        errbuf[sizeof(errbuf)-1] = '\0';
        fprintf(stderr, "%s: error parsing JSON: %s\n", options->command, errbuf);
        free(data);
        return false;
    }

    bool error = output(options, node);
    yajl_tree_free(node);
    free(data);
    return error;
}

static void parse_min_bytes(options_t* options) {
    const char* arg = optarg;
    char* end;
    errno = 0;
    int64_t value = strtol(arg, &end, 10);
    if (errno != 0 || *end != '\0' || value <= 0) {
        fprintf(stderr, "%s: -B requires a positive integer, not \"%s\"\n", options->command, arg);
        exit(EXIT_FAILURE);
    }
    if (SIZE_MAX < INT64_MAX && value > (int64_t)SIZE_MAX) {
        fprintf(stderr, "%s: -B argument is out of bounds: %" PRIi64 "\n", options->command, value);
        exit(EXIT_FAILURE);
    }
    options->base64_min_bytes = (size_t)value;
}

static void usage(const char* command) {
    fprintf(stderr, "Usage: %s [-i <infile>] [-o <outfile>] [-lfb] [-B <min>]\n", command);
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "    -i <infile>  Input filename (default stdin)\n");
    fprintf(stderr, "    -o <outfile>  Output filename (default stdout)\n");
    fprintf(stderr, "    -l  Lax mode, allows comments and trailing commas\n");
    fprintf(stderr, "    -f  Write floats instead of doubles\n");
    fprintf(stderr, "    -b  Convert base64 strings with \"base64:\" prefix to bin\n");
    fprintf(stderr, "    -B <min>  Try to convert any base64 string of at least <min> bytes to bin\n");
    fprintf(stderr, "    -h  Print this help\n");
    fprintf(stderr, "    -v  Print version information\n");
}

static void version(const char* command) {
    fprintf(stderr, "%s version %s\n", command, VERSION);

    static char buf[16];
    snprintf(buf, sizeof(buf), "%u.%u.%u", YAJL_MAJOR, YAJL_MINOR, YAJL_MICRO);
    fprintf(stderr, "YAJL version %s\n", buf);

    fprintf(stderr, "MPack version %s\n", MPACK_VERSION_STRING);
    fprintf(stderr, "libb64 version %s\n", "1.2.1");
}

int main(int argc, char** argv) {
    options_t options;
    memset(&options, 0, sizeof(options));
    options.command = argv[0];

    opterr = 0;
    int opt;
    while ((opt = getopt(argc, argv, "i:o:lfbB:hv?")) != -1) {
        switch (opt) {
            case 'i':
                options.in_filename = optarg;
                break;
            case 'o':
                options.out_filename = optarg;
                break;
            case 'l':
                options.lax = true;
                break;
            case 'f':
                options.use_float = true;
                break;
            case 'b':
                options.base64_prefix = true;
                break;
            case 'B':
                parse_min_bytes(&options);
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
                if (optopt == 'i' || optopt == 'o' || optopt == 'B')
                    fprintf(stderr, "%s: -%c requires an argument\n", options.command, optopt);
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
