#!/bin/bash -e

TAG=`git tag -l --points-at HEAD`
PREFIX=$(basename "$(pwd -P)")

if [ ! -z $TAG ]; then
    PREFIX=$(basename "$(pwd -P)").$TAG
fi

{
  git ls-files
  git submodule foreach --recursive --quiet \
                'git ls-files --with-tree="$sha1" | sed "s#^#$path/#"'
} | sed "s#^#$PREFIX/#" | xargs tar -c -C.. -f "$PREFIX.tar.bz2" --
