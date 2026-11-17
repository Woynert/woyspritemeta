project "tinyfd"
    kind "SharedLib"
    language "C"
    files {
        "./tinyfd/tinyfiledialogs.c"
    }
    usage "PUBLIC"
        includedirs { "./tinyfd/" }
    usage "INTERFACE"
        links { "tinyfd" }
