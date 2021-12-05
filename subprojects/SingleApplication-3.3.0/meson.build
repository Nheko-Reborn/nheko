project('SingleApplication', 'cpp', default_options : ['default_library=static'])

qt5 = import('qt5')
qt5_dep = dependency('qt5', modules: ['Core', 'Network', 'Widgets'], include_type: 'system')

deps = [qt5_dep]

if host_machine.system() == 'windows'
  deps += meson.get_compiler('cpp').find_library('advapi32')
endif

moc_files = qt5.compile_moc(
  headers: ['singleapplication.h', 'singleapplication_p.h'],
  extra_args: '-DQAPPLICATION_CLASS=QApplication')
lib = static_library('SingleApplication', 'singleapplication.cpp', 'singleapplication_p.cpp', moc_files,
  dependencies: qt5_dep,
  cpp_args: '-DQAPPLICATION_CLASS=QApplication')

singleapplication_dep = declare_dependency(link_with: lib,
  include_directories: '.',
  compile_args: '-DQAPPLICATION_CLASS=QApplication')
meson.override_dependency('singleapplication', singleapplication_dep)

