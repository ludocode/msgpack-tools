
## Introduction

**msgpack-tools** contains simple command-line utilities for converting from [MessagePack](http://msgpack.org/) to [JSON](http://json.org/) and vice-versa. They support options for lax parsing, lossy conversions, pretty-printing, and base64 encoding.

- `msgpack2json` -- Convert MessagePack to JSON
- `json2msgpack` -- Convert JSON to MessagePack

They can be used for dumping MessagePack from a file or web API to a human-readable format, or for converting hand-written or generated JSON to MessagePack. The lax parsing mode supports comments and trailing commas in JSON, making it possible to hand-write your app or game data in JSON and convert it at build-time to MessagePack.

## Build Status

| [Travis-CI](https://travis-ci.org/) |
| :-------: |
| [![Build Status](https://travis-ci.org/ludocode/msgpack-tools.svg?branch=master)](https://travis-ci.org/ludocode/msgpack-tools/branches) |

## Examples

To view a MessagePack file in a human-readable format for debugging purposes:

```bash
msgpack2json -di file.mp
```

To convert a hand-written JSON file to a MessagePack file, ignoring comments and trailing commas, and allowing embedded base64 with a `base64:` prefix:

```bash
json2msgpack -bli file.json -o file.mp
```

To fetch MessagePack from a web API and view it in a human-readable format:

```bash
curl 'http://example/api/url' | msgpack2json -d
```

To view the MessagePack-equivalent encoding of a JSON string:

```bash
$ echo '{"compact": true, "schema": 0}' | json2msgpack | hexdump -C
00000000  82 a7 63 6f 6d 70 61 63  74 c3 a6 73 63 68 65 6d  |..compact..schem|
00000010  61 00                                             |a.|
00000012
```

To test a [MessagePack-RPC](https://github.com/msgpack-rpc/msgpack-rpc) server via netcat:

```bash
$ echo '[0,0,"sum",[1,2]]' | json2msgpack | nc -q1 localhost 18800 | msgpack2json -d
[
    1,
    0,
    null,
    3
]
```

## Installation

### Arch Linux

msgpack-tools is available in the [AUR](https://aur.archlinux.org/packages/msgpack-tools/). It can be installed with `makepkg` or with any AUR helper:

    yaourt -S msgpack-tools

### OS X

A Homebrew formula for msgpack-tools is [included](https://github.com/ludocode/msgpack-tools/blob/master/tools/msgpack-tools.rb) in the repository. This requires [Homebrew](http://brew.sh/).

    brew install https://raw.githubusercontent.com/ludocode/msgpack-tools/master/tools/msgpack-tools.rb

### Other

For other platforms, msgpack-tools currently must be built from source. The latest version of the msgpack-tools source archive should be downloaded from the releases page. This includes the library dependencies and has pre-generated man pages:

[https://github.com/ludocode/msgpack-tools/releases](https://github.com/ludocode/msgpack-tools/releases)

msgpack-tools uses CMake. A `configure` wrapper is provided that calls CMake, so on Linux and Mac OS X, you can simply run the usual:

    ./configure && make && sudo make install

If you are building from a clone of the repository, you will need to fetch the dependencies and generate the man pages. These are done with the scripts under `tools/fetch.sh` and `tools/man.sh`. [md2man](https://github.com/sunaku/md2man) is required to generate the man pages.

## Differences between MessagePack and JSON

MessagePack is intended to be very close to JSON in supported features, so they can usually be transparently converted from one to the other. There are some differences, however, which can complicate conversions.

These are the differences in what objects are representable in each format:

- JSON keys must be strings. MessagePack keys can be any type, including maps and arrays.

- JSON supports "bignums", i.e. integers of any size. MessagePack integers must fit within a 64-bit signed or unsigned integer.

- JSON real numbers are specified in decimal scientific notation and can have arbitrary precision. MessagePack real numbers are in IEEE 754 standard 32-bit or 64-bit binary.

- MessagePack supports binary and extension type objects. JSON does not support binary data. Binary data is often encoded into a base64 string to be embedded into a JSON document.

- The top-level object in a JSON document must be either an array or object (map). The top-level object in MessagePack can be any type.

- A JSON document can be encoded in UTF-8, UTF-16 or UTF-32, and the entire document must be in the same encoding. MessagePack strings are required to be UTF-8, although this is not enforced by many encoding/decoding libraries.

By default, `msgpack2json` and `json2msgpack` convert in strict mode. If an object in the source format is not representable in the destination format, the converter aborts with an error. A lax mode is available which performs a "lossy" conversion, and base64 conversion modes are available to support binary data in JSON.
