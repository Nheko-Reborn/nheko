include(ExternalProject)

#
# Build tweeny
#

set(THIRD_PARTY_ROOT ${CMAKE_SOURCE_DIR}/.third-party)
set(TWEENY_ROOT ${THIRD_PARTY_ROOT}/tweeny)

set(TWEENY_INCLUDE_DIRS ${TWEENY_ROOT}/include)

ExternalProject_Add(
  Tweeny

  GIT_REPOSITORY https://github.com/mobius3/tweeny
  GIT_TAG b94ce07cfb02a0eb8ac8aaf66137dabdaea857cf

  BUILD_IN_SOURCE 1
  SOURCE_DIR ${TWEENY_ROOT}
  CONFIGURE_COMMAND ""
  BUILD_COMMAND ""
  INSTALL_COMMAND ""
)

include_directories(SYSTEM ${TWEENY_ROOT}/include)
