project('rdm', 'cpp',
  version : '0.1',
  default_options : ['warning_level=3',
                     'cpp_std=c++20'])

add_global_arguments('-D_DEBUG', language: 'cpp')
# add_global_arguments('-D_NLOG', language: 'cpp')

lua_dep = dependency('lua-5.4')

subdir('src')

exe = executable('rdm',
  sources,
  dependencies: lua_dep,
  install : true)

test('basic', exe)
