cmake_minimum_required(VERSION 3.11)
project(NHEKO_DEPS)

# Point CMake at any custom modules we may ship
list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

set(DEPS_INSTALL_DIR "${CMAKE_BINARY_DIR}/usr"
    CACHE PATH "Dependencies install directory.")
set(DEPS_BIN_DIR "${DEPS_INSTALL_DIR}/bin"
    CACHE PATH "Dependencies binary install directory.")
set(DEPS_LIB_DIR "${DEPS_INSTALL_DIR}/lib"
    CACHE PATH "Dependencies library install directory.")
set(DEPS_BUILD_DIR "${CMAKE_BINARY_DIR}/build"
    CACHE PATH "Dependencies build directory.")
set(DEPS_DOWNLOAD_DIR "${DEPS_BUILD_DIR}/downloads"
    CACHE PATH "Dependencies download directory.")

option(USE_BUNDLED "Use bundled dependencies." ON)

option(USE_BUNDLED_BOOST "Use the bundled version of Boost." ${USE_BUNDLED})
option(USE_BUNDLED_CMARK "Use the bundled version of cmark." ${USE_BUNDLED})
option(USE_BUNDLED_SPDLOG "Use the bundled version of spdlog." ${USE_BUNDLED})
option(USE_BUNDLED_OLM "Use the bundled version of libolm." ${USE_BUNDLED})
option(USE_BUNDLED_TWEENY "Use the bundled version of Tweeny." ${USE_BUNDLED})
option(USE_BUNDLED_LMDBXX "Use the bundled version of lmdbxx." ${USE_BUNDLED})
option(USE_BUNDLED_MATRIX_CLIENT "Use the bundled version of mtxclient."
       ${USE_BUNDLED})
option(USE_BUNDLED_JSON "Use the bundled version of nlohmann json." ${USE_BUNDLED})
option(MTX_STATIC "Compile / link bundled mtx client statically" OFF)

if(USE_BUNDLED_BOOST)
  # bundled boost is 1.70, which requires CMake 3.15 or greater.
  cmake_minimum_required(VERSION 3.15)
endif()

include(ExternalProject)

set(BOOST_URL
    https://dl.bintray.com/boostorg/release/1.70.0/source/boost_1_70_0.tar.bz2)
set(BOOST_SHA256
    430ae8354789de4fd19ee52f3b1f739e1fba576f0aded0897c3c2bc00fb38778)

set(
  MTXCLIENT_URL
  https://github.com/Nheko-Reborn/mtxclient/archive/84c6778cc367bca79755e73e77b2cc69950375b2.zip
  )
set(MTXCLIENT_HASH
    009fc9628cbdb94694eff3bba3fe54415e432385200c886bb1c1182bcf290c0a)
set(
  TWEENY_URL
  https://github.com/mobius3/tweeny/archive/b94ce07cfb02a0eb8ac8aaf66137dabdaea857cf.tar.gz
  )
set(TWEENY_HASH
    9a632b9da84823fae002ad5d9ba02c8d77c0a3810479974c6b637c5504165475)
set(
  LMDBXX_HEADER_URL
  https://raw.githubusercontent.com/bendiken/lmdbxx/0b43ca87d8cfabba392dfe884eb1edb83874de02/lmdb%2B%2B.h
  )
set(LMDBXX_HASH
    c57b501a4e8fa1187fa7fd348da415c7685a50a7cb25b17b3f257b9e9426f73d)

set(OLM_URL https://gitlab.matrix.org/matrix-org/olm.git)
set(OLM_TAG 4065c8e11a33ba41133a086ed3de4da94dcb6bae)

set(CMARK_URL https://github.com/commonmark/cmark/archive/0.28.3.tar.gz)
set(CMARK_HASH acc98685d3c1b515ff787ac7c994188dadaf28a2d700c10c1221da4199bae1fc)

set(SPDLOG_URL https://github.com/gabime/spdlog/archive/v1.1.0.tar.gz)
set(SPDLOG_HASH
    3dbcbfd8c07e25f5e0d662b194d3a7772ef214358c49ada23c044c4747ce8b19)

set(JSON_URL
    https://github.com/nlohmann/json.git)
set(JSON_TAG
    v3.2.0)

if(USE_BUNDLED_JSON)
  include(Json)
endif()

if(USE_BUNDLED_BOOST)
  include(Boost)
endif()

if(USE_BUNDLED_SPDLOG)
  include(SpdLog)
endif()

if(USE_BUNDLED_OLM)
  include(Olm)
endif()

if(USE_BUNDLED_CMARK)
  include(cmark)
endif()

if(USE_BUNDLED_TWEENY)
  include(Tweeny)
endif()

if(USE_BUNDLED_LMDBXX)
  file(DOWNLOAD ${LMDBXX_HEADER_URL} ${DEPS_INSTALL_DIR}/include/lmdb++.h
       EXPECTED_HASH SHA256=${LMDBXX_HASH})
endif()

if(WIN32)
  if("${TARGET_ARCH}" STREQUAL "X86_64")
    set(TARGET_ARCH x64)
  elseif(TARGET_ARCH STREQUAL "X86")
    set(TARGET_ARCH ia32)
  endif()
endif()

add_custom_target(third-party ALL
                  COMMAND ${CMAKE_COMMAND} -E touch .third-party
                  DEPENDS ${THIRD_PARTY_DEPS})

if(USE_BUNDLED_MATRIX_CLIENT)
  include(MatrixClient)
  add_dependencies(MatrixClient third-party)
endif()
