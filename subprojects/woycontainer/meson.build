
# Library

project('woycontainer', 'c')
woycontainer_includes = include_directories('./src')
woycontainer_dep = declare_dependency(
  include_directories: woycontainer_includes
)

# Tests (TODO)

#warning_flags = [
  #'-Wall',
  #'-Wextra',
  #'-Wpedantic',
  #'-Werror',
  #'-Wshadow',
  #'-Wno-language-extension-token',
#]
#san_flags = [
  #'-fsanitize=address,undefined',
  #'-fno-omit-frame-pointer',
  #'-g',
  #'-O1', # O1 supposedly better than O0 or O2 for debug.
#]
#link_flags = ['-fsanitize=address,undefined']

#add_project_arguments(warning_flags + san_flags, language: 'c')
#add_project_link_arguments(link_flags, language: 'c')

#test_map_exe = executable(
  #'test_map'
  #,'src/map_test.c'
  #,include_directories : woycontainer_includes
  #,c_args : my_extra_args
#)
