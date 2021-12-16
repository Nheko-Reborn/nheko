set QT_DIR=C:\Qt\5.15.2\msvc2019_64
set Qt5_DIR=%QT_DIR%\lib\cmake\Qt5
set PATH=%PATH:C:\cygwin64\bin;=%
set PATH=C:\ProgramData\chocolatey\bin;C:\Python310;%QT_DIR%\bin;%PATH%
call "C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" x64

REM Build nheko
meson setup builddir
meson compile -C builddir

REM build qt-jdenticon
git clone https://github.com/Nheko-Reborn/qt-jdenticon.git
cd qt-jdenticon
qmake
nmake
cd ..

