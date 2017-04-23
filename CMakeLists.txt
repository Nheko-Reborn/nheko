cmake_minimum_required(VERSION 3.1)

project(nheko CXX)

find_package(Qt5Widgets REQUIRED)
find_package(Qt5Network REQUIRED)

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
        -std=c++11 \
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
    src/ChatPage.cc
    src/Deserializable.cc
    src/EmojiCategory.cc
    src/EmojiItemDelegate.cc
    src/EmojiPanel.cc
    src/EmojiPickButton.cc
    src/EmojiProvider.cc
    src/HistoryView.cc
    src/HistoryViewItem.cc
    src/HistoryViewManager.cc
    src/InputValidator.cc
    src/Login.cc
    src/LoginPage.cc
    src/MainWindow.cc
    src/MatrixClient.cc
    src/Profile.cc
    src/RoomInfo.cc
    src/RoomInfoListItem.cc
    src/RoomList.cc
    src/Register.cc
    src/RegisterPage.cc
    src/SlidingStackWidget.cc
    src/Sync.cc
    src/TextInputWidget.cc
    src/TopRoomBar.cc
    src/UserInfoWidget.cc
    src/WelcomePage.cc
    src/main.cc

    src/ui/Avatar.cc
    src/ui/Badge.cc
    src/ui/FlatButton.cc
    src/ui/RaisedButton.cc
    src/ui/Ripple.cc
    src/ui/RippleOverlay.cc
    src/ui/OverlayWidget.cc
    src/ui/TextField.cc
    src/ui/Theme.cc
    src/ui/ThemeManager.cc
)

include_directories(include)
include_directories(include/ui)

qt5_wrap_ui (UI_HEADERS
    forms/ChatPage.ui
    forms/MainWindow.ui
    forms/RoomList.ui
)

qt5_wrap_cpp(MOC_HEADERS
    include/ChatPage.h
    include/EmojiCategory.h
    include/EmojiItemDelegate.h
    include/EmojiPanel.h
    include/EmojiPickButton.h
    include/HistoryView.h
    include/HistoryViewItem.h
    include/HistoryViewManager.h
    include/LoginPage.h
    include/MainWindow.h
    include/MatrixClient.h
    include/RegisterPage.h
    include/RoomInfoListItem.h
    include/RoomList.h
    include/UserInfoWidget.h
    include/SlidingStackWidget.h
    include/TopRoomBar.h
    include/TextInputWidget.h
    include/WelcomePage.h

    include/ui/Avatar.h
    include/ui/Badge.h
    include/ui/FlatButton.h
    include/ui/OverlayWidget.h
    include/ui/RaisedButton.h
    include/ui/Ripple.h
    include/ui/RippleOverlay.h
    include/ui/TextField.h
    include/ui/Theme.h
    include/ui/ThemeManager.h
)

qt5_add_resources(QRC resources/res.qrc)

add_executable (nheko ${OS_BUNDLE} ${SRC_FILES} ${UI_HEADERS} ${MOC_HEADERS} ${QRC})
target_link_libraries (nheko Qt5::Widgets Qt5::Network)

if(WIN32)
  target_link_libraries(nheko Qt5::WinMain)
endif(WIN32)
