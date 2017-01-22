msgpack2json 1
=======================================

NAME
----

msgpack2json - convert MessagePack to JSON

SYNOPSIS
--------

`msgpack2json` [`-lpbB`] [`-i` *in-file*] [`-o` *out-file*]

DESCRIPTION
-----------

`msgpack2json` converts a MessagePack object to JSON. It has options for lax conversions, pretty-printing, and base64 conversions.

MessagePack strings are expected to be valid UTF-8. The resulting JSON will be output in UTF-8.

OPTIONS
-------

`-i` *in-file*
  Read from the given file instead of standard input.

`-o` *out-file*
  Write to the given file instead of standard output.

`-d`
  Debugging conversion mode for viewing MessagePack in a human-readable format. MessagePack objects that cannot be expressed in JSON will be converted to pseudo-JSON instead of aborting with error. This means:

- Map keys do not have to be strings
- The root node of the input does not have to be a map or array
- Bin objects will be printed as `<bin of size` *###*`>` (unless in a base64 mode)
- Ext objects will be printed as `<ext of type` *###* `size` *###*`>` (unless in a base64 mode)

  The resulting output may not be parseable as JSON. This implies `-p`.

`-p`
  Pretty-print JSON output. UNIX-style newlines and four space indentation will be used.

`-b`
  Convert bin objects to base64 strings with a "`base64:`" prefix.

  Ext objects will be converted to base64 strings with an "`ext:`*#*`:base64:`" prefix.

`-B`
  Convert bin objects to base64 strings with no prefix.

  Ext objects will be converted to base64 strings with an "`ext:`*#*`:base64:`" prefix.

`-c`
  Continuous mode. The input can contain any number of top-level objects instead of just one. Each object is output as JSON with no delimiter (other than a newline in pretty-printing mode.)

`-C`
  Continuous mode, delimited by commas. The input can contain any number of top-level objects instead of just one. Each object is output as JSON, delimited by commas (and a newline in pretty-printing mode.) This can be used to construct a JSON array containing all input objects by wrapping it in square brackets.

`-h`
  Print usage.

NOTES
-----

If both `-b` and `-B` options are given, only the last one specified will take effect.

`msgpack2json` will preserve the ordering of key-value pairs in objects/maps, and does not check that keys are unique.

`msgpack2json` is under construction.

EXAMPLES
--------

To view a MessagePack file in a human-readable format:

> `msgpack2json -di` *file.mp*

To convert a MessagePack file to a JSON file using base64 for embedded binary data:

> `msgpack2json -Bi` *file.mp* `-o` *file.json*

To fetch MessagePack from a web API and view it in a human-readable format:

> `curl` *ht**tp://example/url* `| msgpack2json -d`

BUGS
----

`msgpack2json` currently truncates strings that contain NUL bytes.

AUTHOR
------

Nicholas Fraser <https://github.com/ludocode>

VERSION
-------

$version

SEE ALSO
--------

json2msgpack(1)

[msgpack-tools](https://github.com/ludocode/msgpack-tools)

[MessagePack](http://msgpack.org/)

[JSON](http://json.org/)

[MPack](https://github.com/ludocode/mpack)

[RapidJSON](http://rapidjson.org/)

[libb64](http://libb64.sourceforge.net/)
