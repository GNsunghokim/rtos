include 'linux'
include 'virtio'
include 'fat'

project 'driver'
    kind        'Makefile'
    location    '.'

    buildcommands {
        'make -C linux',
        'make -C virtio',
        'make -C fat',
    }

    cleancommands {
        'make clean -C linux',
        'make clean -C virtio',
        'make clean -C fat',
    }
