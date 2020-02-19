nheko
----
[![Build Status](https://travis-ci.org/Nheko-Reborn/nheko.svg?branch=master)](https://travis-ci.org/Nheko-Reborn/nheko)
[![Build status](https://ci.appveyor.com/api/projects/status/07qrqbfylsg4hw2h/branch/master?svg=true)](https://ci.appveyor.com/project/redsky17/nheko/branch/master)
[![Stable Version](https://img.shields.io/badge/download-stable-green.svg)](https://github.com/Nheko-Reborn/nheko/releases/v0.6.4)
[![Nightly](https://img.shields.io/badge/download-nightly-green.svg)](https://nheko-reborn-artifacts.s3.us-east-2.amazonaws.com/list.html)
[![#nheko-reborn:matrix.org](https://img.shields.io/matrix/nheko-reborn:matrix.org.svg?label=%23nheko-reborn:matrix.org)](https://matrix.to/#/#nheko-reborn:matrix.org)
[![AUR: nheko](https://img.shields.io/badge/AUR-nheko-blue.svg)](https://aur.archlinux.org/packages/nheko)
<a href='https://flathub.org/apps/details/io.github.NhekoReborn.Nheko'><img width='240' alt='Download on Flathub' src='https://flathub.org/assets/badges/flathub-badge-en.png'/></a>

The motivation behind the project is to provide a native desktop app for [Matrix] that
feels more like a mainstream chat app ([Riot], Telegram etc) and less like an IRC client.

### Translations ###
[![Translation status](http://weblate.nheko.im/widgets/nheko/-/nheko-master/svg-badge.svg)](http://weblate.nheko.im/engage/nheko/?utm_source=widget)

Help us with translations so as many people as possible will be able to use nheko!

### Note regarding End-to-End encryption

Currently the implementation is at best a **proof of concept** and it should only be used for
testing purposes. Most importantly, it is missing device verification, so while your messages
and media are encrypted, nheko doesn't verify who gets the messages.

## Features

Most of the features you would expect from a chat application are missing right now
but we are getting close to a more feature complete client.
Specifically there is support for:
- E2E encryption.
- User registration.
- Creating, joining & leaving rooms.
- Sending & receiving invites.
- Sending & receiving files and emoji (inline widgets for images, audio and file messages).
- Replies with text, images and other media (and actually render them as inline widgets).
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

Releases for Linux (AppImage), macOS (disk image) & Windows (x64 installer)
can be found in the [Github releases](https://github.com/Nheko-Reborn/nheko/releases).

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
sudo eselect repository enable matrix
sudo emerge -a nheko
```

#### Alpine Linux (and postmarketOS)

Make sure you have the testing repositories from `edge` enabled. Note that this is not needed on postmarketOS.

```sh
sudo apk add nheko
```

#### Flatpak

```
flatpak install flathub io.github.NhekoReborn.Nheko
```

#### macOS (10.14 and above)

with [macports](https://www.macports.org/) :

```sh
sudo port install nheko
```

### Build Requirements

- Qt5 (5.10 or greater). Qt 5.7 adds support for color font rendering with
  Freetype, which is essential to properly support emoji, 5.8 adds some features
  to make interopability with Qml easier, 5.10 makes sliders actually visible with different palettes.
- CMake 3.15 or greater. (Lower version may work, but may break boost linking)
- [mtxclient](https://github.com/Nheko-Reborn/mtxclient)
- [LMDB](https://symas.com/lightning-memory-mapped-database/)
- [cmark](https://github.com/commonmark/cmark) 0.29 or greater.
- Boost 1.70 or greater.
- [libolm](https://git.matrix.org/git/olm)
- [libsodium](https://github.com/jedisct1/libsodium)
- [spdlog](https://github.com/gabime/spdlog)
- A compiler that supports C++ 17:
    - Clang 6 (tested on Travis CI)
    - GCC 7 (tested on Travis CI)
    - MSVC 19.13 (tested on AppVeyor)

Nheko can use bundled version for most of those libraries automatically, if the versions in your distro are too old.
To use them, you can enable the hunter integration by passing `-DHUNTER_ENABLED=ON`.
It is probably wise to link those dependencies statically by passing `-DBUILD_SHARED_LIBS=OFF`
You can select which bundled dependencies you want to use py passing various `-DUSE_BUNDLED_*` flags. By default all dependencies are bundled *if* you enable hunter.

The bundle flags are currently:

- USE_BUNDLED_BOOST
- USE_BUNDLED_SPDLOG
- USE_BUNDLED_OLM
- USE_BUNDLED_GTEST
- USE_BUNDLED_CMARK
- USE_BUNDLED_JSON
- USE_BUNDLED_OPENSSL
- USE_BUNDLED_MTXCLIENT
- USE_BUNDLED_SODIUM
- USE_BUNDLED_ZLIB
- USE_BUNDLED_LMDB
- USE_BUNDLED_LMDBXX
- USE_BUNDLED_TWEENY

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
    cmark \
    boost \
    libsodium
```

##### Gentoo Linux

```bash
sudo emerge -a ">=dev-qt/qtgui-5.9.0" media-libs/fontconfig
```

##### Ubuntu 16.04

```bash
sudo add-apt-repository ppa:beineri/opt-qt592-xenial
sudo add-apt-repository ppa:george-edison55/cmake-3.x
sudo add-apt-repository ppa:ubuntu-toolchain-r-test
sudo apt-get update
sudo apt-get install -y g++-7 qt59base qt59svg qt59tools qt59multimedia cmake liblmdb-dev libsodium-dev
```

##### Ubuntu 19.10

```bash
# Build requirements + qml modules needed at runtime (you may not need all of them, but the following seem to work according to reports):
sudo apt install g++-7 cmake liblmdb-dev libsodium-dev qt{base,tools,multimedia}5-dev qml-module-qt{gstreamer,multimedia,quick-extras} libqt5svg5-dev qt{script,quickcontrols2-}5-dev
```

##### Debian Buster (or higher probably)

(User report, not sure if all of those are needed)

```bash
sudo apt install cmake gcc make automake liblmdb-dev libsodium-dev \
    qt5-default libssl-dev libqt5multimedia5-plugins libqt5multimediagsttools5 libqt5multimediaquick5 libqt5svg5-dev \
    qml-module-qtgstreamer qtmultimedia5-dev qtquickcontrols2-5-dev qttools5-dev qttools5-dev-tools \
    qml-module-qtgraphicaleffects qml-module-qtmultimedia qml-module-qtquick-controls2 qml-module-qtquick-layouts
```

##### macOS (Xcode 10.2 or later)

```bash
brew update
brew install qt5 lmdb cmake llvm libsodium spdlog boost cmark
```

##### Windows

1. Install Visual Studio 2017's "Desktop Development" and "Linux Development with C++"
(for the CMake integration) workloads.

2. Download the latest Qt for windows installer and install it somewhere.
Make sure to install the `MSVC 2017 64-bit` toolset for at least Qt 5.10
(lower versions does not support VS2017).

3. If you don't have openssl installed, you will need to install perl to build it (i.e. Strawberry Perl).

### Building

We can now build nheko:

```bash
cmake -H. -Bbuild -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

To use bundled dependencies you can use hunter, i.e.:

```bash
cmake -H. -Bbuild -DHUNTER_ENABLED=ON -DBUILD_SHARED_LIBS=OFF -DUSE_BUNDLED_OPENSSL=OFF
cmake --build build --config Release
```

Adapt the USE_BUNDLED_* as needed.

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

#### Windows

After installing all dependencies, you need to edit the `CMakeSettings.json` to
be able to load and compile nheko within Visual Studio.

You need to fill out the paths for the `Qt5_DIR`.
The Qt5 dir should point to the `lib\cmake\Qt5` dir.

Examples for the paths are:
 - `C:\\Qt\\5.10.1\\msvc2017_64\\lib\\cmake\\Qt5`

You should also enable hunter by setting `HUNTER_ENABLED` to `ON` and `BUILD_SHARED_LIBS` to `OFF`.

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

Also copy the respective cmark.dll to the binary dir from `build/cmark-build/src/Release` (or Debug).


### Contributing

See [CONTRIBUTING](.github/CONTRIBUTING.md)

### Screens

Here are some screen shots to get a feel for the UI, but things will probably change.

![nheko start](https://nheko-reborn.github.io/images/screenshots/Start.png)
![nheko login](https://nheko-reborn.github.io/images/screenshots/login.png)
![nheko chat](https://nheko-reborn.github.io/images/screenshots/chat.png)
![nheko settings](https://nheko-reborn.github.io/images/screenshots/settings.png)

[Matrix]:https://matrix.org
[Riot]:https://riot.im
