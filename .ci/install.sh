#!/usr/bin/env bash

set -ex

if [ "$TRAVIS_OS_NAME" == "osx" ]; then
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


if [ "$TRAVIS_OS_NAME" == "linux" ]; then

    if [ -z "$QT_VERSION" ]; then
        QT_VERSION="592"
        QT_PKG="59"
    fi

    wget https://cmake.org/files/v3.12/cmake-3.12.2-Linux-x86_64.sh
    sudo sh cmake-3.12.2-Linux-x86_64.sh  --skip-license  --prefix=/usr/local

    mkdir -p build-libsodium
    pushd build-libsodium
    curl -L https://download.libsodium.org/libsodium/releases/libsodium-1.0.16.tar.gz -o libsodium-1.0.16.tar.gz
    tar xfz libsodium-1.0.16.tar.gz 
    cd libsodium-1.0.16/
    ./configure && make && make check && sudo make install
    popd

    sudo add-apt-repository -y ppa:beineri/opt-qt${QT_VERSION}-trusty
    sudo apt-get update -qq
    sudo apt-get install -qq -y \
        qt${QT_PKG}base \
        qt${QT_PKG}tools \
        qt${QT_PKG}svg \
        qt${QT_PKG}multimedia \
        liblmdb-dev
fi