project 'kernel'
    kind 'ConsoleApp'

    build.compileProperty('x86_64')
    build.linkingProperty { 'ext', 'tlsf', 'lwip' }

    targetname      'kernel.elf'
    buildoptions    { '-mcmodel=kernel' }
    linkoptions     { '-T elf_x86_64.ld -e main' }

    -- Make version header
    prebuildcommands './mkver.sh > src/version.h'

        -- Compile test sources when configured
--[[
   [        filter 'configurations:Debug'
   [            prebuildcommands {
   [                'java -jar ../../tools/AceUnit-0.12.0.jar ../src/test'
   [            }
   [
   [            files {
   [                'src/test.c', 'src/test/**.c', 'src/test/**.h',
   [                '../tools/aceunit/src/AceUnit.c', 
   [                '../tools/aceunit/src/AceUnitData.c', 
   [                '../tools/aceunit/src/loggers/FullPlainLogger.c', 
   [                '../tools/aceunit/include/**.h'}
   [
   [            defines { 'TEST', 'ACEUNIT_ASSERTION_STYLE=ACEUNIT_ASSERTION_STYLE_RETURN', 'ACEUNIT_SUITES' }
   [            includedirs { '../tools/aceunit/include' }
   ]]
