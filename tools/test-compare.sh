#!/bin/bash
# The first argument is the expected test result, and the
# remaining arguments are the command to run. The output
# is compared to the expected test result.
EXPECTED="$1"
shift
"$@" > "test-out.tmp"
diff "$EXPECTED" "test-out.tmp"
