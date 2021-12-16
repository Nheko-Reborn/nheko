set QT_DIR=C:\Qt\5.15.2\msvc2019_64
set Qt5_DIR=%QT_DIR%\lib\cmake\Qt5
set PATH=%PATH:C:\cygwin64\bin;=%
set PATH=C:\ProgramData\chocolatey\bin;C:\Python310;%QT_DIR%\bin;%PATH%
call "C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" x64

dir \
dir \Qt\
dir \Qt\5.15.2\
dir \Qt\5.15.2\msvc2019_64\
dir \Qt\5.15.2\msvc2019_64\bin

REM Build nheko
pip install --upgrade meson || exit /b

meson setup builddir || exit /b
meson compile -C builddir || exit /b

REM build qt-jdenticon
git clone https://github.com/Nheko-Reborn/qt-jdenticon.git || exit /b
cd qt-jdenticon
qmake || exit /b
nmake || exit /b
cd ..

