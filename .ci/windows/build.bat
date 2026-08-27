:: VERSION format:     v1.2.3/v1.3.4
:: INSTVERSION format: 1.2.3/1.3.4
:: WINVERSION format:  1.2.3.123/1.3.4.234
if defined CI_COMMIT_TAG (
	set VERSION=%CI_COMMIT_TAG%
) else (
	set VERSION=v0.12.1
)
set INSTVERSION=%VERSION:~1%
if defined CI_JOB_ID (
	set WINVERSION=%VERSION:~1%.%CI_JOB_ID%
) else (
	set WINVERSION=%VERSION:~1%.0
)
set DATE=%date:~10,4%-%date:~4,2%-%date:~7,2%
echo %VERSION%
echo %INSTVERSION%
echo %DATE%


call "C:/Program Files (x86)/Microsoft Visual Studio/2022/BuildTools/VC/Auxiliary/Build/vcvarsall.bat" x64

set GSTREAMER_VERSION=1.28.1
set GSTREAMER_BASE_URL=https://gstreamer.freedesktop.org/data/pkg/windows/%GSTREAMER_VERSION%/msvc

:: Resolve to absolute path (gst installer requires it)
pushd %~dp0..\..
set GSTREAMER_LOCAL=%CD%\gstreamer
popd

:: Check system-wide GStreamer
if exist "C:\gstreamer\bin\gst-inspect-1.0.exe" (
    set GSTREAMER_ROOT=C:\gstreamer
) else if exist "%GSTREAMER_LOCAL%\bin\gst-inspect-1.0.exe" (
    set GSTREAMER_ROOT=%GSTREAMER_LOCAL%
) else (
    echo Downloading GStreamer %GSTREAMER_VERSION% installer...
    curl -L -o gstreamer-installer.exe "%GSTREAMER_BASE_URL%/gstreamer-1.0-msvc-x86_64-%GSTREAMER_VERSION%.exe"
    echo Installing GStreamer %GSTREAMER_VERSION%...
:: RunAsInvoker suppresses the UAC elevation prompt that the NSIS installer
:: triggers;
    set __COMPAT_LAYER=RunAsInvoker
    gstreamer-installer.exe /VERYSILENT /SUPPRESSMSGBOXES /NORESTART /TYPE=full /DIR="%GSTREAMER_LOCAL%"
    set __COMPAT_LAYER=
    del gstreamer-installer.exe
    if not exist "%GSTREAMER_LOCAL%\bin\gst-inspect-1.0.exe" (
        echo ERROR: GStreamer installation failed - gst-inspect-1.0.exe not found
        exit /b 1
    )
    set GSTREAMER_ROOT=%GSTREAMER_LOCAL%
)

set PKG_CONFIG_PATH=%GSTREAMER_ROOT%\lib\pkgconfig;%PKG_CONFIG_PATH%
set PATH=%GSTREAMER_ROOT%\bin;%PATH%

if not defined QT_PATH set QT_PATH=C:\Qt\6.7.3\msvc2019_64
set PATH=%QT_PATH%\bin;%PATH%

set CMAKE_POLICY_VERSION_MINIMUM=3.5
cmake -G "Visual Studio 17 2022" -A x64 -S. -Bbuild -DHUNTER_ROOT="C:\hunter" -DHUNTER_ENABLED=ON -DHUNTER_STATUS_DEBUG=ON -DBUILD_SHARED_LIBS=OFF -DUSE_BUNDLED_OPENSSL=ON -DUSE_BUNDLED_KDSINGLEAPPLICATION=ON -DKDSingleApplication_STATIC=ON -DCMAKE_BUILD_TYPE=Release -DHUNTER_CONFIGURATION_TYPES=Release -DVOIP=ON -DCMAKE_PREFIX_PATH="%QT_PATH%"
cmake --build build --config Release -j %NUMBER_OF_PROCESSORS%


if not exist qt-jdenticon\qtjdenticon.pro (
    if exist qt-jdenticon rmdir /s /q qt-jdenticon
    git clone https://github.com/Nheko-Reborn/qt-jdenticon.git
)
cd qt-jdenticon
qmake
nmake
cd ..

:: Build GStreamer Qt6 D3D11 QML plugin (provides qml6d3d11sink)
set GST_QT6D3D11_SRC=gstreamer-src\subprojects\gst-plugins-bad\ext\qt6d3d11
if not exist %GST_QT6D3D11_SRC%\plugin.cpp (
    if exist gstreamer-src rmdir /s /q gstreamer-src
    git clone --depth 1 --branch %GSTREAMER_VERSION% --filter=blob:none --sparse https://gitlab.freedesktop.org/gstreamer/gstreamer.git gstreamer-src
    cd gstreamer-src
    git sparse-checkout set subprojects/gst-plugins-bad/ext/qt6d3d11 subprojects/gst-plugins-bad/gst-libs/gst/d3d11
    cd ..
)
:: Upstream uses meson; provide our own CMakeLists.txt for this plugin
copy /Y %~dp0gst-qt6d3d11-CMakeLists.txt %GST_QT6D3D11_SRC%\CMakeLists.txt
cmake -G "Visual Studio 17 2022" -A x64 -S %GST_QT6D3D11_SRC% -B %GST_QT6D3D11_SRC%\_build -DCMAKE_BUILD_TYPE=Release -DGSTREAMER_VERSION=%GSTREAMER_VERSION%
cmake --build %GST_QT6D3D11_SRC%\_build --config Release
:: create zip bundle
mkdir NhekoRelease
copy build\Release\nheko.exe NhekoRelease\nheko.exe
copy qt-jdenticon\release\qtjdenticon0.dll NhekoRelease\qtjdenticon.dll
copy build\_deps\cmark-build\src\Release\cmark.dll NhekoRelease\cmark.dll
windeployqt --qmldir resources\qml\ NhekoRelease\nheko.exe

:: Overwrite Qt's older FFmpeg DLLs with GStreamer's newer version (avcodec-61 API compat)
copy /Y "%GSTREAMER_ROOT%\bin\avcodec-61.dll" NhekoRelease\
copy /Y "%GSTREAMER_ROOT%\bin\avformat-61.dll" NhekoRelease\
copy /Y "%GSTREAMER_ROOT%\bin\avutil-59.dll" NhekoRelease\
copy /Y "%GSTREAMER_ROOT%\bin\swresample-5.dll" NhekoRelease\

:: Bundle GStreamer runtime DLLs and their dependencies
set GST_BIN=%GSTREAMER_ROOT%\bin
set GST_LIB=%GSTREAMER_ROOT%\lib\gstreamer-1.0
for %%f in (
    "%GST_BIN%\gstreamer-1.0-0.dll"
    "%GST_BIN%\gstbase-1.0-0.dll"
    "%GST_BIN%\gstsdp-1.0-0.dll"
    "%GST_BIN%\gstwebrtc-1.0-0.dll"
    "%GST_BIN%\gstrtp-1.0-0.dll"
    "%GST_BIN%\gstgl-1.0-0.dll"
    "%GST_BIN%\gstnet-1.0-0.dll"
    "%GST_BIN%\gstsctp-1.0-0.dll"
    "%GST_BIN%\gstaudio-1.0-0.dll"
    "%GST_BIN%\gstvideo-1.0-0.dll"
    "%GST_BIN%\gstpbutils-1.0-0.dll"
    "%GST_BIN%\gsttag-1.0-0.dll"
    "%GST_BIN%\gstapp-1.0-0.dll"
    "%GST_BIN%\gstd3d11-1.0-0.dll"
    "%GST_BIN%\glib-2.0-0.dll"
    "%GST_BIN%\gobject-2.0-0.dll"
    "%GST_BIN%\gmodule-2.0-0.dll"
    "%GST_BIN%\gio-2.0-0.dll"
    "%GST_BIN%\gthread-2.0-0.dll"
    "%GST_BIN%\intl-8.dll"
    "%GST_BIN%\ffi-7.dll"
    "%GST_BIN%\pcre2-8-0.dll"
    "%GST_BIN%\z-1.dll"
    "%GST_BIN%\orc-0.4-0.dll"
    "%GST_BIN%\graphene-1.0-0.dll"
    "%GST_BIN%\json-glib-1.0-0.dll"
    "%GST_BIN%\nice-10.dll"
    "%GST_BIN%\opus-0.dll"
    "%GST_BIN%\openh264-7.dll"
    "%GST_BIN%\srtp2-1.dll"
    "%GST_BIN%\libcrypto-3-x64.dll"
    "%GST_BIN%\libssl-3-x64.dll"
    "%GST_BIN%\gstwebrtcnice-1.0-0.dll"
    "%GST_BIN%\gstd3dshader-1.0-0.dll"
    "%GST_BIN%\gstdxva-1.0-0.dll"
    "%GST_BIN%\gstcodecs-1.0-0.dll"
    "%GST_BIN%\gstcodecparsers-1.0-0.dll"
    "%GST_BIN%\gstcontroller-1.0-0.dll"
    "%GST_BIN%\gstwinrt-1.0-0.dll"
) do if exist %%f copy %%f NhekoRelease\
mkdir NhekoRelease\lib\gstreamer-1.0
for %%f in (
    "%GST_LIB%\gstcoreelements.dll"
    "%GST_LIB%\gstd3d11.dll"
    "%GST_LIB%\gstopengl.dll"
    "%GST_LIB%\gstopenh264.dll"
    "%GST_LIB%\gstopus.dll"
    "%GST_LIB%\gstvpx.dll"
    "%GST_LIB%\gstwebrtc.dll"
    "%GST_LIB%\gstrtp.dll"
    "%GST_LIB%\gstrtpmanager.dll"
    "%GST_LIB%\gstdtls.dll"
    "%GST_LIB%\gstnice.dll"
    "%GST_LIB%\gstsrtp.dll"
    "%GST_LIB%\gstplayback.dll"
    "%GST_LIB%\gstvolume.dll"
    "%GST_LIB%\gstaudioresample.dll"
    "%GST_LIB%\gstaudioconvert.dll"
    "%GST_LIB%\gstvideoconvertscale.dll"
    "%GST_LIB%\gstapp.dll"
    "%GST_LIB%\gstautodetect.dll"
    "%GST_LIB%\gstwasapi2.dll"
    "%GST_LIB%\gstmediafoundation.dll"
    "%GST_LIB%\gstcompositor.dll"
    "%GST_LIB%\gstvideorate.dll"
) do if exist %%f copy %%f NhekoRelease\lib\gstreamer-1.0\
:: Copy built GStreamer Qt6 D3D11 plugin
copy %GST_QT6D3D11_SRC%\_build\Release\gstqt6d3d11.dll NhekoRelease\lib\gstreamer-1.0\

7z a nheko_win_64.zip .\NhekoRelease\*


:: create msix
mkdir msix
xcopy .\NhekoRelease\*.* msix\*.* /s /e /c /y
copy .\resources\nheko.png msix
copy .\resources\nheko.png msix\nheko_altform-unplated.png
copy .\resources\nheko-44.png msix\nheko-44.png
copy .\resources\nheko-44.png msix\nheko-44.targetsize-44_altform-unplated.png
copy .\resources\nheko-150.png msix\nheko-150.png
copy .\resources\nheko-150.png msix\nheko-150.targetsize-150_altform-unplated.png
copy .\resources\AppxManifest.xml msix
del msix\vc_redist*
::sed -i "s/ Version=[^ ]*/ Version=\"%WINVERSION%\"/" msix\AppxManifest.xml
@PowerShell "(Get-Content .\msix\AppxManifest.xml)|%%{$_ -creplace ' Version=[^ ]*',' Version=\"%WINVERSION%\"'}|Set-Content .\msix\AppxManifest.xml -Encoding utf8"

::@PowerShell "Get-Content .\msix\AppxManifest.xml"

:: Generate resource files to be able to use unplated icons
cd msix
makepri createconfig /cf priconfig.xml /dq EN-US /o
makepri new /pr . /cf priconfig.xml /of resources.pri /o
cd ..

:: Build the msix
makeappx pack /o /d msix /p nheko.msix


