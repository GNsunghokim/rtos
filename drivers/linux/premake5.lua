project 'linux'
    kind        'ConsoleApp'
    targetname  'linux.ko'

    build.compileProperty('x86_64')
    build.linkingProperty()
    build.targetPath('..')

    linkoptions { '-r' }
    includedirs { '.', '../../kernel/src', '../../lib/ext/include' }
    removefiles { 'packetngin/**.c' }
