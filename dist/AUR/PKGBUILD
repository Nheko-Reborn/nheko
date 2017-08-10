# Maintainer: Konstantinos Sideris <siderisk at auth dot gr>

pkgname=nheko-git
pkgver=0.1.0.r174.c428ef4b
pkgrel=1
pkgdesc="Desktop client for the Matrix protocol"
arch=("i686" "x86_64")

url="https://github.com/mujx/nheko"
license=("GPL3")

depends=("qt5-base" "lmdb")
makedepends=("git" "cmake" "gcc" "fontconfig" "qt5-tools")

source=($pkgname::git://github.com/mujx/nheko.git git://github.com/bendiken/lmdbxx.git)
md5sums=("SKIP" "SKIP")

prepare() {
  cd "$pkgname"
  git submodule init
  git config submodule.lmdbxx.url $srcdir/libs/lmdbxx
  git submodule update
}

pkgver() {
    cd "$pkgname"
    printf "0.1.0.r%s.%s" "$(git rev-list --count HEAD)" "$(git rev-parse --short HEAD)"
}

build() {
    cd "$pkgname"
    cmake -H. -Bbuild -DCMAKE_BUILD_TYPE=Release
    make -C build -j2
}

package() {
    # Creating needed directories
    install -dm755 "$pkgdir/usr/bin"
    install -dm755 "$pkgdir/usr/share/pixmaps/"
    install -dm755 "$pkgdir/usr/share/applications/"

    # Program
    install -Dm755 "$pkgname/build/nheko" "$pkgdir/usr/bin/nheko"

    # Desktop launcher
    install -Dm644 "$srcdir/$pkgname/resources/nheko-256.png" "$pkgdir/usr/share/pixmaps/nheko.png"
    install -Dm644 "$srcdir/$pkgname/resources/nheko.desktop" "$pkgdir/usr/share/applications/nheko.desktop"

    # Icons
    local icon_size icon_dir
    for icon_size in 16 32 48 64 128 256 512; do
        icon_dir="$pkgdir/usr/share/icons/hicolor/${icon_size}x${icon_size}/apps"
        install -d "$icon_dir"
        install -m644 "$srcdir/$pkgname/resources/nheko-${icon_size}.png" "$icon_dir/nheko.png"
    done
}
