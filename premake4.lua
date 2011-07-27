function prepend(s, t)
    for k, v in pairs(t) do t[k] = s .. v end
    return t
end


-- A solution contains projects, and defines the available configurations
solution "CafuEngine"
    configurations { "Debug", "Release" }
    location "build"    -- Generate solution files in "build" subdirectory.

    -- Global settings.
    -- defines { "..." }
    -- flags { "..." }      -- TODO: Check which ones we need!!

    configuration { "vs20*" }
        -- -- /GR    Enable RTTI.
        -- -- /EHsc  Enable exception handling.
        -- buildoptions { "/GR", "/EHsc" }      -- TODO: Does premake4 set this by default?
        defines { "_CRT_SECURE_NO_DEPRECATE", "_CRT_NONSTDC_NO_DEPRECATE" }
        linkoptions { "/incremental:no" }       -- TODO: Why do we want this?

    configuration "Debug"
        -- envDebug.Append(CCFLAGS=Split("/MTd /Od /Z7 /RTC1"));
        -- envDebug.Append(LINKFLAGS=["/debug"]);
        flags { "Symbols" }
        defines { "DEBUG" }

    configuration "Release"
        -- envRelease.Append(CCFLAGS=Split("/MT /O2 /Ob2"));
        flags { "OptimizeSpeed" }
        defines { "NDEBUG" }    -- Need NDEBUG to have the assert macro generate no runtime code.


project "lua"
    kind "StaticLib"
    language "C"
    files(prepend("ExtLibs/lua/src/", string.explode([[lapi.c lcode.c ldebug.c ldo.c ldump.c lfunc.c lgc.c llex.c
            lmem.c lobject.c lopcodes.c lparser.c lstate.c lstring.c
            ltable.c ltm.c lundump.c lvm.c lzio.c
            lauxlib.c lbaselib.c ldblib.c liolib.c lmathlib.c loslib.c
            ltablib.c lstrlib.c loadlib.c linit.c]], "%s+")))


--[[
-- A project defines one build target
project "CaBSP"
    -- CommonWorldObject = env.StaticObject("Common/World.cpp")
    -- env.Program('CaBSP/CaBSP',   # I had preferred writing 'CaBSP' instead of 'CaBSP/CaBSP' here, but then under Linux we would get both a directory *and* an executeable with name 'CaBSP' in the build directory, which is not allowed/possible.
    --     Split("CaBSP/CaBSP.cpp CaBSP/BspTreeBuilder/BspTreeBuilder.cpp") + CommonWorldObject,
    --     LIBS=Split("SceneGraph MatSys ClipSys cfsLib cfs_jpeg bulletcollision lua minizip lightwave png z"))

    kind "ConsoleApp"
    language "C++"
    -- files { "CaBSP/**.hpp", "CaBSP/**.cpp" }
    files { "CaBSP/CaBSP.cpp", "CaBSP/BspTreeBuilder/BspTreeBuilder.cpp" }
    -- links
    -- libdirs
--]]
