#!/usr/bin/env sh

set -ue

# unused
#TAG=$(git tag -l --points-at HEAD)

# Add Qt binaries to path
PATH="$(brew --prefix qt6)/bin/:${PATH}"
export PATH

CMAKE_PREFIX_PATH="$(brew --prefix qt6)"
export CMAKE_PREFIX_PATH

cmake -GNinja -S. -Bbuild \
      -DCMAKE_BUILD_TYPE=RelWithDebInfo \
      -DCMAKE_INSTALL_PREFIX="nheko.app" \
      -DHUNTER_ROOT="../.hunter" \
      -DHUNTER_ENABLED=ON -DBUILD_SHARED_LIBS=OFF \
      -DCMAKE_BUILD_TYPE=RelWithDebInfo -DHUNTER_CONFIGURATION_TYPES=RelWithDebInfo \
      -DUSE_BUNDLED_OPENSSL=ON \
      -DCI_BUILD=ON
cmake --build build
( cd build
  git clone https://github.com/Nheko-Reborn/qt-jdenticon.git
  ( cd qt-jdenticon
    qmake
    make -j 4
    cp libqtjdenticon.dylib ../nheko.app/Contents/MacOS
  )
  # "$(brew --prefix qt6)/bin/macdeployqt" nheko.app -always-overwrite -qmldir=../resources/qml/
  # # workaround for https://bugreports.qt.io/browse/QTBUG-100686
  # cp "$(brew --prefix brotli)/lib/libbrotlicommon.1.dylib" nheko.app/Contents/Frameworks/libbrotlicommon.1.dylib
  cmake --install .
)
