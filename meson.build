project('rdm', 'cpp',
  version : '1.1.0',
  default_options : ['warning_level=3',
                     'cpp_std=c++20',
                     'default_library=static',
                     'default_both_libraries=static',
                     'prefer_static=true'])

# add_global_arguments('-D_NCOLOR', language: 'cpp')
# add_global_arguments('-D_DEBUG', language: 'cpp')
# add_global_arguments('-D_NLOG', language: 'cpp')

lua_dep = subproject('lua').get_variable('lua_dep')
git_dep = dependency('libgit2')

subdir('src')

exe = executable('rdm',
  sources,
  dependencies: [lua_dep, git_dep],
  install : true)

test('basic', exe)
