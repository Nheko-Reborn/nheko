#!/bin/sh

set -u

# Modified version of script found at:
# https://forum.qt.io/topic/96652/how-to-notarize-qt-application-on-macos/18

# Add Qt binaries to path
PATH="/usr/local/opt/qt@5/bin/:${PATH}"

security unlock-keychain -p "${RUNNER_USER_PW}" login.keychain

( cd build || exit
  # macdeployqt does not copy symlinks over.
  # this specifically addresses icu4c issues but nothing else.
  # We might not even need this any longer... 
  # ICU_LIB="$(brew --prefix icu4c)/lib"
  # export ICU_LIB
  # mkdir -p nheko.app/Contents/Frameworks
  # find "${ICU_LIB}" -type l -name "*.dylib" -exec cp -a -n {} nheko.app/Contents/Frameworks/ \; || true

  #macdeployqt nheko.app -dmg -always-overwrite -qmldir=../resources/qml/ -sign-for-notarization="${APPLE_DEV_IDENTITY}"
  macdeployqt nheko.app -always-overwrite -qmldir=../resources/qml/

  # user=$(id -nu)
  # chown "${user}" nheko.dmg
)

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
      cat "$NOTARIZE_STATUS_LOG" 1>&2
      exit 1
  fi
  echo "Notarization not finished yet, sleep 1m then check again..."
done

VERSION=${CI_COMMIT_SHORT_SHA}

if [ -n "$VERSION" ]; then
    mv nheko.dmg "nheko-${VERSION}.dmg"
    mkdir artifacts
    cp "nheko-${VERSION}.dmg" artifacts/
fi