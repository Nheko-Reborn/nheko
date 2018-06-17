#!/usr/bin/env bash

set -ex

TAG=`git tag -l --points-at HEAD`

# Add Qt binaries to path
PATH=/usr/local/opt/qt/bin/:${PATH}

pushd build
sudo macdeployqt nheko.app -dmg
user=$(id -nu)
sudo chown ${user} nheko.dmg
mv nheko.dmg ..
popd

dmgbuild -s ./.ci/macos/settings.json "Nheko" nheko.dmg

if [ ! -z $TRAVIS_TAG ]; then
    mv nheko.dmg nheko-${TRAVIS_TAG}.dmg
fi
