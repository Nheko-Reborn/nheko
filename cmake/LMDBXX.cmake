include(ExternalProject)

#
# Build lmdbxx.
#

set(THIRD_PARTY_ROOT ${CMAKE_SOURCE_DIR}/.third-party)
set(LMDBXX_ROOT ${THIRD_PARTY_ROOT}/lmdbxx)

set(LMDBXX_INCLUDE_DIRS ${LMDBXX_ROOT})

ExternalProject_Add(
  lmdbxx

  GIT_REPOSITORY https://github.com/bendiken/lmdbxx
  GIT_TAG 0b43ca87d8cfabba392dfe884eb1edb83874de02

  BUILD_IN_SOURCE 1
  SOURCE_DIR ${LMDBXX_ROOT}
  CONFIGURE_COMMAND ""
  BUILD_COMMAND ""
  INSTALL_COMMAND ""
)

include_directories(SYSTEM ${LMDBXX_ROOT})
