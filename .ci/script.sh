#!/usr/bin/env bash

set -ex

if [ $TRAVIS_OS_NAME == linux ]; then
    source /opt/qt${QT_PKG}/bin/qt${QT_PKG}-env.sh || true;
fi

if [ $TRAVIS_OS_NAME == osx ]; then
    export CMAKE_PREFIX_PATH=/usr/local/opt/qt5
fi

# Build & install dependencies
cmake -Hdeps -B.deps \
    -DUSE_BUNDLED_BOOST=${USE_BUNDLED_BOOST} \
    -DUSE_BUNDLED_SPDLOG=${USE_BUNDLED_SPDLOG}
cmake --build .deps

# Build nheko
cmake -GNinja -H. -Bbuild -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build

if [ $TRAVIS_OS_NAME == osx ]; then
    make lint;

    if [ $DEPLOYMENT == 1 ] && [ ! -z $TRAVIS_TAG ]; then
        make macos-deploy;
    fi
fi

if [ $TRAVIS_OS_NAME == linux ] && [ $DEPLOYMENT == 1 ] && [ ! -z $TRAVIS_TAG ]; then
    make linux-deploy;
fi
