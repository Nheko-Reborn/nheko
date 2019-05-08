#!/usr/bin/env bash

set -ex

if [ "$TRAVIS_OS_NAME" = "linux" ]; then
    export CC=${C_COMPILER}
    export CXX=${CXX_COMPILER}

    sudo update-alternatives --install /usr/bin/gcc gcc "/usr/bin/${C_COMPILER}" 10
    sudo update-alternatives --install /usr/bin/g++ g++ "/usr/bin/${CXX_COMPILER}" 10

    sudo update-alternatives --set gcc "/usr/bin/${C_COMPILER}"
    sudo update-alternatives --set g++ "/usr/bin/${CXX_COMPILER}"
fi

if [ "$TRAVIS_OS_NAME" = "linux" ]; then
    # shellcheck disable=SC1090
    . "/opt/qt${QT_PKG}/bin/qt${QT_PKG}-env.sh" || true;
fi

if [ "$TRAVIS_OS_NAME" = "osx" ]; then
    export CMAKE_PREFIX_PATH=/usr/local/opt/qt5
fi

# Build & install dependencies
cget init --prefix .deps/usr --std=c++14
cget install --prefix .deps/usr -f requirements.txt -GNinja

# Build nheko
cmake -GNinja -H. -Bbuild \
    -DCMAKE_BUILD_TYPE=RelWithDebInfo \
    -DCMAKE_INSTALL_PREFIX=.deps/usr \
    -DCMAKE_TOOLCHAIN_FILE=.deps/usr/cget/cget.cmake
cmake --build build

if [ "$TRAVIS_OS_NAME" = "osx" ]; then
    make lint;

    if [ "$DEPLOYMENT" = 1 ] && [ ! -z "$VERSION" ] ; then
        make macos-deploy;
    fi
fi

if [ "$TRAVIS_OS_NAME" = "linux" ] && [ "$DEPLOYMENT" = 1 ] && [ ! -z "$VERSION" ]; then
    make linux-deploy;
fi
