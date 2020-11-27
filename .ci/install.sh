#!/usr/bin/env sh

set -ex

if [ "$FLATPAK" ]; then
	sudo apt-get -y install flatpak flatpak-builder elfutils
	flatpak remote-add --user --if-not-exists flathub https://flathub.org/repo/flathub.flatpakrepo
	flatpak --noninteractive install --user flathub org.kde.Platform//5.15
	flatpak --noninteractive install --user flathub org.kde.Sdk//5.15
	exit
fi
if [ "$TRAVIS_OS_NAME" = "osx" ]; then
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
fi
