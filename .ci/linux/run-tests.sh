#!/usr/bin/env bash

set -evx

cmake -DBUILD_TESTS=ON -H. -Bbuild && cmake --build build

cd build && GTEST_COLOR=1 ctest --verbose
