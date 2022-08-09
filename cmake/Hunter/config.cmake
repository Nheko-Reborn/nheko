hunter_config(
    lmdb
    VERSION 0.9.21-p2
)

hunter_config(
    OpenSSL
    VERSION 1.1.1j
)

hunter_config(
    Libevent
    VERSION 2.1.8-p4
)

hunter_config(
    nlohmann_json
    VERSION 3.8.0
    CMAKE_ARGS JSON_MultipleHeaders=ON
)

if (WIN32)
    hunter_config(
        CURL
        VERSION 7.74.0-p2
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
        VERSION 7.74.0-p2
        CMAKE_ARGS
            CMAKE_USE_SCHANNEL=OFF
            BUILD_CURL_TESTS=OFF
            BUILD_CURL_EXE=OFF
            CMAKE_USE_OPENSSL=ON
            CMAKE_USE_LIBSSH2=OFF
            BUILD_TESTING=OFF
    )
endif()
