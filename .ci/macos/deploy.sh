#!/usr/bin/env bash

set -ex

TAG=`git tag -l --points-at HEAD`

# Add Qt binaries to path
PATH=/usr/local/opt/qt/bin/:${PATH}

pushd build
sudo macdeployqt nheko.app -dmg

# macdeployqt does not copy symlinks over.
# this specifically addresses icu4c issues but nothing else.
export ICU_LIB="$(brew --prefix icu4c)/lib"
find ${ICU_LIB} -type l -name "*.dylib" -exec cp {} nheko.app/Contents/Frameworks/ \; || true

user=$(id -nu)
sudo chown ${user} nheko.dmg
mv nheko.dmg ..
popd

dmgbuild -s ./.ci/macos/settings.json "Nheko" nheko.dmg

if [ ! -z $VERSION ]; then
    mv nheko.dmg nheko-${VERSION}.dmg
fi
