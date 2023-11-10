hunter_config(
    nlohmann_json
    VERSION 3.11.2
    CMAKE_ARGS JSON_MultipleHeaders=ON
)

if (WIN32)
    hunter_config(
        CURL
        VERSION 8.4.0-p0
        CMAKE_ARGS
            BUILD_CURL_TESTS=OFF
            BUILD_CURL_EXE=OFF
            CURL_USE_SCHANNEL=ON
            CURL_USE_OPENSSL=OFF
            CURL_USE_LIBSSH2=OFF
            CURL_USE_LIBPSL=OFF
            CURL_DISABLE_FTP=ON
            CURL_DISABLE_FTP=ON
            CURL_DISABLE_FILE=ON
            CURL_DISABLE_TELNET=ON
            CURL_DISABLE_LDAP=ON
            CURL_DISABLE_DICT=ON
            CURL_DISABLE_TFTP=ON
            CURL_DISABLE_GOPHER=ON
            CURL_DISABLE_POP3=ON
            CURL_DISABLE_IMAP=ON
            CURL_DISABLE_SMB=ON
            CURL_DISABLE_SMTP=ON
            CURL_DISABLE_RTSP=ON
            CURL_USE_LIBRTMP=OFF
            CURL_DISABLE_MQTT=ON
            BUILD_TESTING=OFF
    )
else()
    hunter_config(
        CURL
        VERSION 8.4.0-p0
        CMAKE_ARGS
            BUILD_CURL_TESTS=OFF
            BUILD_CURL_EXE=OFF
            CURL_USE_SCHANNEL=OFF
            CURL_USE_OPENSSL=ON
            CURL_USE_LIBSSH2=OFF
            CURL_USE_LIBPSL=OFF
            CURL_DISABLE_FTP=ON
            CURL_DISABLE_FTP=ON
            CURL_DISABLE_FILE=ON
            CURL_DISABLE_TELNET=ON
            CURL_DISABLE_LDAP=ON
            CURL_DISABLE_DICT=ON
            CURL_DISABLE_TFTP=ON
            CURL_DISABLE_GOPHER=ON
            CURL_DISABLE_POP3=ON
            CURL_DISABLE_IMAP=ON
            CURL_DISABLE_SMB=ON
            CURL_DISABLE_SMTP=ON
            CURL_DISABLE_RTSP=ON
            CURL_USE_LIBRTMP=OFF
            CURL_DISABLE_MQTT=ON
            BUILD_TESTING=OFF
    )
endif()
