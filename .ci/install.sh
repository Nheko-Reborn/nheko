#!/usr/bin/env sh

set -ex

if [ "$TRAVIS_OS_NAME" = "osx" ]; then
    brew update
    brew install qt5 lmdb clang-format ninja libsodium cmark
    brew upgrade boost cmake icu4c || true

    brew tap nlohmann/json
    brew install --with-cmake nlohmann_json

    curl https://bootstrap.pypa.io/get-pip.py -o get-pip.py
    sudo python get-pip.py

    sudo pip install --upgrade pip
    sudo pip install dmgbuild

    export CMAKE_PREFIX_PATH=/usr/local/opt/qt5
fi


if [ "$TRAVIS_OS_NAME" = "linux" ]; then

    if [ -z "$QT_VERSION" ]; then
        QT_VERSION="592"
        QT_PKG="59"
    fi

    wget https://cmake.org/files/v3.12/cmake-3.12.2-Linux-x86_64.sh
    sudo sh cmake-3.12.2-Linux-x86_64.sh  --skip-license  --prefix=/usr/local

    mkdir -p build-libsodium
    ( cd build-libsodium
      curl -L https://download.libsodium.org/libsodium/releases/libsodium-1.0.16.tar.gz -o libsodium-1.0.16.tar.gz
      tar xfz libsodium-1.0.16.tar.gz
      cd libsodium-1.0.16/
      ./configure && make && make check && sudo make install )

    sudo add-apt-repository -y ppa:beineri/opt-qt${QT_VERSION}-trusty
    # needed for git-lfs, otherwise the follow apt update fails.
    sudo apt-key adv --keyserver hkp://keyserver.ubuntu.com:80 --recv-keys 6B05F25D762E3157

    # needed for mongodb repository: https://github.com/travis-ci/travis-ci/issues/9037
    sudo apt-key adv --keyserver hkp://keyserver.ubuntu.com:80 --recv 0C49F3730359A14518585931BC711F9BA15703C6

    sudo apt update -qq
    sudo apt install -qq -y \
        qt${QT_PKG}base \
        qt${QT_PKG}tools \
        qt${QT_PKG}svg \
        qt${QT_PKG}multimedia \
        liblmdb-dev
fi
