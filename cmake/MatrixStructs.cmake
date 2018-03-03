include(ExternalProject)

#
# Build matrix-structs.
#

set(THIRD_PARTY_ROOT ${CMAKE_SOURCE_DIR}/.third-party)
set(MATRIX_STRUCTS_ROOT ${THIRD_PARTY_ROOT}/matrix_structs)

set(MATRIX_STRUCTS_INCLUDE_DIRS ${MATRIX_STRUCTS_ROOT}/deps)
set(MATRIX_STRUCTS_LIBRARY
    ${MATRIX_STRUCTS_ROOT}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}matrix_structs${CMAKE_STATIC_LIBRARY_SUFFIX})

include_directories(SYSTEM ${MATRIX_STRUCTS_ROOT}/deps)
include_directories(SYSTEM ${MATRIX_STRUCTS_ROOT}/include)
link_directories(${MATRIX_STRUCTS_ROOT}/lib)

ExternalProject_Add(
  MatrixStructs

  GIT_REPOSITORY https://github.com/mujx/matrix-structs
  GIT_TAG a1beea3b115f037e26c15f22ed911341b3893411

  BUILD_IN_SOURCE 1
  SOURCE_DIR ${MATRIX_STRUCTS_ROOT}
  CONFIGURE_COMMAND ${CMAKE_COMMAND}
    -DCMAKE_BUILD_TYPE=Release ${MATRIX_STRUCTS_ROOT}
    -DCMAKE_INSTALL_PREFIX=${MATRIX_STRUCTS_ROOT}
    -Ax64
  BUILD_COMMAND ${CMAKE_COMMAND} --build ${MATRIX_STRUCTS_ROOT} --config Release
  INSTALL_COMMAND ${CMAKE_COMMAND}
    --build ${MATRIX_STRUCTS_ROOT}
    --config Release
    --target install
)
