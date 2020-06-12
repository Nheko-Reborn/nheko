#!/bin/sh

set -eu

#
# Manually generate icon set for MacOS.
#

INPUT=$1
OUTPUT=nheko

filename=$(basename -- "$1")
extension="${filename##*.}"

mkdir ${OUTPUT}.iconset

if [ extension = "svg" ]; then
    rsvg-convert -h 16   "${INPUT}" > ${OUTPUT}.iconset/icon_16x16.png
    rsvg-convert -h 32   "${INPUT}" > ${OUTPUT}.iconset/icon_16x16@2x.png
    rsvg-convert -h 32   "${INPUT}" > ${OUTPUT}.iconset/icon_32x32.png
    rsvg-convert -h 64   "${INPUT}" > ${OUTPUT}.iconset/icon_32x32@2x.png
    rsvg-convert -h 128  "${INPUT}" > ${OUTPUT}.iconset/icon_128x128.png
    rsvg-convert -h 256  "${INPUT}" > ${OUTPUT}.iconset/icon_128x128@2x.png
    rsvg-convert -h 256  "${INPUT}" > ${OUTPUT}.iconset/icon_256x256.png
    rsvg-convert -h 512  "${INPUT}" > ${OUTPUT}.iconset/icon_256x256@2x.png
    rsvg-convert -h 512  "${INPUT}" > ${OUTPUT}.iconset/icon_512x512.png 
    rsvg-convert -h 1024 "${INPUT}" > ${OUTPUT}.iconset/icon_512x512@2x.png 
else
    sips -z 16 16     "${INPUT}" --out ${OUTPUT}.iconset/icon_16x16.png
    sips -z 32 32     "${INPUT}" --out ${OUTPUT}.iconset/icon_16x16@2x.png
    sips -z 32 32     "${INPUT}" --out ${OUTPUT}.iconset/icon_32x32.png
    sips -z 64 64     "${INPUT}" --out ${OUTPUT}.iconset/icon_32x32@2x.png
    sips -z 128 128   "${INPUT}" --out ${OUTPUT}.iconset/icon_128x128.png
    sips -z 256 256   "${INPUT}" --out ${OUTPUT}.iconset/icon_128x128@2x.png
    sips -z 256 256   "${INPUT}" --out ${OUTPUT}.iconset/icon_256x256.png
    sips -z 512 512   "${INPUT}" --out ${OUTPUT}.iconset/icon_256x256@2x.png
    sips -z 512 512   "${INPUT}" --out ${OUTPUT}.iconset/icon_512x512.png

    cp "${INPUT}" ${OUTPUT}.iconset/icon_512x512@2x.png
fi

iconutil -c icns ${OUTPUT}.iconset

rm -R ${OUTPUT}.iconset
