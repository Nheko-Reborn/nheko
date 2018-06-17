set(WINDOWS_FLAGS "")

if(MSVC)
    set(WINDOWS_FLAGS "-DCMAKE_GENERATOR_PLATFORM=x64")
endif()

ExternalProject_Add(
  MatrixStructs

  DOWNLOAD_DIR ${DEPS_DOWNLOAD_DIR}/matrix_structs
  GIT_REPOSITORY ${MATRIX_STRUCTS_URL}
  GIT_TAG ${MATRIX_STRUCTS_TAG}

  BUILD_IN_SOURCE 1
  SOURCE_DIR ${DEPS_BUILD_DIR}/matrix_structs
  CONFIGURE_COMMAND ${CMAKE_COMMAND} 
        -DCMAKE_INSTALL_PREFIX=${DEPS_INSTALL_DIR}
        -DCMAKE_BUILD_TYPE=Release 
        ${DEPS_BUILD_DIR}/matrix_structs
        ${WINDOWS_FLAGS}
  BUILD_COMMAND ${CMAKE_COMMAND} 
        --build ${DEPS_BUILD_DIR}/matrix_structs 
        --config Release)

list(APPEND THIRD_PARTY_DEPS MatrixStructs)
