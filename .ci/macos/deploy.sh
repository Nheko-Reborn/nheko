#!/usr/bin/env bash

set -e

# Add Qt binaries to path
PATH=/usr/local/opt/qt/bin/:${PATH}

cp -fp ./build/nheko dist/MacOS/Nheko.app/Contents/MacOS

sudo macdeployqt dist/MacOS/Nheko.app -dmg
user=$(id -nu)
sudo chown ${user} dist/MacOS/Nheko.dmg
mv dist/MacOS/Nheko.dmg .
