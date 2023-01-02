#!/bin/sh

set -u

# Modified version of script found at:
# https://forum.qt.io/topic/96652/how-to-notarize-qt-application-on-macos/18

# Add Qt binaries to path
PATH="/usr/local/opt/qt@5/bin/:${PATH}"
export PATH

security unlock-keychain -p "${RUNNER_USER_PW}" login.keychain

if [ -n "${CI_PIPELINE_TRIGGERED:-}" ] && [ "${TRIGGERED_BY:-}" = "cirrus" ]; then
  echo "cirrus build id: ${TRIGGER_BUILD_ID}"
  cat "${TRIGGER_PAYLOAD}"
  # download the build artifacts from cirrus api
  curl "https://api.cirrus-ci.com/v1/artifact/build/${TRIGGER_BUILD_ID}/binaries.zip" -o binaries.zip
  # cirrus ci artifacts task name is 'binaries' so that's the zip name.
  unzip binaries.zip
  # we zip 'build/nheko.app' in cirrus ci, cirrus itself puts it in a 'build' directory
  # so move it to the right place for the rest of the process.
  ( cd build || exit
    unzip nheko.zip
  )
fi

if [ ! -d "build/nheko.app" ]; then
  echo "nheko.app is missing, you did something wrong!"
  exit 1
fi

echo "[INFO] Signing app contents"
find "build/nheko.app/Contents"|while read -r fname; do
    if [ -f "$fname" ]; then
        echo "[INFO] Signing $fname"
        codesign --force --timestamp --options=runtime --sign "${APPLE_DEV_IDENTITY}" "$fname"
    fi
done

codesign --force --timestamp --options=runtime --sign "${APPLE_DEV_IDENTITY}" "build/nheko.app"

NOTARIZE_SUBMIT_LOG=$(mktemp /tmp/notarize-submit.XXXXXX)
NOTARIZE_STATUS_LOG=$(mktemp /tmp/notarize-status.XXXXXX)

finish() {
  rm "$NOTARIZE_SUBMIT_LOG" "$NOTARIZE_STATUS_LOG"
}
trap finish EXIT

dmgbuild -s .ci/macos/settings.json "Nheko" nheko.dmg
codesign -s "${APPLE_DEV_IDENTITY}" nheko.dmg

user=$(id -nu)
chown "${user}" nheko.dmg

echo "--> Start Notarization process"
# OLD altool usage: xcrun altool -t osx -f nheko.dmg --primary-bundle-id "io.github.nheko-reborn.nheko" --notarize-app -u "${APPLE_DEV_USER}" -p "${APPLE_DEV_PASS}" > "$NOTARIZE_SUBMIT_LOG" 2>&1
xcrun notarytool submit nheko.dmg --apple-id "${APPLE_DEV_USER}" --password "${APPLE_DEV_PASS}" --team-id "${APPLE_TEAM_ID}" > "$NOTARIZE_SUBMIT_LOG" 2>&1
# OLD altool usage: requestUUID="$(awk -F ' = ' '/RequestUUID/ {print $2}' "$NOTARIZE_SUBMIT_LOG")"
requestUUID="$(awk -F ': ' '/id/ {print $2}' "$NOTARIZE_SUBMIT_LOG" | head -1)"

if [ -z "${requestUUID}" ]; then
  echo "Something went wrong when submitting the request... we don't have a UUID"
  exit 1
else
  echo "Received requestUUID: \"${requestUUID}\""
fi

while sleep 60 && date; do
  echo "--> Checking notarization status for \"${requestUUID}\""

  # OLD altool usage: xcrun altool --notarization-info "${requestUUID}" -u "${APPLE_DEV_USER}" -p "${APPLE_DEV_PASS}" > "$NOTARIZE_STATUS_LOG" 2>&1
  xcrun notarytool info "${requestUUID}" --apple-id "${APPLE_DEV_USER}" --password "${APPLE_DEV_PASS}" --team-id "${APPLE_TEAM_ID}" > "$NOTARIZE_STATUS_LOG" 2>&1

  sub_status="$(awk -F ': ' '/status/ {print $2}' "$NOTARIZE_STATUS_LOG")"
  #isSuccess=$(grep "success" "$NOTARIZE_STATUS_LOG")
  #isFailure=$(grep "invalid" "$NOTARIZE_STATUS_LOG")

  echo "Status for submission \"${requestUUID}\": \"${sub_status}\""

  if [ "${sub_status}" = "Accepted" ]; then
      echo "Notarization done!"
      xcrun stapler staple -v nheko.dmg
      echo "Stapler done!"
      break
  fi
  if [ "${sub_status}" = "Invalid" ] || [ "${sub_status}" = "Rejected" ]; then
      echo "Notarization failed"
      xcrun notarytool log "${requestUUID}" --apple-id "${APPLE_DEV_USER}" --password "${APPLE_DEV_PASS}" --team-id "${APPLE_TEAM_ID}" > "$NOTARIZE_STATUS_LOG" 2>&1
      cat "$NOTARIZE_STATUS_LOG" 1>&2
      exit 1
  fi
  echo "Notarization not finished yet, sleep 1m then check again..."
done

VERSION=${CI_COMMIT_SHORT_SHA}

if [ -n "$VERSION" ]; then
    mv nheko.dmg "nheko-${VERSION}_${PLAT}.dmg"
    mkdir artifacts
    cp "nheko-${VERSION}_${PLAT}.dmg" artifacts/
fi