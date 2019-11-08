cmake_minimum_required(VERSION 3.11)

option(APPVEYOR_BUILD "Build on appveyor" OFF)
option(ASAN "Compile with address sanitizers" OFF)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

add_definitions(-DBOOST_MPL_LIMIT_LIST_SIZE=30)
add_definitions(-DBOOST_MPL_CFG_NO_PREPROCESSED_HEADERS)

include(GNUInstallDirs)

# Include Qt basic functions
include(QtCommon)

project(nheko LANGUAGES C CXX)
set(CPACK_PACKAGE_VERSION_MAJOR "0")
set(CPACK_PACKAGE_VERSION_MINOR "6")
set(CPACK_PACKAGE_VERSION_PATCH "4")
set(PROJECT_VERSION_MAJOR ${CPACK_PACKAGE_VERSION_MAJOR})
set(PROJECT_VERSION_MINOR ${CPACK_PACKAGE_VERSION_MINOR})
set(PROJECT_VERSION_PATCH ${CPACK_PACKAGE_VERSION_PATCH})

# Set PROJECT_VERSION_PATCH & PROJECT_VERSION_TWEAK to 0 if not present
# Needed by add_project_meta.
fix_project_version()

# Set additional project information
set(COMPANY "Nheko")
set(COPYRIGHT "Copyright (c) 2018 Nheko Contributors")
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
find_package(Qt5 COMPONENTS Core Widgets LinguistTools Concurrent Svg Multimedia Qml QuickControls2 QuickWidgets REQUIRED)
find_package(Qt5QuickCompiler)
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
        -Wunreachable-code \
	-std=c++14"
    )
    if (NOT CMAKE_COMPILER_IS_GNUCXX)
        # -Wshadow is buggy and broken in GCC, so do not enable it.
        # see https://gcc.gnu.org/bugzilla/show_bug.cgi?id=79328
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wshadow")
    endif()
endif()

if (MSVC)
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
    if(${CMAKE_BUILD_TYPE} STREQUAL "Debug")
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
set(BUILD_USER $ENV{USER})
configure_file(cmake/nheko.h config/nheko.h)


#
# Declare source and header files.
#
set(SRC_FILES
    # Dialogs
    src/dialogs/CreateRoom.cpp
    src/dialogs/ImageOverlay.cpp
    src/dialogs/PreviewUploadOverlay.cpp
    src/dialogs/InviteUsers.cpp
    src/dialogs/JoinRoom.cpp
    src/dialogs/MemberList.cpp
    src/dialogs/LeaveRoom.cpp
    src/dialogs/Logout.cpp
    src/dialogs/UserProfile.cpp
    src/dialogs/ReadReceipts.cpp
    src/dialogs/ReCaptcha.cpp
    src/dialogs/RoomSettings.cpp

    # Emoji
    src/emoji/Category.cpp
    src/emoji/ItemDelegate.cpp
    src/emoji/Panel.cpp
    src/emoji/PickButton.cpp
    src/emoji/Provider.cpp

    # Timeline
    src/timeline2/TimelineViewManager.cpp
    src/timeline2/TimelineModel.cpp
    src/timeline2/DelegateChooser.cpp
    #src/timeline/TimelineViewManager.cpp
    #src/timeline/TimelineItem.cpp
    #src/timeline/TimelineView.cpp
    #src/timeline/widgets/AudioItem.cpp
    #src/timeline/widgets/FileItem.cpp
    #src/timeline/widgets/ImageItem.cpp
    #src/timeline/widgets/VideoItem.cpp

    # UI components
    src/ui/Avatar.cpp
    src/ui/Badge.cpp
    src/ui/LoadingIndicator.cpp
    src/ui/InfoMessage.cpp
    src/ui/FlatButton.cpp
    src/ui/FloatingButton.cpp
    src/ui/Label.cpp
    src/ui/OverlayModal.cpp
    src/ui/SnackBar.cpp
    src/ui/RaisedButton.cpp
    src/ui/Ripple.cpp
    src/ui/RippleOverlay.cpp
    src/ui/OverlayWidget.cpp
    src/ui/TextField.cpp
    src/ui/TextLabel.cpp
    src/ui/ToggleButton.cpp
    src/ui/Theme.cpp
    src/ui/ThemeManager.cpp

    src/AvatarProvider.cpp
    src/Cache.cpp
    src/ChatPage.cpp
    src/CommunitiesListItem.cpp
    src/CommunitiesList.cpp
    src/InviteeItem.cpp
    src/LoginPage.cpp
    src/Logging.cpp
    src/MainWindow.cpp
    src/MatrixClient.cpp
    src/MxcImageProvider.cpp
    src/ColorImageProvider.cpp
    src/QuickSwitcher.cpp
    src/Olm.cpp
    src/RegisterPage.cpp
    src/RoomInfoListItem.cpp
    src/RoomList.cpp
    src/RunGuard.cpp
    src/SideBarActions.cpp
    src/Splitter.cpp
    src/popups/SuggestionsPopup.cpp
    src/popups/PopupItem.cpp
    src/popups/ReplyPopup.cpp
    src/popups/UserMentions.cpp
    src/TextInputWidget.cpp
    src/TopRoomBar.cpp
    src/TrayIcon.cpp
    src/TypingDisplay.cpp
    src/Utils.cpp
    src/UserInfoWidget.cpp
    src/UserSettingsPage.cpp
    src/WelcomePage.cpp
    src/main.cpp
)

# ExternalProject dependencies
set(EXTERNAL_PROJECT_DEPS "")

include(FeatureSummary)

set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_STATIC_RUNTIME OFF)
set(Boost_USE_MULTITHREADED ON)
find_package(Boost 1.66 REQUIRED
             COMPONENTS atomic
                        chrono
                        date_time
                        iostreams
                        random
                        regex
                        system
                        thread)
find_package(ZLIB REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(MatrixClient 0.3.0 REQUIRED)
find_package(Olm 2 REQUIRED)
find_package(spdlog 1.0.0 CONFIG REQUIRED)
find_package(cmark REQUIRED)
find_package(nlohmann_json 3.2.0)
set_package_properties(nlohmann_json PROPERTIES
    DESCRIPTION "JSON for Modern C++, a C++11 header-only JSON class"
    URL "https://nlohmann.github.io/json/"
    TYPE REQUIRED
)

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

include_directories(${CMAKE_SOURCE_DIR}/src)
include_directories(${Boost_INCLUDE_DIRS})

# local inclue directory
include_directories(includes)

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)

qt5_wrap_cpp(MOC_HEADERS
    # Dialogs
    src/dialogs/CreateRoom.h
    src/dialogs/ImageOverlay.h
    src/dialogs/PreviewUploadOverlay.h
    src/dialogs/InviteUsers.h
    src/dialogs/JoinRoom.h
    src/dialogs/MemberList.h
    src/dialogs/LeaveRoom.h
    src/dialogs/Logout.h
    src/dialogs/UserProfile.h
    src/dialogs/RawMessage.h
    src/dialogs/ReadReceipts.h
    src/dialogs/ReCaptcha.h
    src/dialogs/RoomSettings.h

    # Emoji
    src/emoji/Category.h
    src/emoji/ItemDelegate.h
    src/emoji/Panel.h
    src/emoji/PickButton.h

    # Timeline
    src/timeline2/TimelineViewManager.h
    src/timeline2/TimelineModel.h
    src/timeline2/DelegateChooser.h
    #src/timeline/TimelineItem.h
    #src/timeline/TimelineView.h
    #src/timeline/TimelineViewManager.h
    #src/timeline/widgets/AudioItem.h
    #src/timeline/widgets/FileItem.h
    #src/timeline/widgets/ImageItem.h
    #src/timeline/widgets/VideoItem.h

    # UI components
    src/ui/Avatar.h
    src/ui/Badge.h
    src/ui/LoadingIndicator.h
    src/ui/InfoMessage.h
    src/ui/FlatButton.h
    src/ui/Label.h
    src/ui/FloatingButton.h
    src/ui/Menu.h
    src/ui/OverlayWidget.h
    src/ui/SnackBar.h
    src/ui/RaisedButton.h
    src/ui/Ripple.h
    src/ui/RippleOverlay.h
    src/ui/TextField.h
    src/ui/TextLabel.h
    src/ui/ToggleButton.h
    src/ui/Theme.h
    src/ui/ThemeManager.h

    src/notifications/Manager.h

    src/AvatarProvider.h
    src/Cache.h
    src/ChatPage.h
    src/CommunitiesListItem.h
    src/CommunitiesList.h
    src/LoginPage.h
    src/MainWindow.h
    src/InviteeItem.h
    src/QuickSwitcher.h
    src/RegisterPage.h
    src/RoomInfoListItem.h
    src/RoomList.h
    src/SideBarActions.h
    src/Splitter.h
    src/popups/SuggestionsPopup.h
    src/popups/ReplyPopup.h
    src/popups/PopupItem.h
    src/popups/UserMentions.h
    src/TextInputWidget.h
    src/TopRoomBar.h
    src/TrayIcon.h
    src/TypingDisplay.h
    src/UserInfoWidget.h
    src/UserSettingsPage.h
    src/WelcomePage.h
)

#
# Bundle translations.
#
include(Translations)
set(TRANSLATION_DEPS ${LANG_QRC} ${QRC} ${QM_SRC})

set(COMMON_LIBS
    MatrixClient::MatrixClient
    ${Boost_LIBRARIES}
    cmark::cmark
    Qt5::Widgets
    Qt5::Svg
    Qt5::Concurrent
    Qt5::Multimedia
    Qt5::Qml
    Qt5::QuickControls2
    Qt5::QuickWidgets
    nlohmann_json::nlohmann_json)

if(APPVEYOR_BUILD)
    set(NHEKO_LIBS ${COMMON_LIBS} lmdb)
else()
    set(NHEKO_LIBS ${COMMON_LIBS} ${LMDB_LIBRARY})
endif()

if (APPLE)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -framework Foundation -framework Cocoa")
    set(SRC_FILES ${SRC_FILES} src/notifications/ManagerMac.mm src/emoji/MacHelper.mm)
elseif (WIN32)
    file(DOWNLOAD
         "https://raw.githubusercontent.com/mohabouje/WinToast/41ed1c58d5dce0ee9c01dbdeac05be45358d4f57/src/wintoastlib.cpp"
         ${PROJECT_SOURCE_DIR}/src/wintoastlib.cpp
         EXPECTED_HASH SHA256=1A1A7CE41C1052B12946798F4A6C67CE1FAD209C967F5ED4D720B173527E2073)

    file(DOWNLOAD
         "https://raw.githubusercontent.com/mohabouje/WinToast/41ed1c58d5dce0ee9c01dbdeac05be45358d4f57/src/wintoastlib.h"
         ${PROJECT_SOURCE_DIR}/src/wintoastlib.h
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
    target_compile_definitions(nheko PRIVATE WIN32_LEAN_AND_MEAN)
    target_link_libraries (nheko ${NTDLIB} ${NHEKO_LIBS} Qt5::WinMain)
else()
    add_executable (nheko ${OS_BUNDLE} ${NHEKO_DEPS})
    target_link_libraries (nheko ${NHEKO_LIBS} Qt5::DBus)
endif()

if(EXTERNAL_PROJECT_DEPS)
    add_dependencies(nheko ${EXTERNAL_PROJECT_DEPS})
endif()

set_target_properties(nheko PROPERTIES SKIP_BUILD_RPATH TRUE)

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
