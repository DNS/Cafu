import os, platform, sys
import subprocess
import CompilerSetup

Import('env', 'buildMode', 'compiler')


envMapCompilers = env.Clone()

envMapCompilers.Append(CPPPATH=['ExtLibs/lua/src'])
envMapCompilers.Append(LIBS=Split("SceneGraph MatSys SoundSys ClipSys cfsLib ClipSys cfs_jpeg bulletdynamics bulletcollision bulletmath lua minizip lightwave png z"))

if sys.platform=="win32":
    envMapCompilers.Append(LIBS=Split("wsock32"))
elif sys.platform.startswith("linux"):
    pass # envMapCompilers.Append(LIBS=Split(""))

CommonWorldObject = envMapCompilers.StaticObject("Common/World.cpp")

envMapCompilers.Program('CaBSP/CaBSP',   # I had preferred writing 'CaBSP' instead of 'CaBSP/CaBSP' here, but then under Linux we would get both a directory *and* an executeable with name 'CaBSP' in the build directory, which is not allowed/possible.
    Split("CaBSP/CaBSP.cpp CaBSP/BspTreeBuilder/BspTreeBuilder.cpp") + CommonWorldObject)

envMapCompilers.Program('CaPVS/CaPVS',
    Split("CaPVS/CaPVS.cpp CaPVS/CaPVSWorld.cpp") + CommonWorldObject)

envMapCompilers.Program('CaLight/CaLight',
    Split("CaLight/CaLight.cpp CaLight/CaLightWorld.cpp") + CommonWorldObject)

envMapCompilers.Program('CaSHL/CaSHL',
    Split("CaSHL/CaSHL.cpp CaSHL/CaSHLWorld.cpp") + CommonWorldObject)



envTools = env.Clone()
envTools.Append(CPPPATH=['ExtLibs/glfw/include', 'ExtLibs/tclap/include'])

if sys.platform=="win32":
    # shell32 is required by glfw, which uses DragQueryFile() etc.
    envTools.Append(LIBS=Split("SceneGraph MatSys ClipSys cfsLib cfs_jpeg bulletcollision bulletmath glfw lua minizip lightwave png z")
                       + Split("gdi32 opengl32 user32 shell32"))
elif sys.platform.startswith("linux"):
    envTools.Append(CPPPATH=['/usr/include/freetype2'])  # As of 2009-09-10, this line is to become unnecessary in the future, see /usr/include/ftbuild.h for details.
    envTools.Append(LINKFLAGS=['-Wl,-rpath,.'])          # Have dlopen() consider "." when searching for SOs (e.g. libCg.so).
    envTools.Append(LINKFLAGS=['-Wl,--export-dynamic'])  # Have our symbols available for dynamically loaded SOs (e.g. the renderer DLLs).

    envTools.Append(LIBS=Split("SceneGraph MatSys ClipSys cfsLib cfs_jpeg bulletcollision bulletmath glfw lua minizip lightwave png z")
                       + Split("GL X11 Xrandr Xinerama Xxf86vm Xcursor dl pthread"))

envTools.Program("MakeFont", "CaTools/MakeFont.cpp", LIBS=envTools["LIBS"]+["freetype"])
envTools.Program('CaSanity', ['CaTools/CaSanity.cpp'] + CommonWorldObject)
envTools.Program('MaterialViewer', "CaTools/MaterialViewer.cpp")
envTools.Program('TerrainViewer', "CaTools/TerrainViewer.cpp")



envCafu = env.Clone()
envCafu.Append(CPPPATH=['ExtLibs/lua/src'])
envCafu.Append(CPPPATH=['ExtLibs/bullet/src'])
envCafu.Append(CPPPATH=['ExtLibs/glfw/include'])
envCafu.Append(CPPPATH=['ExtLibs/tclap/include'])
envCafu.Append(LIBS=Split("SceneGraph MatSys SoundSys ClipSys cfsLib cfs_jpeg bulletdynamics bulletcollision bulletmath glfw lightwave lua minizip png z"))

if sys.platform=="win32":
    WinResource = envCafu.RES("Ca3DE/Cafu.rc")
    envCafu.Append(LIBS=Split("gdi32 opengl32 user32 shell32 wsock32"))   # opengl32 is for glReadPixels.

elif sys.platform.startswith("linux"):
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
    envCafu.Append(LIBS=Split("GL X11 Xrandr Xinerama Xxf86vm Xcursor rt dl pthread"))

appCafu = envCafu.Program('Ca3DE/Cafu',
    Glob("Ca3DE/*.cpp") +
    Glob("Ca3DE/Client/*.cpp") +
    Glob("Ca3DE/Server/*.cpp") +
    CommonWorldObject + ["Common/CompGameEntity.cpp", "Common/WorldMan.cpp"] + WinResource)



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
    if   wxEnv["TARGET_ARCH"] in ["x86_64", "amd64", "emt64"]: LibPath += "_x64"
    elif wxEnv["TARGET_ARCH"] in ["ia64"]:                     LibPath += "_ia64"

    LibPath += "_lib"

    wxEnv.Append(LIBPATH=[wxPath+LibPath])

    # Note that at this time, we link *two* copies of libpng to the programs: That of wxWidgets, and our own.
    # See http://thread.gmane.org/gmane.comp.lib.wxwidgets.general/80245 for some details.
    # A similar consideration applies to libjpeg, and possibly to zlib in the future...
    if buildMode=="dbg":
        wxEnv.Append(CPPPATH=[wxPath+LibPath+"/mswud"])
        wxEnv.Append(LIBS=Split("wxbase30ud wxbase30ud_net wxjpegd wxpngd wxmsw30ud_adv wxmsw30ud_core wxmsw30ud_gl wxmsw30ud_aui wxmsw30ud_propgrid wxregexud"))
    else:
        wxEnv.Append(CPPPATH=[wxPath+LibPath+"/mswu"])
        wxEnv.Append(LIBS=Split("wxbase30u wxbase30u_net wxjpeg wxpng wxmsw30u_adv wxmsw30u_core wxmsw30u_gl wxmsw30u_aui wxmsw30u_propgrid wxregexu"))

    wxEnv.Append(CPPPATH=[wxPath+'/include'])   # This must be appended *after* the LibPath-specific paths.
    wxEnv.Append(LIBS=Split("advapi32 comctl32 comdlg32 gdi32 ole32 oleaut32 opengl32 rpcrt4 shell32 user32 winspool wsock32"))

elif sys.platform.startswith("linux"):
    wxEnv.ParseConfig(Dir("#/ExtLibs/wxWidgets").abspath + "/build-gtk/wx-config --cxxflags --libs std,aui,gl,propgrid | sed 's/-l\\S*jpeg\\S*\\ //g'")
    wxEnv.Append(LIBS=Split("cairo pangocairo-1.0 X11"))



envCaWE = wxEnv.Clone()
envCaWE.Append(CPPPATH=['ExtLibs/lua/src', 'ExtLibs/bullet/src', 'ExtLibs/noise/src'])
envCaWE.Append(LIBS=Split("SceneGraph MatSys SoundSys ClipSys cfsLib ClipSys ModelLoaders cfs_jpeg bulletdynamics bulletcollision bulletmath noise lua minizip lightwave freetype png z"))

SourceFilesList = (Glob("CaWE/*.cpp")
    + Glob("CaWE/FontWizard/*.cpp")
    + Glob("CaWE/GuiEditor/*.cpp") + Glob("CaWE/GuiEditor/Commands/*.cpp")
    + Glob("CaWE/MapEditor/*.cpp") + Glob("CaWE/MapEditor/Commands/*.cpp")
    + Glob("CaWE/MaterialBrowser/*.cpp")
    + Glob("CaWE/ModelEditor/*.cpp")+Glob("CaWE/ModelEditor/Commands/*.cpp")
    + Glob("CaWE/MapEditor/wxExt/*.cpp")
    + Glob("CaWE/MapEditor/wxFB/*.cpp"))

if sys.platform=="win32":
    SourceFilesList += envCaWE.RES("CaWE/CaWE.rc")
    # The next two lines are mirrored in Libs/SConscript.
    fbx_lib_path = "#/ExtLibs/fbx/lib/" + \
        {"vc8": "vs2005", "vc9": "vs2008", "vc10": "vs2010", "vc11": "vs2012", "vc12": "vs2013", "vc14": "vs2015"}[compiler]

    if os.path.exists(Dir(fbx_lib_path).abspath):
        envCaWE.Append(LIBPATH=[fbx_lib_path + "/" +
            ("x64" if envCaWE["TARGET_ARCH"] in ["x86_64", "amd64", "emt64"] else "x86") + "/" +
            ("debug" if buildMode == "dbg" else "release")])
        envCaWE.Append(LIBS=["libfbxsdk-md"])

elif sys.platform.startswith("linux"):
    envCaWE.Append(CPPPATH=['/usr/include/freetype2'])  # As of 2009-09-10, this line is to become unnecessary in the future, see /usr/include/ftbuild.h for details.
    envCaWE.Append(LINKFLAGS=['-Wl,-rpath,.'])          # Have dlopen() consider "." when searching for SOs (e.g. libCg.so).
    envCaWE.Append(LINKFLAGS=['-Wl,--export-dynamic'])  # Have our symbols available for dynamically loaded SOs (e.g. the renderer DLLs).

    if os.path.exists(Dir("#/ExtLibs/fbx/lib").abspath):
        fbx_lib_path = "#/ExtLibs/fbx/lib/gcc4/{}/{}".format(
            "x64" if platform.machine() == "x86_64" else "x86",
            "debug" if buildMode == "dbg" else "release")
        # envCaWE.Append(LIBPATH=[fbx_lib_path])
        # envCaWE.Append(LIBS=["fbxsdk"])   # This links libfbxsdk.so
        envCaWE.Append(LIBS=[File(fbx_lib_path + "/libfbxsdk.a")])  # link statically

CaWE_exe = envCaWE.Program('CaWE/CaWE', SourceFilesList + CommonWorldObject)

# If CaWE_exe is cleaned, clean wxWidgets along with it. This is done here
# because Clean() requires a target that files to be cleaned are associated with.
envCaWE.Clean(CaWE_exe, [
    "#/ExtLibs/wxWidgets/build/msw/" + compiler + "_mswu",
    "#/ExtLibs/wxWidgets/build/msw/" + compiler + "_mswud",
    "#/ExtLibs/wxWidgets/lib/" + compiler + "_lib",
    "#/ExtLibs/wxWidgets/build-gtk",
])



def UpdateDocTemplates(target, source, env):
    """
    Runs `.../CaWE --update-doxygen` in order to update the `Doxygen/scripting/tmpl/*.hpp` files.
    """
    subprocess.call([str(source[0]), "--update-doxygen"])

    # For all target files (`Doxygen/scripting/tmpl/*.hpp`), make sure that they
    #   - don't contain the string "// WARNING" (e.g. about mismatches),
    #   - don't contain any lines that the related files in `../src/` don't have.
    for t in target:
        tmplPath = str(t)
        path, basename = os.path.split(tmplPath)
        srcPath = os.path.join(os.path.dirname(path), "src", basename)

        with open(tmplPath, 'r') as tmplFile:
            with open(srcPath, 'r') as srcFile:
                srcLines  = srcFile.readlines()
                srcLineNr = 0

                for tmplLine in tmplFile:
                    if "// WARNING" in tmplLine:
                        raise Exception('Found a "// WARNING ..." comment in file "{0}".'.format(tmplPath))

                    while tmplLine != srcLines[srcLineNr]:
                        srcLineNr += 1
                        if srcLineNr >= len(srcLines):
                            raise Exception('A line in template file "{0}" does not exist in the related source '
                                            'file:\n\n{1}\nUse e.g. BeyondCompare to review the situation.'.format(tmplPath, tmplLine))

    return None


if buildMode == "dbg":
    # Unfortunately, the list of target files must be known in advance: it is not possible
    # to run `.../CaWE --update-doxygen` first and then see what it produced. For details,
    # see https://pairlist4.pair.net/pipermail/scons-users/2015-February/003409.html
    TargetFiles = [
        "GameComponents.hpp", "GameEntities.hpp", "GameWorld.hpp",
        "GuiComponents.hpp", "GuiGui.hpp", "GuiWindows.hpp"
    ]

    envCaWE.Command(["#/Doxygen/scripting/tmpl/" + f for f in TargetFiles], CaWE_exe, UpdateDocTemplates)
