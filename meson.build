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
qt5_dep = dependency('qt5', modules: ['Core', 'Qml', 'Quick', 'QuickWidgets', 'Widgets', 'DBus', 'Multimedia', 'Svg'], include_type: 'system')
inc = include_directories('src', 'includes', 'third_party/cpp-httplib-0.5.12')

deps = [
	qt5_dep,
 dependency('mtxclient', static: true, version : '>=0.6.0', fallback: ['mtxclient', 'matrix_client_dep']),
 dependency('blurhash'),
 dependency('spdlog'),
 dependency('cmark'),
 dependency('lmdb'),
 dependency('Qt5Keychain'),
 dependency('threads'),
]

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
	sources += ['src/notifications/ManagerMac.mm', 'src/notifications/ManagerMac.cpp', 'src/emoji/MacHelper.mm']
elif host_machine.system() == 'windows'
	sources += ['src/notifications/ManagerWin.cpp', 'src/wintoastlib.cpp']
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
	install: true)

