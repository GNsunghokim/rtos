rootPath = os.getcwd()
build = require 'build'

workspace 'PacketNgin'
    configurations  { 'debug' , 'release' }

    filter 'configurations:debug'
        symbols 'On'
    filter 'configurations:release'
        optimize 'On'
    filter {}

include 'lib'
include 'loader'
include 'kernel'
include 'drivers'

project 'build'
    kind        'Makefile'
    location    '.'

    buildcommands {
        'make -C loader',
        'make -C kernel',
        'make -C lib',
        'make -C tools',

        'bin/lua image.lua build',
    }

    cleancommands {
        'make clean -C loader',
        'make clean -C kernel',
        'make clean -C lib',
        'make clean -C tools',

        'bin/lua image.lua clean'
    }
