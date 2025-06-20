#!/usr/bin/env bash

set -ue

# unused
#TAG=$(git tag -l --points-at HEAD)

# Add Qt binaries to path
QT_BASEPATH=(${HOME}/Qt/6.*/macos/)
PATH="${QT_BASEPATH}/bin/:${PATH}"
pipx ensurepath
. ~/.zshrc
export PATH

CMAKE_PREFIX_PATH="${QT_BASEPATH}/lib/cmake"
export CMAKE_PREFIX_PATH

export CMAKE_BUILD_PARALLEL_LEVEL="$(sysctl -n hw.ncpu)"

export CMAKE_POLICY_VERSION_MINIMUM="3.5"

cmake -GNinja -S. -Bbuild \
      -DCMAKE_BUILD_TYPE=RelWithDebInfo \
      -DCMAKE_INSTALL_PREFIX="nheko.temp" \
      -DHUNTER_ROOT="../.hunter" \
      -DHUNTER_ENABLED=ON -DBUILD_SHARED_LIBS=OFF \
      -DCMAKE_BUILD_TYPE=RelWithDebInfo -DHUNTER_CONFIGURATION_TYPES=RelWithDebInfo \
      -DQt6_DIR=${QT_BASEPATH}/lib/cmake \
      -DCI_BUILD=ON
cmake --build build
cmake --install build
( cd build
  git clone https://github.com/Nheko-Reborn/qt-jdenticon.git
  ( cd qt-jdenticon
    qmake
    make -j "$CMAKE_BUILD_PARALLEL_LEVEL"
    cp libqtjdenticon.dylib ../../nheko.temp/nheko.app/Contents/MacOS
  )
  # "$(brew --prefix qt6)/bin/macdeployqt" nheko.app -always-overwrite -qmldir=../resources/qml/
  # # workaround for https://bugreports.qt.io/browse/QTBUG-100686
  # cp "$(brew --prefix brotli)/lib/libbrotlicommon.1.dylib" nheko.app/Contents/Frameworks/libbrotlicommon.1.dylib
)

mv nheko.temp/nheko.app nheko.app
