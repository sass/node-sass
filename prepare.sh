#!/bin/bash
# Exists to fully update the git repo that you are sitting in...
#sudo git pull && git submodule init && git submodule update && git submodule status
#sudo git submodule foreach
set -e

git config -f .gitmodules --get-regexp '^submodule\..*\.path$' |
    while read path_key path
    do
        url_key=$(echo $path_key | sed 's/\.path/.url/')
        url=$(git config -f .gitmodules --get "$url_key")
        git clone $url $path
    done