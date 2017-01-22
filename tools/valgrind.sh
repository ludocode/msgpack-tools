#!/bin/bash

# If we have no valgrind, we just run without it.
if ! which valgrind &> /dev/null; then
    "$@"
    exit $?
fi

# The arguments are the command to run under valgrind.
VALGRIND_COMMAND="valgrind --leak-check=full --error-exitcode=2"

# travis version of valgrind is too old, doesn't support leak kinds
if [ "x$TRAVIS" == "x" ]; then
    VALGRIND_COMMAND="$VALGRIND_COMMAND --show-leak-kinds=all --errors-for-leak-kinds=all"
    VALGRIND_COMMAND="$VALGRIND_COMMAND --suppressions=`dirname $0`/valgrind-suppressions"
fi

$VALGRIND_COMMAND "$@"
