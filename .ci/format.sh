#!/usr/bin/env sh

# Runs the Clang Formatter
# Return codes:
#  - 1 there are files to be formatted
#  - 0 everything looks fine

set -eu

FILES=$(find src -type f \( -iname "*.cpp" -o -iname "*.h" \))
QML_FILES=$(find resources/qml -type f \( -iname "*.qml"  \))

for f in $FILES
do
    clang-format -i "$f"
done;

git diff --exit-code

if command -v /usr/lib64/qt6/bin/qmllint &> /dev/null; then
    /usr/lib64/qt6/bin/qmllint $QML_FILES
elif command -v /usr/lib/qt6/bin/qmllint &> /dev/null; then
    /usr/lib/qt6/bin/qmllint $QML_FILES
else
    echo "No qmllint found, skipping check!"
fi
