cmake_minimum_required(VERSION 3.1)

option(APPVEYOR_BUILD "Build on appveyor" OFF)
option(ASAN "Compile with address sanitizers" OFF)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

include(GNUInstallDirs)

# Include Qt basic functions
include(QtCommon)

project(nheko LANGUAGES C CXX)
set(CPACK_PACKAGE_VERSION_MAJOR "0")
set(CPACK_PACKAGE_VERSION_MINOR "4")
set(CPACK_PACKAGE_VERSION_PATCH "3")
set(PROJECT_VERSION_MAJOR ${CPACK_PACKAGE_VERSION_MAJOR})
set(PROJECT_VERSION_MINOR ${CPACK_PACKAGE_VERSION_MINOR})
set(PROJECT_VERSION_PATCH ${CPACK_PACKAGE_VERSION_PATCH})

# Set PROJECT_VERSION_PATCH & PROJECT_VERSION_TWEAK to 0 if not present
# Needed by add_project_meta.
fix_project_version()

# Set additional project information
set(COMPANY "Nheko")
set(COPYRIGHT "Copyright (c) 2017 Nheko Contributors")
set(IDENTIFIER "com.github.mujx.nheko")

add_project_meta(META_FILES_TO_INCLUDE)

if(APPLE)
    set(OPENSSL_ROOT_DIR /usr/local/opt/openssl)
endif()

if(NOT MSVC AND NOT APPLE)
  set(THREADS_PREFER_PTHREAD_FLAG ON)
  find_package(Threads REQUIRED)
endif()

if (BUILD_DOCS)
    find_package(Doxygen)

    if (DOXYGEN_FOUND)
        set(DOXYGEN_IN ${CMAKE_CURRENT_SOURCE_DIR}/cmake/Doxyfile.in)
        set(DOXYGEN_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)

        configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT})

        add_custom_target(docs ALL
            COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            COMMENT "Generating API documentation with Doxygen"
            VERBATIM )
    else (DOXYGEN_FOUND)
        message("Doxygen need to be installed to generate the doxygen documentation")
    endif (DOXYGEN_FOUND)
endif()


#
# LMDB
#
include(LMDB)

#
# Discover Qt dependencies.
#
find_package(Qt5 COMPONENTS Core Widgets LinguistTools Concurrent Svg Multimedia REQUIRED)
find_package(Qt5DBus)

if (APPLE)
    find_package(Qt5MacExtras REQUIRED)
endif(APPLE)

if (Qt5Widgets_FOUND)
    if (Qt5Widgets_VERSION VERSION_LESS 5.7.0)
        message(STATUS "Qt version ${Qt5Widgets_VERSION}")
        message(WARNING "Minimum supported Qt5 version is 5.7!")
    endif()
endif(Qt5Widgets_FOUND)

#
# Set up compiler flags.
#
if (NOT MSVC)
    set(CMAKE_C_COMPILER gcc)
endif(NOT MSVC)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
if(NOT MSVC)
  set(
    CMAKE_CXX_FLAGS
    "${CMAKE_CXX_FLAGS} \
        -Wall \
        -Wextra \
        -Werror \
        -pipe \
        -pedantic \
        -fsized-deallocation \
        -fdiagnostics-color=always \
        -Wunreachable-code"
    )
    if (NOT CMAKE_COMPILER_IS_GNUCXX)
        # -Wshadow is buggy and broken in GCC, so do not enable it.
        # see https://gcc.gnu.org/bugzilla/show_bug.cgi?id=79328
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wshadow")
    endif()
endif()

if(NOT (CMAKE_BUILD_TYPE OR CMAKE_CONFIGURATION_TYPES))
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING
        "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel."
        FORCE)
    message("Setting build type to '${CMAKE_BUILD_TYPE}'")
else(NOT (CMAKE_BUILD_TYPE OR CMAKE_CONFIGURATION_TYPES))
    message("Build type set to '${CMAKE_BUILD_TYPE}'")
endif(NOT (CMAKE_BUILD_TYPE OR CMAKE_CONFIGURATION_TYPES))

find_program(GIT git)
if(GIT)
    execute_process(
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMAND ${GIT} rev-parse --short HEAD
        OUTPUT_VARIABLE GIT_OUT OUTPUT_STRIP_TRAILING_WHITESPACE
        )
    set(CPACK_PACKAGE_VERSION_PATCH "${CPACK_PACKAGE_VERSION_PATCH}-${GIT_OUT}")
endif(GIT)

set(CPACK_PACKAGE_VERSION ${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH})
set(PROJECT_VERSION ${CPACK_PACKAGE_VERSION})

message(STATUS "Version: ${PROJECT_VERSION}")

cmake_host_system_information(RESULT BUILD_HOST QUERY HOSTNAME)
set(BUILD_USER $ENV{USER})
configure_file(cmake/version.hpp config/version.hpp)


#
# Declare source and header files.
#
set(SRC_FILES
    # Dialogs
    src/dialogs/CreateRoom.cc
    src/dialogs/ImageOverlay.cc
    src/dialogs/PreviewUploadOverlay.cc
    src/dialogs/InviteUsers.cc
    src/dialogs/JoinRoom.cc
    src/dialogs/MemberList.cpp
    src/dialogs/LeaveRoom.cc
    src/dialogs/Logout.cc
    src/dialogs/ReadReceipts.cc
    src/dialogs/ReCaptcha.cpp
    src/dialogs/RoomSettings.cpp

    # Emoji
    src/emoji/Category.cc
    src/emoji/ItemDelegate.cc
    src/emoji/Panel.cc
    src/emoji/PickButton.cc
    src/emoji/Provider.cc

    # Timeline
    src/timeline/TimelineViewManager.cc
    src/timeline/TimelineItem.cc
    src/timeline/TimelineView.cc
    src/timeline/widgets/AudioItem.cc
    src/timeline/widgets/FileItem.cc
    src/timeline/widgets/ImageItem.cc
    src/timeline/widgets/VideoItem.cc

    # UI components
    src/ui/Avatar.cc
    src/ui/Badge.cc
    src/ui/LoadingIndicator.cc
    src/ui/InfoMessage.cpp
    src/ui/FlatButton.cc
    src/ui/FloatingButton.cc
    src/ui/Label.cc
    src/ui/OverlayModal.cc
    src/ui/ScrollBar.cc
    src/ui/SnackBar.cc
    src/ui/RaisedButton.cc
    src/ui/Ripple.cc
    src/ui/RippleOverlay.cc
    src/ui/OverlayWidget.cc
    src/ui/TextField.cc
    src/ui/ToggleButton.cc
    src/ui/Theme.cc
    src/ui/ThemeManager.cc

    src/AvatarProvider.cc
    src/Cache.cc
    src/ChatPage.cc
    src/CommunitiesListItem.cc
    src/CommunitiesList.cc
    src/Community.cc
    src/InviteeItem.cc
    src/LoginPage.cc
    src/Logging.cpp
    src/MainWindow.cc
    src/MatrixClient.cc
    src/QuickSwitcher.cc
    src/Olm.cpp
    src/RegisterPage.cc
    src/RoomInfoListItem.cc
    src/RoomList.cc
    src/RunGuard.cc
    src/SideBarActions.cc
    src/Splitter.cc
    src/SuggestionsPopup.cpp
    src/TextInputWidget.cc
    src/TopRoomBar.cc
    src/TrayIcon.cc
    src/TypingDisplay.cc
    src/Utils.cc
    src/UserInfoWidget.cc
    src/UserSettingsPage.cc
    src/WelcomePage.cc
    src/main.cc
)

# ExternalProject dependencies
set(EXTERNAL_PROJECT_DEPS "")

find_package(ZLIB REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(MatrixStructs 0.1.0 REQUIRED)
find_package(MatrixClient 0.1.0 REQUIRED)
find_package(Olm 2 REQUIRED)
find_package(spdlog 0.16.0 CONFIG REQUIRED)

if(NOT LMDBXX_INCLUDE_DIR)
    find_path(LMDBXX_INCLUDE_DIR
            NAMES lmdb++.h
            PATHS /usr/include
                  /usr/local/include
                  $ENV{LIB_DIR}/include
                  $ENV{LIB_DIR}/include/lmdbxx)
endif()
include_directories(SYSTEM ${LMDBXX_INCLUDE_DIR})

if(NOT TWEENY_INCLUDE_DIR)
    find_path(TWEENY_INCLUDE_DIR
            NAMES tweeny/tweeny.h
            PATHS /usr/include/
                  /usr/local/include/
                  $ENV{LIB_DIR}/include/
                  $ENV{LIB_DIR}/include/tweeny)
endif()
include_directories(SYSTEM ${TWEENY_INCLUDE_DIR})

include_directories(include)
include_directories(include/ui)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/config)

qt5_wrap_cpp(MOC_HEADERS
    # Dialogs
    include/dialogs/CreateRoom.h
    include/dialogs/ImageOverlay.h
    include/dialogs/PreviewUploadOverlay.h
    include/dialogs/InviteUsers.h
    include/dialogs/JoinRoom.h
    include/dialogs/MemberList.hpp
    include/dialogs/LeaveRoom.h
    include/dialogs/Logout.h
    include/dialogs/ReadReceipts.h
    include/dialogs/ReCaptcha.hpp
    include/dialogs/RoomSettings.hpp

    # Emoji
    include/emoji/Category.h
    include/emoji/ItemDelegate.h
    include/emoji/Panel.h
    include/emoji/PickButton.h

    # Timeline
    include/timeline/TimelineItem.h
    include/timeline/TimelineView.h
    include/timeline/TimelineViewManager.h
    include/timeline/widgets/AudioItem.h
    include/timeline/widgets/FileItem.h
    include/timeline/widgets/ImageItem.h
    include/timeline/widgets/VideoItem.h

    # UI components
    include/ui/Avatar.h
    include/ui/Badge.h
    include/ui/LoadingIndicator.h
    include/ui/InfoMessage.hpp
    include/ui/FlatButton.h
    include/ui/Label.h
    include/ui/FloatingButton.h
    include/ui/OverlayWidget.h
    include/ui/ScrollBar.h
    include/ui/SnackBar.h
    include/ui/RaisedButton.h
    include/ui/Ripple.h
    include/ui/RippleOverlay.h
    include/ui/TextField.h
    include/ui/ToggleButton.h
    include/ui/Theme.h
    include/ui/ThemeManager.h

    include/notifications/Manager.h

    include/AvatarProvider.h
    include/Cache.h
    include/ChatPage.h
    include/CommunitiesListItem.h
    include/CommunitiesList.h
    include/LoginPage.h
    include/MainWindow.h
    include/InviteeItem.h
    include/QuickSwitcher.h
    include/RegisterPage.h
    include/RoomInfoListItem.h
    include/RoomList.h
    include/SideBarActions.h
    include/Splitter.h
    include/SuggestionsPopup.hpp
    include/TextInputWidget.h
    include/TopRoomBar.h
    include/TrayIcon.h
    include/TypingDisplay.h
    include/UserInfoWidget.h
    include/UserSettingsPage.h
    include/WelcomePage.h
)

#
# Bundle translations.
#
include(Translations)
set(TRANSLATION_DEPS ${LANG_QRC} ${QRC} ${QM_SRC})

set(COMMON_LIBS
    MatrixStructs::MatrixStructs
    MatrixClient::MatrixClient
    Qt5::Widgets
    Qt5::Svg
    Qt5::Concurrent
    Qt5::Multimedia)

if(APPVEYOR_BUILD)
    set(NHEKO_LIBS ${COMMON_LIBS} lmdb)
else()
    set(NHEKO_LIBS ${COMMON_LIBS} ${LMDB_LIBRARY})
endif()

if (APPLE)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -framework Foundation")
    set(SRC_FILES ${SRC_FILES} src/notifications/ManagerMac.mm)
elseif (WIN32)
    file(DOWNLOAD
         "https://raw.githubusercontent.com/mohabouje/WinToast/41ed1c58d5dce0ee9c01dbdeac05be45358d4f57/src/wintoastlib.cpp"
         ${PROJECT_SOURCE_DIR}/src/wintoastlib.cpp
         EXPECTED_HASH SHA256=1A1A7CE41C1052B12946798F4A6C67CE1FAD209C967F5ED4D720B173527E2073)

    file(DOWNLOAD
         "https://raw.githubusercontent.com/mohabouje/WinToast/41ed1c58d5dce0ee9c01dbdeac05be45358d4f57/src/wintoastlib.h"
         ${PROJECT_SOURCE_DIR}/include/wintoastlib.h
         EXPECTED_HASH SHA256=b4481023c5782733795838be22bf1a75f45d87458cd4d9a5a75f664a146eea11)

    set(SRC_FILES ${SRC_FILES} src/notifications/ManagerWin.cpp src/wintoastlib.cpp)
else ()
    set(SRC_FILES ${SRC_FILES} src/notifications/ManagerLinux.cpp)
endif ()

set(NHEKO_DEPS
    ${SRC_FILES}
    ${UI_HEADERS}
    ${MOC_HEADERS}
    ${TRANSLATION_DEPS}
    ${META_FILES_TO_INCLUDE})

if(ASAN)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address,undefined")
endif()

if(APPLE)
    add_executable (nheko ${OS_BUNDLE} ${NHEKO_DEPS})
    target_link_libraries (nheko ${NHEKO_LIBS} Qt5::MacExtras)
elseif(WIN32)
    add_executable (nheko ${OS_BUNDLE} ${ICON_FILE} ${NHEKO_DEPS})
    target_link_libraries (nheko ${NTDLIB} ${NHEKO_LIBS} Qt5::WinMain)
else()
    add_executable (nheko ${OS_BUNDLE} ${NHEKO_DEPS})
    target_link_libraries (nheko ${NHEKO_LIBS} Qt5::DBus)
endif()

if(EXTERNAL_PROJECT_DEPS)
    add_dependencies(nheko ${EXTERNAL_PROJECT_DEPS})
endif()

if(UNIX AND NOT APPLE)
    install (TARGETS nheko RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
    install (FILES "resources/nheko-16.png" DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/16x16/apps" RENAME "nheko.png")
    install (FILES "resources/nheko-32.png" DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/32x32/apps" RENAME "nheko.png")
    install (FILES "resources/nheko-48.png" DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/48x48/apps" RENAME "nheko.png")
    install (FILES "resources/nheko-64.png" DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/64x64/apps" RENAME "nheko.png")
    install (FILES "resources/nheko-128.png" DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/128x128/apps" RENAME "nheko.png")
    install (FILES "resources/nheko-256.png" DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/256x256/apps" RENAME "nheko.png")
    install (FILES "resources/nheko-512.png" DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/512x512/apps" RENAME "nheko.png")
    install (FILES "resources/nheko.desktop" DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/applications")
    install (FILES "resources/nheko.appdata.xml" DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/metainfo")

    if(NOT TARGET uninstall)
        configure_file(
	    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in"
            "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
            IMMEDIATE @ONLY)
        add_custom_target(uninstall
            COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)
    endif()
endif()
