project 'virtio'
    kind        'ConsoleApp'
    targetname  'virtio.ko'

    build.compileProperty('x86_64')
    build.linkingProperty()
    build.targetPath('..')

    linkoptions { '-r ../linux.ko' }
    includedirs { '../../kernel/src', '../../lib/ext/include' }


