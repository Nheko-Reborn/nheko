nheko
----
[![Build Status](https://travis-ci.org/mujx/nheko.svg?branch=master)](https://travis-ci.org/mujx/nheko)
[![Build status](https://ci.appveyor.com/api/projects/status/07qrqbfylsg4hw2h/branch/master?svg=true)](https://ci.appveyor.com/project/mujx/nheko/branch/master)
[![Chat on Matrix](https://img.shields.io/badge/chat-on%20matrix-blue.svg)](https://matrix.to/#/#nheko:matrix.org)
[![License: GPL v3](https://img.shields.io/badge/license-GPL%20v3-red.svg)](https://www.gnu.org/licenses/gpl-3.0)
[![AUR: nheko-git](https://img.shields.io/badge/AUR-nheko--git-blue.svg)](https://aur.archlinux.org/packages/nheko-git)
[![Snap Status](https://build.snapcraft.io/badge/mujx/nheko.svg)](https://build.snapcraft.io/user/mujx/nheko)

The motivation behind the project is to provide a native desktop app for [Matrix] that
feels more like a mainstream chat app ([Riot], Telegram etc) and less like an IRC client.

### Features

Most of the features you would expect from a chat application are missing right now
but we are getting close to a more feature complete client.
Specifically there is support for:
- Joining & leaving rooms
- Sending & receiving images and emoji.
- Receiving typing notifications.

### Installation

#### Arch Linux
```bash
pacaur -S nheko-git
```

#### Fedora
```bash
sudo dnf copr enable xvitaly/matrix
sudo dnf install nheko
```

#### Gentoo Linux
```bash
sudo layman -a matrix
sudo emerge -a nheko
```

#### Windows

You can find an installer [here](https://ci.appveyor.com/project/mujx/nheko/branch/master/artifacts).

### Build Requirements

- Qt5 (5.7 or greater). Qt 5.7 adds support for color font rendering with
  Freetype, which is essential to properly support emoji.
- CMake 3.1 or greater.
- [LMDB](https://symas.com/lightning-memory-mapped-database/).
- A compiler that supports C++11.
    - Clang 3.3 (or greater).
    - GCC 4.8 (or greater).

##### Arch Linux

```bash
sudo pacman -S qt5-base qt5-tools cmake gcc fontconfig lmdb
```

##### Gentoo Linux

```bash
sudo emerge -a ">=dev-qt/qtgui-5.7.1" media-libs/fontconfig
```

##### Ubuntu (e.g 14.04)

```bash
sudo add-apt-repository ppa:beineri/opt-qt58-trusty
sudo add-apt-repository ppa:george-edison55/cmake-3.x
sudo apt-get update
sudo apt-get install qt58base qt58tools cmake liblmdb-dev
```

##### OSX (Xcode 8 or later)

```bash
brew update
brew install qt5 lmdb
```

N.B. you will need to pass `-DCMAKE_PREFIX_PATH=/usr/local/opt/qt5`
to cmake to point it at your qt5 install (tweaking the path as needed)

### Building

Clone the repo with its submodules

```bash
git clone --recursive https://github.com/mujx/nheko
```
or 
```bash
git clone https://github.com/mujx/nheko
cd nheko
git submodule update --init
```

and then use the following

```bash
cmake -H. -Bbuild -DCMAKE_BUILD_TYPE=Release # Default is Debug.
make -C build
```

The `nheko` binary will be located in the `build` directory.

##### MacOS

You can create an app bundle with `make app`. The output will be located at
`dist/MacOS/Nheko.app` which can be copied to `/Applications/Nheko.app`.

You can also create a disk image with `make dmg`. The output will be located at
`dist/MacOS/Nheko.dmg`

##### Nix

Download the repo as mentioned above and run

```bash
nix-build
```

in the project folder. This will output a binary to `result/bin/nheko`.

You can also install nheko by running `nix-env -f . -i`

### Contributing

Any kind of contribution to the project is greatly appreciated. You are also
encouraged to open feature request issues.

### Screens

Here is a screen shot to get a feel for the UI, but things will probably change.

![nheko](https://dl.dropboxusercontent.com/s/5iydk5r3b9zyycd/nheko-ui.png)

### Third party

- [Emoji One](http://emojione.com)
- [Font Awesome](http://fontawesome.io/)
- [Open Sans](https://fonts.google.com/specimen/Open+Sans)

[Matrix]:https://matrix.org
[Riot]:https://riot.im
