include 'cache'

project 'test'
    kind        'Makefile'
    location    '.'

    buildcommands {
        'make -C cache'
    }

    cleancommands {
        'make clean -C cache'
    }


