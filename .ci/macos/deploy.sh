#!/usr/bin/env bash

set -e

make app

sudo macdeployqt dist/MacOS/Nheko.app -dmg
user=$(id -nu)
sudo chown ${user} dist/MacOS/Nheko.dmg
mv dist/MacOS/Nheko.dmg .

export ARTIFACT=Nheko.dmg
