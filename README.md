nheko
----
[![Build Status](https://travis-ci.org/mujx/nheko.svg?branch=master)](https://travis-ci.org/mujx/nheko)
[![Build status](https://ci.appveyor.com/api/projects/status/07qrqbfylsg4hw2h/branch/master?svg=true)](https://ci.appveyor.com/project/mujx/nheko/branch/master)
[![Latest Stable Version](https://img.shields.io/badge/download-stable-green.svg)](https://bintray.com/mujx/matrix/nheko/_latestVersion)
[![Nightly](https://img.shields.io/badge/download-nightly-green.svg)](https://bintray.com/mujx/matrix/nheko/nightly)
[![Chat on Matrix](https://img.shields.io/badge/chat-on%20matrix-blue.svg)](https://matrix.to/#/#nheko:matrix.org)
[![AUR: nheko](https://img.shields.io/badge/AUR-nheko-blue.svg)](https://aur.archlinux.org/packages/nheko)

The motivation behind the project is to provide a native desktop app for [Matrix] that
feels more like a mainstream chat app ([Riot], Telegram etc) and less like an IRC client.

## Features

Most of the features you would expect from a chat application are missing right now
but we are getting close to a more feature complete client.
Specifically there is support for:
- E2E encryption.
- User registration.
- Creating, joining & leaving rooms.
- Sending & receiving invites.
- Sending & receiving files and emoji (inline widgets for images, audio and file messages).
- Typing notifications.
- Username auto-completion.
- Message & mention notifications.
- Redacting messages.
- Read receipts.
- Basic communities support.
- Room switcher (ctrl-K).
- Light, Dark & System themes.

## Installation

### Releases

Releases for Linux (AppImage), macOS (disk image) & Windows (x64 installer) can be found on the [Bintray repo](https://bintray.com/mujx/matrix/nheko).

### Repositories

#### Arch Linux
```bash
pacaur -S nheko # nheko-git
```

#### Fedora
```bash
sudo dnf install nheko
```

#### Gentoo Linux
```bash
sudo layman -a matrix
sudo emerge -a nheko
```

#### Alpine Linux (and postmarketOS)

Make sure you have the testing repositories from `edge` enabled. Note that this is not needed on postmarketOS.

```sh
sudo apk add nheko
```

### Build Requirements

- Qt5 (5.7 or greater). Qt 5.7 adds support for color font rendering with
  Freetype, which is essential to properly support emoji.
- CMake 3.1 or greater.
- [mtxclient](https://github.com/mujx/mtxclient)
- [matrix-structs](https://github.com/mujx/matrix-structs)
- [LMDB](https://symas.com/lightning-memory-mapped-database/)
- Boost 1.66 or greater.
- [libolm](https://git.matrix.org/git/olm)
- [libsodium](https://github.com/jedisct1/libsodium)
- [spdlog](https://github.com/gabime/spdlog)
- A compiler that supports C++ 14:
    - Clang 5 (tested on Travis CI)
    - GCC 7 (tested on Travis CI)
    - MSVC 19.13 (tested on AppVeyor)

#### Linux 

If you don't want to install any external dependencies, you can generate an AppImage locally using docker.

```bash
make docker-app-image
```

##### Arch Linux

```bash
sudo pacman -S qt5-base \
    qt5-tools \
    qt5-multimedia \
    qt5-svg \
    cmake \
    gcc \
    fontconfig \
    lmdb \
    boost \
    libsodium
```

##### Gentoo Linux

```bash
sudo emerge -a ">=dev-qt/qtgui-5.7.1" media-libs/fontconfig
```

##### Ubuntu (e.g 14.04)

```bash
sudo add-apt-repository ppa:beineri/opt-qt592-trusty
sudo add-apt-repository ppa:george-edison55/cmake-3.x
sudo add-apt-repository ppa:ubuntu-toolchain-r-test
sudo apt-get update
sudo apt-get install -y g++-7 qt59base qt59svg qt59tools qt59multimedia cmake liblmdb-dev libsodium-dev 
```

##### macOS (Xcode 8 or later)

```bash
brew update
brew install qt5 lmdb cmake llvm libsodium spdlog boost
```

##### Windows

1. Install Visual Studio 2017's "Desktop Development" and "Linux Development with C++"
(for the CMake integration) workloads.

2. Download the latest Qt for windows installer and install it somewhere.
Make sure to install the `MSVC 2017 64-bit` toolset for at least Qt 5.9
(lower versions does not support VS2017).

3. Install lmdb and openssl with `vcpkg`. You can simply clone it into a subfolder
of the root nheko source directory.

```powershell
git clone http:\\github.com\Microsoft\vcpkg
cd vcpkg
.\bootstrap-vcpkg.bat
.\vcpkg install --triplet x64-windows lmdb openssl
```

### Building

First we need to install the rest of the dependencies that are not available in our system

```bash
cmake -Hdeps -B.deps \
    -DUSE_BUNDLED_BOOST=OFF # if we already have boost & spdlog installed.
    -DUSE_BUNDLED_SPDLOG=OFF
cmake --build .deps
```

We can now build nheko by pointing it to the path that we installed the dependencies.

```bash
cmake -H. -Bbuild -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=.deps/usr
cmake --build build
```

If the build fails with the following error
```
Could not find a package configuration file provided by "Qt5Widgets" with
any of the following names:

Qt5WidgetsConfig.cmake
qt5widgets-config.cmake
```
You might need to pass `-DCMAKE_PREFIX_PATH` to cmake to point it at your qt5 install.

e.g on macOS

```
cmake -H. -Bbuild -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=$(brew --prefix qt5)
cmake --build build
```

The `nheko` binary will be located in the `build` directory.

#### Nix

Download the repo as mentioned above and run

```bash
nix-build
```

in the project folder. This will output a binary to `result/bin/nheko`.

You can also install nheko by running `nix-env -f . -i`

#### Windows

After installing all dependencies, you need to edit the `CMakeSettings.json` to
be able to load and compile nheko within Visual Studio.

You need to fill out the paths for the `CMAKE_TOOLCHAIN_FILE` and the `Qt5_DIR`.
The toolchain file should point to the `vcpkg.cmake` and the Qt5 dir to the `lib\cmake\Qt5` dir.

Examples for the paths are:
 - `C:\\vcpkg\\scripts\\buildsystems\\vcpkg.cmake`
 - `C:\\Qt\\5.10.1\\msvc2017_64\\lib\\cmake\\Qt5`

Now right click into the root nheko source directory and choose `Open in Visual Studio`.
You can choose the build type Release and Debug in the top toolbar. 
After a successful CMake generation you can select the `nheko.exe` as the run target.
Now choose `Build all` in the CMake menu or press `F7` to compile the executable.

To be able to run the application the last step is to install the needed Qt dependencies next to the
nheko binary.

Start the "Qt x.xx.x 64-bit for Desktop (MSVC 2017)" command promt and run `windeployqt`.
```cmd
cd <path-to-nheko>\build-vc\Release\Release
windeployqt nheko.exe
```

The final binary will be located inside `build-vc\Release\Release` for the Release build
and `build-vc\Debug\Debug` for the Debug build.

### Contributing

See [CONTRIBUTING](.github/CONTRIBUTING.md)

### Screens

Here is a screen shot to get a feel for the UI, but things will probably change.

![nheko](https://dl.dropboxusercontent.com/s/3zjcezdtk8kqe4i/nheko-v0.4.0.png)

### Third party

- [Emoji One](http://emojione.com)
- [Font Awesome](http://fontawesome.io/)
- [Open Sans](https://fonts.google.com/specimen/Open+Sans)

[Matrix]:https://matrix.org
[Riot]:https://riot.im
