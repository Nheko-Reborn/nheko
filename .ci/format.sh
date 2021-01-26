#!/usr/bin/env sh

# Runs the Clang Formatter
# Return codes:
#  - 1 there are files to be formatted
#  - 0 everything looks fine

set -eu

FILES=$(find src -type f -type f \( -iname "*.cpp" -o -iname "*.h" \))

for f in $FILES
do
    clang-format -i "$f"
done;

QMLFORMAT_PATH=$(which qmlformat)
if [ ! -z "$QMLFORMAT_PATH" ]; then
    QML_FILES=$(find resources -type f -iname "*.qml")

    for f in $QML_FILES
    do
        qmlformat -i "$f"
    done;
else
    echo "qmlformat not found; skipping qml formatting"
fi

git diff --exit-code
