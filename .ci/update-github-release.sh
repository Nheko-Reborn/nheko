#!/bin/sh

if [ -z "$CI_COMMIT_TAG" ]; then
    echo "CI_COMMIT_TAG is unset or empty; exiting"
    exit 1
fi

echo "Checking if release exists for ${CI_COMMIT_TAG}"
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
    echo "Release does not exist... getting notes from CHANGELOG.md:"
    release_notes="$(perl -0777 -ne '/.*?(## .*?)\n(## |\Z)/s && print $1' CHANGELOG.md | jq -R -s '.')"
    echo "$release_notes"

    echo "Creating new release for ${CI_COMMIT_TAG}"
    # Doing a 'fresh' release, not just updating the assets.
    release_json="$(curl \
        -X POST \
        -H "Accept: application/vnd.github+json" \
        -H "Authorization: Bearer ${GITHUB_AUTH_TOKEN}"\
        -H "X-GitHub-Api-Version: 2022-11-28" \
        https://api.github.com/repos/Nheko-Reborn/nheko/releases \
        -d "{\"tag_name\":\"${CI_COMMIT_TAG}\",\"target_commitish\":\"master\",\"name\":\"${CI_COMMIT_TAG}\",\"body\":${release_notes},\"draft\":true,\"prerelease\":true,\"generate_release_notes\":false}")"
elif [ "$http_code" = "200" ]; then
    echo "Release already exists for ${CI_COMMIT_TAG}; Updating"
    # Updating a release (probably because of cirrus-ci or so)
    release_json=$(curl \
        -s \
        -H "Accept: application/vnd.github+json" \
        -H "Authorization: Bearer ${GITHUB_AUTH_TOKEN}"\
        -H "X-GitHub-Api-Version: 2022-11-28" \
        "https://api.github.com/repos/Nheko-Reborn/nheko/releases/tags/$CI_COMMIT_TAG")
fi

echo "Getting upload URL..."
upload_url="$(echo "$release_json" | jq -r '."upload_url"')"
# get rid of the 'hypermedia' stuff at the end and use a 'real' URL
echo "Upload URL (hypermedia): ${upload_url}"
upload_url="$(echo "$upload_url" | sed 's/{?name,label\}/?name/g')"

ls -la .
echo "Uploading artifacts"
for file in ./artifacts/*; do
    name="${file##*/}"
    echo "Uploading $file"
    [ -e "$file" ] && curl \
    -X POST \
    -H "Accept: application/vnd.github+json" \
    -H "Authorization: Bearer ${GITHUB_AUTH_TOKEN}"\
    -H "X-GitHub-Api-Version: 2022-11-28" \
    -H "Content-Type: application/octet-stream" \
    "${upload_url}=$name" \
    --data-binary "@$file"
done


# TODO: AppVeyor stuffs?