#!/usr/bin/env bash

set -ex

if [ "$TRAVIS_OS_NAME" == "osx" ]; then
    brew update
    brew install qt5 lmdb clang-format ninja libsodium cmark
    brew upgrade boost cmake icu4c || true

    brew tap nlohmann/json
    brew install nlohmann_json

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

    sudo apt-get update -qq
    sudo apt-get install -qq -y \
        qtbase5-dev \
        qttools5-dev \
        libqt5svg5-dev \
        qtmultimedia5-dev \
        liblmdb-dev
fi