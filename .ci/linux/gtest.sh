#!/usr/bin/env bash

set -evx

sudo apt-get -qq update
sudo apt-get install -y libgtest-dev
wget https://github.com/google/googletest/archive/release-1.8.0.tar.gz
tar xf release-1.8.0.tar.gz
cd googletest-release-1.8.0

cmake -DBUILD_SHARED_LIBS=ON .
make
sudo cp -a googletest/include/gtest /usr/include
sudo cp -a googlemock/gtest/*.so /usr/lib/

sudo ldconfig -v | grep gtest

cd $TRAVIS_BUILD_DIR

