#!/bin/bash
# The arguments are the command to run. The command is expected
# to fail with error code 1. This script exits with 0 if the
# command exits with 1, and exits with 1 otherwise. (Valgrind
# uses 2 as an error code which causes a failure result from
# this script.)
"$@"
RESULT=$?
echo "Command finished with result $RESULT"
if [ $RESULT -ne 1 ]; then
    exit 1
fi
exit 0
