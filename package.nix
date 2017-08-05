{ pkgs, stdenv, qtbase, qttranslations, lmdb, mkDerivation, cmake }:
stdenv.mkDerivation rec {
  version = "0.1.0";
  name = "nheko-${version}";
  src = ./.;
  nativeBuildInputs = [ cmake ];
  buildInputs = [ qtbase qttranslations lmdb ];
  installPhase = ''
    mkdir -p $out/bin
    cp nheko $out/bin/nheko
  '';
}

