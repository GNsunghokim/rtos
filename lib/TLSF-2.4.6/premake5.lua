project 'tlsf'
    kind 'StaticLib'

    build.compileProperty('x86_64')
    build.linkingProperty { 'hal' }
    build.targetPath('..')

    removefiles     { 'examples/*' }

    buildoptions    { '-Wextra -Wwrite-strings' }
    buildoptions    { '-Wstrict-prototypes -Wmissing-prototypes' }
    buildoptions    { '-Wno-long-long -Wstrict-aliasing=1' }

    defines         { 'USE_MMAP=0', 'US_SBRK=0', 'USE_PRINTF=0' }
    defines         { 'TLSF_STATISTIC=1', 'TLSF_USE_LOCKS=1' }
