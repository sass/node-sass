#!/bin/sh

git clone --depth=1 git@github.com:sass/libsass.git ./src/libsass
cd ./src/libsass
git checkout $LIBSASS_GIT_VERSION
