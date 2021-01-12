#!/usr/bin/env sh

# Runs the Clang Formatter
# Return codes:
#  - 1 there are files to be formatted
#  - 0 everything looks fine

set -eu

FILES=$(find src -type f -type f \( -iname "*.cpp" -o -iname "*.h" \))
QML_FILES=$(find resources -type f -iname "*.qml")

for f in $FILES
do
    clang-format -i "$f"
done;

for f in $QML_FILES
do
    qmlformat -i $f
done;

git diff --exit-code
