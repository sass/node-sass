#!/bin/bash
# Exists to fully update the git repo that you are sitting in...
set -e

if ! [ $(PWD) -ef $(git rev-parse --show-toplevel) ];
    then
        git config -f .gitmodules --get-regexp '^submodule\..*\.path$' |
        while read path_key path
        do
            url_key=$(echo $path_key | sed 's/\.path/.url/')
            url=$(git config -f .gitmodules --get "$url_key")
            git clone $url $path
        done
fi