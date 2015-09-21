#!/bin/bash
# The arguments are the command to run. The command is expected
# to fail; this script exits with 1 if the command exits with 0,
# and exits with 1 otherwise.
"$@"
if [ $? -eq 0 ]; then
    exit 1
fi
exit 0
