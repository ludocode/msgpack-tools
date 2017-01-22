msgpack-tools v0.6
------------------

New features:

- Added `msgpack2json` continuous mode (`-c` and `-C`) to convert a stream of top-level objects (#4)

Bug fixes:

- Fixed a bug that could cause a newline to be printed incorrectly in pretty-printing mode

msgpack-tools v0.5
------------------

Changes:

- Removed top-level array/map requirement (#3)
- Moved dependency downloading and man page generation into CMake build process (#2)
- Added support for build .deb package (#1)
- Fixed incorrect buffer sizes

msgpack-tools v0.4
------------------

Changes:

- Fixed issues with stream parsing (e.g. piping to/from netcat)
- Fixed man page installation directory (`prefix/share/man`)

msgpack-tools v0.3.1
--------------------

Changes:

- Fixed build errors on old compilers that default to C89

msgpack-tools v0.3
------------------

Changes:

- Switched JSON parsing library to RapidJSON
- Updated [`configure`](https://github.com/nemequ/configure-cmake) script to support `LDFLAGS` and non-bash shells

msgpack-tools v0.2
------------------

Changes:

- Renamed project to **msgpack-tools**
- Added `./configure` CMake wrapper from [nemequ/configure-cmake](https://github.com/nemequ/configure-cmake)
- Updated MPack and YAJL versions

msgpack2json v0.1
-----------------

Initial release
