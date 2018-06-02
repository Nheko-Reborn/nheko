{ stdenv, qtbase, qttranslations, qtmultimedia, lmdb, cmake }:
stdenv.mkDerivation rec {
  version = "0.4.3";
  name = "nheko-${version}";
  src = builtins.filterSource
    (path: type:
      let name = baseNameOf path;
      in !((type == "directory" && (name == ".git" || name == "build")) ||
           (type == "symlink" && name == "result") ))
    ./.;
  nativeBuildInputs = [ cmake ];
  buildInputs = [ qtbase qttranslations qtmultimedia lmdb ];
  installPhase = "install -Dm755 nheko $out/bin/nheko";
}

