json2msgpack 1
=======================================

NAME
----

json2msgpack - convert JSON to MessagePack

SYNOPSIS
--------

`json2msgpack` [`-lfb`] [`-B` *min-bytes*] [`-i` *in-file*] [`-o` *out-file*]

DESCRIPTION
-----------

`json2msgpack` converts a JSON object to MessagePack. It has options for lax parsing and base64 conversions.

The JSON input is expected to be valid UTF-8. Strings in the resulting MessagePack will be encoded in UTF-8.

OPTIONS
-------

`-i` *in-file*
  Read from the given file instead of standard input.

`-o` *out-file*
  Write to the given file instead of standard output.

`-l`
  Lax parsing mode. The JSON parser will be more forgiving to invalid JSON. This means:

- Comments are allowed and ignored
- Trailing commas in maps and arrays are allowed and ignored

`-f`
  Convert real numbers to floats instead of doubles.

`-b`
  Parse strings with a "`base64:`" prefix as base64 and convert them to bin objects.

  Base64 strings with an "`ext:`*#*`:base64:`" prefix will be converted to ext objects.

  If base64 parsing fails for a string prefixed with `base64:` or `ext:`, the conversion will abort with error.

`-B` *min-bytes*
  Try to parse any non-key string *min-bytes* bytes or longer as base64, and if successful, convert it to a bin object. If base64 parsing fails, the string will be left as-is and written as a UTF-8 string.

  Be careful not to set this too low, since any string without spaces (such UNIX paths) might be coincidentally parseable as base64. Carriage returns and line feeds are allowed and ignored, but spaces are not allowed in order to reduce the risk incorrect detection.

  This option does not imply `-b`; you may want to specify both.

  *min-bytes* must be larger than zero.

`-h`
  Print usage.

NOTES
-----

`json2msgpack` will preserve the ordering of key-value pairs in objects/maps, and does not check that keys are unique.

EXAMPLES
--------

To convert a hand-written JSON file to a MessagePack file, ignoring comments and trailing commas, and allowing embedded base64 with a "`base64:`" prefix:

> `json2msgpack -bli` *file.json* `-o` *file.mp*

BUGS
----

Big integers outside of the range \[INT64\_MIN, UINT64\_MAX\] are converted to real numbers instead of causing a parse error. They will be output in MessagePack as doubles, or floats if `-f` was specified, with associated loss of precision.

AUTHOR
------

Nicholas Fraser <https://github.com/ludocode>

VERSION
-------

$version

SEE ALSO
--------

msgpack2json(1)

[msgpack-tools](https://github.com/ludocode/msgpack-tools)

[JSON](http://json.org/)

[MessagePack](http://msgpack.org/)

[RapidJSON](http://rapidjson.org/)

[MPack](https://github.com/ludocode/mpack)

[libb64](http://libb64.sourceforge.net/)
