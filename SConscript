import os, platform, sys
import CompilerSetup

Import('env', 'buildMode', 'compiler')


CommonWorldObject = env.StaticObject("Common/World.cpp")

env.Program('CaBSP/CaBSP',   # I had preferred writing 'CaBSP' instead of 'CaBSP/CaBSP' here, but then under Linux we would get both a directory *and* an executeable with name 'CaBSP' in the build directory, which is not allowed/possible.
    Split("CaBSP/CaBSP.cpp CaBSP/BspTreeBuilder/BspTreeBuilder.cpp") + CommonWorldObject,
    LIBS=Split("SceneGraph MatSys ClipSys cfsLib cfs_jpeg bulletcollision lua minizip lightwave png z"))

env.Program('CaPVS/CaPVS',
    Split("CaPVS/CaPVS.cpp CaPVS/CaPVSWorld.cpp") + CommonWorldObject,
    LIBS=Split("SceneGraph MatSys ClipSys cfsLib cfs_jpeg bulletcollision lua minizip lightwave png z"))

env.Program('CaLight/CaLight',
    Split("CaLight/CaLight.cpp CaLight/CaLightWorld.cpp") + CommonWorldObject,
    LIBS=Split("SceneGraph MatSys ClipSys cfsLib cfs_jpeg bulletcollision lua minizip lightwave png z"))

env.Program('CaSHL/CaSHL',
    Split("CaSHL/CaSHL.cpp CaSHL/CaSHLWorld.cpp") + CommonWorldObject,
    LIBS=Split("SceneGraph MatSys ClipSys cfsLib cfs_jpeg bulletcollision lua minizip lightwave png z"))



envTools = env.Clone()

if sys.platform=="win32":
    envTools.Append(LIBPATH=['ExtLibs/DirectX7/lib'])
    # glu32 is only needed for the TerrainViewerOld...
    envTools.Append(LIBS=Split("SceneGraph MatSys ClipSys cfsLib cfs_jpeg bulletcollision lua minizip lightwave png z")
                       + Split("gdi32 glu32 opengl32 user32") + ['cfsOpenGL', 'dinput', 'dxguid'])
elif sys.platform=="linux2":
    # GLU is only needed for the TerrainViewerOld...
    envTools.Append(CPPPATH=['/usr/include/freetype2'])         # As of 2009-09-10, this line is to become unnecessary in the future, see /usr/include/ftbuild.h for details.
    envTools.Append(LIBS=Split("SceneGraph MatSys cfsOpenGL ClipSys cfsLib cfs_jpeg bulletcollision lua minizip lightwave png z")
                       + Split("GL GLU X11 dl"))

envTools.Program("MakeFont", "CaTools/MakeFont.cpp", LIBS=envTools["LIBS"]+["freetype"])

if sys.platform!="win32" or envTools["TARGET_ARCH"]=="x86":
    # Don't build these programs under 64-bit Windows, as they still depend on our legacy 32-bit-only DirectInput code.
    envTools.Program('CaSanity', ['CaTools/CaSanity.cpp'] + CommonWorldObject)
    envTools.Program('MaterialViewer', "CaTools/MaterialViewer.cpp")
    envTools.Program('TerrainViewer', "CaTools/TerrainViewer.cpp")
    envTools.Program('TerrainViewerOld', "CaTools/TerrainViewerOld.cpp")



# Create a common construction environment for our wxWidgets-based programs (Cafu and CaWE).
wxEnv = env.Clone()

if sys.platform=="win32":
    wxPath="#/ExtLibs/wxWidgets";

    # TODO: Move this into the SConstruct file (including the wx include path above).
    #   Note that we only (want to) determine the right library path matching the used compiler here.
    #   The specific wx-version used (e.g. latest stable vs. trunk) is still determined locally (here),
    #   BUT if this is moved into the SConstruct file, also the wx-version (wxPath above) must be fixed there.
    LibPath="/lib/"+compiler

    # Append wxWidgets-specific suffixes matching the TARGET_CPU setting for the Makefiles.
    if   wxEnv["TARGET_ARCH"] in ["x86_64", "amd64", "emt64"]: LibPath += "_amd64"
    elif wxEnv["TARGET_ARCH"] in ["ia64"]:                     LibPath += "_ia64"

    LibPath += "_lib"

    wxEnv.Append(LIBPATH=[wxPath+LibPath])

    if buildMode=="dbg":
        wxEnv.Append(CPPPATH=[wxPath+LibPath+"/mswud"])
        wxEnv.Append(LIBS=Split("wxbase29ud wxbase29ud_net wxjpegd wxmsw29ud_adv wxmsw29ud_core wxmsw29ud_gl wxmsw29ud_aui wxmsw29ud_propgrid wxregexud"))
    else:
        wxEnv.Append(CPPPATH=[wxPath+LibPath+"/mswu"])
        wxEnv.Append(LIBS=Split("wxbase29u wxbase29u_net wxjpeg wxmsw29u_adv wxmsw29u_core wxmsw29u_gl wxmsw29u_aui wxmsw29u_propgrid wxregexu"))

    wxEnv.Append(CPPPATH=[wxPath+'/include'])   # This must be appended *after* the LibPath-specific paths.
    wxEnv.Append(LIBS=Split("advapi32 comctl32 comdlg32 gdi32 ole32 oleaut32 opengl32 rpcrt4 shell32 user32 winspool wsock32"))

elif sys.platform=="linux2":
    wxEnv.ParseConfig(Dir("#/ExtLibs/wxWidgets").abspath + "/build-gtk/wx-config --cxxflags --libs std,aui,gl,propgrid | sed 's/-l\\S*jpeg\\S*\\ //g'")
    wxEnv.Append(LIBS=Split("cairo pangocairo-1.0 X11"))



envCafu = wxEnv.Clone()

envCafu.Append(CPPPATH=['ExtLibs/lua/src'])
envCafu.Append(CPPPATH=['ExtLibs/bullet/src'])

envCafu.Append(LIBS=Split("DeathMatch"))
envCafu.Append(LIBS=Split("SceneGraph MatSys SoundSys ClipSys cfsLib cfs_jpeg bulletdynamics bulletcollision bulletmath lightwave lua minizip png z"))

if sys.platform=="win32":
    WinResource = envCafu.RES("Ca3DE/Cafu.rc")

elif sys.platform=="linux2":
    WinResource = []

    # -Wl,-rpath,.           is so that also the . directory is searched for dynamic libraries when they're opened.
    # -Wl,--export-dynamic   is so that the exe exports its symbols so that the MatSys, SoundSys and game .so libs can in turn resolve theirs.
    envCafu.Append(LINKFLAGS=['-Wl,-rpath,.', '-Wl,--export-dynamic'])

    # pthread is needed because some libraries that we load (possibly indirectly), e.g. the libCg.so and libopenal.so, use functions
    # from the pthread library, but have not been linked themselves against it. They rely on the executable to be linked appropriately
    # in order to resolve the pthread symbols. Paul Pluzhnikov states in a newsgroup posting (see [1]) that even if the .so libs were
    # linked against libpthread.so, the main exe still *must* link with -lpthread, too, because:
    # "Note that dlopen()ing an MT library from non-MT executable is not supported on most platforms, certainly not on Linux."
    # [1] http://groups.google.de/group/gnu.gcc.help/browse_thread/thread/1e8f8dfd6027d7fa/
    # rt is required in order to resolve clock_gettime() in openal-soft.
    envCafu.Append(LIBS=Split("GL rt pthread"))

appCafu = envCafu.Program('Ca3DE/Cafu',
    Glob("Ca3DE/*.cpp") + Glob("Ca3DE/Server/*.cpp") + CommonWorldObject + ["Common/WorldMan.cpp"] + WinResource +
    Glob("Ca3DE/Client/*.cpp"))



envCaWE = wxEnv.Clone()
envCaWE.Append(CPPPATH=['ExtLibs/lua/src', 'ExtLibs/noise/src'])
envCaWE.Append(LIBS=Split("SceneGraph MatSys ClipSys cfsLib ModelLoaders cfs_jpeg bulletcollision noise lua minizip lightwave freetype png z"))

SourceFilesList = (Glob("CaWE/*.cpp")
    +Glob("CaWE/FontWizard/*.cpp")
    +Glob("CaWE/GuiEditor/*.cpp")+Glob("CaWE/GuiEditor/Commands/*.cpp")+Glob("CaWE/GuiEditor/Windows/*.cpp")
    +Glob("CaWE/MapCommands/*.cpp")
    +Glob("CaWE/MaterialBrowser/*.cpp")
    +Glob("CaWE/ModelEditor/*.cpp")+Glob("CaWE/ModelEditor/Commands/*.cpp")
    +Glob("CaWE/wxExt/*.cpp")
    +Glob("CaWE/wxFB/*.cpp"))

if sys.platform=="win32":
    SourceFilesList += envCaWE.RES("CaWE/CaWE.rc")

    if os.path.exists(Dir("#/ExtLibs/fbx/lib").abspath):
        envCaWE.Append(LIBPATH=["ExtLibs/fbx/lib/" +
            {"vc8": "vs2005", "vc9": "vs2008", "vc10": "vs2010"}[compiler] + "/" +
            ("x64" if envCaWE["TARGET_ARCH"] in ["x86_64", "amd64", "emt64"] else "x86")])
        envCaWE.Append(LIBS=["fbxsdk-2012.1-mdd" if buildMode=="dbg" else "fbxsdk-2012.1-md", "wininet"])

elif sys.platform=="linux2":
    envCaWE.Append(CPPPATH=['/usr/include/freetype2'])  # As of 2009-09-10, this line is to become unnecessary in the future, see /usr/include/ftbuild.h for details.
    envCaWE.Append(LINKFLAGS=['-Wl,-rpath,.'])          # Have dlopen() consider "." when searching for SOs (e.g. libCg.so).
    envCaWE.Append(LINKFLAGS=['-Wl,--export-dynamic'])  # Have our symbols available for dynamically loaded SOs (e.g. the renderer DLLs).

    if os.path.exists(Dir("#/ExtLibs/fbx/lib").abspath):
        envCaWE.Append(LIBPATH=["ExtLibs/fbx/lib/gcc4/" +
            ("x64" if platform.machine()=="x86_64" else "x86")])
        envCaWE.Append(LIBS=["fbxsdk-2012.1-staticd" if buildMode=="dbg" else "fbxsdk-2012.1-static"])

envCaWE.Program('CaWE/CaWE', SourceFilesList + CommonWorldObject)
