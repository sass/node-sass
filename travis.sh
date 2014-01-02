#!/bin/bash

set -e

if [ "x$AUTOTOOLS" == "xyes" ]; then
	autoreconf -i
	./configure --enable-tests \
		    --with-sassc-dir=$SASS_SASSC_PATH \
		    --with-sass-spec-dir=$SASS_SPEC_PATH \
		    --prefix=$TRAVIS_BUILD_DIR
	make install
	cp $TRAVIS_BUILD_DIR/lib/libsass.a $TRAVIS_BUILD_DIR
fi

make LOG_FLAGS=--skip VERBOSE=1 test_build
