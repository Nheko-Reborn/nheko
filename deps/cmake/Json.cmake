ExternalProject_Add(
    Json
    PREFIX ${CMAKE_CURRENT_SOURCE_DIR}/Json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    # For shallow git clone (without downloading whole history)
    # GIT_SHALLOW 1
    # For point at certain tag
    GIT_TAG v3.2.0
    #disables auto update on every build
    UPDATE_DISCONNECTED 1
    #disable following
    CONFIGURE_COMMAND "" BUILD_COMMAND "" INSTALL_DIR "" INSTALL_COMMAND ""
    )
# Update json target
add_custom_target(external-Json-update
    COMMENT "Updated Nlohmann/Json"
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Json/src/Json
    COMMAND ${GIT_EXECUTABLE} pull
    DEPENDS Json)

#ExternalProject_Add(
#  json
#
#
#  DOWNLOAD_COMMAND  file(DOWNLOAD ${JSON_HEADER_URL} ${DEPS_INSTALL_DIR}/include/json.hpp
#    EXPECTED_HASH SHA256=${JSON_HEADER_HASH})
#)

list(APPEND THIRD_PARTY_DEPS Json)
