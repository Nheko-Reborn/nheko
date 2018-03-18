include(ExternalProject)

#
# Build matrix-structs.
#

set(THIRD_PARTY_ROOT ${CMAKE_SOURCE_DIR}/.third-party)
set(MATRIX_STRUCTS_ROOT ${THIRD_PARTY_ROOT}/matrix_structs)

set(MATRIX_STRUCTS_INCLUDE_DIRS ${MATRIX_STRUCTS_ROOT}/deps)

include_directories(SYSTEM ${MATRIX_STRUCTS_ROOT}/deps)
include_directories(SYSTEM ${MATRIX_STRUCTS_ROOT}/include)
link_directories(${MATRIX_STRUCTS_ROOT}/lib)

set(WINDOWS_FLAGS "")

if(MSVC)
    set(WINDOWS_FLAGS "-DCMAKE_GENERATOR_PLATFORM=x64")
endif()

ExternalProject_Add(
  MatrixStructs

  GIT_REPOSITORY https://github.com/mujx/matrix-structs
  GIT_TAG e503c4ff27cd8159c377d9a7d2e6e91698a5517b

  BUILD_IN_SOURCE 1
  SOURCE_DIR ${MATRIX_STRUCTS_ROOT}
  CONFIGURE_COMMAND ${CMAKE_COMMAND}
    -DCMAKE_BUILD_TYPE=Release ${MATRIX_STRUCTS_ROOT}
    -DCMAKE_INSTALL_PREFIX=${MATRIX_STRUCTS_ROOT}
    ${WINDOWS_FLAGS}
  BUILD_COMMAND ${CMAKE_COMMAND} --build ${MATRIX_STRUCTS_ROOT} --config Release
  INSTALL_COMMAND ${CMAKE_COMMAND}
    --build ${MATRIX_STRUCTS_ROOT}
    --config Release
    --target install
)
