project 'ext'
    kind 'StaticLib'

    build.compileProperty('x86_64')
    build.linkingProperty { 'hal', 'tlsf' }
    build.targetPath('..')

    filter { 'configurations:linux' }
        defines     { 'LINUX' }
        removefiles { 'src/malloc.c' }
    filter {}

--include 'doc'
--include 'tests'

