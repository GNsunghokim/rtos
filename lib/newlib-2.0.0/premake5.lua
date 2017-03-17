project 'newlib'
    kind        'Makefile'
    location    '.'
    targetname  'newlib.make'

    buildcommands {
        'CFLAGS="-fno-stack-protector -D__SSE_4_1__ -msse4.1" ./configure --target=x86_64-pc-packetngin --disable-multilib --enable-newlib-elix-level=4',
    }
