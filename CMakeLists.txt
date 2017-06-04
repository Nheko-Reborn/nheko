cmake_minimum_required(VERSION 3.1)

project(nheko CXX)

option(BUILD_TESTS "Build all tests" OFF)

find_package(Qt5Widgets REQUIRED)
find_package(Qt5Network REQUIRED)
find_package(Qt5LinguistTools REQUIRED)

if (Qt5Widgets_FOUND)
    if (Qt5Widgets_VERSION VERSION_LESS 5.7.0)
        message(STATUS "Qt version ${Qt5Widgets_VERSION}")
        message(WARNING "Minimum supported Qt5 version is 5.7!")
    endif()
endif(Qt5Widgets_FOUND)

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
        -Werror \
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

set(SRC_FILES
    src/AvatarProvider.cc
    src/ChatPage.cc
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
    src/Login.cc
    src/LoginPage.cc
    src/LoginSettings.cc
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
    src/SlidingStackWidget.cc
    src/Splitter.cc
    src/Sync.cc
    src/TextInputWidget.cc
    src/TrayIcon.cc
    src/TopRoomBar.cc
    src/UserInfoWidget.cc
    src/WelcomePage.cc
    src/main.cc

    src/ui/Avatar.cc
    src/ui/Badge.cc
    src/ui/CircularProgress.cc
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

qt5_wrap_cpp(MOC_HEADERS
    include/AvatarProvider.h
    include/ChatPage.h
    include/EmojiCategory.h
    include/EmojiItemDelegate.h
    include/EmojiPanel.h
    include/EmojiPickButton.h
    include/ImageItem.h
    include/ImageOverlayDialog.h
    include/TimelineItem.h
    include/TimelineView.h
    include/TimelineViewManager.h
    include/LoginPage.h
    include/LoginSettings.h
    include/LogoutDialog.h
    include/MainWindow.h
    include/MatrixClient.h
    include/RegisterPage.h
    include/RoomInfoListItem.h
    include/RoomList.h
    include/Splitter.h
    include/UserInfoWidget.h
    include/SlidingStackWidget.h
    include/TopRoomBar.h
    include/TrayIcon.h
    include/TextInputWidget.h
    include/WelcomePage.h

    include/ui/Avatar.h
    include/ui/Badge.h
    include/ui/CircularProgress.h
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

add_library(matrix_events ${MATRIX_EVENTS} src/Deserializable.cc)
target_link_libraries(matrix_events Qt5::Core)

if (BUILD_TESTS)
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
    add_executable (nheko ${OS_BUNDLE} ${SRC_FILES} ${UI_HEADERS} ${MOC_HEADERS} ${QRC} ${LANG_QRC} ${QM_SRC})
    target_link_libraries (nheko matrix_events Qt5::Widgets Qt5::Network)

    if(WIN32)
        target_link_libraries(nheko Qt5::WinMain)
    endif(WIN32)
endif()
