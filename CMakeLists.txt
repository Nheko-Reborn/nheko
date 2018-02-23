cmake_minimum_required(VERSION 3.1)

option(APPVEYOR_BUILD "Build on appveyor" OFF)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

include(GNUInstallDirs)

# Include Qt basic functions
include(QtCommon)

project(nheko LANGUAGES C CXX VERSION 0.1.0)

# Set PROJECT_VERSION_PATCH and PROJECT_VERSION_TWEAK to 0 if not present, needed by add_project_meta
fix_project_version()

# Set additional project information
set(COMPANY "Nheko")
set(COPYRIGHT "Copyright (c) 2017 Mujx")
set(IDENTIFIER "com.mujx.nheko")

add_project_meta(META_FILES_TO_INCLUDE)

#
# LMDB
#
if(APPVEYOR_BUILD)
    set(LMDB_VERSION "LMDB_0.9.21")
    set(NTDLIB "C:/WINDDK/7600.16385.1/lib/win7/amd64/ntdll.lib")

    execute_process(
        COMMAND git clone --depth=1 --branch ${LMDB_VERSION} https://github.com/LMDB/lmdb)

    set(LMDB_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/lmdb/libraries/liblmdb)

    add_library(lmdb
        ${CMAKE_SOURCE_DIR}/lmdb/libraries/liblmdb/lmdb.h
        ${CMAKE_SOURCE_DIR}/lmdb/libraries/liblmdb/mdb.c
        ${CMAKE_SOURCE_DIR}/lmdb/libraries/liblmdb/midl.h
        ${CMAKE_SOURCE_DIR}/lmdb/libraries/liblmdb/midl.c)
else()
    find_path (LMDB_INCLUDE_DIR NAMES lmdb.h PATHS "$ENV{LMDB_DIR}/include")
    find_library (LMDB_LIBRARY NAMES lmdb PATHS "$ENV{LMDB_DIR}/lib" )
    include(FindPackageHandleStandardArgs)
    find_package_handle_standard_args(LMDB DEFAULT_MSG LMDB_INCLUDE_DIR LMDB_LIBRARY)
endif()

if (NOT EXISTS "${CMAKE_SOURCE_DIR}/libs/lmdbxx/.git" OR
    NOT EXISTS "${CMAKE_SOURCE_DIR}/libs/matrix-structs/.git")
    message(WARNING "The git submodules are not available.")
    message(STATUS "Running git submodule update --init --recursive ...")
    execute_process(COMMAND git submodule update --init --recursive)
endif()

#
# Discover Qt dependencies.
#
find_package(Qt5Widgets REQUIRED)
find_package(Qt5Network REQUIRED)
find_package(Qt5LinguistTools REQUIRED)
find_package(Qt5Concurrent REQUIRED)
find_package(Qt5Multimedia REQUIRED)

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
set(CMAKE_C_COMPILER gcc)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING
        "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel."
        FORCE)
    message("Setting build type to '${CMAKE_BUILD_TYPE}'")
else(NOT CMAKE_BUILD_TYPE)
    message("Build type set to '${CMAKE_BUILD_TYPE}'")
endif(NOT CMAKE_BUILD_TYPE)

if("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
    set(PATCH_OUT "0")
else("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
    find_program(GIT git)
    if(GIT)
        execute_process(
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            COMMAND ${GIT} rev-parse --short HEAD
            OUTPUT_VARIABLE GIT_OUT OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        set(PATCH_OUT "0-${GIT_OUT}")
    else(GIT)
        set(PATCH_OUT "0")
    endif(GIT)
endif("${CMAKE_BUILD_TYPE}" STREQUAL "Release")

set(PROJECT_VERSION_PATCH ${PATCH_OUT})
set(CPACK_PACKAGE_VERSION_MAJOR "0")
set(CPACK_PACKAGE_VERSION_MINOR "1")
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})
set(CPACK_PACKAGE_VERSION ${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH})
set(PROJECT_VERSION_MAJOR ${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR})
set(PROJECT_VERSION ${CPACK_PACKAGE_VERSION})

message(STATUS "Version: ${PROJECT_VERSION}")

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} \
        -Wall \
        -Wextra \
        -Werror \
        -pipe \
        -pedantic \
        -Wunreachable-code")

    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
        execute_process(COMMAND ${CMAKE_CXX_COMPILER} -dumpversion OUTPUT_VARIABLE GCC_VERSION)

        if (GCC_VERSION VERSION_GREATER 4.9)
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fdiagnostics-color=always" )
        endif()
    endif()

    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fdiagnostics-color=always" )
    endif()
endif()

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
    src/dialogs/LeaveRoom.cc
    src/dialogs/Logout.cc
    src/dialogs/ReadReceipts.cc

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
    src/Deserializable.cc
    src/InviteeItem.cc
    src/InputValidator.cc
    src/LoginPage.cc
    src/MainWindow.cc
    src/MatrixClient.cc
    src/QuickSwitcher.cc
    src/Register.cc
    src/RegisterPage.cc
    src/RoomInfoListItem.cc
    src/RoomList.cc
    src/RoomMessages.cc
    src/RoomState.cc
    src/RunGuard.cc
    src/SideBarActions.cc
    src/Splitter.cc
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

include_directories(include)
include_directories(include/ui)

include_directories(libs/lmdbxx)
include_directories(${LMDB_INCLUDE_DIR})

qt5_wrap_cpp(MOC_HEADERS
    # Dialogs
    include/dialogs/CreateRoom.h
    include/dialogs/ImageOverlay.h
    include/dialogs/PreviewUploadOverlay.h
    include/dialogs/InviteUsers.h
    include/dialogs/JoinRoom.h
    include/dialogs/LeaveRoom.h
    include/dialogs/Logout.h
    include/dialogs/ReadReceipts.h

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

    include/AvatarProvider.h
    include/Cache.h
    include/ChatPage.h
    include/CommunitiesListItem.h
    include/CommunitiesList.h
    include/Community.h
    include/LoginPage.h
    include/MainWindow.h
    include/InviteeItem.h
    include/MatrixClient.h
    include/QuickSwitcher.h
    include/RegisterPage.h
    include/RoomInfoListItem.h
    include/RoomList.h
    include/SideBarActions.h
    include/Splitter.h
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
FILE(GLOB LANG_TS_SRC "${CMAKE_CURRENT_SOURCE_DIR}/resources/langs/*.ts")

qt5_add_translation(QM_SRC ${LANG_TS_SRC})
add_custom_target(LANG_QRC ALL DEPENDS ${QM_SRC})

# Generate a qrc file for the translations
set(_qrc ${CMAKE_CURRENT_BINARY_DIR}/translations.qrc)

if(NOT EXISTS ${_qrc})
    file(WRITE ${_qrc} "<RCC> <qresource prefix=\"/translations\">")
    foreach(_lang ${QM_SRC})
        get_filename_component(_filename ${_lang} NAME)
        file(APPEND ${_qrc} "<file>${_filename}</file>")
    endforeach(_lang)
    file(APPEND ${_qrc} "</qresource> </RCC>")
endif()

qt5_add_resources(LANG_QRC ${_qrc})
qt5_add_resources(QRC resources/res.qrc)

add_subdirectory(libs/matrix-structs)
include_directories(${matrix_structs_SOURCE_DIR}/include)
include_directories(${matrix_structs_SOURCE_DIR}/deps)

set(COMMON_LIBS matrix_structs Qt5::Widgets Qt5::Network Qt5::Concurrent)

if(APPVEYOR_BUILD)
    set(NHEKO_LIBS ${COMMON_LIBS} lmdb)
else()
    set(NHEKO_LIBS ${COMMON_LIBS} ${LMDB_LIBRARY})
endif()

set (NHEKO_DEPS ${SRC_FILES} ${UI_HEADERS} ${MOC_HEADERS} ${QRC} ${LANG_QRC} ${QM_SRC} ${META_FILES_TO_INCLUDE})

if(APPLE)
    add_executable (nheko ${OS_BUNDLE} ${NHEKO_DEPS})
    target_link_libraries (nheko ${NHEKO_LIBS} Qt5::MacExtras Qt5::Multimedia)
elseif(WIN32)
    add_executable (nheko ${OS_BUNDLE} ${ICON_FILE} ${NHEKO_DEPS})
    target_link_libraries (nheko ${NTDLIB} ${NHEKO_LIBS} Qt5::WinMain Qt5::Multimedia)
else()
    add_executable (nheko ${OS_BUNDLE} ${NHEKO_DEPS})
    target_link_libraries (nheko ${NHEKO_LIBS} Qt5::Multimedia)
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
