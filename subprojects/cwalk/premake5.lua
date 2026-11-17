project "cwalk"
    kind "SharedLib"
    language "C"
    files {
        "./cwalk/src/cwalk.c",
    }
    usage "PUBLIC"
        includedirs { "./cwalk/include" }
    usage "INTERFACE"
        links { "cwalk" }
