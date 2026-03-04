project('rdm', 'cpp',
  version : '1.4.0',
  default_options : ['warning_level=3','cpp_std=c++20'])

# add_global_arguments('-D_NCOLOR', language: 'cpp')
# add_global_arguments('-D_DEBUG', language: 'cpp')
# add_global_arguments('-D_NLOG', language: 'cpp')

lua_dep = subproject('lua').get_variable('lua_dep')

subdir('src')

includes = include_directories('include')

exe = executable('rdm',
  sources,
  dependencies: [lua_dep],
  include_directories: includes,
  install : true)

test('basic', exe)
