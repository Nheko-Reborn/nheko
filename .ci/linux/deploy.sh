#!/usr/bin/env bash

set -e

mkdir -p appdir 
cp build/nheko appdir/
cp resources/nheko.desktop appdir/
cp resources/nheko*.png appdir/

wget -c "https://github.com/probonopd/linuxdeployqt/releases/download/continuous/linuxdeployqt-continuous-x86_64.AppImage"
chmod a+x linuxdeployqt*.AppImage

unset QTDIR
unset QT_PLUGIN_PATH 
unset LD_LIBRARY_PATH

./linuxdeployqt*.AppImage ./appdir/*.desktop -bundle-non-qt-libs
./linuxdeployqt*.AppImage ./appdir/*.desktop -appimage

chmod +x nheko-x86_64.AppImage
