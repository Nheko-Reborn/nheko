#!/usr/bin/env bash

set -ex

if [ $TRAVIS_OS_NAME == osx ]; then
    brew update
    brew install qt5 lmdb clang-format ninja libsodium spdlog
    brew upgrade boost cmake

    curl https://bootstrap.pypa.io/get-pip.py -o get-pip.py
    sudo python get-pip.py

    sudo pip install --upgrade pip
    sudo pip install dmgbuild

    export CMAKE_PREFIX_PATH=/usr/local/opt/qt5
fi


if [ $TRAVIS_OS_NAME == linux ]; then

    if [ -z "$QT_VERSION" ]; then
        QT_VERSION="592"
        QT_PKG="59"
    fi

    wget https://cmake.org/files/v3.11/cmake-3.11.4-Linux-x86_64.sh
    sudo sh cmake-3.11.4-Linux-x86_64.sh  --skip-license  --prefix=/usr/local

    sudo add-apt-repository -y ppa:chris-lea/libsodium
    sudo add-apt-repository -y ppa:beineri/opt-qt${QT_VERSION}-trusty
    sudo apt-get update -qq
    sudo apt-get install -qq -y \
        qt${QT_PKG}base \
        qt${QT_PKG}tools \
        qt${QT_PKG}svg \
        qt${QT_PKG}multimedia \
        liblmdb-dev \
        libsodium-dev
fi
