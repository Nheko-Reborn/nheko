#!/usr/bin/env bash

set -ex

#
# Create deb & rpm packages from the AppImage.
#

DIR=package.dir
TAG=`git tag -l --points-at HEAD`

# Strip `v` from the version tag.
if [[ $TAG == v* ]]; then
    TAG=${TAG#?};
fi

# Prepend nightly with the latest version.
if [[ $TAG == "nightly" ]]; then
    LATEST_VERSION=`git tag -l | grep "^v" | sort | head -n 1`
    TAG=${LATEST_VERSION#?}.nightly
fi

# Installing dependencies on travis.
if [ ! -z "$TRAVIS_OS_NAME" ]; then
    sudo apt-add-repository -y ppa:brightbox/ruby-ng
    sudo apt-get update -qq
    sudo apt-get install -y ruby2.1 ruby-switch
    sudo ruby-switch --set ruby2.1
    sudo apt-get install -y ruby2.1-dev rpm libffi-dev

    sudo gem install --no-ri --no-rdoc fpm
fi

# Set up deb structure.
mkdir -p ${DIR}/usr/{bin,share/pixmaps,share/applications}

# Copy resources.
cp nheko*.AppImage ${DIR}/usr/bin/nheko
cp resources/nheko.desktop ${DIR}/usr/share/applications/nheko.desktop
cp resources/nheko.png ${DIR}/usr/share/pixmaps/nheko.png

for iconSize in 16 32 48 64 128 256 512; do
    IconDir=${DIR}/usr/share/icons/hicolor/${iconSize}x${iconSize}/apps
    mkdir -p ${IconDir}
    cp resources/nheko-${iconSize}.png ${IconDir}/nheko.png
done

fpm --force \
    -s dir \
    --output-type deb \
    --name nheko \
    --description "Desktop client for the Matrix protocol" \
    --url "https://github.com/mujx/nheko" \
    --version ${TAG} \
    --architecture x86_64 \
    --maintainer "mujx (https://github.com/mujx)" \
    --license "GPLv3" \
    --prefix / \
    --deb-no-default-config-files \
    --chdir ${DIR} usr

fpm -s deb -t rpm nheko_${TAG}_amd64.deb
