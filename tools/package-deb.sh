#!/bin/bash

if ! [ -e /etc/debian_version ]; then
    echo "This must be run on Debian (or a Debian-derived distribution.)"
fi

./configure
make
make test
make package
