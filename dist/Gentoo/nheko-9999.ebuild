# Copyright 1999-2017 Gentoo Foundation
# Distributed under the terms of the GNU General Public License v2

EAPI=6

DESCRIPTION="Desktop client for the Matrix protocol"
HOMEPAGE="https://github.com/mujx/nheko"

inherit git-r3 eutils cmake-utils

if [[ ${PV} == "9999" ]]; then
	SRC_URI=""
	EGIT_REPO_URI="git://github.com/mujx/nheko.git"
fi

LICENSE="GPL-3"
SLOT="0"
IUSE=""

DEPEND=">=dev-qt/qtgui-5.7.1
		media-libs/fontconfig"
RDEPEND="${DEPEND}"

src_configure() {
	cmake-utils_src_configure
}

src_compile() {
	emake DESTDIR="${D}"
}

src_install() {
	local icon_size
    for icon_size in 16 32 48 64 128 256 512; do
        newicon -s "${icon_size}" \
			"${S}/resources/nheko-${icon_size}.png" \
			nheko.png
    done

	domenu ${S}/resources/nheko.desktop

	dobin ${S}/build/nheko
}
