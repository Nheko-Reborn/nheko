if(WIN32)
  message(STATUS "Building Boost in Windows is not supported (skipping)")
  return()
endif()

ExternalProject_Add(
  Boost

  URL ${BOOST_URL}
  URL_HASH SHA256=${BOOST_SHA256}
  DOWNLOAD_DIR ${DEPS_DOWNLOAD_DIR}/boost
  DOWNLOAD_NO_PROGRESS 0

  BUILD_IN_SOURCE 1
  SOURCE_DIR ${DEPS_BUILD_DIR}/boost
  CONFIGURE_COMMAND ${DEPS_BUILD_DIR}/boost/bootstrap.sh
    --with-libraries=random,thread,system,iostreams,atomic,chrono,date_time,regex
    --prefix=${DEPS_INSTALL_DIR}
  BUILD_COMMAND ${DEPS_BUILD_DIR}/boost/b2 -d0 cxxstd=14 variant=release link=shared runtime-link=shared threading=multi --layout=system
  INSTALL_COMMAND ${DEPS_BUILD_DIR}/boost/b2 -d0 install
)

list(APPEND THIRD_PARTY_DEPS Boost)
