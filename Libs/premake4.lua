project "cfsLib"
    kind "StaticLib"
    language "C++"
    cf_global_Libs()
    includedirs { "../ExtLibs/lua/src", "../ExtLibs/lwo", "../ExtLibs/jpeg" }
    files "Math3D/*.cpp"
    files(string.explode([[
        Bitmap/Bitmap.cpp Bitmap/jdatasrc.cpp
        ConsoleCommands/ConVar.cpp ConsoleCommands/ConFunc.cpp
        ConsoleCommands/ConsoleInterpreterImpl.cpp ConsoleCommands/ConsoleInterpreter_LuaBinding.cpp
        ConsoleCommands/Console.cpp ConsoleCommands/Console_Lua.cpp ConsoleCommands/ConsoleComposite.cpp ConsoleCommands/ConsoleStdout.cpp ConsoleCommands/ConsoleStringBuffer.cpp ConsoleCommands/ConsoleWarningsOnly.cpp ConsoleCommands/ConsoleFile.cpp
        Fonts/Font.cpp Fonts/FontTT.cpp
        FileSys/FileManImpl.cpp FileSys/FileSys_LocalPath.cpp FileSys/FileSys_ZipArchive_GV.cpp FileSys/File_local.cpp FileSys/File_memory.cpp FileSys/Password.cpp
        GuiSys/ConsoleByWindow.cpp GuiSys/Coroutines.cpp GuiSys/InitWindowTypes.cpp GuiSys/GuiImpl.cpp GuiSys/GuiManImpl.cpp
        GuiSys/Window.cpp GuiSys/WindowCreateParams.cpp GuiSys/WindowChoice.cpp GuiSys/WindowEdit.cpp GuiSys/WindowListBox.cpp GuiSys/WindowModel.cpp GuiSys/WindowPtr.cpp GuiSys/EditorData.cpp
        MapFile.cpp
        Models/Loader.cpp Models/Loader_ase.cpp Models/Loader_cmdl.cpp Models/Loader_lwo.cpp Models/Loader_md5.cpp
        Models/Loader_mdl.cpp
        Models/Model_cmdl.cpp Models/Model_dlod.cpp Models/Model_dummy.cpp Models/Model_mdl.cpp Models/Model_proxy.cpp
        Network/Network.cpp ParticleEngine/ParticleEngineMS.cpp PlatformAux.cpp Terrain/Terrain.cpp
        TextParser/TextParser.cpp
        Plants/Tree.cpp Plants/PlantDescription.cpp Plants/PlantDescrMan.cpp
        Util/Util.cpp
        Win32/Win32PrintHelp.cpp
        DebugLog.cpp TypeSys.cpp]], "%s+"))


-- The model loader classes are in a separate library in order to not have to compile many unrelated source files
-- with their header search paths, and to keep them from cfsLib, which is linked to Cafu with "--whole-archive"
-- and thus would involve all related external libraries as well.
project "ModelLoaders"
    kind "StaticLib"
    language "C++"
    cf_global_Libs()
    if os.isdir("ExtLibs/fbx/include") then
        defines "HAVE_FBX_SDK"
        includedirs "ExtLibs/fbx/include"
    end
    includedirs { "../ExtLibs/assimp/include" }
    files { "Models/Loader_assimp.cpp", "Models/Loader_fbx.cpp" }


-- This library is obsolete, and only used for our legacy programs in CaTools.
project "cfsOpenGL"
    kind "StaticLib"
    language "C++"
    cf_global_Libs()
    files { "OpenGL/OpenGLWindow.cpp" }

    configuration "windows"
        files "DirectX/DirectInput.cpp"
        includedirs "../ExtLibs/DirectX7/include"


project "GuiSysNullEditor"
    kind "StaticLib"
    language "C++"
    cf_global_Libs()
    files { "GuiSys/*_NullEditor.cpp" }


project "ClipSys"
    kind "StaticLib"
    language "C++"
    cf_global_Libs()
    includedirs "../ExtLibs/bullet/src"
    files { "ClipSys/*.cpp" }


project "MatSys"
    kind "StaticLib"
    language "C++"
    cf_global_Libs()
    files { "MaterialSystem/*.cpp" }


project "SoundSys"
    kind "StaticLib"
    language "C++"
    cf_global_Libs()
    files { "SoundSystem/*.cpp" }


project "SceneGraph"
    kind "StaticLib"
    language "C++"
    cf_global_Libs()
    files { "SceneGraph/*.cpp" }


project "MaterialSystem/RendererARBprogs"
    kind "SharedLib"
    language "C++"
    cf_global_Libs()
    files { "MaterialSystem/Common/*.cpp", "MaterialSystem/RendererARBprogs/**.cpp" }
    configuration "windows"
        links { "cfsLib", "png", "cfs_jpeg", "z", "MatSys", "opengl32", "glu32" }
    -- configuration "linux"
    --     linkoptions { "Libs/MaterialSystem/Common/linker-script" }


project "MaterialSystem/RendererCgARB1"
    kind "SharedLib"
    language "C++"
    cf_global_Libs()
    includedirs "../ExtLibs/Cg/include"
    files { "MaterialSystem/Common/*.cpp", "MaterialSystem/RendererCgARB1/**.cpp" }
    libdirs { "../ExtLibs/Cg/lib" .. (os.is64bit() and ".x64" or "") }
    links { "Cg", "CgGL" }
    configuration "windows"
        links { "cfsLib", "png", "cfs_jpeg", "z", "MatSys", "opengl32", "glu32" }
    -- configuration "linux"
    --     linkoptions { "Libs/MaterialSystem/Common/linker-script" }


project "MaterialSystem/RendererCgNV2X"
    kind "SharedLib"
    language "C++"
    cf_global_Libs()
    includedirs "../ExtLibs/Cg/include"
    files { "MaterialSystem/Common/*.cpp", "MaterialSystem/RendererCgNV2X/**.cpp" }
    libdirs { "../ExtLibs/Cg/lib" .. (os.is64bit() and ".x64" or "") }
    links { "Cg", "CgGL" }
    configuration "windows"
        links { "cfsLib", "png", "cfs_jpeg", "z", "MatSys", "opengl32", "glu32" }
    -- configuration "linux"
    --     linkoptions { "Libs/MaterialSystem/Common/linker-script" }


project "MaterialSystem/RendererNull"
    kind "SharedLib"
    language "C++"
    cf_global_Libs()
    files { "MaterialSystem/RendererNull/**.cpp" }
    configuration "windows"
        links { "cfsLib", "MatSys" }
    -- configuration "linux"
    --     linkoptions { "Libs/MaterialSystem/Common/linker-script" }


project "MaterialSystem/RendererOpenGL12"
    kind "SharedLib"
    language "C++"
    cf_global_Libs()
    files { "MaterialSystem/Common/*.cpp", "MaterialSystem/RendererOpenGL12/**.cpp" }
    configuration "windows"
        links { "cfsLib", "png", "cfs_jpeg", "z", "MatSys", "opengl32", "glu32" }
    -- configuration "linux"
    --     linkoptions { "Libs/MaterialSystem/Common/linker-script" }


-- We have the FMod libraries only for 32-bit platforms...
if not os.is64bit() then
    project "SoundSystem/SoundSysFMOD3"
        kind "SharedLib"
        language "C++"
        cf_global_Libs()
        includedirs { "../ExtLibs/fmod/api/inc" }
        files { "SoundSystem/SoundSysFMOD3/**.cpp" }
        configuration "windows"
            libdirs { "../ExtLibs/fmod/api/lib" }
            links { "cfsLib", "SoundSys", "fmodvc" }
        configuration "linux"
            libdirs { "../ExtLibs/fmod/api" }
            links { "fmod-3.75" }
            linkoptions { "Libs/SoundSystem/Common/linker-script" }
end


project "SoundSystem/SoundSysNull"
    kind "SharedLib"
    language "C++"
    cf_global_Libs()
    files { "SoundSystem/SoundSysNull/**.cpp" }
    configuration "windows"
        links { "SoundSys" }
    configuration "linux"
        linkoptions { "Libs/SoundSystem/Common/linker-script" }


project "SoundSystem/SoundSysOpenAL"
    kind "SharedLib"
    language "C++"
    cf_global_Libs()
    includedirs { "../ExtLibs/freealut/include", "../ExtLibs/mpg123/src/libmpg123", "../ExtLibs/libvorbis/include", "../ExtLibs/libogg/include" }
    files { "SoundSystem/Common/*.cpp", "SoundSystem/SoundSysOpenAL/**.cpp" }
    libdirs { "../ExtLibs/openal-win/libs/Win" .. (os.is64bit() and "64" or "32") }
    configuration "windows"
        includedirs { "../ExtLibs/openal-win/include" }
        links { "cfsLib", "SoundSys", "OpenAL32", "alut", "mpg123", "ogg", "vorbis", "vorbisfile" }
    configuration "linux"
        includedirs { "../ExtLibs/openal-soft/include" }
        linkoptions { "Libs/SoundSystem/Common/linker-script" }
