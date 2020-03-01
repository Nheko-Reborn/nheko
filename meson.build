project('blurhash', 'cpp',
  version : '0.1',
  default_options : ['warning_level=3', 'cpp_std=c++17'])

static_library('blurhash',
           'blurhash.cpp',
           install : true)

doctest_proj = subproject('doctest')
doctest_dep = doctest_proj.get_variable('doctest_dep')
tests = executable('blurhash-tests',
           'blurhash.cpp',
	   cpp_args : '-DDOCTEST_CONFIG_IMPLEMENT_WITH_MAIN',
	   dependencies: doctest_dep,
           install : false)
test('blurhash-tests', tests)
