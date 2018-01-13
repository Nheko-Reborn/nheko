#!/usr/bin/env bash

set -ex

TAG=`git tag -l --points-at HEAD`

# Add Qt binaries to path
PATH=/usr/local/opt/qt/bin/:${PATH}

sudo macdeployqt build/nheko.app -dmg
user=$(id -nu)
sudo chown ${user} build/nheko.dmg
mv build/nheko.dmg .
