project(
  'wintoast', 'cpp',
  version: '0.2.0',
  default_options: [
    'warning_level=2'
  ],
  meson_version: '>= 0.54.0'
)

install_headers(
  'src/wintoastlib.h',
)

inc = include_directories('src')
wintoastlib = library(
  'wintoast',
  'src/wintoastlib.cpp',
  include_directories: inc,
)

wintoast_dep = declare_dependency(
  link_with: wintoastlib,
  include_directories: inc,
)
meson.override_dependency('wintoast', wintoast_dep)

# Generate pc file
pkg = import('pkgconfig')

pkg.generate(wintoastlib,
             name: 'wintoast',
             description: 'WinToast is a lightly library written in C++ which brings a complete integration of the modern toast notifications of Windows 8 & Windows 10.')

