function link_pkgconfig(lib_list)
    for _, libname in ipairs(lib_list) do
        cflags, err = os.outputof("pkg-config --cflags " .. libname)
        libs, _     = os.outputof("pkg-config --libs " .. libname)
        if (err ~= 0) then print("WAR: pkg-config error for " .. libname) end
        buildoptions { cflags }
        linkoptions  { libs }
end end


-- This looks horrible, fixme.
local MY_WARNINGS = {
  '-Wall',
  '-Werror',
  '-Werror=incompatible-pointer-types',
  '-Wuninitialized',
  '-Wmissing-field-initializers',

  '-Wextra',
  --'-Wpedantic',
  '-Wnull-dereference',
  -- The following should be included in -Wextra, but including them anyway
  '-Wshadow',
  '-Wconversion',
  '-Werror=incompatible-pointer-types',
  --'-Wc++-compat',

  '-Wno-language-extension-token',  -- Extension: (typeof).
  '-Wno-gnu-statement-expression',  -- Extension: GNU statement expressions.
  --'-Wno-error=gnu-statement-expression',  -- Extension: GNU statement expressions.


  '-Wno-type-limits',
  '-Wno-error=float-equal',
  '-Wno-error=unused-parameter',
  '-Wno-error=unused-function',
  '-Wno-error=unused-variable',
  '-Wno-error=unused-but-set-variable',
  '-Wno-unused-function',
  ---Wunused-function'
  '-Wbuiltin-macro-redefined',

  --'-Wmaybe-uninitialized',           <-- Add only if GCC.
  --'-Wno-error=stringop-truncation',  <-- Add only if GCC.

  --'-lundef=false', ---- <-- TODO: Find out what this flag does.
  --'-Wno-unknown-warning-option'
  --'-Wno-error=unknown-warning-option' -- CLANG ONLY
}


workspace "wpicker_demo_workspace"
    configurations { "Debug", "Release" }
    location "build"
    cdialect "gnu11"         -- -std=gnu11

    filter "toolset:gcc"   buildoptions { "-fdiagnostics-color=always" }
    filter "toolset:clang" buildoptions { "-fcolor-diagnostics" }
    filter {}



include "./subprojects/cwalk"
include "./subprojects/mickstr"
include "./subprojects/woycontainer"
include "./subprojects/raylib"
include "./subprojects/tinyfd"


project "meta"
    kind "ConsoleApp"
    language "C"
    includedirs { "./src", "./src/standalone" }

    -- Dependencies
    uses {
      "cwalk",
      "mickstr",
      "woycontainer",
      "raylib",
      "tinyfd",
    }
    link_raylib()

    files {
        "./src/main.c",
    }

    -- note(woy): Consider moving me to workspace scope.
    filter "configurations:Debug"
        defines { "DEBUG" }
        symbols "On"                                -- -g
        sanitize { "Address", "UndefinedBehavior" } -- -fsanitize=address,undefined
        optimize "Debug"                            -- -Og
        omitframepointer "On"                       -- -fno-omit-frame-pointer
    filter "configurations:Release"
        defines { "NDEBUG" }
        optimize "On"
    buildoptions { MY_WARNINGS }

