#!/usr/bin/env bash

# Runs the Clang Formatter
# Return codes:
#  - 1 there are files to be formatted
#  - 0 everything looks fine

set -o errexit
set -o pipefail
set -o nounset

FILES=`find include src tests -type f -type f \( -iname "*.cc" -o -iname "*.h" \)`

clang-format -i $FILES && git diff --exit-code
