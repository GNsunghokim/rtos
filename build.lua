local includePath = {}
local function exportPath(lib, paths)
    includePath[lib] = {}
	for _, path in pairs(paths) do
        local dir = rootPath .. '/lib/' .. lib .. '/' .. path
        includedirs { dir }
        table.insert(includePath[lib], dir)
    end
end

local function linkingProperty(libs)
    linkoptions '-nostdlib'
    includedirs { './include' }

    if libs == nil then
        return
    end

	for _, lib in pairs(libs) do
        local dir = rootPath .. '/lib/' .. lib
        links       { lib }
        libdirs     { dir }
        -- Default include path
		includedirs { dir .. '/include' }
        -- Custom include path
        if type(includePath[lib]) ~= nil then
            includedirs { includePath[lib] }
        end
	end
end

-- Add assembly file build command
local function assemblyProperty(format)
    filter 'files:src/**.asm'
		buildmessage
			'Compiling %{file.relpath}'
		buildcommands
			{ 'nasm -o "%{cfg.objdir}/%{file.basename}.o" "%{file.relpath}" -f ' .. format }
		buildoutputs
			'%{cfg.objdir}/%{file.basename}.o'

	filter {}
end

local function targetPath(path)
    postbuildcommands   { 'ln -sfrv %{cfg.buildtarget.relpath} ' .. path}
end

local function compileProperty(arch)
    architecture(arch)
    location	    '.'

    warnings        'Extra'
    buildoptions    '-ffreestanding'
    buildoptions    '-std=gnu99'
    buildoptions    '-Wno-unused-parameter'

    files		    { 'src/**.c', 'src/**.asm', 'src/**.S' }

	local format
	if(arch == 'x86') then
		format = 'elf32'
	elseif(arch == 'x86_64') then
		format = 'elf64'
        buildoptions { '-mcmodel=kernel' }
	end

    assemblyProperty(format)
    targetPath('.')
end

-- Build.lua interface
return {
    -- Add additonal target path
    targetPath          = targetPath,
    -- Export custom include path
    exportPath          = exportPath,
    -- Add default compile property
    compileProperty     = compileProperty,
    -- Add default linking property
    linkingProperty     = linkingProperty,
}
