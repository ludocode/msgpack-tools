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

// The code that checks that object keys are strings just uses
// RAPIDJSON_ASSERT(). We define it to nothing to disable this
// so we can write non-string keys in debug mode. (We actually
// never call Key(), and always write whatever type we want.)
#define RAPIDJSON_ASSERT(x) ((void)(x))

#include "common.h"
#include "rapidjson/filewritestream.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/writer.h"

using namespace rapidjson;

typedef enum continuous_mode_t {
    continuous_off = 0,
    continuous_undelimited,
    continuous_commas,
} continuous_mode_t;

typedef struct options_t {
    const char* command;
    const char* out_filename;
    const char* in_filename;
    continuous_mode_t continuous_mode;
    bool debug;
    bool pretty;
    bool base64;
    bool base64_prefix;
} options_t;

// Reads MessagePack string bytes and outputs a JSON string
template <class WriterType>
static bool string(mpack_reader_t* reader, WriterType& writer, options_t* options, uint32_t len) {
    if (mpack_should_read_bytes_inplace(reader, len)) {
        const char* str = mpack_read_bytes_inplace(reader, len);
        if (mpack_reader_error(reader) != mpack_ok) {
            fprintf(stderr, "%s: error reading string bytes\n", options->command);
            return false;
        }
        bool ok = writer.String(str, len);
        mpack_done_str(reader);
        return ok;
    }

    char* str = (char*)malloc(len);
    mpack_read_bytes(reader, str, len);
    if (mpack_reader_error(reader) != mpack_ok) {
        fprintf(stderr, "%s: error reading string bytes\n", options->command);
        free(str);
        return false;
    }
    mpack_done_str(reader);

    bool ok = writer.String(str, len);
    free(str);
    return ok;
}

static const char* ext_str = "ext:";
static const char* b64_str = "base64:";

static uint32_t base64_len(uint32_t len) {
    return ((len + 3) * 4) / 3; // TODO check this
}

// Converts MessagePack bin/ext bytes to JSON base64 string
template <class WriterType>
static bool base64(mpack_reader_t* reader, WriterType& writer, options_t* options, uint32_t len, char* output, char* p, bool prefix) {
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

    return writer.String(output, p - output);
}

// Reads MessagePack bin bytes and outputs a JSON base64 string
template <class WriterType>
static bool base64_bin(mpack_reader_t* reader, WriterType& writer, options_t* options, uint32_t len, bool prefix) {
    uint32_t new_len = base64_len(len) + (prefix ? strlen(b64_str) : 0);
    char* output = (char*)malloc(new_len);

    bool ret = base64(reader, writer, options, len, output, output, prefix);

    mpack_done_bin(reader);
    free(output);
    return ret;
}

// Reads MessagePack ext bytes and outputs a JSON base64 string
template <class WriterType>
static bool base64_ext(mpack_reader_t* reader, WriterType& writer, options_t* options, int8_t exttype, uint32_t len) {
    uint32_t new_len = base64_len(len) + strlen(ext_str) + 5 + strlen(b64_str);
    char* output = (char*)malloc(new_len);

    char* p = output;
    strcpy(p, ext_str);
    p += strlen(ext_str);
    sprintf(p, "%i", exttype);
    p += strlen(p);
    *p++ = ':';

    bool ret = base64(reader, writer, options, len, output, p, true);

    mpack_done_ext(reader);
    free(output);
    return ret;
}

template <class WriterType>
static bool element(mpack_reader_t* reader, WriterType& writer, options_t* options) {
    const mpack_tag_t tag = mpack_read_tag(reader);
    if (mpack_reader_error(reader) != mpack_ok)
        return false;

    switch (tag.type) {
        case mpack_type_bool:   return writer.Bool(tag.v.b);
        case mpack_type_nil:    return writer.Null();
        case mpack_type_int:    return writer.Int64(tag.v.i);
        case mpack_type_uint:   return writer.Uint64(tag.v.u);
        case mpack_type_float:  return writer.Double((double)tag.v.f);
        case mpack_type_double: return writer.Double(tag.v.d);

        case mpack_type_str:
            return string(reader, writer, options, tag.v.l);

        case mpack_type_bin:
            if (options->base64) {
                return base64_bin(reader, writer, options, tag.v.l, options->base64_prefix);
            } else if (options->debug) {
                mpack_skip_bytes(reader, tag.v.l);
                mpack_done_bin(reader);
                char buf[64];
                snprintf(buf, sizeof(buf), "<bin of size %u>", tag.v.l);
                return writer.RawValue(buf, strlen(buf), kStringType);
            } else {
                fprintf(stderr, "%s: bin unencodable in JSON. Try debug viewing mode (-d)\n", options->command);
                return false;
            }

        case mpack_type_ext:
            if (options->base64) {
                return base64_ext(reader, writer, options, tag.exttype, tag.v.l);
            } else if (options->debug) {
                mpack_skip_bytes(reader, tag.v.l);
                mpack_done_ext(reader);
                char buf[64];
                snprintf(buf, sizeof(buf), "<ext of type %i size %u>", tag.exttype, tag.v.l);
                return writer.RawValue(buf, strlen(buf), kStringType);
            } else {
                fprintf(stderr, "%s: ext type %i unencodable in JSON. Try debug viewing mode (-d)\n", options->command, tag.exttype);
                return false;
            }

        case mpack_type_array:
            if (!writer.StartArray())
                return false;
            for (size_t i = 0; i < tag.v.l; ++i)
                if (!element(reader, writer, options))
                    return false;
            mpack_done_array(reader);
            return writer.EndArray();

        case mpack_type_map:
            if (!writer.StartObject())
                return false;
            for (size_t i = 0; i < tag.v.l; ++i) {

                if (options->debug) {
                    element(reader, writer, options);
                } else {
                    uint32_t len = mpack_expect_str(reader);
                    if (mpack_reader_error(reader) != mpack_ok) {
                        fprintf(stderr, "%s: map key is not a string. Try debug viewing mode (-d)\n", options->command);
                        return false;
                    }
                    if (!string(reader, writer, options, len))
                        return false;
                }

                if (!element(reader, writer, options))
                    return false;
            }
            mpack_done_map(reader);
            return writer.EndObject();
    }

    return true;
}

template <class WriterType>
static bool convert_all_elements(mpack_reader_t* reader, WriterType& writer, FileWriteStream& stream, options_t* options) {
    do {
        // Convert an element
        if (!element(reader, writer, options))
            return false;

        // If we're not in continuous mode, we're done
        if (options->continuous_mode == continuous_off)
            return true;

        // See if there's more. An EOF error at this point is OK since we're
        // between elements. EOF at any other time fails conversion.
        mpack_peek_tag(reader);
        if (mpack_reader_error(reader) == mpack_error_eof)
            return true;

        // Output a delimiter
        if (options->continuous_mode == continuous_commas)
            stream.Put(',');
        if (options->pretty)
            stream.Put('\n');
    } while (true);
}

static bool convert(options_t* options) {

    // Open input file with MPack
    mpack_reader_t reader;
    if (options->in_filename)
        mpack_reader_init_file(&reader, options->in_filename);
    else
        mpack_reader_init_stdfile(&reader, stdin, true);

    // Open output file for RapidJSON
    static FILE* out_file = NULL;
    if (options->out_filename) {
        out_file = fopen(options->out_filename, "wb");
        if (out_file == NULL) {
            fprintf(stderr, "%s: could not open \"%s\" for writing.\n", options->command, options->out_filename);
            mpack_reader_destroy(&reader);
            return false;
        }
    } else {
        out_file = stdout;
    }

    bool ret;
    char* buffer = (char*)malloc(BUFFER_SIZE);
    {
        FileWriteStream stream(out_file, buffer, BUFFER_SIZE);

        if (options->pretty) {
            {
                PrettyWriter<FileWriteStream> writer(stream);
                ret = convert_all_elements(&reader, writer, stream, options);
            }

            // RapidJSON's PrettyWriter does not add a final
            // newline at the end of the JSON
            stream.Put('\n');
            stream.Flush();

        } else {
            Writer<FileWriteStream> writer(stream);
            ret = convert_all_elements(&reader, writer, stream, options);
        }
    }

    free(buffer);
    mpack_error_t error = mpack_reader_destroy(&reader);
    fclose(out_file);

    if (!ret)
        fprintf(stderr, "%s: parse error: %s (%i)\n", options->command,
                mpack_error_to_string(error), (int)error);
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
    fprintf(stderr, "    -c  Continuous mode, no delimiter");
    fprintf(stderr, "    -C  Continuous mode, comma delimited");
    fprintf(stderr, "    -h  Print this help\n");
    fprintf(stderr, "    -v  Print version information\n");
    fprintf(stderr, "For viewing MessagePack, you probably want -d or -di <filename>.\n");
}

static void version(const char* command) {
    fprintf(stderr, "%s version %s\n", command, VERSION);
    fprintf(stderr, "MPack version %s -- %s\n", MPACK_VERSION_STRING, "https://github.com/ludocode/mpack");
    fprintf(stderr, "RapidJSON version %s -- %s\n", RAPIDJSON_VERSION_STRING, "http://rapidjson.org/");
    fprintf(stderr, "libb64 version %s -- %s\n", LIBB64_VERSION, "http://libb64.sourceforge.net/");
}

int main(int argc, char** argv) {
    options_t options;
    memset(&options, 0, sizeof(options));
    options.command = argv[0];

    opterr = 0;
    int opt;
    while ((opt = getopt(argc, argv, "i:o:dpbBcChv?")) != -1) {
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
            case 'c':
                if (options.continuous_mode == continuous_commas) {
                    fprintf(stderr, "You cannot specify both -c and -C.\n");
                    usage(options.command);
                    return EXIT_FAILURE;
                }
                options.continuous_mode = continuous_undelimited;
                break;
            case 'C':
                if (options.continuous_mode == continuous_undelimited) {
                    fprintf(stderr, "You cannot specify both -c and -C.\n");
                    usage(options.command);
                    return EXIT_FAILURE;
                }
                options.continuous_mode = continuous_commas;
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
