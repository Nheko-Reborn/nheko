#!/usr/bin/env sh

set -eux

# unused
#TAG=$(git tag -l --points-at HEAD)

# Add Qt binaries to path
PATH=/usr/local/opt/qt/bin/:${PATH}

( cd build
  # macdeployqt does not copy symlinks over.
  # this specifically addresses icu4c issues but nothing else.
  ICU_LIB="$(brew --prefix icu4c)/lib"
  export ICU_LIB
  mkdir -p nheko.app/Contents/Frameworks
  find "${ICU_LIB}" -type l -name "*.dylib" -exec cp -a -n {} nheko.app/Contents/Frameworks/ \; || true

  sudo macdeployqt nheko.app -dmg -always-overwrite -qmldir=../resources/qml/

  user=$(id -nu)
  sudo chown "${user}" nheko.dmg
  mv nheko.dmg ..
)

dmgbuild -s ./.ci/macos/settings.json "Nheko" nheko.dmg

if [ -n "$VERSION" ]; then
    mv nheko.dmg "nheko-${VERSION}.dmg"
    mkdir artifacts
    cp "nheko-${VERSION}.dmg" artifacts/
fi
