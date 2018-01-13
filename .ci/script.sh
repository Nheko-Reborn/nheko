#!/usr/bin/env bash

set -ex

if [ $TRAVIS_OS_NAME == linux ]; then
    QT_PKG=${QT_VERSION:0:2}
    source /opt/qt${QT_PKG}/bin/qt${QT_PKG}-env.sh || true;
fi

make ci

if [ $TRAVIS_OS_NAME == osx ]; then
    make lint;

    if [ $DEPLOYMENT == 1 ] && [ ! -z $TRAVIS_TAG ]; then
        make macos-deploy;
    fi
fi

if [ $TRAVIS_OS_NAME == linux ] && [ $DEPLOYMENT == 1 ] && [ ! -z $TRAVIS_TAG ]; then
    make linux-deploy;
fi
