cmake_minimum_required(VERSION 3.13)

option(APPVEYOR_BUILD "Build on appveyor" OFF)
option(CI_BUILD "Set when building in CI. Enables -Werror where possible" OFF)
option(ASAN "Compile with address sanitizers" OFF)
option(QML_DEBUGGING "Enable qml debugging" OFF)
option(COMPILE_QML "Compile Qml. It will make Nheko faster, but you will need to recompile it, when you update Qt." OFF)
if(UNIX AND NOT APPLE)
	option(MAN "Build man page" ON)
else()
	option(MAN "Build man page" OFF)
endif()
option(FLATPAK "Set this only if Nheko is built as a flatpak" OFF)

set(
	CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_LIST_DIR}/toolchain.cmake"
	CACHE
	FILEPATH "Default toolchain"
	)
set(CMAKE_CXX_STANDARD 17 CACHE STRING "C++ standard")
set(CMAKE_CXX_STANDARD_REQUIRED ON CACHE BOOL "Require C++ standard to be supported")
set(CMAKE_POSITION_INDEPENDENT_CODE ON CACHE BOOL "compile as PIC by default")

option(HUNTER_ENABLED "Enable Hunter package manager" OFF)
include("cmake/HunterGate.cmake")
HunterGate(
	URL "https://github.com/cpp-pm/hunter/archive/v0.23.305.tar.gz"
	SHA1 "fc8d7a6dac2fa23681847b3872d88d3839b657b0"
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

option(USE_BUNDLED_SPDLOG "Use the bundled version of spdlog." ${HUNTER_ENABLED})
option(USE_BUNDLED_OLM "Use the bundled version of libolm." ${HUNTER_ENABLED})
option(USE_BUNDLED_GTEST "Use the bundled version of Google Test." ${HUNTER_ENABLED})
option(USE_BUNDLED_CMARK "Use the bundled version of cmark." ${HUNTER_ENABLED})
option(USE_BUNDLED_JSON "Use the bundled version of nlohmann json." ${HUNTER_ENABLED})
option(USE_BUNDLED_OPENSSL "Use the bundled version of OpenSSL." OFF)
option(USE_BUNDLED_MTXCLIENT "Use the bundled version of the Matrix Client library." ${HUNTER_ENABLED})
option(USE_BUNDLED_LMDB "Use the bundled version of lmdb." ${HUNTER_ENABLED})
option(USE_BUNDLED_LMDBXX "Use the bundled version of lmdb++." ${HUNTER_ENABLED})
option(USE_BUNDLED_QTKEYCHAIN "Use the bundled version of Qt5Keychain." ${HUNTER_ENABLED})
option(USE_BUNDLED_COEURL "Use a bundled version of the Curl wrapper"
	${HUNTER_ENABLED})
option(USE_BUNDLED_LIBEVENT "Use the bundled version of spdlog." ${HUNTER_ENABLED})
option(USE_BUNDLED_LIBCURL "Use the bundled version of spdlog." ${HUNTER_ENABLED})

include(CMakeDependentOption)
set(VOIP_DEFAULT ON)
if (APPLE OR WIN32)
	set(VOIP_DEFAULT OFF)
endif()
option(VOIP "Whether to enable voip support. Disable this, if you don't have gstreamer." ${VOIP_DEFAULT})
cmake_dependent_option(SCREENSHARE_X11 "Whether to enable screenshare support on X11." ON "VOIP" OFF)

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

project(nheko LANGUAGES CXX C)

include(GNUInstallDirs)

set(CPACK_PACKAGE_VERSION_MAJOR "0")
set(CPACK_PACKAGE_VERSION_MINOR "9")
set(CPACK_PACKAGE_VERSION_PATCH "3")
set(PROJECT_VERSION_MAJOR ${CPACK_PACKAGE_VERSION_MAJOR})
set(PROJECT_VERSION_MINOR ${CPACK_PACKAGE_VERSION_MINOR})
set(PROJECT_VERSION_PATCH ${CPACK_PACKAGE_VERSION_PATCH})

# Set PROJECT_VERSION_PATCH & PROJECT_VERSION_TWEAK to 0 if not present
# Needed by add_project_meta.
fix_project_version()

# Set additional project information
set(COMPANY "Nheko")
set(COPYRIGHT "Copyright (c) 2022 Nheko Contributors")
set(IDENTIFIER "io.github.nheko-reborn.nheko")

add_project_meta(META_FILES_TO_INCLUDE)

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
## coeurl
#
## Need to repeat all libevent deps?!?
if (USE_BUNDLED_LIBEVENT)
	hunter_add_package_safe(Libevent)
	find_package(Libevent CONFIG REQUIRED)
else()
	find_package(PkgConfig REQUIRED) 
	pkg_check_modules(libevent_core REQUIRED IMPORTED_TARGET libevent_core)
	if (WIN32)
		pkg_check_modules(libevent_windows REQUIRED IMPORTED_TARGET libevent_windows)
	else()
		pkg_check_modules(libevent_pthreads REQUIRED IMPORTED_TARGET
			libevent_pthreads)
	endif()
endif()

# curl
if (USE_BUNDLED_LIBCURL)
	hunter_add_package_safe(CURL)
	find_package(CURL CONFIG REQUIRED)
else()
	find_package(PkgConfig REQUIRED) 
	pkg_check_modules(libcurl REQUIRED IMPORTED_TARGET libcurl)
endif()

# spdlog
if(USE_BUNDLED_SPDLOG)
	hunter_add_package_safe(spdlog)
endif()
find_package(spdlog 1.0.0 CONFIG REQUIRED)

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
find_package(Qt5 5.15 COMPONENTS Core Widgets LinguistTools Concurrent Svg Multimedia Qml QuickControls2 QuickWidgets REQUIRED)
find_package(Qt5QuickCompiler)
find_package(Qt5DBus)

if (USE_BUNDLED_QTKEYCHAIN)
	include(FetchContent)
	FetchContent_Declare(
		qt5keychain
		GIT_REPOSITORY https://github.com/frankosterfeld/qtkeychain.git
		GIT_TAG        v0.13.1
		)
	if (BUILD_SHARED_LIBS)
		set(QTKEYCHAIN_STATIC OFF CACHE INTERNAL "")
	else()
		set(QTKEYCHAIN_STATIC ON CACHE INTERNAL "")
	endif()
	set(BUILD_TEST_APPLICATION OFF CACHE INTERNAL "")
	FetchContent_MakeAvailable(qt5keychain)
else()
	find_package(Qt5Keychain REQUIRED)
endif()

if (APPLE)
	find_package(Qt5MacExtras REQUIRED)
endif(APPLE)

if (Qt5Widgets_FOUND)
	if (Qt5Widgets_VERSION VERSION_LESS 5.15.0)
		message(STATUS "Qt version ${Qt5Widgets_VERSION}")
		message(WARNING "Minimum supported Qt5 version is 5.15!")
	endif()
endif(Qt5Widgets_FOUND)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
if(NOT MSVC)
	set(
		CMAKE_CXX_FLAGS
		"${CMAKE_CXX_FLAGS} \
		-Wall \
		-Wextra \
		-pipe \
		-pedantic \
		-fsized-deallocation \
		-fdiagnostics-color=always \
		-Wunreachable-code \
		-Wno-attributes"
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
	# Dialogs
	src/dialogs/FallbackAuth.cpp
	src/dialogs/ReCaptcha.cpp

	# Emoji
	src/emoji/EmojiModel.cpp
	src/emoji/Provider.cpp


	# Timeline
	src/timeline/CommunitiesModel.cpp
	src/timeline/EventStore.cpp
	src/timeline/InputBar.cpp
	src/timeline/Reaction.cpp
	src/timeline/TimelineViewManager.cpp
	src/timeline/TimelineModel.cpp
	src/timeline/DelegateChooser.cpp
	src/timeline/Permissions.cpp
	src/timeline/PresenceEmitter.cpp
	src/timeline/RoomlistModel.cpp

	# UI components
	src/ui/HiddenEvents.cpp
	src/ui/MxcAnimatedImage.cpp
	src/ui/MxcMediaProxy.cpp
	src/ui/NhekoCursorShape.cpp
	src/ui/NhekoEventObserver.cpp
	src/ui/NhekoDropArea.cpp
	src/ui/NhekoGlobalObject.cpp
	src/ui/RoomSettings.cpp
	src/ui/Theme.cpp
	src/ui/ThemeManager.cpp
	src/ui/UIA.cpp
	src/ui/UserProfile.cpp

	src/voip/CallDevices.cpp
	src/voip/CallManager.cpp
	src/voip/WebRTCSession.cpp

	src/encryption/DeviceVerificationFlow.cpp
	src/encryption/Olm.cpp
	src/encryption/SelfVerificationStatus.cpp
	src/encryption/VerificationManager.cpp

	# Generic notification stuff
	src/notifications/Manager.cpp

	src/AvatarProvider.cpp
	src/BlurhashProvider.cpp
	src/Cache.cpp
	src/ChatPage.cpp
	src/Clipboard.cpp
	src/ColorImageProvider.cpp
	src/CompletionProxyModel.cpp
	src/EventAccessors.cpp
	src/InviteesModel.cpp
	src/JdenticonProvider.cpp
	src/Logging.cpp
	src/LoginPage.cpp
	src/MainWindow.cpp
	src/MatrixClient.cpp
	src/MemberList.cpp
	src/MxcImageProvider.cpp
	src/ReadReceiptsModel.cpp
	src/RegisterPage.cpp
	src/SSOHandler.cpp
	src/CombinedImagePackModel.cpp
	src/SingleImagePackModel.cpp
	src/ImagePackListModel.cpp
	src/TrayIcon.cpp
	src/UserSettingsPage.cpp
	src/UsersModel.cpp
	src/RoomDirectoryModel.cpp
	src/RoomsModel.cpp
	src/Utils.cpp
	src/main.cpp

	third_party/blurhash/blurhash.cpp
	)


include(FeatureSummary)


if(USE_BUNDLED_OPENSSL)
	hunter_add_package_safe(OpenSSL)
endif()
find_package(OpenSSL 1.1.0 REQUIRED)
if(USE_BUNDLED_MTXCLIENT)
	include(FetchContent)
	FetchContent_Declare(
		MatrixClient
		GIT_REPOSITORY https://github.com/Nheko-Reborn/mtxclient.git
		GIT_TAG        f1250b3f212171cacca791ed585cab53cf34c199
		)
	set(BUILD_LIB_EXAMPLES OFF CACHE INTERNAL "")
	set(BUILD_LIB_TESTS OFF CACHE INTERNAL "")
	FetchContent_MakeAvailable(MatrixClient)
else()
	find_package(MatrixClient 0.6.2 REQUIRED)
endif()
if(USE_BUNDLED_OLM)
	include(FetchContent)
	FetchContent_Declare(
		Olm
		GIT_REPOSITORY https://gitlab.matrix.org/matrix-org/olm.git
		GIT_TAG        3.2.8
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
		GIT_TAG        0.30.2
		CMAKE_ARGS     "CMARK_STATIC=ON CMARK_SHARED=OFF CMARK_TESTS=OFF CMARK_TESTS=OFF"
		)
	FetchContent_MakeAvailable(cmark)
	if (MSVC)
		add_library(cmark::cmark ALIAS cmark)
	else()
		add_library(cmark::cmark ALIAS cmark_static)
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

if (VOIP)
	include(FindPkgConfig)
	pkg_check_modules(GSTREAMER REQUIRED IMPORTED_TARGET gstreamer-sdp-1.0>=1.18 gstreamer-webrtc-1.0>=1.18)
	if (SCREENSHARE_X11 AND NOT WIN32 AND NOT APPLE)
		pkg_check_modules(XCB REQUIRED IMPORTED_TARGET xcb xcb-ewmh)
	endif()
endif()

# single instance functionality
set(QAPPLICATION_CLASS QApplication CACHE STRING "Inheritance class for SingleApplication")
add_subdirectory(third_party/SingleApplication-3.3.2/)

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)

# this must be defined here to make the moc work properly
if (NOT APPLE AND NOT WIN32)
	add_compile_definitions(NHEKO_DBUS_SYS)
endif()

qt5_wrap_cpp(MOC_HEADERS
	# Dialogs
	src/dialogs/FallbackAuth.h
	src/dialogs/ReCaptcha.h

	# Emoji
	src/emoji/EmojiModel.h
	src/emoji/Provider.h

	# Timeline
	src/timeline/CommunitiesModel.h
	src/timeline/EventStore.h
	src/timeline/InputBar.h
	src/timeline/Reaction.h
	src/timeline/TimelineViewManager.h
	src/timeline/TimelineModel.h
	src/timeline/DelegateChooser.h
	src/timeline/Permissions.h
	src/timeline/PresenceEmitter.h
	src/timeline/RoomlistModel.h

	# UI components
	src/ui/HiddenEvents.h
	src/ui/MxcAnimatedImage.h
	src/ui/MxcMediaProxy.h
	src/ui/NhekoCursorShape.h
	src/ui/NhekoEventObserver.h
	src/ui/NhekoDropArea.h
	src/ui/NhekoGlobalObject.h
	src/ui/RoomSettings.h
	src/ui/Theme.h
	src/ui/ThemeManager.h
	src/ui/UIA.h
	src/ui/UserProfile.h

	src/voip/CallDevices.h
	src/voip/CallManager.h
	src/voip/WebRTCSession.h

	src/encryption/DeviceVerificationFlow.h
	src/encryption/Olm.h
	src/encryption/SelfVerificationStatus.h
	src/encryption/VerificationManager.h

	src/notifications/Manager.h

	src/AvatarProvider.h
	src/BlurhashProvider.h
	src/CacheCryptoStructs.h
	src/Cache_p.h
	src/ChatPage.h
	src/Clipboard.h
	src/CombinedImagePackModel.h
	src/CompletionProxyModel.h
	src/ImagePackListModel.h
	src/InviteesModel.h
	src/JdenticonProvider.h
	src/LoginPage.h
	src/MainWindow.h
	src/MemberList.h
	src/MxcImageProvider.h
	src/RegisterPage.h
	src/RoomsModel.h
	src/SSOHandler.h
	src/SingleImagePackModel.h
	src/TrayIcon.h
	src/UserSettingsPage.h
	src/UsersModel.h
	src/RoomDirectoryModel.h
	src/RoomsModel.h
	src/ReadReceiptsModel.h
)

#
# Bundle translations.
#
include(Translations)
set(TRANSLATION_DEPS ${LANG_QRC} ${QRC} ${QM_SRC})

if (APPLE)
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -framework Foundation -framework Cocoa -framework UserNotifications")
	set(SRC_FILES ${SRC_FILES} src/notifications/ManagerMac.mm src/notifications/ManagerMac.cpp src/emoji/MacHelper.mm)
	if(${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.16.0")
		set_source_files_properties( src/notifications/ManagerMac.mm src/emoji/MacHelper.mm PROPERTIES SKIP_PRECOMPILE_HEADERS ON)
	endif()
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
	set(SRC_FILES ${SRC_FILES}
        src/dbus/NhekoDBusApi.cpp
        src/dbus/NhekoDBusBackend.cpp
        src/notifications/ManagerLinux.cpp
	)
    qt5_wrap_cpp(MOC_HEADERS
        src/dbus/NhekoDBusApi.h
        src/dbus/NhekoDBusBackend.h
	)
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

if(WIN32)
	add_executable (nheko WIN32 ${OS_BUNDLE} ${NHEKO_DEPS})
	target_compile_definitions(nheko PRIVATE _WIN32_WINNT=0x0601 NOMINMAX WIN32_LEAN_AND_MEAN STRICT)
else()
	add_executable (nheko ${OS_BUNDLE} ${NHEKO_DEPS})

	if (HAVE_BACKTRACE_SYMBOLS_FD AND NOT CMAKE_BUILD_TYPE STREQUAL "Release")
		set_target_properties(nheko PROPERTIES ENABLE_EXPORTS ON)
	endif()
endif()

if(APPLE)
	target_link_libraries (nheko PRIVATE Qt5::MacExtras)
elseif(WIN32)
	target_compile_definitions(nheko PRIVATE WIN32_LEAN_AND_MEAN)
	target_link_libraries (nheko PRIVATE ${NTDLIB} Qt5::WinMain)
	if(MSVC)
		target_compile_options(nheko PUBLIC "/Zc:__cplusplus")
	endif()
else()
	target_link_libraries (nheko PRIVATE Qt5::DBus)
	if (FLATPAK)
		target_compile_definitions(nheko PRIVATE NHEKO_FLATPAK)
	endif()
endif()
target_include_directories(nheko PRIVATE src includes third_party/blurhash third_party/cpp-httplib-0.5.12)

# Fixup bundled keychain include dirs
if (USE_BUNDLED_QTKEYCHAIN)
	target_include_directories(nheko PRIVATE ${qt5keychain_SOURCE_DIR} ${qt5keychain_BINARY_DIR})
endif()

target_link_libraries(nheko PRIVATE
	MatrixClient::MatrixClient
	cmark::cmark
	spdlog::spdlog
	Qt5::Widgets
	Qt5::Svg
	Qt5::Concurrent
	Qt5::Multimedia
	Qt5::Qml
	Qt5::QuickControls2
	Qt5::QuickWidgets
	qt5keychain
	nlohmann_json::nlohmann_json
	lmdbxx::lmdbxx
	liblmdb::lmdb
	SingleApplication::SingleApplication)

if(${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.16.0")
	target_precompile_headers(nheko
		PRIVATE
		<string>
		<algorithm>
		)
endif()

if (TARGET PkgConfig::GSTREAMER)
	target_link_libraries(nheko PRIVATE PkgConfig::GSTREAMER)
	target_compile_definitions(nheko PRIVATE GSTREAMER_AVAILABLE)
	if (TARGET PkgConfig::XCB)
		target_link_libraries(nheko PRIVATE PkgConfig::XCB)
		target_compile_definitions(nheko PRIVATE XCB_AVAILABLE)
	endif()
endif()

if(USE_BUNDLED_COEURL)
	include(FetchContent)
	FetchContent_Declare(
		coeurl
		GIT_REPOSITORY https://nheko.im/Nheko-Reborn/coeurl.git
		GIT_TAG        v0.1.2
		)
	FetchContent_MakeAvailable(coeurl)
	target_link_libraries(nheko PUBLIC coeurl::coeurl)
elseif(coeurl_DIR)
	find_package(coeurl CONFIG REQUIRED)
	target_link_libraries(nheko PUBLIC coeurl::coeurl)
else()
	find_package(PkgConfig REQUIRED)
	pkg_check_modules(coeurl REQUIRED IMPORTED_TARGET coeurl>=0.1.1)
	target_link_libraries(nheko PUBLIC PkgConfig::coeurl)
endif()

if(MSVC)
	target_link_libraries(nheko PRIVATE ntdll)
endif()


if(QML_DEBUGGING)
	target_compile_definitions(nheko PRIVATE QML_DEBUGGING)
endif()


if(NOT MSVC AND NOT HAIKU)
	if("${CMAKE_BUILD_TYPE}" STREQUAL "Debug" OR CI_BUILD)
		target_compile_options(nheko PRIVATE "-Werror")
	endif()
endif()

if(MAN)
	add_subdirectory(man)
endif()

set_target_properties(nheko PROPERTIES CMAKE_SKIP_INSTALL_RPATH TRUE)

if(UNIX AND NOT APPLE)
	if(FLATPAK)
		set(APPID "io.github.NhekoReborn.Nheko")
	else()
		set(APPID "nheko")
	endif()

	install (TARGETS nheko RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
	install (FILES "resources/nheko-16.png" DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/16x16/apps" RENAME "${APPID}.png")
	install (FILES "resources/nheko-32.png" DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/32x32/apps" RENAME "${APPID}.png")
	install (FILES "resources/nheko-48.png" DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/48x48/apps" RENAME "${APPID}.png")
	install (FILES "resources/nheko-64.png" DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/64x64/apps" RENAME "${APPID}.png")
	install (FILES "resources/nheko-128.png" DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/128x128/apps" RENAME "${APPID}.png")
	install (FILES "resources/nheko-256.png" DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/256x256/apps" RENAME "${APPID}.png")
	install (FILES "resources/nheko-512.png" DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/512x512/apps" RENAME "${APPID}.png")
	install (FILES "resources/nheko.svg" DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/scalable/apps" RENAME "${APPID}.svg")
	install (FILES "resources/_nheko" DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/zsh/site-functions")

	configure_file("resources/nheko.desktop.in" "resources/nheko.desktop" @ONLY)
	install (FILES "${CMAKE_CURRENT_BINARY_DIR}/resources/nheko.desktop" DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/applications" RENAME "${APPID}.desktop")
	configure_file("resources/nheko.appdata.xml.in" "resources/nheko.appdata.xml" @ONLY)
	install (FILES "${CMAKE_CURRENT_BINARY_DIR}/resources/nheko.appdata.xml" DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/metainfo" RENAME "${APPID}.appdata.xml")

	if(NOT TARGET uninstall)
		configure_file(
			"${CMAKE_CURRENT_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in"
			"${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
			IMMEDIATE @ONLY)
		add_custom_target(uninstall
			COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)
	endif()
endif()
