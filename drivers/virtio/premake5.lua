project 'virtio'
    kind        'ConsoleApp'
    targetname  'virtio.ko'

    build.compileProperty('x86_64')
    build.targetPath('..')

    linkoptions { '-r ../linux.ko' }


