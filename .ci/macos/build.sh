#!/usr/bin/env bash

set -ue

# unused
#TAG=$(git tag -l --points-at HEAD)

# Add Qt binaries to path
QT_BASEPATH=(${HOME}/Qt/6.*/macos/)
PATH="${QT_BASEPATH}/bin/:${PATH}"
pipx ensurepath
. ~/.zshrc
export PATH

CMAKE_PREFIX_PATH="${QT_BASEPATH}/lib/cmake"
export CMAKE_PREFIX_PATH

export CMAKE_BUILD_PARALLEL_LEVEL="$(sysctl -n hw.ncpu)"

export CMAKE_POLICY_VERSION_MINIMUM="3.5"

# GStreamer / VoIP
VOIP_FLAG="-DVOIP=OFF"

# This assumes that gstreamer is installed via homebrew
GST_PREFIX="$(brew --prefix gstreamer)"
GST_VERSION="$(brew list --versions gstreamer | awk '{print $2}')"

# Extract the Qt version
QT_VER="$(basename "$(dirname "${QT_BASEPATH}")")"

echo "Building GStreamer qml6glsink plugin (GStreamer ${GST_VERSION}, Qt ${QT_VER})…"

if [[ -f "${GST_PREFIX}/lib/gstreamer-1.0/libgstqml6.dylib" ]]; then
    echo "GStreamer qml6glsink plugin already installed; skipping build."
else
    echo "GStreamer qml6glsink plugin not found; building from source."
    # check if the repository is already cloned
    if [[ ! -d "/tmp/gstreamer-src" ]]; then
        git clone --depth 1 --filter=blob:none --sparse \
            --branch "${GST_VERSION}" \
            https://gitlab.freedesktop.org/gstreamer/gstreamer.git \
            /tmp/gstreamer-src
    else
        echo "GStreamer source already cloned; Making sure it's up to date"
        (
            cd /tmp/gstreamer-src
            git fetch --depth 1 origin "${GST_VERSION}"
            git checkout FETCH_HEAD
            git sparse-checkout set subprojects/gst-plugins-good
        )
    fi
    (
        cd /tmp/gstreamer-src
        git sparse-checkout set subprojects/gst-plugins-good
        cd subprojects/gst-plugins-good

        # Disable cmake-based Qt6 discovery: cmake searches system paths (homebrew)
        # causing a conflict (probably doesn't apply in CI)
        MESON_NATIVE_FILE="/tmp/nheko-qml6-native.ini"
        printf '[binaries]\nmoc = '"'"'%s/bin/moc'"'"'\nrcc = '"'"'%s/bin/rcc'"'"'\nuic = '"'"'%s/bin/uic'"'"'\nqmake = '"'"'%s/bin/qmake6'"'"'\n[cmake]\nCMAKE_DISABLE_FIND_PACKAGE_Qt6 = '"'"'true'"'"'\n' \
            "${QT_BASEPATH}" "${QT_BASEPATH}" "${QT_BASEPATH}" "${QT_BASEPATH}" \
            > "${MESON_NATIVE_FILE}"

        PKG_CONFIG_PATH="${GST_PREFIX}/lib/pkgconfig" \
        CXXFLAGS="-I${QT_BASEPATH}/lib/QtGui.framework/Headers -I${QT_BASEPATH}/lib/QtGui.framework/Headers/${QT_VER}/QtGui -F${QT_BASEPATH}/lib" \
        PATH="${QT_BASEPATH}/bin:${PATH}" \
        meson setup build \
            --native-file "${MESON_NATIVE_FILE}" \
            --prefix="${GST_PREFIX}" \
            -Dauto_features=disabled \
            -Dqt6=enabled

        ninja -C build ext/qt6/libgstqml6.dylib
        ninja -C build install
    )
fi

export PKG_CONFIG_PATH="${GST_PREFIX}/lib/pkgconfig:$(brew --prefix)/lib/pkgconfig"
VOIP_FLAG="-DVOIP=ON -DGSTREAMER_AVAILABLE=ON"
echo "GStreamer qml6glsink installed; VoIP support enabled."


# Build nheko
cmake -GNinja -S. -Bbuild \
      -DCMAKE_BUILD_TYPE=RelWithDebInfo \
      -DCMAKE_INSTALL_PREFIX="nheko.temp" \
      -DHUNTER_ROOT="../.hunter" \
      -DHUNTER_ENABLED=ON -DBUILD_SHARED_LIBS=OFF \
      -DKDSingleApplication_STATIC=ON -DKDSingleApplication_EXAMPLES=OFF \
      -DCMAKE_BUILD_TYPE=RelWithDebInfo -DHUNTER_CONFIGURATION_TYPES=RelWithDebInfo \
      -DQt6_DIR="${QT_BASEPATH}/lib/cmake/Qt6" \
      ${GST_PREFIX:+-DCMAKE_PREFIX_PATH="${GST_PREFIX};${QT_BASEPATH}/lib/cmake"} \
      "${VOIP_FLAG}" \
      -DCI_BUILD=ON \
      -DGSTREAMER_AVAILABLE="${GST_PREFIX:+ON}" \
      -DMACOS_DEPLOY_QT="${MACOS_DEPLOY_QT:-ON}"
cmake --build build
cmake --install build
# When not running macdeployqt, Qt frameworks stay in ~/Qt — add the rpath so
# the binary can find them without macdeployqt copying them into the bundle.
if [[ "${MACOS_DEPLOY_QT:-ON}" != "ON" ]]; then
    install_name_tool -add_rpath "${QT_BASEPATH}/lib" \
        nheko.temp/nheko.app/Contents/MacOS/nheko
fi
( cd build
  [[ -d qt-jdenticon ]] || git clone https://github.com/Nheko-Reborn/qt-jdenticon.git
  ( cd qt-jdenticon
    qmake
    make -j "$CMAKE_BUILD_PARALLEL_LEVEL"
    cp libqtjdenticon.dylib ../../nheko.temp/nheko.app/Contents/MacOS
  )
  # "$(brew --prefix qt6)/bin/macdeployqt" nheko.app -always-overwrite -qmldir=../resources/qml/
  # # workaround for https://bugreports.qt.io/browse/QTBUG-100686
  # cp "$(brew --prefix brotli)/lib/libbrotlicommon.1.dylib" nheko.app/Contents/Frameworks/libbrotlicommon.1.dylib
)

# Bundle GStreamer plugins only for distribution builds
# otherwise there are conflicts
if [[ "${VOIP_FLAG}" == *"-DVOIP=ON"* ]] && [[ "${MACOS_DEPLOY_QT:-ON}" == "ON" ]]; then
    echo "Bundling GStreamer plugins into nheko.app…"
    "$(dirname "$0")/bundle-gstreamer.sh" nheko.temp/nheko.app
fi

# remove the old app if it exists, then move the new one into place
rm -rf nheko.app
mv nheko.temp/nheko.app nheko.app

# Ad-hoc sign for local testing (enables TCC permission dialogs for mic/camera).
# Sign each component manually — codesign --deep chokes on the flat gstreamer-1.0
# plugin dir since it's not a proper macOS bundle structure.
# Usage: export MACOS_ADHOC_SIGN=ON before running this script.
if [[ "${MACOS_ADHOC_SIGN:-OFF}" == "ON" ]]; then
    echo "Ad-hoc signing nheko.app for local TCC permission testing…"
    # Only sign bundled GStreamer plugins if they exist (distribution builds only)
    if [[ -d nheko.app/Contents/Resources/gstreamer-1.0 ]]; then
        find nheko.app/Contents/Resources/gstreamer-1.0 -name "*.dylib" \
            -exec codesign --force --options=runtime --sign - {} \;
    fi
    find nheko.app/Contents/Frameworks -name "*.dylib" \
        -exec codesign --force --options=runtime --sign - {} \; 2>/dev/null || true
    codesign --force --options=runtime --sign - \
        --entitlements cmake/nheko.entitlements \
        nheko.app/Contents/MacOS/nheko
    codesign --force --options=runtime --sign - nheko.app
    echo "Ad-hoc signing done."
fi
