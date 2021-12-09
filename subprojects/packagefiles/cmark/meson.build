project(
  'cmark', 'c',
  version: '0.30.2',
  default_options: [
    'warning_level=2'
  ],
  meson_version: '>= 0.54.0'
)

sources = files(
  'src/cmark.c',
  'src/node.c',
  'src/iterator.c',
  'src/blocks.c',
  'src/inlines.c',
  'src/scanners.c',
  #'src/scanners.re',
  'src/utf8.c',
  'src/buffer.c',
  'src/references.c',
  'src/render.c',
  'src/man.c',
  'src/xml.c',
  'src/html.c',
  'src/commonmark.c',
  'src/latex.c',
  'src/houdini_href_e.c',
  'src/houdini_html_e.c',
  'src/houdini_html_u.c',
  'src/cmark_ctype.c'
)

compiler = meson.get_compiler('c')

conf_data = configuration_data()
conf_data.set('HAVE_STDBOOL_H', compiler.has_header('stdbool.h'))
conf_data.set('HAVE___BUILTIN_EXPECT', compiler.compiles('int main() { __builtin_expect(0,0); return 0; }'), name: 'Has __builtin_expect')
conf_data.set('HAVE___ATTRIBUTE__', compiler.compiles('''
  int f(void) __attribute__ (());
  int main() { return 0; }
''', name: 'Has __attribute__'))

config_file = configure_file(input: 'src/config.h.in',
  output: 'config.h',
  configuration: conf_data,
  format: 'cmake',
)

split_version = meson.project_version().split('.')
version_file = configure_file(input: 'src/cmark_version.h.in',
  output: 'cmark_version.h',
  configuration: {
    'PROJECT_VERSION_MAJOR': split_version[0].to_int(),
    'PROJECT_VERSION_MINOR': split_version[1].to_int(),
    'PROJECT_VERSION_PATCH': split_version[2].to_int(),
    },
  format: 'cmake@',
)

install_headers(
  'src/cmark.h',
  config_file,
  version_file,
)

inc = include_directories('src')
cmark = library(
  'cmark',
  sources,
  config_file,
  version_file,
  include_directories: inc,
)

cmark_dep = declare_dependency(
  link_with: cmark,
  include_directories: inc,
)
meson.override_dependency('cmark', cmark_dep)

# Generate pc file
pkg = import('pkgconfig')

pkg.generate(cmark,
             name: 'cmark',
             description: 'CommonMark parsing and rendering library and program in C')

