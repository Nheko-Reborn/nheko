#!/usr/bin/env sh

# Runs the Clang Formatter
# Return codes:
#  - 1 there are files to be formatted
#  - 0 everything looks fine

set -eu

FILES=$(find src -type f \( -iname "*.cpp" -o -iname "*.h" \))

for f in $FILES
do
    clang-format -i "$f"
done;

git diff --exit-code
