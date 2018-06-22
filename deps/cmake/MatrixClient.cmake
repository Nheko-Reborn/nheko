set(PLATFORM_FLAGS "")

if(MSVC)
    set(PLATFORM_FLAGS "-DCMAKE_GENERATOR_PLATFORM=x64")
endif()

if(APPLE)
    set(PLATFORM_FLAGS "-DOPENSSL_ROOT_DIR=/usr/local/opt/openssl")
endif()

# Force to build with the bundled version of Boost. This is necessary because
# if an outdated version of Boost is installed, then CMake will grab that
# instead of the bundled version of Boost, like we wanted.
set(BOOST_BUNDLE_ROOT "-DBOOST_ROOT=${DEPS_BUILD_DIR}/boost")

ExternalProject_Add(
  MatrixClient

  DOWNLOAD_DIR ${DEPS_DOWNLOAD_DIR}/mtxclient
  GIT_REPOSITORY ${MTXCLIENT_URL}
  GIT_TAG ${MTXCLIENT_TAG}

  BUILD_IN_SOURCE 1
  SOURCE_DIR ${DEPS_BUILD_DIR}/mtxclient
  CONFIGURE_COMMAND ${CMAKE_COMMAND}
        -DCMAKE_INSTALL_PREFIX=${DEPS_INSTALL_DIR}
        -DCMAKE_BUILD_TYPE=Release 
        -DBUILD_LIB_TESTS=OFF
        -DBUILD_LIB_EXAMPLES=OFF
        -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}
        ${BOOST_BUNDLE_ROOT}
        ${PLATFORM_FLAGS}
        ${DEPS_BUILD_DIR}/mtxclient
  BUILD_COMMAND 
        ${CMAKE_COMMAND} --build ${DEPS_BUILD_DIR}/mtxclient --config Release)

list(APPEND THIRD_PARTY_DEPS MatrixClient)
