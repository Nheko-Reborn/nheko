#!/usr/bin/env bash

# Runs the Clang Formatter
# Return codes:
#  - 1 there are files to be formatted
#  - 0 everything looks fine

set -o errexit
set -o pipefail
set -o nounset

FILES=`find src -type f -type f \( -iname "*.cpp" -o -iname "*.h" \)`

clang-format -i $FILES && git diff --exit-code
