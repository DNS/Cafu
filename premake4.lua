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
        includedirs { "ExtLibs/libpng" }
        includedirs { "ExtLibs/zlib" }
        flags { "StaticRuntime", "No64BitChecks" }

    configuration "Debug"
        -- envDebug.Append(CCFLAGS=Split("/MTd /Od /Z7 /RTC1"));
        -- envDebug.Append(LINKFLAGS=["/debug"]);
        flags { "Symbols" }
        defines { "DEBUG", "SCONS_BUILD_DIR=" .. BUILD_PATH .. "/bin/Debug" }
        targetdir(BUILD_PATH .. "/bin/Debug")

    configuration "Release"
        -- envRelease.Append(CCFLAGS=Split("/MT /O2 /Ob2"));
        flags { "OptimizeSpeed", "NoIncrementalLink" }
        defines { "NDEBUG", "SCONS_BUILD_DIR=" .. BUILD_PATH .. "/bin/Release" }    -- Need NDEBUG to have the assert macro generate no runtime code.
        targetdir(BUILD_PATH .. "/bin/Release")


--[[
####################################################
###########   Build external libraries   ###########
####################################################
--]]

project "alut"
    kind "SharedLib"
    language "C"
    includedirs { "ExtLibs/freealut/include" }
    files "ExtLibs/freealut/src/*.c"

    -- "defines" created using original VC++ project file as template (removed Windows specific defines for Linux build).
    configuration "windows"
        defines { "_WINDOWS", "_USRDLL", "ALUT_EXPORTS", "WIN32", "_MBCS", "ALUT_BUILD_LIBRARY", "HAVE__STAT", "HAVE_BASETSD_H", "HAVE_SLEEP", "HAVE_WINDOWS_H" }
        includedirs { "ExtLibs/openal-win/include" }
        links "OpenAL32"
        libdirs { "ExtLibs/openal-win/libs/Win" .. (os.is64bit() and "64" or "32") }

    configuration "linux"
        defines { "HAVE_STDINT_H", "ALUT_EXPORTS", "_MBCS", "ALUT_BUILD_LIBRARY", "HAVE_STAT", "HAVE_UNISTD_H", "HAVE_BASETSD_H", "HAVE_NANOSLEEP", "HAVE_TIME_H" }
        includedirs { "ExtLibs/openal-soft/include" }


project "assimp"
    kind "StaticLib"
    language "C++"
    -- Note that assimp currently uses the _DEBUG macro, that we never define (as opposed to DEBUG in debug builds).
    defines { "ASSIMP_BUILD_BOOST_WORKAROUND", "ASSIMP_BUILD_NO_OWN_ZLIB", "ASSIMP_BUILD_NO_Q3BSP_IMPORTER" }
    includedirs { "ExtLibs/assimp/include", "ExtLibs/assimp/code/BoostWorkaround", "ExtLibs/zlib" }
    files { "ExtLibs/assimp/code/**.cpp", "ExtLibs/assimp/contrib/ConvertUTF/ConvertUTF.c", "ExtLibs/assimp/contrib/irrXML/irrXML.cpp" }


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


-- TOOD: ONLY ON WINDOWS!
project "freetype"
    kind "StaticLib"
    language "C"
    defines { "FT2_BUILD_LIBRARY" }
    includedirs { "ExtLibs/freetype/include" }
    -- The list of source files has been taken directly from the freetype/docs/INSTALL.ANY file.
    files(prepend("ExtLibs/freetype/", string.explode([[src/base/ftsystem.c src/base/ftinit.c src/base/ftdebug.c
        src/base/ftbase.c
        src/base/ftbbox.c src/base/ftglyph.c src/base/ftbdf.c src/base/ftbitmap.c src/base/ftcid.c src/base/ftfstype.c
        src/base/ftgasp.c src/base/ftgxval.c src/base/ftlcdfil.c src/base/ftmm.c src/base/ftotval.c src/base/ftpatent.c
        src/base/ftpfr.c src/base/ftstroke.c src/base/ftsynth.c src/base/fttype1.c src/base/ftwinfnt.c src/base/ftxf86.c

        src/bdf/bdf.c src/cff/cff.c src/cid/type1cid.c src/pcf/pcf.c src/pfr/pfr.c src/sfnt/sfnt.c src/truetype/truetype.c
        src/type1/type1.c src/type42/type42.c src/winfonts/winfnt.c

        src/raster/raster.c src/smooth/smooth.c

        src/autofit/autofit.c src/cache/ftcache.c src/gzip/ftgzip.c src/lzw/ftlzw.c src/gxvalid/gxvalid.c
        src/otvalid/otvalid.c src/psaux/psaux.c src/pshinter/pshinter.c src/psnames/psnames.c]], "%s+")))


project "cfs_jpeg"
    kind "StaticLib"
    language "C"
    -- includedirs { "ExtLibs/lwo" }
    files "ExtLibs/jpeg/j*.c"
    excludes { "ExtLibs/jpeg/jpegtran.c", "ExtLibs/jpeg/jmemansi.c", "ExtLibs/jpeg/jmemname.c", "ExtLibs/jpeg/jmemdos.c", "ExtLibs/jpeg/jmemmac.c" }


project "lightwave"
    kind "StaticLib"
    language "C"
    includedirs { "ExtLibs/lwo" }
    files "ExtLibs/lwo/*.c"
    excludes "ExtLibs/lwo/main.c"


project "lua"
    kind "StaticLib"
    language "C"
    files { "ExtLibs/lua/src/l*.c" }
    excludes { "ExtLibs/lua/src/lua*.c" }


project "minizip"
    kind "StaticLib"
    language "C"
    files { "ExtLibs/minizip/unzip.c", "ExtLibs/minizip/ioapi.c" }


project "mpg123"
    kind "SharedLib"
    language "C"
    defines { "OPT_GENERIC" }
    files { "ExtLibs/mpg123/src/libmpg123/*.c" }
    excludes { "ExtLibs/mpg123/src/libmpg123/dither.c", "ExtLibs/mpg123/src/libmpg123/*altivec.c",
               "ExtLibs/mpg123/src/libmpg123/*86.c", "ExtLibs/mpg123/src/libmpg123/test*.c" }

    configuration "windows"
        defines { "BUILD_MPG123_DLL" }


project "noise"
    kind "StaticLib"
    language "C++"
    includedirs { "ExtLibs/noise/src" }
    files "ExtLibs/noise/src/**.cpp"


project "ogg"
    kind "StaticLib"
    language "C"
    includedirs { "ExtLibs/libogg/include" }
    files "ExtLibs/libogg/src/*.c"


project "png"
    kind "StaticLib"
    language "C"
    files "ExtLibs/libpng/*.c"
    excludes { "ExtLibs/libpng/example.c", "ExtLibs/libpng/pngtest.c" }


project "vorbis"
    kind "StaticLib"
    language "C"
    includedirs { "ExtLibs/libvorbis/include", "ExtLibs/libvorbis/lib", "ExtLibs/libogg/include" }
    files "ExtLibs/libvorbis/lib/*.c"
    excludes { "ExtLibs/libvorbis/lib/barkmel.c", "ExtLibs/libvorbis/lib/psytune.c", "ExtLibs/libvorbis/lib/tone.c", "ExtLibs/libvorbis/lib/vorbisfile.c" }


project "vorbisfile"
    kind "StaticLib"
    language "C"
    includedirs { "ExtLibs/libvorbis/include", "ExtLibs/libvorbis/lib", "ExtLibs/libogg/include" }
    files "ExtLibs/libvorbis/lib/vorbisfile.c"


project "z"
    kind "StaticLib"
    language "C"
    files "ExtLibs/zlib/*.c"
    excludes { "ExtLibs/zlib/example.c", "ExtLibs/zlib/minigzip.c" }


--[[
################################################
###########   Build Cafu libraries   ###########
################################################
--]]

function cf_global()
    includedirs { "Libs", "ExtLibs" }
    flags { "FatalWarnings" }
    configuration "windows"
        buildoptions { "/J" }
    configuration {}
end

function cf_global_Libs()
    includedirs { ".", "../ExtLibs" }
    flags { "FatalWarnings" }
    configuration "windows"
        buildoptions { "/J" }
    configuration {}
end

include "Libs"


--[[
###############################################
###########   Build Cafu programs   ###########
###############################################
--]]

project "CaBSP"
    kind "ConsoleApp"
    language "C++"
    cf_global()
    files { "CaBSP/CaBSP.cpp", "CaBSP/BspTreeBuilder/BspTreeBuilder.cpp", "Common/World.cpp" }  -- TODO: Move Common/World.cpp into Libs/ ???
    links { "SceneGraph", "MatSys", "ClipSys", "cfsLib", "cfs_jpeg", "bulletcollision", "lua", "minizip", "lightwave", "png", "z" }


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
