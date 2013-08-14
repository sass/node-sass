#!/bin/bash

set -e

if [ "x$AUTOTOOLS" == "xyes" ]; then
	autoreconf
	./configure
	make
fi

make test

