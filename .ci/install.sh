#!/usr/bin/env sh

set -ex

if [ "$TRAVIS_OS_NAME" = "osx" ]; then
    brew tap nlohmann/json
    brew install --with-cmake nlohmann_json

    curl https://bootstrap.pypa.io/get-pip.py -o get-pip.py
    sudo python get-pip.py

    sudo pip install --upgrade pip
    sudo pip install dmgbuild

    export CMAKE_PREFIX_PATH=/usr/local/opt/qt5
fi


if [ "$TRAVIS_OS_NAME" = "linux" ]; then
    sudo update-alternatives --install /usr/bin/gcc gcc "/usr/bin/${CC}" 10
    sudo update-alternatives --install /usr/bin/g++ g++ "/usr/bin/${CXX}" 10

    sudo update-alternatives --set gcc "/usr/bin/${CC}"
    sudo update-alternatives --set g++ "/usr/bin/${CXX}"

    wget https://cmake.org/files/v3.15/cmake-3.15.5-Linux-x86_64.sh
    sudo sh cmake-3.15.5-Linux-x86_64.sh  --skip-license  --prefix=/usr/local

    mkdir -p build-libsodium
    ( cd build-libsodium
      curl -L https://download.libsodium.org/libsodium/releases/libsodium-1.0.17.tar.gz -o libsodium-1.0.17.tar.gz
      tar xfz libsodium-1.0.17.tar.gz
      cd libsodium-1.0.17/
      ./configure && make && sudo make install )
fi
