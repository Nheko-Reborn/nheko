#!/bin/sh

set -eu

PREFIX=$(basename "$(pwd -P)")
{
  git ls-files
  git submodule foreach --recursive --quiet \
                'git ls-files --with-tree="$sha1" | sed "s#^#$path/#"'
} | sed "s#^#$PREFIX/#" | xargs tar -c -j -C.. -f "$PREFIX.tar.bz2" --
