#!/usr/bin/env bash

set -e

# Add Qt binaries to path
PATH=/usr/local/opt/qt/bin/:${PATH}

sudo macdeployqt build/nheko.app -dmg
user=$(id -nu)
sudo chown ${user} build/nheko.dmg
mv build/nheko.dmg .
