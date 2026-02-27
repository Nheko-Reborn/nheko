hunter_config(
    nlohmann_json
    VERSION 3.11.2
    CMAKE_ARGS JSON_MultipleHeaders=ON
)

if (WIN32)
    hunter_config(
        CURL
    VERSION "8.18.0"
    URL "https://github.com/curl/curl/releases/download/curl-8_18_0/curl-8.18.0.tar.gz"
    SHA1 1a13070ccd2c5fe1d47cb34dd56138f69708f3f9
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
    VERSION "8.18.0"
    URL "https://github.com/curl/curl/releases/download/curl-8_18_0/curl-8.18.0.tar.gz"
    SHA1 1a13070ccd2c5fe1d47cb34dd56138f69708f3f9
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

hunter_config(ZLIB
	URL "https://codeload.github.com/madler/zlib/zip/tags/v1.3.2"
	VERSION "1.3.2-chromium"
	SHA1 "40b515519dceca271d77e97c680ab4871f3a8556"
	CMAKE_ARGS
		ZLIB_BUILD_TESTING=OFF
    ZLIB_BUILD_SHARED=OFF
		ZLIB_BUILD_STATIC=ON
		ZLIB_BUILD_MINIZIP=OFF
		ZLIB_INSTALL=ON
		ZLIB_PREFIX=OFF
)

hunter_config(
    Libevent
    VERSION "2.2.0-a994a52d5373d6284b27576efa617aff2baa7bd3"
    URL "https://github.com/hunter-packages/libevent/archive/a994a52d5373d6284b27576efa617aff2baa7bd3.tar.gz"
    SHA1 "a6f87f2b76465ccc1e5d75024475aae7d4c766a4"
    CMAKE_ARGS
        EVENT__DISABLE_TESTS=ON
        EVENT__DISABLE_SAMPLES=ON
        EVENT__DISABLE_REGRESS=ON
        EVENT__DISABLE_BENCHMARK=ON
        EVENT__LIBRARY_TYPE=STATIC
        CMAKE_C_BYTE_ORDER=LITTLE_ENDIAN
)
