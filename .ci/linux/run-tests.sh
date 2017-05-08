#!/usr/bin/env bash

set -evx

cmake -DBUILD_TESTS=ON -H. -Bbuild -DCMAKE_BUILD_TYPE=Release
make -C build -j2

cd build && GTEST_COLOR=1 ctest --verbose
