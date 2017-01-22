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

#include "common.h"
#include <errno.h>
#include "rapidjson/document.h"

using namespace rapidjson;

typedef struct options_t {
    const char* command;
    const char* out_filename;
    const char* in_filename;
    bool lax;
    bool use_float;
    bool base64_prefix;
    size_t base64_min_bytes;
} options_t;

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

static bool write_string(options_t* options, mpack_writer_t* writer, const char* string, size_t length, bool allow_detection) {

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

static bool write_value(options_t* options, Value& value, mpack_writer_t* writer) {
    switch (value.GetType()) {
        case kNullType:   mpack_write_nil(writer);    break;
        case kTrueType:   mpack_write_true(writer);   break;
        case kFalseType:  mpack_write_false(writer);  break;

        case kNumberType:
            if (value.IsDouble()) {
                if (options->use_float)
                    mpack_write_float(writer, (float)value.GetDouble());
                else
                    mpack_write_double(writer, value.GetDouble());
            } else if (value.IsUint64()) {
                mpack_write_u64(writer, value.GetUint64());
            } else {
                mpack_write_i64(writer, value.GetInt64());
            }
            break;

        case kStringType:
            if (!write_string(options, writer, value.GetString(), value.GetStringLength(), true))
                return false;
            break;

        case kArrayType: {
            mpack_start_array(writer, value.Size());
            Value::ValueIterator it = value.Begin(), end = value.End();
            for (; it != end; ++it) {
                if (!write_value(options, *it, writer))
                    return false;
            }
            mpack_finish_array(writer);
            break;
        }

        case kObjectType: {
            mpack_start_map(writer, value.MemberCount());
            Value::MemberIterator it = value.MemberBegin(), end = value.MemberEnd();
            for (; it != end; ++it) {
                if (!write_string(options, writer, it->name.GetString(), it->name.GetStringLength(), false))
                    return false;
                if (!write_value(options, it->value, writer))
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

static bool output(options_t* options, Document& document) {
    mpack_writer_t writer;
    if (options->out_filename != NULL)
        mpack_writer_init_file(&writer, options->out_filename);
    else
        mpack_writer_init_stdfile(&writer, stdout, true);

    write_value(options, document, &writer);

    mpack_error_t error = mpack_writer_destroy(&writer);
    if (error != mpack_ok) {
        fprintf(stderr, "%s: error writing MessagePack: %s (%i)\n", options->command,
                mpack_error_to_string(error), (int)error);
    }
    return true;
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

        // RapidJSON in-situ requires a null-terminated string, so we need to scan the
        // data to make sure it has no null bytes. They are not legal JSON anyway.
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
    Document document;
    if (options->lax)
        document.ParseInsitu<kParseFullPrecisionFlag | kParseCommentsFlag | kParseTrailingCommasFlag>(data);
    else
        document.ParseInsitu<kParseFullPrecisionFlag>(data);

    if (document.HasParseError()) {
        fprintf(stderr, "%s: error parsing JSON at offset %i:\n    %s\n", options->command,
                (int)document.GetErrorOffset(), GetParseError_En(document.GetParseError()));
        free(data);
        return false;
    }

    bool error = output(options, document);
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
    fprintf(stderr, "%s version %s -- %s\n", command, VERSION, "https://github.com/ludocode/msgpack2json");
    fprintf(stderr, "RapidJSON version %s -- %s\n", RAPIDJSON_VERSION_STRING, "http://rapidjson.org/");
    fprintf(stderr, "MPack version %s -- %s\n", MPACK_VERSION_STRING, "https://github.com/ludocode/mpack");
    fprintf(stderr, "libb64 version %s -- %s\n", LIBB64_VERSION, "http://libb64.sourceforge.net/");
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
