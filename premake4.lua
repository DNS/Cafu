function prepend(s, t)
    for k, v in pairs(t) do t[k] = s .. v end
    return t
end


-- Global variable _ACTION can be nil, for instance if you type `premake4 --help`.
BUILD_PATH = "build/" .. (_ACTION or "")


-- A solution contains projects, and defines the available configurations
solution "CafuEngine"
    configurations { "Debug", "Release" }
    location(BUILD_PATH)   -- Generate solution files in this subdirectory.

    -- Global settings.
    -- defines { "..." }
    -- flags { "..." }      -- TODO: Check which ones we need!!

    configuration { "vs20*" }
        -- -- /GR    Enable RTTI.
        -- -- /EHsc  Enable exception handling.
        -- buildoptions { "/GR", "/EHsc" }      -- TODO: Does premake4 set this by default?
        defines { "_CRT_SECURE_NO_DEPRECATE", "_CRT_NONSTDC_NO_DEPRECATE" }
        includedirs { "ExtLibs/zlib" }
        flags { "StaticRuntime", "No64BitChecks" }

    configuration "Debug"
        -- envDebug.Append(CCFLAGS=Split("/MTd /Od /Z7 /RTC1"));
        -- envDebug.Append(LINKFLAGS=["/debug"]);
        flags { "Symbols" }
        defines { "DEBUG" }
        targetdir(BUILD_PATH .. "/bin/Debug")

    configuration "Release"
        -- envRelease.Append(CCFLAGS=Split("/MT /O2 /Ob2"));
        flags { "OptimizeSpeed", "NoIncrementalLink" }
        defines { "NDEBUG" }    -- Need NDEBUG to have the assert macro generate no runtime code.
        targetdir(BUILD_PATH .. "/bin/Release")


project "lua"
    kind "StaticLib"
    language "C"
    -- I've taken the list of source files directly from the lua/INSTALL file.
    files(prepend("ExtLibs/lua/src/", string.explode([[lapi.c lcode.c ldebug.c ldo.c ldump.c lfunc.c lgc.c llex.c
            lmem.c lobject.c lopcodes.c lparser.c lstate.c lstring.c
            ltable.c ltm.c lundump.c lvm.c lzio.c
            lauxlib.c lbaselib.c ldblib.c liolib.c lmathlib.c loslib.c
            ltablib.c lstrlib.c loadlib.c linit.c]], "%s+")))


project "bulletmath"
    kind "StaticLib"
    language "C++"
    includedirs { "ExtLibs/bullet/src" }
    files "ExtLibs/bullet/src/LinearMath/**.cpp"

project "bulletcollision"
    kind "StaticLib"
    language "C++"
    includedirs { "ExtLibs/bullet/src" }
    files "ExtLibs/bullet/src/BulletCollision/**.cpp"

project "bulletdynamics"
    kind "StaticLib"
    language "C++"
    includedirs { "ExtLibs/bullet/src" }
    files "ExtLibs/bullet/src/BulletDynamics/**.cpp"

project "bulletsoftbody"
    kind "StaticLib"
    language "C++"
    includedirs { "ExtLibs/bullet/src" }
    files "ExtLibs/bullet/src/BulletSoftBody/**.cpp"


project "lightwave"
    kind "StaticLib"
    language "C"
    includedirs { "ExtLibs/lwo" }
    files "ExtLibs/lwo/*.c"
    excludes "ExtLibs/lwo/main.c"


project "minizip"
    kind "StaticLib"
    language "C"
    files { "ExtLibs/minizip/unzip.c", "ExtLibs/minizip/ioapi.c" }


project "png"
    kind "StaticLib"
    language "C"
    files "ExtLibs/libpng/*.c"
    excludes { "ExtLibs/libpng/example.c", "ExtLibs/libpng/pngtest.c" }


project "z"
    kind "StaticLib"
    language "C"
    files "ExtLibs/zlib/*.c"
    excludes { "ExtLibs/zlib/example.c", "ExtLibs/zlib/minigzip.c" }




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


if _ACTION == "clean" then
    for a in premake.action.each() do   -- action trigger is "vs2008", "gmake", etc.
        dir = "build/" .. a.trigger
        if os.isdir(dir) then
            local result, msg=os.rmdir(dir)
            -- os.rmdir() always seems to return nil...
            -- print("Removing directory " .. dir .. "... " .. (result and 'ok!' or msg))
            print("Removing directory " .. dir .. "...")
        end
    end
end
