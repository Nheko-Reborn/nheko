hunter_config(
    nlohmann_json
    VERSION 3.11.1
    CMAKE_ARGS JSON_MultipleHeaders=ON
)

if (WIN32)
    hunter_config(
        CURL
        VERSION 8.4.0
        CMAKE_ARGS
            CMAKE_USE_SCHANNEL=ON
            BUILD_CURL_TESTS=OFF
            BUILD_CURL_EXE=OFF
            CMAKE_USE_OPENSSL=OFF
            CMAKE_USE_LIBSSH2=OFF
            BUILD_TESTING=OFF
    )
else()
    hunter_config(
        CURL
        VERSION 8.4.0
        CMAKE_ARGS
            CMAKE_USE_SCHANNEL=OFF
            BUILD_CURL_TESTS=OFF
            BUILD_CURL_EXE=OFF
            CMAKE_USE_OPENSSL=ON
            CMAKE_USE_LIBSSH2=OFF
            BUILD_TESTING=OFF
    )
endif()
