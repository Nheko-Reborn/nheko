# Blurhash encoder/decoder for C++

Simple encoder and decoder for [blurhashes](https://blurha.sh/). In large parts inspired by the [reference implementation](https://github.com/woltapp/blurhash).

## Build Requirements

- A C++17 compiler, specifically with support for parsing integers via `std::from_chars` and some other smaller features.
- The meson build system, if you don't want to embed the library into your project.

## Usage as a library

Just add `blurhash.h` and `blurhash.cpp` to your project. Use `blurhash::encode` for encoding and `blurhash::decode` for decoding.

## Usage from the command line

After building, run `blurhash` for creating a hash and `blurhash2bmp` for decoding a hash. You need to specify the intended components for encoding and the intended dimensions and file name for decoding.

## Attributions

Projects that made this project possible:

- The [blurhash project](https://github.com/woltapp/blurhash) for creating and documenting the algorithm and reference implementations.
- The [stb project](https://github.com/nothings/stb) for creating the image encoder and decoder used in the command line tools.
- [Doctest](https://github.com/onqtam/doctest) for providing the easy to use testing framework.
