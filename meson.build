project(
  'nheko',
  'cpp',
	license: 'GPL-3.0-or-later',
	version: '0.9.0',
	default_options: ['cpp_std=c++17', 'b_pie=true', 'default_library=static'],
	meson_version: '>=0.59.0'
)

compiler = meson.get_compiler('cpp')

qt5 = import('qt5')
qt5_modules = ['Core', 'Gui', 'Qml', 'Quick', 'QuickWidgets', 'Widgets', 'Multimedia', 'Svg']
if target_machine.system() == 'windows'
	#qt5_modules += 'WinMain'
elif target_machine.system() == 'darwin'
	qt5_modules += 'MacExtras'
else
	qt5_modules += 'DBus'
endif
qt5_dep = dependency('qt5', modules: qt5_modules, include_type: 'system')
inc = include_directories('src', 'includes', 'third_party/cpp-httplib-0.5.12')

deps = [
	qt5_dep,
	dependency('mtxclient', static: true, version : '>=0.6.0', fallback: ['mtxclient', 'matrix_client_dep']),
	dependency('blurhash'),
	dependency('spdlog'),
	dependency('lmdb'),
	dependency('threads'),
]

# Older cmark versions only provide the libcmark pkgconfig name, but meson only supports multiple names since 0.60.0
if meson.version().version_compare('>=0.60.0')
	deps += dependency('cmark', 'libcmark', version: '>=0.29.0')
else
	deps += dependency('cmark', version: '>=0.29.0')
endif

single_app_dep = subproject('SingleApplication-3.3.0').get_variable('singleapplication_dep')

deps += [single_app_dep]

lmdbxx_dep = dependency('', required : false)
if compiler.has_header('lmdb++.h')
	lmdbxx_dep = declare_dependency()
elif compiler.has_header('lmdbxx/lmdb++.h')
	lmdbxx_dep = declare_dependency(include_directories: 'lmdbxx')
endif

if (not lmdbxx_dep.found()
  or get_option('wrap_mode') == 'forcefallback'
  or 'lmdbxx' in get_option('force_fallback_for'))
    lmdbxx_dep = dependency('lmdb++', fallback: 'lmdb++')
endif

if not lmdbxx_dep.found()
	error('lmdb++ not found and fallback was disabled. Check https://github.com/hoytech/lmdbxx')
endif
deps += lmdbxx_dep

qtkeychain_dep = dependency('Qt5Keychain', required: get_option('wrap_mode') == 'nofallback')
if (not qtkeychain_dep.found()
  or get_option('wrap_mode') == 'forcefallback'
  or 'QtKeychain' in get_option('force_fallback_for'))
		cmake = import('cmake')
		qtkeychain_options = cmake.subproject_options()
    qtkeychain_options.add_cmake_defines({
      'BUILD_SHARED_LIBS': false,
      })
    if target_machine.system() != 'windows'
        qtkeychain_options.add_cmake_defines({
          'CMAKE_C_FLAGS':  '-fPIC',
        })
    endif
    qtkeychain_options.set_override_option('werror', 'false')
    qtkeychain_options.set_override_option('warning_level', '0')
    qtkeychain_proj = cmake.subproject('QtKeychain', options: qtkeychain_options)
    qtkeychain_dep = qtkeychain_proj.dependency('qt5keychain')

    if target_machine.system() == 'linux' or target_machine.system() == 'freebsd' or target_machine.system() == 'netbsd' or target_machine.system() == 'openbsd' or target_machine.system() == 'dragonfly'
			deps += dependency('libsecret-1', default_options: ['manpage=false', 'vapi=false', 'gtk_doc=false', 'introspection=false',]) # 'bash_completion=disabled'])
		endif
endif
deps += qtkeychain_dep

sdp_dep = dependency('gstreamer-sdp-1.0', version: '>=1.18', required: get_option('voip'))
webrtc_dep = dependency('gstreamer-webrtc-1.0', version: '>=1.18', required: get_option('voip'))
if sdp_dep.found() and webrtc_dep.found()
	deps += [sdp_dep, webrtc_dep]
	add_project_arguments('-DGSTREAMER_AVAILABLE', language: 'cpp')

	xcb_dep = dependency('xcb', required: get_option('screenshare_x11'))
	xcb_ewmh_dep = dependency('xcb-ewmh', required: get_option('screenshare_x11'))
	if xcb_dep.found() and xcb_ewmh_dep.found()
		deps += [xcb_dep, xcb_ewmh_dep]
		add_project_arguments('-DXCB_AVAILABLE', language: 'cpp')
	endif
endif

backtrace_check = '''#include<execinfo.h>
#ifndef HAVE_BACKTRACE_SYMBOLS_FD
#error "No backtrace support"
#endif
void func() {
    void *array[50];
    size_t size = backtrace(array, 50);
    backtrace_symbols_fd(array, size, STDERR_FILENO);
}
'''
backtrace_exists = compiler.links(backtrace_check)

conf_data = configuration_data()
conf_data.set('PROJECT_VERSION', meson.project_version())
conf_data.set('HOST_SYSTEM_NAME', host_machine.system())
conf_data.set10('HAVE_BACKTRACE_SYMBOLS_FD', backtrace_exists)
conf_data.set10('SPDLOG_DEBUG_ON', get_option('buildtype') == 'debug')
configure_file(input : 'meson/nheko.h',
               output : 'config_nheko.h',
               configuration : conf_data)

vcs_tag(input: 'meson/version.h', output: 'nheko_version.h', replace_string: '@PROJECT_VERSION@')

moc_files = qt5.preprocess(moc_headers :
	[
	# Dialogs
	'src/dialogs/CreateRoom.h',
	'src/dialogs/FallbackAuth.h',
	'src/dialogs/ImageOverlay.h',
	'src/dialogs/Logout.h',
	'src/dialogs/PreviewUploadOverlay.h',
	'src/dialogs/ReCaptcha.h',

	# Emoji
	'src/emoji/EmojiModel.h',
	'src/emoji/Provider.h',

	# Timeline
	'src/timeline/CommunitiesModel.h',
	'src/timeline/EventStore.h',
	'src/timeline/InputBar.h',
	'src/timeline/Reaction.h',
	'src/timeline/TimelineViewManager.h',
	'src/timeline/TimelineModel.h',
	'src/timeline/DelegateChooser.h',
	'src/timeline/Permissions.h',
	'src/timeline/RoomlistModel.h',

	# UI components
	'src/ui/Badge.h',
	'src/ui/FlatButton.h',
	'src/ui/FloatingButton.h',
	'src/ui/InfoMessage.h',
	'src/ui/Label.h',
	'src/ui/LoadingIndicator.h',
	'src/ui/MxcAnimatedImage.h',
	'src/ui/MxcMediaProxy.h',
	'src/ui/Menu.h',
	'src/ui/NhekoCursorShape.h',
	'src/ui/NhekoDropArea.h',
	'src/ui/NhekoGlobalObject.h',
	'src/ui/OverlayWidget.h',
	'src/ui/RaisedButton.h',
	'src/ui/Ripple.h',
	'src/ui/RippleOverlay.h',
	'src/ui/RoomSettings.h',
	'src/ui/SnackBar.h',
	'src/ui/TextField.h',
	'src/ui/TextLabel.h',
	'src/ui/Theme.h',
	'src/ui/ThemeManager.h',
	'src/ui/ToggleButton.h',
	'src/ui/UIA.h',
	'src/ui/UserProfile.h',

	'src/voip/CallDevices.h',
	'src/voip/CallManager.h',
	'src/voip/WebRTCSession.h',

	'src/encryption/DeviceVerificationFlow.h',
	'src/encryption/Olm.h',
	'src/encryption/SelfVerificationStatus.h',
	'src/encryption/VerificationManager.h',

	'src/notifications/Manager.h',

	'src/AvatarProvider.h',
	'src/BlurhashProvider.h',
	'src/CacheCryptoStructs.h',
	'src/Cache_p.h',
	'src/ChatPage.h',
	'src/Clipboard.h',
	'src/CombinedImagePackModel.h',
	'src/CompletionProxyModel.h',
	'src/ImagePackListModel.h',
	'src/InviteesModel.h',
	'src/JdenticonProvider.h',
	'src/LoginPage.h',
	'src/MainWindow.h',
	'src/MemberList.h',
	'src/MxcImageProvider.h',
	'src/RegisterPage.h',
	'src/SSOHandler.h',
	'src/SingleImagePackModel.h',
	'src/TrayIcon.h',
	'src/UserSettingsPage.h',
	'src/UsersModel.h',
	'src/RoomDirectoryModel.h',
	'src/RoomsModel.h',
	'src/WelcomePage.h',
	'src/ReadReceiptsModel.h',
	],
	include_directories: inc,
	dependencies: deps)

sources = [
	# Dialogs
	'src/dialogs/CreateRoom.cpp',
	'src/dialogs/FallbackAuth.cpp',
	'src/dialogs/ImageOverlay.cpp',
	'src/dialogs/Logout.cpp',
	'src/dialogs/PreviewUploadOverlay.cpp',
	'src/dialogs/ReCaptcha.cpp',

	# Emoji
	'src/emoji/EmojiModel.cpp',
	'src/emoji/Provider.cpp',


	# Timeline
	'src/timeline/CommunitiesModel.cpp',
	'src/timeline/EventStore.cpp',
	'src/timeline/InputBar.cpp',
	'src/timeline/Reaction.cpp',
	'src/timeline/TimelineViewManager.cpp',
	'src/timeline/TimelineModel.cpp',
	'src/timeline/DelegateChooser.cpp',
	'src/timeline/Permissions.cpp',
	'src/timeline/RoomlistModel.cpp',

	# UI components
	'src/ui/Badge.cpp',
	'src/ui/DropShadow.cpp',
	'src/ui/FlatButton.cpp',
	'src/ui/FloatingButton.cpp',
	'src/ui/InfoMessage.cpp',
	'src/ui/Label.cpp',
	'src/ui/LoadingIndicator.cpp',
	'src/ui/MxcAnimatedImage.cpp',
	'src/ui/MxcMediaProxy.cpp',
	'src/ui/NhekoCursorShape.cpp',
	'src/ui/NhekoDropArea.cpp',
	'src/ui/NhekoGlobalObject.cpp',
	'src/ui/OverlayModal.cpp',
	'src/ui/OverlayWidget.cpp',
	'src/ui/RaisedButton.cpp',
	'src/ui/Ripple.cpp',
	'src/ui/RippleOverlay.cpp',
	'src/ui/RoomSettings.cpp',
	'src/ui/SnackBar.cpp',
	'src/ui/TextField.cpp',
	'src/ui/TextLabel.cpp',
	'src/ui/Theme.cpp',
	'src/ui/ThemeManager.cpp',
	'src/ui/ToggleButton.cpp',
	'src/ui/UIA.cpp',
	'src/ui/UserProfile.cpp',

	'src/voip/CallDevices.cpp',
	'src/voip/CallManager.cpp',
	'src/voip/WebRTCSession.cpp',

	'src/encryption/DeviceVerificationFlow.cpp',
	'src/encryption/Olm.cpp',
	'src/encryption/SelfVerificationStatus.cpp',
	'src/encryption/VerificationManager.cpp',

	# Generic notification stuff
	'src/notifications/Manager.cpp',

	'src/AvatarProvider.cpp',
	'src/BlurhashProvider.cpp',
	'src/Cache.cpp',
	'src/ChatPage.cpp',
	'src/Clipboard.cpp',
	'src/ColorImageProvider.cpp',
	'src/CompletionProxyModel.cpp',
	'src/EventAccessors.cpp',
	'src/InviteesModel.cpp',
	'src/JdenticonProvider.cpp',
	'src/Logging.cpp',
	'src/LoginPage.cpp',
	'src/MainWindow.cpp',
	'src/MatrixClient.cpp',
	'src/MemberList.cpp',
	'src/MxcImageProvider.cpp',
	'src/ReadReceiptsModel.cpp',
	'src/RegisterPage.cpp',
	'src/SSOHandler.cpp',
	'src/CombinedImagePackModel.cpp',
	'src/SingleImagePackModel.cpp',
	'src/ImagePackListModel.cpp',
	'src/TrayIcon.cpp',
	'src/UserSettingsPage.cpp',
	'src/UsersModel.cpp',
	'src/RoomDirectoryModel.cpp',
	'src/RoomsModel.cpp',
	'src/Utils.cpp',
	'src/WelcomePage.cpp',
	'src/main.cpp',

	'third_party/blurhash/blurhash.cpp',
]

if host_machine.system() == 'darwin'
	add_languages('objcpp')
	sources += ['src/notifications/ManagerMac.mm', 'src/notifications/ManagerMac.cpp', 'src/emoji/MacHelper.mm']
	deps += dependency('Foundation', method: 'extraframework')
	deps += dependency('Cocoa', method: 'extraframework')
	deps += dependency('UserNotifications', method: 'extraframework')
elif host_machine.system() == 'windows'
	sources += ['src/notifications/ManagerWin.cpp']
	add_project_arguments('-DNOMINMAX WIN32_LEAN_AND_MEAN', '-DSTRICT', '/Zc:__cplusplus', language: 'cpp')
	deps += dependency('wintoast')
else
	sources += ['src/notifications/ManagerLinux.cpp']
endif

subdir('resources/langs')
resources = qt5.compile_resources(name: 'res', sources: 'resources/res.qrc')

executable('nheko',
	sources, moc_files, resources, translations,
	cpp_args: '-DQAPPLICATION_CLASS=QApplication',
	include_directories: inc,
	dependencies : deps,
	win_subsystem: 'windows,6.1',
	install: true)

if target_machine.system() != 'windows' and target_machine.system() != 'darwin'
	install_data([
			'resources/nheko-16.png',
			'resources/nheko-32.png',
			'resources/nheko-48.png',
			'resources/nheko-64.png',
			'resources/nheko-128.png',
			'resources/nheko-256.png',
			'resources/nheko-512.png',
			'resources/nheko.svg',
		],
		rename: [
			'16x16/apps/nheko.png',
			'32x32/apps/nheko.png',
			'48x48/apps/nheko.png',
			'64x64/apps/nheko.png',
			'128x128/apps/nheko.png',
			'256x256/apps/nheko.png',
			'512x512/apps/nheko.png',
			'scalable/apps/nheko.svg',
		], install_dir: 'share/icons/hicolor')
	install_data('resources/nheko.desktop', install_dir: 'share/applications')
	install_data('resources/nheko.appdata.xml', install_dir: 'share/metainfo')
endif

