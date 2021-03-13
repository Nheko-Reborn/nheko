#!/usr/bin/env sh

# Runs the license update
# Return codes:
#  - 1 there are files to be formatted
#  - 0 everything looks fine

set -eu

FILES=$(find src resources/qml -type f \( -iname "*.cpp" -o -iname "*.h" -o -iname "*.qml" \))

reuse addheader --copyright="Nheko Contributors" --license="GPL-3.0-or-later" $FILES

git diff --exit-code
