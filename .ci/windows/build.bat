:: VERSION format:     v1.2.3/v1.3.4
:: INSTVERSION format: 1.2.3/1.3.4
:: WINVERSION format:  1.2.3.123/1.3.4.234
if defined CI_COMMIT_TAG (
	set VERSION=%CI_COMMIT_TAG%
) else (
	set VERSION=v0.11.3
)
set INSTVERSION=%VERSION:~1%
set WINVERSION=%VERSION:~1%.%CI_JOB_ID%
set DATE=%date:~10,4%-%date:~4,2%-%date:~7,2%
echo %VERSION%
echo %INSTVERSION%
echo %DATE%


call "C:/Program Files (x86)/Microsoft Visual Studio/2022/BuildTools/VC/Auxiliary/Build/vcvarsall.bat" x64
cmake -G "Visual Studio 17 2022" -A x64 -S. -Bbuild -DHUNTER_ROOT="C:\hunter" -DHUNTER_ENABLED=ON -DBUILD_SHARED_LIBS=OFF -DUSE_BUNDLED_OPENSSL=ON -DUSE_BUNDLED_KDSINGLEAPPLICATION=ON -DKDSingleApplication_STATIC=ON -DCMAKE_BUILD_TYPE=Release -DHUNTER_CONFIGURATION_TYPES=Release
cmake --build build --config Release


git clone https://github.com/Nheko-Reborn/qt-jdenticon.git
cd qt-jdenticon
qmake
nmake
cd ..

:: create zip bundle
mkdir NhekoRelease
copy build\Release\nheko.exe NhekoRelease\nheko.exe
copy qt-jdenticon\release\qtjdenticon0.dll NhekoRelease\qtjdenticon.dll
copy build\_deps\cmark-build\src\Release\cmark.dll NhekoRelease\cmark.dll
windeployqt --qmldir resources\qml\ NhekoRelease\nheko.exe

7z a nheko_win_64.zip .\NhekoRelease\*


:: create msix
mkdir msix
xcopy .\NhekoRelease\*.* msix\*.* /s /e /c /y
copy .\resources\nheko.png msix
copy .\resources\AppxManifest.xml msix
del msix\vc_redist*
::sed -i "s/ Version=[^ ]*/ Version=\"%WINVERSION%\"/" msix\AppxManifest.xml
@PowerShell "(Get-Content .\msix\AppxManifest.xml)|%%{$_ -creplace ' Version=[^ ]*',' Version=\"%WINVERSION%\"'}|Set-Content .\msix\AppxManifest.xml -Encoding utf8"

::@PowerShell "Get-Content .\msix\AppxManifest.xml"

"C:\Program Files (x86)\Windows Kits\10\App Certification Kit\makeappx.exe" pack -d msix -p nheko.msix

