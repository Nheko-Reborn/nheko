cmake_minimum_required(VERSION 3.1)

project(nheko CXX)

set(nheko_VERSION_MAJOR 0)
set(nheko_VERSION_MINOR 1)
set(nheko_VERSION_PATCH 0)

find_package(Qt5Widgets REQUIRED)
find_package(Qt5Network REQUIRED)

set(CMAKE_C_COMPILER gcc)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_CXX_FLAGS
    "${CMAKE_CXX_FLAGS} \
    -std=gnu++11 \
    -Wall \
    -Wextra \
    -Werror \
    -pedantic")

set(SRC_FILES
    src/ChatPage.cc
    src/Deserializable.cc
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

add_executable (nheko ${SRC_FILES} ${UI_HEADERS} ${MOC_HEADERS} ${QRC})
target_link_libraries (nheko Qt5::Widgets Qt5::Network)
