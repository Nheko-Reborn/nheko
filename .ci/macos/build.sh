#!/usr/bin/env sh

set -u

# unused
#TAG=$(git tag -l --points-at HEAD)

# Add Qt binaries to path
PATH="$(brew --prefix qt5):${PATH}"
export PATH

CMAKE_PREFIX_PATH="$(brew --prefix qt5)"
export CMAKE_PREFIX_PATH

cmake -GNinja -S. -Bbuild \
      -DCMAKE_BUILD_TYPE=RelWithDebInfo \
      -DCMAKE_INSTALL_PREFIX=.deps/usr \
      -DHUNTER_ROOT="../.hunter" \
      -DHUNTER_ENABLED=ON -DBUILD_SHARED_LIBS=OFF \
      -DCMAKE_BUILD_TYPE=RelWithDebInfo -DHUNTER_CONFIGURATION_TYPES=RelWithDebInfo \
      -DUSE_BUNDLED_OPENSSL=ON \
      -DCI_BUILD=ON
cmake --build build
( cd build || exit 
  git clone https://github.com/Nheko-Reborn/qt-jdenticon.git
  ( cd qt-jdenticon || exit 
    qmake
    make -j 4
    cp libqtjdenticon.dylib ../nheko.app/Contents/MacOS
  )
  "$(brew --prefix qt5)/bin/macdeployqt" nheko.app -always-overwrite -qmldir=../resources/qml/
)
