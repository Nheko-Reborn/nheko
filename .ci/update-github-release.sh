#!/bin/sh

if [ -z "$CI_COMMIT_TAG" ]; then
    echo "CI_COMMIT_TAG is unset or empty; exiting"
    exit 1
fi


# check if we already have a release for the current tag or not
http_code=$(curl \
  -s \
  -o /dev/null \
  -I \
  -w "%{http_code}" \
  -H "Accept: application/vnd.github+json" \
  -H "Authorization: Bearer ${GITHUB_AUTH_TOKEN}"\
  -H "X-GitHub-Api-Version: 2022-11-28" \
  "https://api.github.com/repos/Nheko-Reborn/nheko/releases/tags/$CI_COMMIT_TAG")

if [ "$http_code" = "404" ]; then
    # Doing a 'fresh' release, not just updating the assets.
    release_json="$(curl \
        -X POST \
        -H "Accept: application/vnd.github+json" \
        -H "Authorization: Bearer ${GITHUB_AUTH_TOKEN}"\
        -H "X-GitHub-Api-Version: 2022-11-28" \
        https://api.github.com/repos/Nheko-Reborn/nheko/releases \
        -d "{\"tag_name\":\"${CI_COMMIT_TAG}\",\"target_commitish\":\"master\",\"name\":\"${CI_COMMIT_TAG}\",\"body\":\"Description of the release\",\"draft\":true,\"prerelease\":true,\"generate_release_notes\":false}")"
elif [ "$http_code" = "200" ]; then
    # Updating a release (probably because of cirrus-ci or so)
    release_json=$(curl \
        -s \
        -H "Accept: application/vnd.github+json" \
        -H "Authorization: Bearer ${GITHUB_AUTH_TOKEN}"\
        -H "X-GitHub-Api-Version: 2022-11-28" \
        "https://api.github.com/repos/Nheko-Reborn/nheko/releases/tags/$CI_COMMIT_TAG")
fi

upload_url="$(echo "$release_json" | jq ".upload-url")"
# get rid of the 'hypermedia' stuff at the end and use a 'real' URL
upload_url="$(echo "$upload_url" | sed 's/{?name,label\}/?name/g')"

for file in ./artifacts/*; do
    name="${file##*/}"
    [ -e "$file" ] && curl \
    -X POST \
    -H "Accept: application/vnd.github+json" \
    -H "Authorization: Bearer ${GITHUB_AUTH_TOKEN}"\
    -H "X-GitHub-Api-Version: 2022-11-28" \
    -H "Content-Type: application/octet-stream" \
    "${upload_url}=$name" \
    --data-binary "@$name"
done


# TODO: AppVeyor stuffs?