
# DEBUG NOTE: All of these debug options are set (albeit implicitely)
# in the Makefile. Check "make mesonSetupDebug".
#
# Compiler args:
#   -g
#   -fsanitize=address,undefined
#   -fno-omit-frame-pointer
#   -Og
# Linker args:
#   -fsanitize=address,undefined
#   -rdynamic   <-- NOT set by meson.

project('raylib-game', 'c', default_options : ['c_std=gnu11'])


# ↓↓↓ Includes function names for printing the stack.
add_project_link_arguments(['-rdynamic'], language:'c')
# ↑↑↑ @Note: Should I disable it on release? This can improve logs...


my_debug_args = [
  '-Wall',
  '-Werror',
  '-Werror=incompatible-pointer-types',
  '-Wuninitialized',
  '-Wmaybe-uninitialized',
  '-Wmissing-field-initializers',

  '-Wextra',
  '-Wpedantic',
  '-Wnull-dereference',
  # The following should be included in -Wextra, but including them anyway
  '-Wshadow',
  '-Wconversion',
  '-Wno-language-extension-token',
  '-Werror=incompatible-pointer-types',
  #'-Wc++-compat',

  '-Wno-type-limits',
  '-Wno-error=float-equal',
  '-Wno-error=unused-parameter',
  '-Wno-error=unused-function',
  '-Wno-error=unused-variable',
  '-Wno-error=stringop-truncation',
  '-Wno-unused-function',

  #'-lundef=false', ## <-- TODO: Find out what this flag does.
]



# strbuf

add_global_arguments('-DSTRBUF_PROVIDE_PRINTF', language: 'c')

# General dependencies

cc = meson.get_compiler('c')
math_dep = cc.find_library('m', required : false)

cwalk_proj = subproject('cwalk',
  default_options: [ 'default_library=shared' ])
cwalk_dep = cwalk_proj.get_variable('cwalk_dep')

mickstr_proj = subproject('mickstr',
  default_options: [ 'default_library=shared' ])
mickstr_dep = mickstr_proj.get_variable('mickstr_dep')

tinyfd_proj = subproject('tinyfd',
  default_options: [ 'default_library=shared' ])
tinyfd_dep = tinyfd_proj.get_variable('tinyfd_dep')

cmake = import('cmake')
raylib_opts = cmake.subproject_options()
raylib_opts.add_cmake_defines({'PLATFORM': 'Desktop'})
raylib_opts.add_cmake_defines({'BUILD_EXAMPLES': false})
raylib_proj = cmake.subproject('raylib', options: raylib_opts)
raylib_dep = raylib_proj.dependency('raylib')

gl_dep = dependency('gl') # Only needed to shut up clang LSP.

woycontainer_proj = subproject('woycontainer')
woycontainer_dep = woycontainer_proj.get_variable('woycontainer_dep')

# Artifacts

core_includes = include_directories('./src', './src/standalone')

gdbwui_exe = executable(
  'sprite'
  ,'src/main.c'
  ,include_directories : core_includes
  ,dependencies : [
    raylib_dep,
    math_dep,
    mickstr_dep,
    gl_dep,
    woycontainer_dep,
    tinyfd_dep,
    cwalk_dep,
  ]
  ,c_args : my_debug_args
)

drawbuffer_lab = executable(
  'drawbuffer_lab'
  ,'./src/lab/drawbuffer_lab.c'
  ,include_directories : core_includes
  ,dependencies : [
    mickstr_dep,
    raylib_dep,
    woycontainer_dep
  ]
  ,c_args : my_debug_args
)

subdir('./src/standalone')
