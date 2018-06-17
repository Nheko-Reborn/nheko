cmake_minimum_required(VERSION 3.1)

project(olm VERSION 2.2.2 LANGUAGES CXX C)

add_definitions(-DOLMLIB_VERSION_MAJOR=${PROJECT_VERSION_MAJOR})
add_definitions(-DOLMLIB_VERSION_MINOR=${PROJECT_VERSION_MINOR})
add_definitions(-DOLMLIB_VERSION_PATCH=${PROJECT_VERSION_PATCH})

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

add_library(olm
    src/account.cpp
    src/base64.cpp
    src/cipher.cpp
    src/crypto.cpp
    src/memory.cpp
    src/message.cpp
    src/pickle.cpp
    src/ratchet.cpp
    src/session.cpp
    src/utility.cpp

    src/ed25519.c
    src/error.c
    src/inbound_group_session.c
    src/megolm.c
    src/olm.cpp
    src/outbound_group_session.c
    src/pickle_encoding.c

    lib/crypto-algorithms/aes.c
    lib/crypto-algorithms/sha256.c
    lib/curve25519-donna/curve25519-donna.c)
add_library(Olm::Olm ALIAS olm)

target_include_directories(olm
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/lib)

set_target_properties(olm PROPERTIES
   SOVERSION ${PROJECT_VERSION_MAJOR}
   VERSION ${PROJECT_VERSION})

set_target_properties(olm PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_BINDIR}
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR}
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR})

#
# Installation
#
include(GNUInstallDirs)
set(INSTALL_CONFIGDIR ${CMAKE_INSTALL_LIBDIR}/cmake/Olm)
install(TARGETS olm
    EXPORT olm-targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

# The exported target will be named Olm.
set_target_properties(olm PROPERTIES EXPORT_NAME Olm)
install(FILES
    ${CMAKE_SOURCE_DIR}/include/olm/olm.h
    ${CMAKE_SOURCE_DIR}/include/olm/outbound_group_session.h
    ${CMAKE_SOURCE_DIR}/include/olm/inbound_group_session.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/olm)

# Export the targets to a script.
install(EXPORT olm-targets
  FILE OlmTargets.cmake
  NAMESPACE Olm::
  DESTINATION ${INSTALL_CONFIGDIR})

# Create a ConfigVersion.cmake file.
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/OlmConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion)

configure_package_config_file(
    ${CMAKE_CURRENT_LIST_DIR}/cmake/OlmConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/OlmConfig.cmake
    INSTALL_DESTINATION ${INSTALL_CONFIGDIR})

#Install the config & configversion.
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/OlmConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/OlmConfigVersion.cmake
    DESTINATION ${INSTALL_CONFIGDIR})

# Register package in user's package registry
export(EXPORT olm-targets
    FILE ${CMAKE_CURRENT_BINARY_DIR}/OlmTargets.cmake
    NAMESPACE Olm::)
export(PACKAGE Olm)
