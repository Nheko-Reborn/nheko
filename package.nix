{ pkgs, stdenv, qtbase, qttranslations, qtmultimedia, lmdb, mkDerivation, cmake }:
stdenv.mkDerivation rec {
  version = "0.1.0";
  name = "nheko-${version}";
  src = builtins.filterSource
    (path: type:
      let name = baseNameOf path;
      in !((type == "directory" && (name == ".git" || name == "build")) ||
           (type == "symlink" && name == "result") ))
    ./.;
  nativeBuildInputs = [ cmake ];
  buildInputs = [ qtbase qttranslations qtmultimedia lmdb ];
  installPhase = ''
    mkdir -p $out/bin
    cp nheko $out/bin/nheko
  '';
}

