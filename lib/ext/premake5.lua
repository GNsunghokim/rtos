--include 'doc'

project 'ext'
    kind 'StaticLib'

    build.compileProperty('x86_64')
    build.linkingProperty { 'tlsf' }
    build.targetPath('..')

    filter { 'configurations:linux' }
        defines     { 'LINUX' }
        removefiles { 'src/malloc.c' }
    filter {}



--[[
   [group "tests"
   [    project "cache"
   [        filename    "cache"
   [        kind        "ConsoleApp"
   [        links       { "cmocka", "tlsf" }
   [        libdirs     { ".." }
   [        linkoptions { "../libtlsf.a" }
   [        includedirs { "./include", "../TLSF/src" }
   [        files       { "src/cache.c", "src/test/cache.c", "src/_malloc.c" }
   ]]
