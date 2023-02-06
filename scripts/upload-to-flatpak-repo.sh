#!/bin/bash

if [ -z "$1" ]; then
	echo "Missing repo to upload!"
	exit 1
fi

if [ -n "${CI_COMMIT_TAG}" ]; then
	BUILD_URL=$(./flat-manager-client create --public_download https://flatpak.neko.dev stable)
elif [ "master" = "${CI_COMMIT_REF_NAME}" ]; then
	BUILD_URL=$(./flat-manager-client create --public_download https://flatpak.neko.dev nightly)
fi

if [ -z "${BUILD_URL}" ]; then
	echo "No upload to repo."
	exit 0
fi

BUILD_URL=${BUILD_URL/http:/https:}

./flat-manager-client push --commit --wait --wait-update $BUILD_URL $1
./flat-manager-client publish --wait --wait-update $BUILD_URL

