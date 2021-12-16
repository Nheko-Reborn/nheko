set QT_DIR="C:\Qt\5.15.2\msvc2019_64"
set Qt5_DIR="%QT_DIR%\lib\cmake\Qt5"
set PATH=%PATH:C:\cygwin64\bin;=%
set PATH=C:\Python310;%QT_DIR%\bin;%PATH%
call "C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" x64

@REM set BUILD=%CI_BUILDS_DIR%
echo %BUILD%
mkdir NhekoRelease
copy builddir\Release\nheko.exe NhekoRelease\nheko.exe
copy qt-jdenticon\release\qtjdenticon0.dll NhekoRelease\qtjdenticon.dll
windeployqt --qmldir resources\qml\ NhekoRelease\nheko.exe

7z a nheko_win_64.zip .\NhekoRelease\*
ls -lh build\Release\
ls -lh NhekoRelease\
mkdir NhekoData
xcopy .\NhekoRelease\*.* NhekoData\*.* /s /e /c /y

REM Create the Qt Installer Framework version

mkdir installer
mkdir installer\config
mkdir installer\packages
mkdir installer\packages\io.github.nhekoreborn.nheko
mkdir installer\packages\io.github.nhekoreborn.nheko\data
mkdir installer\packages\io.github.nhekoreborn.nheko\meta

REM Copy installer data
copy %BUILD%\resources\nheko.ico installer\config
copy %BUILD%\resources\nheko.png installer\config
copy %BUILD%\COPYING installer\packages\io.github.nhekoreborn.nheko\meta\license.txt
copy %BUILD%\deploy\installer\config.xml installer\config
copy %BUILD%\deploy\installer\controlscript.qs installer\config
copy %BUILD%\deploy\installer\uninstall.qs installer\packages\io.github.nhekoreborn.nheko\data
copy %BUILD%\deploy\installer\gui\package.xml installer\packages\io.github.nhekoreborn.nheko\meta
copy %BUILD%\deploy\installer\gui\installscript.qs installer\packages\io.github.nhekoreborn.nheko\meta

REM Amend version and date
sed -i "s/__VERSION__/0.9.0/" installer\config\config.xml
sed -i "s/__VERSION__/0.9.0/" installer\packages\io.github.nhekoreborn.nheko\meta\package.xml
sed -i "s/__DATE__/%DATE%/" installer\packages\io.github.nhekoreborn.nheko\meta\package.xml

REM Copy nheko data
xcopy NhekoData\*.* installer\packages\io.github.nhekoreborn.nheko\data\*.* /s /e /c /y
move NhekoRelease\nheko.exe installer\packages\io.github.nhekoreborn.nheko\data
mkdir tools

REM curl -L -O https://download.qt.io/official_releases/qt-installer-framework/3.0.4/QtInstallerFramework-win-x86.exe
REM Since the official download.qt.io is down atm
curl -L -O https://qt-mirror.dannhauer.de/official_releases/qt-installer-framework/4.0.1/QtInstallerFramework-win-x86.exe
7z x QtInstallerFramework-win-x86.exe -otools -aoa
set PATH=%BUILD%\tools\bin;%PATH%
binarycreator.exe -f -c installer\config\config.xml -p installer\packages nheko-installer.exe

copy nheko-installer.exe nheko-%APPVEYOR_REPO_TAG_NAME%-installer.exe
copy nheko-installer.exe nheko-%APPVEYOR_PULL_REQUEST_HEAD_COMMIT%-installer.exe

