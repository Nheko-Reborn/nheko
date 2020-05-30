#!/usr/bin/env bash

set -ex

if [ "$FLATPAK" ]; then
	mkdir -p build-flatpak
	cd build-flatpak

	flatpak-builder --ccache --repo=repo --subject="Build of Nheko ${VERSION} `date`" app ../io.github.NhekoReborn.Nheko.json
	flatpak build-bundle repo nheko-${VERSION}-${ARCH}.flatpak io.github.NhekoReborn.Nheko master

	mkdir ../artifacts
	mv nheko-*.flatpak ../artifacts

	exit
fi

if [ "$TRAVIS_OS_NAME" = "linux" ]; then
    # make build use all available cores
    export CMAKE_BUILD_PARALLEL_LEVEL=$(cat /proc/cpuinfo | awk '/^processor/{print $3}' | wc -l)

    export PATH="/usr/local/bin/:${PATH}"
    cmake --version
fi

if [ "$TRAVIS_OS_NAME" = "linux" ]; then
    # shellcheck disable=SC1090
    . "/opt/qt${QT_PKG}/bin/qt${QT_PKG}-env.sh" || true;
fi

if [ "$TRAVIS_OS_NAME" = "osx" ]; then
    export CMAKE_PREFIX_PATH=/usr/local/opt/qt5
fi

mkdir -p .deps/usr .hunter

# Build nheko

if [ "$TRAVIS_OS_NAME" = "osx" ]; then
cmake -GNinja -H. -Bbuild \
    -DCMAKE_BUILD_TYPE=RelWithDebInfo \
    -DCMAKE_INSTALL_PREFIX=.deps/usr \
    -DHUNTER_ROOT=".hunter" \
    -DHUNTER_ENABLED=ON -DBUILD_SHARED_LIBS=OFF \
    -DCMAKE_BUILD_TYPE=RelWithDebInfo -DHUNTER_CONFIGURATION_TYPES=RelWithDebInfo \
    -DCMAKE_PREFIX_PATH=/usr/local/opt/qt5 \
    -DCI_BUILD=ON
else
cmake -GNinja -H. -Bbuild \
    -DCMAKE_BUILD_TYPE=RelWithDebInfo \
    -DCMAKE_INSTALL_PREFIX=.deps/usr \
    -DHUNTER_ROOT=".hunter" \
    -DHUNTER_ENABLED=ON -DBUILD_SHARED_LIBS=OFF \
    -DCMAKE_BUILD_TYPE=RelWithDebInfo -DHUNTER_CONFIGURATION_TYPES=RelWithDebInfo \
    -DUSE_BUNDLED_OPENSSL=OFF \
    -DCI_BUILD=ON
fi
cmake --build build

if [ "$TRAVIS_OS_NAME" = "osx" ]; then
    make lint;

    if [ "$DEPLOYMENT" = 1 ] && [ -n "$VERSION" ] ; then
        make macos-deploy;
    fi
fi

if [ "$TRAVIS_OS_NAME" = "linux" ] && [ "$DEPLOYMENT" = 1 ] && [ -n "$VERSION" ]; then
    make linux-deploy;
fi
