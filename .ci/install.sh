#!/usr/bin/env bash

set -ex

if [ $TRAVIS_OS_NAME == osx ]; then
    brew update
    brew install qt5 lmdb clang-format ninja libsodium spdlog
    brew upgrade boost

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

    sudo add-apt-repository -y ppa:chris-lea/libsodium
    sudo add-apt-repository -y ppa:beineri/opt-qt${QT_VERSION}-trusty
    sudo add-apt-repository -y ppa:george-edison55/cmake-3.x
    sudo apt-get update -qq
    sudo apt-get install -qq -y \
        qt${QT_PKG}base \
        qt${QT_PKG}tools \
        qt${QT_PKG}svg \
        qt${QT_PKG}multimedia \
        cmake \
        liblmdb-dev \
        libsodium-dev
fi
