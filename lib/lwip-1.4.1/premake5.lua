project "lwip"
    kind "StaticLib"

    build.compileProperty('x86_64')
    build.targetPath('..')
    build.exportIncludePath('lwip', {
        'src/include',
        'src/include/ipv4',
    })

    removefiles         { 'src/core/ipv6/**.c' }
    includedirs         { '../ext/include' }
