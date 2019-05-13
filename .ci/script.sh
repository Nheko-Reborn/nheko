#!/usr/bin/env bash

set -ex

if [ "$TRAVIS_OS_NAME" = "linux" ]; then
    export CC=${C_COMPILER}
    export CXX=${CXX_COMPILER}
    # make build use all available cores
    export CMAKE_BUILD_PARALLEL_LEVEL=$(cat /proc/cpuinfo | awk '/^processor/{print $3}' | wc -l)

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
cmake -GNinja -Hdeps -B.deps \
    -DUSE_BUNDLED_BOOST="${USE_BUNDLED_BOOST}" \
    -DUSE_BUNDLED_CMARK="${USE_BUNDLED_CMARK}" \
    -DUSE_BUNDLED_JSON="${USE_BUNDLED_JSON}"
cmake --build .deps

# Build nheko
cmake -GNinja -H. -Bbuild \
    -DCMAKE_BUILD_TYPE=RelWithDebInfo \
    -DCMAKE_INSTALL_PREFIX=.deps/usr
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
