#!/usr/bin/env sh

set -ex

if [ "$TRAVIS_OS_NAME" = "osx" ]; then
    brew update
    brew install qt5 lmdb clang-format ninja libsodium cmark autoconf automake libtool pkg-config || true

    # probably should update this to check if these are actually installed or not
    # but this installs boost, cmake, and icu4c if they aren't installed already
    # and upgrades them if they are
    brew install boost cmake icu4c || true
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

    UBUNTU_RELEASE=$(lsb_release -sc)

    if [ "$UBUNTU_RELEASE" = "trusty" ]; then
        curl https://apt.llvm.org/llvm-snapshot.gpg.key | sudo apt-key add -
        sudo apt-add-repository -y "deb http://apt.llvm.org/trusty/ llvm-toolchain-trusty-5.0 main"

        sudo apt-get update -qq
        sudo apt-get install -qq -y clang

        sudo add-apt-repository -y ppa:beineri/opt-qt${QT_VERSION}-trusty

    elif [ "$UBUNTU_RELEASE" = "xenial" ]; then
        curl https://apt.llvm.org/llvm-snapshot.gpg.key | sudo apt-key add -
        sudo apt-add-repository -y "deb http://apt.llvm.org/xenial/ llvm-toolchain-xenial-6.0 main"
        sudo apt-get update -qq
        sudo apt-get install -qq -y clang-6.0
        sudo add-apt-repository -y ppa:beineri/opt-qt-5.11.1-xenial
    fi

    # needed for git-lfs, otherwise the follow apt update fails.
    sudo apt-key adv --keyserver hkp://keyserver.ubuntu.com:80 --recv-keys 6B05F25D762E3157
    # needed for mongodb repository: https://github.com/travis-ci/travis-ci/issues/9037
    sudo apt-key adv --keyserver hkp://keyserver.ubuntu.com:80 --recv 0C49F3730359A14518585931BC711F9BA15703C6

    sudo add-apt-repository -y ppa:ubuntu-toolchain-r/test

    sudo apt update -qq
    sudo apt install -qq -y \
        ninja-build \
        qt${QT_PKG}base \
        qt${QT_PKG}tools \
        qt${QT_PKG}svg \
        qt${QT_PKG}multimedia \
        liblmdb-dev
fi
