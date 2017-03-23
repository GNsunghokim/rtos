project 'fat'
    kind        'ConsoleApp'
    targetname  'fat.ko'

    build.compileProperty('x86_64')
    build.targetPath('..')

    linkoptions { '-r ../linux.ko' }
    includedirs {  '../../kernel/src', '../../lib/ext/include' }
