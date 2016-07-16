#!/bin/bash
# The first argument is the expected test result, and the
# remaining arguments are the command to run. The output
# is compared to the expected test result.
EXPECTED="$1"
shift
"$@" > "test-out.tmp"

RESULT=$?
echo "Command finished with result $RESULT"
if [ $RESULT -ne 0 ]; then
    exit 1
fi

diff -u "$EXPECTED" "test-out.tmp"
