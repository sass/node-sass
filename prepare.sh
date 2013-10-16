#!/bin/bash
#clone git submodules because the submodules aren't a npm project
set -e

git config -f .gitmodules --get-regexp '^submodule\..*\.path$' |
while read path_key path
do
    url_key=$(echo $path_key | sed 's/\.path/.url/')
    url=$(git config -f .gitmodules --get "$url_key")
    rm -rf $path
    git clone $url $path
done