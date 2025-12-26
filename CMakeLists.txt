cmake_minimum_required(VERSION 3.13...3.21)

option(APPVEYOR_BUILD "Build on appveyor" OFF)
option(CI_BUILD "Set when building in CI. Enables -Werror where possible" OFF)
option(ASAN "Compile with address sanitizers" OFF)
option(QML_DEBUGGING "Enable qml debugging" OFF)
if(UNIX AND NOT APPLE)
    option(MAN "Build man page" ON)
else()
    option(MAN "Build man page" OFF)
endif()
option(FLATPAK "Set this only if Nheko is built as a flatpak" OFF)
option(JSON_ImplicitConversions "Disable implicit conversions in nlohmann/json" ON)

set(
    CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_LIST_DIR}/toolchain.cmake"
    CACHE
    FILEPATH "Default toolchain"
)
set(CMAKE_CXX_STANDARD 20 CACHE STRING "C++ standard")
set(CMAKE_CXX_STANDARD_REQUIRED ON CACHE BOOL "Require C++ standard to be supported")
set(CMAKE_POSITION_INDEPENDENT_CODE ON CACHE BOOL "compile as PIC by default")
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.15" CACHE STRING "macos deployment target")

option(HUNTER_ENABLED "Enable Hunter package manager" OFF)
include("cmake/HunterGate.cmake")
HunterGate(
    URL "https://github.com/cpp-pm/hunter/archive/v0.26.1.tar.gz"
    SHA1 "e41ac7a18c49b35ebac99ff2b5244317b2638a65"
    LOCAL
)

macro(hunter_add_package_safe)
    set(pkg_temp_backup_libdir "$ENV{PKG_CONFIG_LIBDIR}")
    set(pkg_temp_backup_path "$ENV{PKG_CONFIG_PATH}")
    hunter_add_package(${ARGV})
    if("${pkg_temp_backup_path}" STREQUAL "")
        unset(ENV{PKG_CONFIG_PATH})
    else()
        set(ENV{PKG_CONFIG_PATH} "${pkg_temp_backup_path}")
    endif()
    if("${pkg_temp_backup_libdir}" STREQUAL "")
        unset(ENV{PKG_CONFIG_LIBDIR})
    else()
        set(ENV{PKG_CONFIG_LIBDIR} "${pkg_temp_backup_libdir}")
    endif()
    message("pkg_conf_path: '$ENV{PKG_CONFIG_PATH}', pkg_conf_libdir: '$ENV{PKG_CONFIG_LIBDIR}'")
endmacro()

project(nheko LANGUAGES CXX C)

option(USE_BUNDLED_SPDLOG "Use the bundled version of spdlog." ${HUNTER_ENABLED})
option(USE_BUNDLED_FMT "Use the bundled version of {fmt}." ${HUNTER_ENABLED})
option(USE_BUNDLED_OLM "Use the bundled version of libolm." ${HUNTER_ENABLED})
option(USE_BUNDLED_GTEST "Use the bundled version of Google Test." ${HUNTER_ENABLED})
option(USE_BUNDLED_CMARK "Use the bundled version of cmark." ${HUNTER_ENABLED})
option(USE_BUNDLED_JSON "Use the bundled version of nlohmann json." ${HUNTER_ENABLED})
option(USE_BUNDLED_OPENSSL "Use the bundled version of OpenSSL." OFF)
option(USE_BUNDLED_MTXCLIENT "Use the bundled version of the Matrix Client library." ${HUNTER_ENABLED})
option(USE_BUNDLED_LMDB "Use the bundled version of lmdb." ${HUNTER_ENABLED})
option(USE_BUNDLED_LMDBXX "Use the bundled version of lmdb++." ${HUNTER_ENABLED})
option(USE_BUNDLED_QTKEYCHAIN "Use the bundled version of Qt6Keychain." ${HUNTER_ENABLED})
option(USE_BUNDLED_COEURL "Use a bundled version of the Curl wrapper"
    ${HUNTER_ENABLED})
option(USE_BUNDLED_LIBEVENT "Use the bundled version of libevent." ${HUNTER_ENABLED})
option(USE_BUNDLED_LIBCURL "Use the bundled version of libcurl." ${HUNTER_ENABLED})
option(USE_BUNDLED_RE2 "Use the bundled version of re2." ${HUNTER_ENABLED})
option(USE_BUNDLED_KDSINGLEAPPLICATION "Use the bundled version of KDSingleApplication." ${HUNTER_ENABLED})
option(USE_BUNDLED_CPPHTTPLIB "Use the bundled version of cpp-httplib." ON)
option(USE_BUNDLED_BLURHASH "Use the bundled version of blurhash." ON)

include(CMakeDependentOption)
set(VOIP_DEFAULT ON)
if(APPLE OR WIN32 OR HAIKU)
    set(VOIP_DEFAULT OFF)
endif()
option(VOIP "Whether to enable voip support. Disable this, if you don't have gstreamer." ${VOIP_DEFAULT})
set(X11_DEFAULT)
if(WIN32 OR APPLE OR HAIKU)
    set(X11_DEFAULT OFF)
endif()
option(X11 "Whether to enable X11 specific features (screenshare, window roles)." ${X11_DEFAULT})
cmake_dependent_option(SCREENSHARE_XDP "Whether to enable screenshare support using xdg-desktop-portal." ON "VOIP" OFF)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

if(${CMAKE_VERSION} VERSION_LESS "3.14.0")
    message("Adding FetchContent_MakeAvailable")
    # from cmakes sources
    macro(FetchContent_MakeAvailable)
        foreach(contentName IN ITEMS ${ARGV})
            string(TOLOWER ${contentName} contentNameLower)
            FetchContent_GetProperties(${contentName})
            if(NOT ${contentNameLower}_POPULATED)
                FetchContent_Populate(${contentName})

                # Only try to call add_subdirectory() if the populated content
                # can be treated that way. Protecting the call with the check
                # allows this function to be used for projects that just want
                # to ensure the content exists, such as to provide content at
                # a known location.
                if(EXISTS ${${contentNameLower}_SOURCE_DIR}/CMakeLists.txt)
                    add_subdirectory(${${contentNameLower}_SOURCE_DIR}
                        ${${contentNameLower}_BINARY_DIR})
                endif()
            endif()
        endforeach()
    endmacro()
endif()

# Include Qt basic functions
include(QtCommon)

include(GNUInstallDirs)

set(CPACK_PACKAGE_VERSION_MAJOR "0")
set(CPACK_PACKAGE_VERSION_MINOR "12")
set(CPACK_PACKAGE_VERSION_PATCH "1")
set(PROJECT_VERSION_MAJOR ${CPACK_PACKAGE_VERSION_MAJOR})
set(PROJECT_VERSION_MINOR ${CPACK_PACKAGE_VERSION_MINOR})
set(PROJECT_VERSION_PATCH ${CPACK_PACKAGE_VERSION_PATCH})

# Set PROJECT_VERSION_PATCH & PROJECT_VERSION_TWEAK to 0 if not present
# Needed by add_project_meta.
fix_project_version()

# Set additional project information
set(COMPANY "Nheko")
set(COPYRIGHT "Copyright (c) 2024 Nheko Contributors")
set(IDENTIFIER "im.nheko.Nheko")

add_project_meta(META_FILES_TO_INCLUDE)

if(NOT MSVC AND NOT APPLE)
    set(THREADS_PREFER_PTHREAD_FLAG ON)
    find_package(Threads REQUIRED)
endif()

if(BUILD_DOCS)
    find_package(Doxygen)

    if(DOXYGEN_FOUND)
        set(DOXYGEN_IN ${CMAKE_CURRENT_SOURCE_DIR}/cmake/Doxyfile.in)
        set(DOXYGEN_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)

        configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT})

        add_custom_target(docs ALL
            COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            COMMENT "Generating API documentation with Doxygen"
            VERBATIM )
    else(DOXYGEN_FOUND)
        message("Doxygen need to be installed to generate the doxygen documentation")
    endif(DOXYGEN_FOUND)
endif()

#
## coeurl
#
## Need to repeat all libevent deps?!?
if(USE_BUNDLED_LIBEVENT)
    hunter_add_package_safe(Libevent)
    find_package(Libevent CONFIG REQUIRED)
else()
    find_package(PkgConfig REQUIRED) 
    pkg_check_modules(libevent_core REQUIRED IMPORTED_TARGET libevent_core)
    if(WIN32)
        pkg_check_modules(libevent_windows REQUIRED IMPORTED_TARGET libevent_windows)
    else()
        pkg_check_modules(libevent_pthreads REQUIRED IMPORTED_TARGET libevent_pthreads)
    endif()
endif()

# curl
if(USE_BUNDLED_LIBCURL)
    hunter_add_package_safe(CURL)
    find_package(CURL CONFIG REQUIRED)
else()
    find_package(PkgConfig REQUIRED) 
    pkg_check_modules(libcurl REQUIRED IMPORTED_TARGET libcurl)
endif()

# fmt
if(USE_BUNDLED_FMT)
    hunter_add_package_safe(fmt)
endif()
find_package(fmt 8.0.0 CONFIG REQUIRED)

# spdlog
if(USE_BUNDLED_SPDLOG)
    hunter_add_package_safe(spdlog)
endif()
find_package(spdlog 1.0.0 CONFIG REQUIRED)

if(USE_BUNDLED_COEURL)
    include(FetchContent)
    FetchContent_Declare(
        coeurl
        GIT_REPOSITORY https://nheko.im/Nheko-Reborn/coeurl.git
        GIT_TAG        831e2ee8e9cf08ea1ee9736cde8370f9d0312abc
    )
    FetchContent_MakeAvailable(coeurl)
    set(COEURL_TARGET_NAME coeurl::coeurl)
else()
    find_package(PkgConfig)
    pkg_check_modules(coeurl IMPORTED_TARGET coeurl>=0.3.0)
    if(TARGET PkgConfig::coeurl)
        set(COEURL_TARGET_NAME PkgConfig::coeurl)
    endif()
endif()

if(NOT TARGET PkgConfig::coeurl)
    find_package(coeurl 0.2.1 CONFIG)
    if(TARGET coeurl::coeurl)
        set(COEURL_TARGET_NAME coeurl::coeurl)
    endif()
endif()

if(NOT COEURL_TARGET_NAME)
    message(ERROR "Couldn't find coeurl")
endif()

if(USE_BUNDLED_RE2)
    hunter_add_package(re2)
    find_package(re2 CONFIG REQUIRED)
else()
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(re2 REQUIRED IMPORTED_TARGET re2)
endif()

#
# LMDB
#
#include(LMDB)
if(USE_BUNDLED_LMDB)
    hunter_add_package_safe(lmdb)
    find_package(liblmdb CONFIG REQUIRED)

    target_include_directories(liblmdb::lmdb INTERFACE
        "${HUNTER_INSTALL_PREFIX}/include/lmdb")
else()
    find_package(LMDB REQUIRED)
endif()

#
# Discover Qt dependencies.
#
find_package(Qt6 6.5 COMPONENTS Core Widgets Gui LinguistTools Svg Multimedia Qml QuickControls2 REQUIRED)
if (Qt6Qml_VERSION VERSION_GREATER_EQUAL "6.10.0")
    find_package(Qt6 REQUIRED COMPONENTS GuiPrivate QmlPrivate)
endif()
find_package(Qt6DBus)

if(USE_BUNDLED_QTKEYCHAIN)
    include(FetchContent)
    set(BUILD_WITH_QT6 ON)
    FetchContent_Declare(
        qt6keychain
        GIT_REPOSITORY https://github.com/frankosterfeld/qtkeychain.git
        GIT_TAG        v0.14.0
    )
    set(BUILD_TEST_APPLICATION OFF CACHE INTERNAL "")
    FetchContent_MakeAvailable(qt6keychain)
else()
    find_package(Qt6Keychain REQUIRED)
endif()

if(USE_BUNDLED_KDSINGLEAPPLICATION)
    include(FetchContent)
    set(KDSingleApplication_QT6 ON CACHE BOOL INTERNAL)
    set(KDSingleApplication_EXAMPLES OFF CACHE BOOL INTERNAL)
    FetchContent_Declare(
        kdsingleapplication
        GIT_REPOSITORY https://github.com/KDAB/KDSingleApplication.git
        GIT_TAG        v1.0.0
    )
    FetchContent_MakeAvailable(kdsingleapplication)
else()
    find_package(KDSingleApplication-qt6 REQUIRED)
endif()

if(Qt6Widgets_FOUND)
    if(Qt6Widgets_VERSION VERSION_LESS 6.5.0)
        message(STATUS "Qt version ${Qt6Widgets_VERSION}")
        message(WARNING "Minimum supported Qt6 version is 6.5!")
    endif()
endif(Qt6Widgets_FOUND)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
if(NOT MSVC)
    set(
        CMAKE_CXX_FLAGS
        "${CMAKE_CXX_FLAGS} \
        -Wall \
        -Wextra \
        -pedantic \
        -fsized-deallocation \
        -fdiagnostics-color=always \
        -Wunreachable-code \
        -Wno-attributes"
    )
    if(NOT CMAKE_COMPILER_IS_GNUCXX)
        # -Wshadow is buggy and broken in GCC, so do not enable it.
        # see https://gcc.gnu.org/bugzilla/show_bug.cgi?id=79328
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wshadow")
    endif()
endif()

if(MSVC)
    set(
        CMAKE_CXX_FLAGS
        "${CMAKE_CXX_FLAGS} /bigobj"
    )
endif()

if(NOT (CMAKE_BUILD_TYPE OR CMAKE_CONFIGURATION_TYPES))
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING
        "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel."
        FORCE)
    message("Setting build type to '${CMAKE_BUILD_TYPE}'")
else(NOT (CMAKE_BUILD_TYPE OR CMAKE_CONFIGURATION_TYPES))
    message("Build type set to '${CMAKE_BUILD_TYPE}'")
endif(NOT (CMAKE_BUILD_TYPE OR CMAKE_CONFIGURATION_TYPES))

set(SPDLOG_DEBUG_ON false)

# Windows doesn't handle CMAKE_BUILD_TYPE.
if(NOT WIN32)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(SPDLOG_DEBUG_ON true)
    else()
        set(SPDLOG_DEBUG_ON false)
    endif()
endif()

find_program(GIT git)
if(GIT)
    execute_process(
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMAND ${GIT} rev-parse --short HEAD
        OUTPUT_VARIABLE GIT_OUT OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    if(GIT_OUT)
        set(CPACK_PACKAGE_VERSION_PATCH "${CPACK_PACKAGE_VERSION_PATCH}-${GIT_OUT}")
    else()
        set(CPACK_PACKAGE_VERSION_PATCH "${CPACK_PACKAGE_VERSION_PATCH}")
    endif()
endif(GIT)

set(CPACK_PACKAGE_VERSION ${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH})
set(PROJECT_VERSION ${CPACK_PACKAGE_VERSION})

message(STATUS "Version: ${PROJECT_VERSION}")

cmake_host_system_information(RESULT BUILD_HOST QUERY HOSTNAME)

include(CheckSymbolExists)
check_symbol_exists(backtrace_symbols_fd "execinfo.h" HAVE_BACKTRACE_SYMBOLS_FD)

configure_file(cmake/nheko.h config/nheko.h)

#
# Declare source and header files.
#
set(SRC_FILES
    # Emoji
    src/emoji/Provider.cpp
    src/emoji/Provider.h
    src/emoji/Emoji.cpp
    src/emoji/Emoji.h

    # Timeline
    src/timeline/CommunitiesModel.cpp
    src/timeline/CommunitiesModel.h
    src/timeline/DelegateChooser.cpp
    src/timeline/DelegateChooser.h
    src/timeline/EventStore.cpp
    src/timeline/EventStore.h
    src/timeline/EventDelegateChooser.cpp
    src/timeline/EventDelegateChooser.h
    src/timeline/InputBar.cpp
    src/timeline/InputBar.h
    src/timeline/Permissions.cpp
    src/timeline/Permissions.h
    src/timeline/PresenceEmitter.cpp
    src/timeline/PresenceEmitter.h
    src/timeline/Reaction.cpp
    src/timeline/Reaction.h
    src/timeline/RoomlistModel.cpp
    src/timeline/RoomlistModel.h
    src/timeline/TimelineFilter.cpp
    src/timeline/TimelineFilter.h
    src/timeline/TimelineModel.cpp
    src/timeline/TimelineModel.h
    src/timeline/TimelineViewManager.cpp
    src/timeline/TimelineViewManager.h

    # UI components
    src/ui/HiddenEvents.cpp
    src/ui/HiddenEvents.h
    src/ui/EventExpiry.cpp
    src/ui/EventExpiry.h
    src/ui/MxcAnimatedImage.cpp
    src/ui/MxcAnimatedImage.h
    src/ui/MxcMediaProxy.cpp
    src/ui/MxcMediaProxy.h
    src/ui/NhekoCursorShape.cpp
    src/ui/NhekoCursorShape.h
    src/ui/NhekoDropArea.cpp
    src/ui/NhekoDropArea.h
    src/ui/NhekoGlobalObject.cpp
    src/ui/NhekoGlobalObject.h
    src/ui/NhekoMenuVisibilityFilter.h
    src/ui/NhekoMenuVisibilityFilter.cpp
    src/ui/RoomSettings.cpp
    src/ui/RoomSettings.h
    src/ui/RoomSummary.cpp
    src/ui/RoomSummary.h
    src/ui/Theme.cpp
    src/ui/Theme.h
    src/ui/UIA.cpp
    src/ui/UIA.h
    src/ui/UserProfile.cpp
    src/ui/UserProfile.h

    src/voip/CallDevices.cpp
    src/voip/CallDevices.h
    src/voip/CallManager.cpp
    src/voip/CallManager.h
    src/voip/ScreenCastPortal.cpp
    src/voip/ScreenCastPortal.h
    src/voip/WebRTCSession.h
    src/voip/WebRTCSession.cpp
    src/voip/WebRTCSession.h

    src/encryption/DeviceVerificationFlow.cpp
    src/encryption/DeviceVerificationFlow.h
    src/encryption/Olm.cpp
    src/encryption/Olm.h
    src/encryption/SelfVerificationStatus.cpp
    src/encryption/SelfVerificationStatus.h
    src/encryption/VerificationManager.cpp
    src/encryption/VerificationManager.h

    # Generic notification stuff
    src/notifications/Manager.cpp
    src/notifications/Manager.h

    src/dock/Dock.cpp
    src/dock/Dock.h

    src/AliasEditModel.cpp
    src/AliasEditModel.h
    src/AvatarProvider.cpp
    src/AvatarProvider.h
    src/BlurhashProvider.cpp
    src/BlurhashProvider.h
    src/Cache.cpp
    src/Cache.h
    src/CacheCryptoStructs.cpp
    src/CacheCryptoStructs.h
    src/CacheStructs.h
    src/Cache_p.h
    src/ChatPage.cpp
    src/ChatPage.h
    src/Clipboard.cpp
    src/Clipboard.h
    src/ColorImageProvider.cpp
    src/ColorImageProvider.h
    src/CombinedImagePackModel.cpp
    src/CombinedImagePackModel.h
    src/GridImagePackModel.cpp
    src/GridImagePackModel.h
    src/CommandCompleter.cpp
    src/CommandCompleter.h
    src/CompletionModelRoles.h
    src/CompletionProxyModel.cpp
    src/CompletionProxyModel.h
    src/Config.h
    src/EventAccessors.cpp
    src/EventAccessors.h
    src/FallbackAuth.cpp
    src/FallbackAuth.h
    src/ImagePackListModel.cpp
    src/ImagePackListModel.h
    src/InviteesModel.cpp
    src/InviteesModel.h
    src/JdenticonProvider.cpp
    src/JdenticonProvider.h
    src/Logging.cpp
    src/Logging.h
    src/LoginPage.cpp
    src/LoginPage.h
    src/MainWindow.cpp
    src/MainWindow.h
    src/MatrixClient.cpp
    src/MatrixClient.h
    src/MemberList.cpp
    src/MemberList.h
    src/MxcImageProvider.cpp
    src/MxcImageProvider.h
    src/PowerlevelsEditModels.cpp
    src/PowerlevelsEditModels.h
    src/ReadReceiptsModel.cpp
    src/ReadReceiptsModel.h
    src/ReCaptcha.cpp
    src/ReCaptcha.h
    src/RegisterPage.cpp
    src/RegisterPage.h
    src/RoomDirectoryModel.cpp
    src/RoomDirectoryModel.h
    src/RoomsModel.cpp
    src/RoomsModel.h
    src/SSOHandler.cpp
    src/SSOHandler.h
    src/SingleImagePackModel.cpp
    src/SingleImagePackModel.h
    src/TrayIcon.cpp
    src/TrayIcon.h
    src/UserSettingsPage.cpp
    src/UserSettingsPage.h
    src/UsersModel.cpp
    src/UsersModel.h
    src/Utils.cpp
    src/Utils.h

    includes/jdenticoninterface.h

    src/main.cpp
)

include(FeatureSummary)

if(USE_BUNDLED_OPENSSL)
    hunter_add_package_safe(OpenSSL)
endif()
find_package(OpenSSL 1.1.0 REQUIRED)
if(USE_BUNDLED_OLM)
    include(FetchContent)
    FetchContent_Declare(
        Olm
        GIT_REPOSITORY https://gitlab.matrix.org/matrix-org/olm.git
        GIT_TAG        3.2.16
        PATCH_COMMAND  git apply ${CMAKE_CURRENT_SOURCE_DIR}/third_party/olm-patches/0001-fix-list-const-ptr.patch
        UPDATE_DISCONNECTED 1
    )
    set(OLM_TESTS OFF CACHE INTERNAL "")
    FetchContent_MakeAvailable(Olm)
else()
    find_package(Olm 3.2.7 REQUIRED)
    set_package_properties(Olm PROPERTIES
        DESCRIPTION "An implementation of the Double Ratchet cryptographic ratchet"
        URL "https://git.matrix.org/git/olm/about/"
        TYPE REQUIRED
    )
endif()
if(USE_BUNDLED_SPDLOG)
    hunter_add_package_safe(spdlog)
endif()
find_package(spdlog 1.0.0 CONFIG REQUIRED)

if(USE_BUNDLED_CMARK)
    include(FetchContent)
    FetchContent_Declare(
        cmark
        GIT_REPOSITORY https://github.com/commonmark/cmark.git
        GIT_TAG        0.31.1
        CMAKE_ARGS     "BUILD_TESTING=OFF"
    )
    FetchContent_MakeAvailable(cmark)
    if (NOT TARGET cmark::cmark)
        add_library(cmark::cmark ALIAS cmark)
    endif()
else()
    find_package(cmark REQUIRED 0.29.0)
endif()

if(USE_BUNDLED_JSON)
    hunter_add_package_safe(nlohmann_json)
endif()
find_package(nlohmann_json 3.2.0)
set_package_properties(nlohmann_json PROPERTIES
    DESCRIPTION "JSON for Modern C++, a C++11 header-only JSON class"
    URL "https://nlohmann.github.io/json/"
    TYPE REQUIRED
)

if(USE_BUNDLED_LMDBXX)
    include(FetchContent)
    FetchContent_Declare(
        lmdbxx
        URL "https://raw.githubusercontent.com/hoytech/lmdbxx/1.0.0/lmdb++.h"
        DOWNLOAD_NO_EXTRACT TRUE
        CONFIGURE_COMMAND ""
        BUILD_COMMAND ""
    )
    FetchContent_Populate(lmdbxx)
    add_library(lmdbxx INTERFACE)
    target_include_directories(lmdbxx INTERFACE ${lmdbxx_SOURCE_DIR})
    add_library(lmdbxx::lmdbxx ALIAS lmdbxx)
else()
    if(NOT LMDBXX_INCLUDE_DIR)
        find_path(LMDBXX_INCLUDE_DIR
            NAMES lmdb++.h
            PATHS /usr/include
            /usr/local/include
            $ENV{LIB_DIR}/include
            $ENV{LIB_DIR}/include/lmdbxx)
    endif()
    add_library(lmdbxx INTERFACE)
    target_include_directories(lmdbxx INTERFACE ${LMDBXX_INCLUDE_DIR})
    add_library(lmdbxx::lmdbxx ALIAS lmdbxx)
endif()

if(USE_BUNDLED_MTXCLIENT)
    include(FetchContent)
    FetchContent_Declare(
        MatrixClient
            GIT_REPOSITORY https://github.com/Nheko-Reborn/mtxclient.git
            GIT_TAG        v0.10.1
    )
    set(BUILD_LIB_EXAMPLES OFF CACHE INTERNAL "")
    set(BUILD_LIB_TESTS OFF CACHE INTERNAL "")
    FetchContent_MakeAvailable(MatrixClient)
else()
    find_package(MatrixClient 0.10.0 REQUIRED)
endif()

if(VOIP)
    include(FindPkgConfig)
    pkg_check_modules(GSTREAMER REQUIRED IMPORTED_TARGET gstreamer-sdp-1.0>=1.18 gstreamer-webrtc-1.0>=1.18 gstreamer-gl-1.0)
    if (WIN32)
        pkg_check_modules(GSTREAMER REQUIRED IMPORTED_TARGET gstreamer-d3d11-1.0)
    endif()
endif()

if(X11 AND NOT WIN32 AND NOT APPLE AND NOT HAIKU)
    pkg_check_modules(XCB REQUIRED IMPORTED_TARGET xcb xcb-ewmh)
endif()

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)

# this must be defined here to make the moc work properly
if(NOT APPLE AND NOT WIN32)
    add_compile_definitions(NHEKO_DBUS_SYS)
endif()

#
# Bundle resources
#
qt_add_resources(QRC resources/res.qrc)

if(APPLE)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -framework Foundation -framework Cocoa -framework UserNotifications")
    set(SRC_FILES ${SRC_FILES} src/notifications/NotificationManagerProxy.h src/notifications/MacNotificationDelegate.h src/notifications/MacNotificationDelegate.mm src/notifications/ManagerMac.mm src/notifications/ManagerMac.cpp)
    if(${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.16.0")
        set_source_files_properties( src/notifications/NotificationManagerProxy.h src/notifications/MacNotificationDelegate.h src/notifications/MacNotificationDelegate.mm src/notifications/ManagerMac.mm PROPERTIES SKIP_PRECOMPILE_HEADERS ON)
    endif()
elseif(WIN32)
    file(DOWNLOAD
        "https://raw.githubusercontent.com/mohabouje/WinToast/v1.3.0/src/wintoastlib.cpp"
        ${PROJECT_SOURCE_DIR}/src/wintoastlib.cpp
        EXPECTED_HASH SHA256=1ddf8ee2604e1757d383a2f60d4d597c4f5d9496369e6a22586cedf4af77ca8f)

    file(DOWNLOAD
        "https://raw.githubusercontent.com/mohabouje/WinToast/v1.3.0/include/wintoastlib.h"
        ${PROJECT_SOURCE_DIR}/src/wintoastlib.h
        EXPECTED_HASH SHA256=08cb30d28db9c851d3d4259ae6108ce7dafa966957df6735a06cd55e0716fded)

    set(SRC_FILES ${SRC_FILES} src/notifications/ManagerWin.cpp src/wintoastlib.cpp src/wintoastlib.h)
else()
    set(SRC_FILES ${SRC_FILES}
        src/dbus/NhekoDBusApi.h
        src/dbus/NhekoDBusBackend.h
        src/dbus/NhekoDBusApi.cpp
        src/dbus/NhekoDBusBackend.cpp
        src/notifications/ManagerLinux.cpp
    )
endif()

set(NHEKO_DEPS
    ${SRC_FILES}
    ${QRC}
    ${META_FILES_TO_INCLUDE})

if(ASAN)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address,undefined")
endif()

if(WIN32)
    qt_add_executable(nheko WIN32 ${OS_BUNDLE} ${NHEKO_DEPS})
    target_compile_definitions(nheko PRIVATE _WIN32_WINNT=0x0601 NOMINMAX WIN32_LEAN_AND_MEAN STRICT)
else()
    qt_add_executable(nheko ${OS_BUNDLE} ${NHEKO_DEPS})

    if(APPLE)
        set_target_properties(nheko PROPERTIES MACOSX_BUNDLE_INFO_PLIST
            "${CMAKE_CURRENT_SOURCE_DIR}/cmake/MacOSXBundleInfo.plist.in")
    endif()

    if(HAVE_BACKTRACE_SYMBOLS_FD AND NOT CMAKE_BUILD_TYPE STREQUAL "Release")
        set_target_properties(nheko PROPERTIES ENABLE_EXPORTS ON)
    endif()
endif()

set_target_properties(nheko
    PROPERTIES
        CMAKE_SKIP_INSTALL_RPATH TRUE
        AUTOMOC ON)

#
# Add qml files
#
if(QT_KNOWN_POLICY_QTP0004)
    qt_policy(SET QTP0004 NEW)
endif()
set(QML_SOURCES
    resources/qml/Root.qml
    resources/qml/ChatPage.qml
    resources/qml/CommunitiesList.qml
    resources/qml/RoomList.qml
    resources/qml/Avatar.qml
    resources/qml/Completer.qml
    resources/qml/EncryptionIndicator.qml
    resources/qml/ImageButton.qml
    resources/qml/ElidedLabel.qml
    resources/qml/MatrixText.qml
    resources/qml/MatrixTextField.qml
    resources/qml/ToggleButton.qml
    resources/qml/UploadBox.qml
    resources/qml/MessageInput.qml
    resources/qml/MessageView.qml
    resources/qml/PrivacyScreen.qml
    resources/qml/Reactions.qml
    resources/qml/ReplyPopup.qml
    resources/qml/StatusIndicator.qml
    resources/qml/TimelineEvent.qml
    resources/qml/TimelineSectionHeader.qml
    resources/qml/TimelineDefaultMessageStyle.qml
    resources/qml/TimelineBubbleMessageStyle.qml
    resources/qml/TimelineMetadata.qml
    resources/qml/TimelineView.qml
    resources/qml/TopBar.qml
    resources/qml/QuickSwitcher.qml
    resources/qml/ForwardCompleter.qml
    resources/qml/SelfVerificationCheck.qml
    resources/qml/TypingIndicator.qml
    resources/qml/MessageInputWarning.qml
    resources/qml/components/AdaptiveLayout.qml
    resources/qml/components/AdaptiveLayoutElement.qml
    resources/qml/components/AvatarListTile.qml
    resources/qml/components/FlatButton.qml
    resources/qml/components/MainWindowDialog.qml
    resources/qml/components/NhekoTabButton.qml
    resources/qml/components/NotificationBubble.qml
    resources/qml/components/PowerlevelIndicator.qml
    resources/qml/components/ReorderableListview.qml
    resources/qml/components/SpaceMenu.qml
    resources/qml/components/SpaceMenuLevel.qml
    resources/qml/components/TextButton.qml
    resources/qml/components/UserListRow.qml
    resources/qml/delegates/Encrypted.qml
    resources/qml/delegates/FileMessage.qml
    resources/qml/delegates/ImageMessage.qml
    resources/qml/delegates/NoticeMessage.qml
    resources/qml/delegates/Pill.qml
    resources/qml/delegates/Placeholder.qml
    resources/qml/delegates/PlayableMediaMessage.qml
    resources/qml/delegates/Redacted.qml
    resources/qml/delegates/Reply.qml
    resources/qml/delegates/TextMessage.qml
    resources/qml/device-verification/DeviceVerification.qml
    resources/qml/device-verification/DigitVerification.qml
    resources/qml/device-verification/EmojiVerification.qml
    resources/qml/device-verification/Failed.qml
    resources/qml/device-verification/NewVerificationRequest.qml
    resources/qml/device-verification/Success.qml
    resources/qml/device-verification/Waiting.qml
    resources/qml/dialogs/AliasEditor.qml
    resources/qml/dialogs/ConfirmJoinRoomDialog.qml
    resources/qml/dialogs/CreateDirect.qml
    resources/qml/dialogs/CreateRoom.qml
    resources/qml/dialogs/FallbackAuthDialog.qml
    resources/qml/dialogs/HiddenEventsDialog.qml
    resources/qml/dialogs/EventExpirationDialog.qml
    resources/qml/dialogs/ImageOverlay.qml
    resources/qml/dialogs/ImagePackEditorDialog.qml
    resources/qml/dialogs/ImagePackDeleteDialog.qml
    resources/qml/dialogs/ImagePackSettingsDialog.qml
    resources/qml/dialogs/InputDialog.qml
    resources/qml/dialogs/InviteDialog.qml
    resources/qml/dialogs/JoinRoomDialog.qml
    resources/qml/dialogs/LeaveRoomDialog.qml
    resources/qml/dialogs/LogoutDialog.qml
    resources/qml/dialogs/PhoneNumberInputDialog.qml
    resources/qml/dialogs/PowerLevelEditor.qml
    resources/qml/dialogs/PowerLevelSpacesApplyDialog.qml
    resources/qml/dialogs/RawMessageDialog.qml
    resources/qml/dialogs/ReadReceipts.qml
    resources/qml/dialogs/ReCaptchaDialog.qml
    resources/qml/dialogs/ReportMessage.qml
    resources/qml/dialogs/RoomDirectory.qml
    resources/qml/dialogs/RoomMembers.qml
    resources/qml/dialogs/AllowedRoomsSettingsDialog.qml
    resources/qml/dialogs/RoomSettingsDialog.qml
    resources/qml/dialogs/UserProfile.qml
    resources/qml/dialogs/IgnoredUsers.qml
    resources/qml/emoji/StickerPicker.qml
    resources/qml/pages/LoginPage.qml
    resources/qml/pages/RegisterPage.qml
    resources/qml/pages/UserSettingsPage.qml
    resources/qml/pages/WelcomePage.qml
    resources/qml/ui/NhekoSlider.qml
    resources/qml/ui/Ripple.qml
    resources/qml/ui/Snackbar.qml
    resources/qml/ui/Spinner.qml
    resources/qml/ui/animations/BlinkAnimation.qml
    resources/qml/ui/media/MediaControls.qml
    resources/qml/voip/ActiveCallBar.qml
    resources/qml/voip/CallDevices.qml
    resources/qml/voip/CallInvite.qml
    resources/qml/voip/CallInviteBar.qml
    resources/qml/voip/DeviceError.qml
    resources/qml/voip/PlaceCall.qml
    resources/qml/voip/ScreenShare.qml
    resources/qml/voip/VideoCall.qml
	resources/qml/voip/VideoCallD3D11.qml
    resources/qml/delegates/EncryptionEnabled.qml
    resources/qml/ui/TimelineEffects.qml
)
qt_add_qml_module(nheko
    URI im.nheko
    NO_RESOURCE_TARGET_PATH
    RESOURCE_PREFIX "/"
    VERSION 1.1
    DEPENDENCIES QtQuick QtQml.Models
    QML_FILES
    ${QML_SOURCES}
    SOURCES
    src/UserDirectoryModel.cpp
    src/UserDirectoryModel.h
)
    #qt_target_qml_sources(nheko
    #   #PREFIX "/"
    #)

#set_target_properties(nheko PROPERTIES
#    QT_QMLCACHEGEN_ARGUMENTS "--verbose"
#)

#
# Bundle translations
#
file(GLOB LANG_TS_SRC "${CMAKE_CURRENT_SOURCE_DIR}/resources/langs/*.ts")
qt_add_translations(nheko RESOURCE_PREFIX "/translations" TS_FILES
    ${LANG_TS_SRC} SOURCES ${QML_SOURCES} ${SRC_FILES})

if(WIN32)
    target_compile_definitions(nheko PRIVATE WIN32_LEAN_AND_MEAN)
    target_link_libraries(nheko PRIVATE Qt6::DBus)
    if(MSVC)
        target_compile_options(nheko PUBLIC "/Zc:__cplusplus")
    endif()
else()
    target_link_libraries(nheko PRIVATE Qt6::DBus)
    if(FLATPAK)
        target_compile_definitions(nheko PRIVATE NHEKO_FLATPAK)
    endif()
endif()

target_include_directories(nheko PRIVATE src includes src/timeline/ src/ui/ src/encryption/ src/voip/)

if(USE_BUNDLED_CPPHTTPLIB)
    target_include_directories(nheko PRIVATE third_party/cpp-httplib-0.5.12)
    target_sources(nheko PRIVATE third_party/cpp-httplib-0.5.12/httplib.h)
else()
    find_package(httplib REQUIRED)
    target_link_libraries(nheko PRIVATE httplib::httplib)
endif()

if(USE_BUNDLED_BLURHASH)
    target_include_directories(nheko PRIVATE third_party/blurhash)
    set(BLURHASH_SRC_FILES
        third_party/blurhash/blurhash.cpp
        third_party/blurhash/blurhash.hpp
    )
    target_sources(nheko PRIVATE ${BLURHASH_SRC_FILES})
else()
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(blurhash REQUIRED IMPORTED_TARGET blurhash)
    target_link_libraries(nheko PRIVATE PkgConfig::blurhash)
endif()

# Fixup bundled keychain include dirs
if(USE_BUNDLED_QTKEYCHAIN)
    target_include_directories(nheko PRIVATE ${qt6keychain_SOURCE_DIR} ${qt6keychain_BINARY_DIR})
endif()
if(USE_BUNDLED_KDSINGLEAPPLICATION)
    target_include_directories(nheko PRIVATE ${kdsingleapplication_SOURCE_DIR} ${kdsingleapplication_BINARY_DIR})
endif()

if(NOT JSON_ImplicitConversions)
    set_target_properties(nlohmann_json::nlohmann_json PROPERTIES
        INTERFACE_COMPILE_DEFINITIONS "JSON_USE_IMPLICIT_CONVERSIONS=\$<BOOL:OFF>;JSON_DIAGNOSTICS=\$<BOOL:OFF>"
    )
    target_compile_definitions(nheko PUBLIC JSON_USE_IMPLICIT_CONVERSIONS=0)
endif()

target_link_libraries(nheko PRIVATE
    ${COEURL_TARGET_NAME}
    MatrixClient::MatrixClient
    cmark::cmark
    fmt::fmt
    spdlog::spdlog
    Qt::Widgets
    Qt::Svg
    Qt::Gui
    Qt::Multimedia
    Qt::Qml
    Qt::QmlPrivate
    Qt::QuickControls2
    qt6keychain
    KDAB::kdsingleapplication
    nlohmann_json::nlohmann_json
    lmdbxx::lmdbxx
    liblmdb::lmdb)
    
if(UNIX)
    # for wayland activation tokens
    target_link_libraries(nheko PRIVATE Qt::GuiPrivate)
endif()

if(${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.16.0")
    target_precompile_headers(nheko
        PRIVATE
        <string>
        <algorithm>
    )
endif()

if(TARGET PkgConfig::GSTREAMER)
    target_link_libraries(nheko PRIVATE PkgConfig::GSTREAMER)
    target_compile_definitions(nheko PRIVATE GSTREAMER_AVAILABLE)
endif()
if(TARGET PkgConfig::XCB)
    target_link_libraries(nheko PRIVATE PkgConfig::XCB)
    target_compile_definitions(nheko PRIVATE XCB_AVAILABLE)
endif()

if(MSVC)
    target_link_libraries(nheko PRIVATE ntdll)
endif()

if(HAIKU)
    target_link_libraries(nheko PRIVATE network)
endif()

if(QML_DEBUGGING)
    target_compile_definitions(nheko PRIVATE QML_DEBUGGING)
endif()

if(NOT MSVC AND NOT HAIKU)
    if("${CMAKE_BUILD_TYPE}" STREQUAL "Debug" OR CI_BUILD)
        target_compile_options(nheko PRIVATE "-Werror")
    endif()
endif()
#if(NOT MSVC)
#   target_link_options(nheko PRIVATE "LINKER:,--gc-sections")
#endif()

if(MAN)
    add_subdirectory(man)
endif()

# potential workaround for macdeployqt issues
if(APPLE)
    install(TARGETS nheko
        BUNDLE  DESTINATION .
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
    qt_generate_deploy_qml_app_script(
        TARGET nheko
        OUTPUT_SCRIPT deploy_script
        NO_UNSUPPORTED_PLATFORM_ERROR
    )
    install(SCRIPT ${deploy_script})
endif()

if(UNIX AND NOT APPLE)
    if(FLATPAK)
        set(APPID "im.nheko.Nheko")
        set_target_properties(nheko PROPERTIES OUTPUT_NAME "${APPID}")
    else()
        set(APPID "nheko")
    endif()

    install(TARGETS nheko RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
    install(FILES "resources/nheko-16.png" DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/16x16/apps" RENAME "${APPID}.png")
    install(FILES "resources/nheko-32.png" DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/32x32/apps" RENAME "${APPID}.png")
    install(FILES "resources/nheko-48.png" DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/48x48/apps" RENAME "${APPID}.png")
    install(FILES "resources/nheko-64.png" DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/64x64/apps" RENAME "${APPID}.png")
    install(FILES "resources/nheko-128.png" DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/128x128/apps" RENAME "${APPID}.png")
    install(FILES "resources/nheko-256.png" DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/256x256/apps" RENAME "${APPID}.png")
    install(FILES "resources/nheko-512.png" DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/512x512/apps" RENAME "${APPID}.png")
    install(FILES "resources/nheko.svg" DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/scalable/apps" RENAME "${APPID}.svg")
    install(FILES "resources/_nheko" DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/zsh/site-functions")

    configure_file("resources/nheko.desktop.in" "resources/nheko.desktop" @ONLY)
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/resources/nheko.desktop" DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/applications" RENAME "${APPID}.desktop")
    configure_file("resources/nheko.appdata.xml.in" "resources/nheko.appdata.xml" @ONLY)
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/resources/nheko.appdata.xml" DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/metainfo" RENAME "${APPID}.appdata.xml")

    if(NOT TARGET uninstall)
        configure_file(
            "${CMAKE_CURRENT_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in"
            "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
            IMMEDIATE @ONLY)
        add_custom_target(uninstall
            COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)
    endif()
endif()
# vim: tabstop=4 shiftwidth=4 expandtab
