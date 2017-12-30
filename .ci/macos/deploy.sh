#!/usr/bin/env bash

set -e

TAG=`git tag -l --points-at HEAD`

if [ -z "$TAG" ]; then
    exit 0
fi

# Add Qt binaries to path
PATH=/usr/local/opt/qt/bin/:${PATH}

sudo macdeployqt build/nheko.app -dmg
user=$(id -nu)
sudo chown ${user} build/nheko.dmg
mv build/nheko.dmg .
