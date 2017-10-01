cmake_minimum_required(VERSION 3.1)

project(nheko C CXX)

option(BUILD_TESTS "Build all tests" OFF)
option(APPVEYOR_BUILD "Build on appveyor" OFF)

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

if (NOT EXISTS "${CMAKE_SOURCE_DIR}/libs/lmdbxx/.git" )
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

set(CMAKE_CXX_STANDARD 11)
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
        -march=native \
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
    src/AvatarProvider.cc
    src/ChatPage.cc
    src/Cache.cc
    src/Deserializable.cc
    src/EmojiCategory.cc
    src/EmojiItemDelegate.cc
    src/EmojiPanel.cc
    src/EmojiPickButton.cc
    src/EmojiProvider.cc
    src/ImageItem.cc
    src/ImageOverlayDialog.cc
    src/TimelineItem.cc
    src/TimelineView.cc
    src/TimelineViewManager.cc
    src/InputValidator.cc
    src/JoinRoomDialog.cc
    src/LeaveRoomDialog.cc
    src/Login.cc
    src/LoginPage.cc
    src/LogoutDialog.cc
    src/MainWindow.cc
    src/MatrixClient.cc
    src/Profile.cc
    src/RoomInfoListItem.cc
    src/RoomMessages.cc
    src/RoomList.cc
    src/RoomState.cc
    src/Register.cc
    src/RegisterPage.cc
    src/Splitter.cc
    src/Sync.cc
    src/TextInputWidget.cc
    src/TrayIcon.cc
    src/TopRoomBar.cc
    src/UserInfoWidget.cc
    src/Versions.cc
    src/WelcomePage.cc
    src/QuickSwitcher.cc
    src/main.cc

    src/ui/Avatar.cc
    src/ui/Badge.cc
    src/ui/LoadingIndicator.cc
    src/ui/FlatButton.cc
    src/ui/OverlayModal.cc
    src/ui/ScrollBar.cc
    src/ui/RaisedButton.cc
    src/ui/Ripple.cc
    src/ui/RippleOverlay.cc
    src/ui/OverlayWidget.cc
    src/ui/TextField.cc
    src/ui/Theme.cc
    src/ui/ThemeManager.cc
)

set(MATRIX_EVENTS
    src/events/Event.cc

    src/events/AliasesEventContent.cc
    src/events/AvatarEventContent.cc
    src/events/CanonicalAliasEventContent.cc
    src/events/CreateEventContent.cc
    src/events/HistoryVisibilityEventContent.cc
    src/events/JoinRulesEventContent.cc
    src/events/MemberEventContent.cc
    src/events/MessageEventContent.cc
    src/events/NameEventContent.cc
    src/events/PowerLevelsEventContent.cc
    src/events/TopicEventContent.cc

    src/events/messages/Audio.cc
    src/events/messages/Emote.cc
    src/events/messages/File.cc
    src/events/messages/Image.cc
    src/events/messages/Location.cc
    src/events/messages/Notice.cc
    src/events/messages/Text.cc
    src/events/messages/Video.cc
)

include_directories(include)
include_directories(include/ui)
include_directories(include/events)
include_directories(include/events/messages)

include_directories(libs/lmdbxx)
include_directories(${LMDB_INCLUDE_DIR})

qt5_wrap_cpp(MOC_HEADERS
    include/AvatarProvider.h
    include/ChatPage.h
    include/EmojiCategory.h
    include/EmojiItemDelegate.h
    include/EmojiPanel.h
    include/EmojiPickButton.h
    include/ImageItem.h
    include/ImageOverlayDialog.h
    include/JoinRoomDialog.h
    include/TimelineItem.h
    include/TimelineView.h
    include/TimelineViewManager.h
    include/LeaveRoomDialog.h
    include/LoginPage.h
    include/LogoutDialog.h
    include/MainWindow.h
    include/MatrixClient.h
    include/RegisterPage.h
    include/RoomInfoListItem.h
    include/RoomList.h
    include/Splitter.h
    include/UserInfoWidget.h
    include/TopRoomBar.h
    include/TrayIcon.h
    include/TextInputWidget.h
    include/QuickSwitcher.h
    include/WelcomePage.h

    include/ui/Avatar.h
    include/ui/Badge.h
    include/ui/LoadingIndicator.h
    include/ui/FlatButton.h
    include/ui/OverlayWidget.h
    include/ui/ScrollBar.h
    include/ui/RaisedButton.h
    include/ui/Ripple.h
    include/ui/RippleOverlay.h
    include/ui/TextField.h
    include/ui/Theme.h
    include/ui/ThemeManager.h
)

#
# Bundle translations.
#
FILE(GLOB LANG_TS_SRC "${CMAKE_CURRENT_SOURCE_DIR}/resources/langs/*.ts")

qt5_add_translation(QM_SRC ${LANG_TS_SRC})
add_custom_target(LANG_QRC ALL DEPENDS ${QM_SRC})

# Generate a qrc file for the translations
set(_qrc ${CMAKE_CURRENT_BINARY_DIR}/translations.qrc)
file(WRITE ${_qrc} "<RCC> <qresource prefix=\"/translations\">")
foreach(_lang ${QM_SRC})
    get_filename_component(_filename ${_lang} NAME)
    file(APPEND ${_qrc} "<file>${_filename}</file>")
endforeach(_lang)
file(APPEND ${_qrc} "</qresource> </RCC>")

qt5_add_resources(LANG_QRC ${_qrc})
qt5_add_resources(QRC resources/res.qrc)

#
# Matrix events library.
#
add_library(matrix_events ${MATRIX_EVENTS} src/Deserializable.cc)
target_link_libraries(matrix_events Qt5::Core)

#
# Bundle icons.
#
if (APPLE)
    set(ICON_FILE resources/nheko.icns)
    set_source_files_properties(${ICON_FILE} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
endif()

if (BUILD_TESTS)
    #
    # Build tests.
    #
    enable_testing()

    find_package(GTest REQUIRED)
    include_directories(${GTEST_INCLUDE_DIRS})

    add_executable(events_test tests/events.cc)
    target_link_libraries(events_test matrix_events ${GTEST_BOTH_LIBRARIES})

    add_executable(event_collection_test tests/event_collection.cc)
    target_link_libraries(event_collection_test matrix_events ${GTEST_BOTH_LIBRARIES})

    add_executable(message_events tests/message_events.cc)
    target_link_libraries(message_events matrix_events ${GTEST_BOTH_LIBRARIES})

    add_test(MatrixEvents events_test)
    add_test(MatrixEventCollection event_collection_test)
    add_test(MatrixMessageEvents message_events)
else()
    #
    # Build the executable.
    #
if(APPVEYOR_BUILD)
    set (NHEKO_LIBS matrix_events Qt5::Widgets Qt5::Network lmdb)
else()
    set (NHEKO_LIBS matrix_events Qt5::Widgets Qt5::Network ${LMDB_LIBRARY})
endif()

    set (NHEKO_DEPS ${OS_BUNDLE} ${SRC_FILES} ${UI_HEADERS} ${MOC_HEADERS} ${QRC} ${LANG_QRC} ${QM_SRC})

    if(APPLE)
        add_executable (nheko ${NHEKO_DEPS})
        target_link_libraries (nheko ${NHEKO_LIBS} Qt5::MacExtras)
    elseif(WIN32)
        add_executable (nheko ${ICON_FILE} ${NHEKO_DEPS})
        target_link_libraries (nheko ${NTDLIB} ${NHEKO_LIBS} Qt5::WinMain)
    else()
        add_executable (nheko ${NHEKO_DEPS})
        target_link_libraries (nheko ${NHEKO_LIBS})
    endif()
endif()
