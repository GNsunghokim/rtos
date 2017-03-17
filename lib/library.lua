#!/usr/bin/lua
-- [[ Premake script for kernel workspace ]] 
workspace "Kernel"

    -- Set the target directory for a generated target file 
    targetdir "."

    -- Assembly compiler build commands --
    filter 'files:**.asm'
        buildmessage
            'Compiling %{file.relpath}'
        buildcommands
            'nasm -f elf64 -o "%{cfg.objdir}/%{file.basename}.o" "%{file.relpath}"'
        buildoutputs
            '%{cfg.objdir}/%{file.basename}.o'

    -- [[ 1. Core library ]]
    project "core"
        kind "StaticLib"
        location "core/build"
        includedirs { "core/include", "TLSF/src", "jsmn/", "../cmocka/include" }
        files { "core/**.asm", "core/**.S", "core/**.h", "core/**.c" }
         -- O0 optimziation
        optimize "On"
       -- Exclude test sources
        removefiles { "core/test/*"} 
        -- Enable exntension instruction for SSE. Do not need stack protector 
        buildoptions { "-msse4.1 -fno-stack-protector" }

    -- [[ 2. Core library for Linux ]]
    project "core_linux"
        kind "StaticLib"
        location "core/build"
        includedirs { "core/include", "TLSF/src", "jsmn/", "../cmocka/include" }
        files { "core/**.asm", "core/**.S", "core/**.h", "core/**.c" }
         -- O0 optimziation
        optimize "On"
        -- Exclude test sources and standard C library functions
        removefiles { "core/test/*", "core/src/malloc.c" }
        buildoptions { "-msse4.1 -fno-stack-protector" }
        -- Define "LINUX" to make core library for Linux OS
        defines { "LINUX" }

    -- [[ 3. TLSF library ]]
    project "tlsf"
        kind "StaticLib"
        location "TLSF/build"
        includedirs { "TLSF/src" }
        files { "TLSF/**.h", "TLSF/**.c" }
         -- O0 optimziation
        optimize "On"
        removefiles { "TLSF/examples/*" }
        -- Enable extra waring flag
        buildoptions { "-Wextra -Wwrite-strings -Wstrict-prototypes -Wmissing-prototypes -Wno-long-long -Wstrict-aliasing=1"}
        -- Define flags related TLSF
        defines { "USE_MMAP=0", "US_SBRK=0", "USE_PRINTF=0", "TLSF_STATISTIC=1", "TLSF_USE_LOCKS=1" }

    -- [[ 4. LWIP library ]]
    project "lwip"
        kind "StaticLib"
        location "lwip/build"
        includedirs { "core/include", "lwip/src/include", "lwip/src/include/ipv4" }
        files { "lwip/src/**.h", "lwip/src/**.c" }
        removefiles { "lwip/src/core/ipv6/**", "lwip/src/include/ipv6/**" }

    -- [[ 5. JSMN library ]]
    project "jsmn"
        kind "StaticLib"
        location "jsmn/build"
        files { "jsmn/**.h", "jsmn/**.c" }
        -- Disable extra waring flag
        buildoptions { "-Wno-sign-compare -Wno-unused-variable" }

    -- [[ X. External Makefile libraries ]]
    project "external"
        kind "Makefile"
        location "build"

        buildcommands {
            "cd ..; make -f Makefile" 
        }

        cleancommands {
            "cd ..; make -f Makefile clean" 
        }

    -- [[ X+1. Integration ]]
    project "integration"
        kind "Makefile"
        location "build"

        buildcommands {
            -- PacketNgin Application Library
            "ar x ../libc.a",
            "ar x ../libm.a",
            "ar x ../libtlsf.a",
            "ar x ../libcore.a",
            "ar x ../libexpat.a",
            "ar rcs ../libpacketngin.a *.o",

	    "cp -rL ../core/include/* ../../include",
	    "cp -rL ../expat/include/* ../../include",
	    "cp -rL ../openssl/include/* ../../include",
	    "cp -rL ../zlib/*.h ../../include",

            "rm ./*.o -rf",

            -- Linux Application library
            "ar x ../libtlsf.a ",		-- Blank is added at the end on purpose
            "ar x ../libcore_linux.a",
            "ar rcs ../libumpn.a *.o",
            "rm ./*.o -rf ",			-- Blank is added at the end on purpose
        }

        cleancommands {
            "rm *.o -rf",
            "rm ../*.a -rf"
        }
