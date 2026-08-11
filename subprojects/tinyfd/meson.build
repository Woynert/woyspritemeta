
project('tinyfd', 'c')

tinyfd_includes = include_directories('./tinyfd')
tinyfd_lib = library('tinyfd',
  './tinyfd/tinyfiledialogs.c',
  include_directories: tinyfd_includes,
  install: true,
  c_args: [ '-Wall', '-Wextra' ]
)
tinyfd_dep = declare_dependency(
  include_directories: tinyfd_includes,
  link_with: tinyfd_lib
)
