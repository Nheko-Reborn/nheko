#!/bin/bash

#
# Manually generate icon set for MacOS.
#

INPUT=$1
OUTPUT=nheko

mkdir ${OUTPUT}.iconset

sips -z 16 16     ${INPUT} --out ${OUTPUT}.iconset/icon_16x16.png
sips -z 32 32     ${INPUT} --out ${OUTPUT}.iconset/icon_16x16@2x.png
sips -z 32 32     ${INPUT} --out ${OUTPUT}.iconset/icon_32x32.png
sips -z 64 64     ${INPUT} --out ${OUTPUT}.iconset/icon_32x32@2x.png
sips -z 128 128   ${INPUT} --out ${OUTPUT}.iconset/icon_128x128.png
sips -z 256 256   ${INPUT} --out ${OUTPUT}.iconset/icon_128x128@2x.png
sips -z 256 256   ${INPUT} --out ${OUTPUT}.iconset/icon_256x256.png
sips -z 512 512   ${INPUT} --out ${OUTPUT}.iconset/icon_256x256@2x.png
sips -z 512 512   ${INPUT} --out ${OUTPUT}.iconset/icon_512x512.png

cp ${INPUT} ${OUTPUT}.iconset/icon_512x512@2x.png

iconutil -c icns ${OUTPUT}.iconset

rm -R ${OUTPUT}.iconset
