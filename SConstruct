import os, shutil, sys


# See the SCons manual, http://www.scons.org/wiki/GoFastButton and the man page for more information about the next two lines.
Decider("MD5-timestamp")
SetOption("implicit_cache", 1)

# Print an empty line, so that there is a better visual separation at the command line between successive calls.
print ""


try:
    import CompilerSetup
except ImportError:
    # In order to make getting started with the Cafu source code more convenient, install the
    # CompilerSetup.py file from the related template file (which is under version control) automatically.
    shutil.copy("CompilerSetup.py.tmpl", "CompilerSetup.py")
    import CompilerSetup

# Import the (user-configured) base environment from the setup file.
# The base environment is evaluated and further refined (e.g. with compiler-specific settings) below.
envCommon=CompilerSetup.envCommon;


# This big if-else tree has a branch for each supported platform and each supported compiler.
# For the chosen combination of platform and compiler, it prepares the environments envDebug, envRelease and envProfile.
if sys.platform=="win32":
    if envCommon["MSVC_VERSION"] in ["8.0", "8.0Exp"]:
        ##############################
        ### Win32, Visual C++ 2005 ###
        ##############################

        compiler="vc8"

        # Reference of commonly used compiler switches:
        # /EHsc  Enable exception handling.
        # /GR    Enable RTTI.
        # /J     Treat char as unsigned char.
        # /MT    Use multi-threaded run-time libraries (statically linked into the exe). Defines _MT.
        # /MTd   Use multi-threaded debug run-time libraries (statically linked into the exe). Defines _DEBUG and _MT.
        # /Od    Disable optimizations.
        # /O2    Fastest possible code.
        # /Ob2   Inline expansion at compilers discretion.
        # /RTC1  Run-Time Error Checks: Catch release-build errors in debug build.
        # /W3    Warning level.
        # /WX    Treat warnings as errors.
        # /Z7    Include full debug info.

        # Begin with an environment with settings that are common for debug, release and profile builds.
        envCommon.Append(CCFLAGS = Split("/GR /EHsc"))   # CCFLAGS is also taken as the default value for CXXFLAGS.
        envCommon.Append(CPPDEFINES = ["_CRT_SECURE_NO_DEPRECATE", "_CRT_NONSTDC_NO_DEPRECATE"])
        envCommon.Append(LINKFLAGS = Split("/incremental:no"))

        # Explicitly instruct SCons to detect and use the Microsoft Platform SDK, as it is not among the default tools.
        # See thread "Scons 2010/01/17 doesn't look for MS SDK?" at <http://scons.tigris.org/ds/viewMessage.do?dsForumId=1272&dsMessageId=2455554>
        # for further information.
        envCommon.Tool('mssdk')

        # Environment for debug builds:
        envDebug=envCommon.Clone();
        envDebug.Append(CCFLAGS=Split("/MTd /Od /Z7 /RTC1"));
        envDebug.Append(LINKFLAGS=["/debug"]);

        # Environment for release builds:
        envRelease=envCommon.Clone();
        envRelease.Append(CCFLAGS=Split("/MT /O2 /Ob2"));

        # Environment for profile builds:
        envProfile=envCommon.Clone();
        envProfile.Append(CCFLAGS=Split("/MT /O2 /Ob2 /Z7"));
        envProfile.Append(LINKFLAGS=["/fixed:no", "/debug"]);

    elif envCommon["MSVC_VERSION"] in ["9.0", "9.0Exp"]:
        ##############################
        ### Win32, Visual C++ 2008 ###
        ##############################

        compiler="vc9"

        # Reference of commonly used compiler switches:
        # Identical to the compiler switches for Visual C++ 2005, see there for more details.

        # Begin with an environment with settings that are common for debug, release and profile builds.
        envCommon.Append(CCFLAGS = Split("/GR /EHsc"))   # CCFLAGS is also taken as the default value for CXXFLAGS.
        envCommon.Append(CPPDEFINES = ["_CRT_SECURE_NO_DEPRECATE", "_CRT_NONSTDC_NO_DEPRECATE"])
        envCommon.Append(LINKFLAGS = Split("/incremental:no"))

        # Explicitly instruct SCons to detect and use the Microsoft Platform SDK, as it is not among the default tools.
        # See thread "Scons 2010/01/17 doesn't look for MS SDK?" at <http://scons.tigris.org/ds/viewMessage.do?dsForumId=1272&dsMessageId=2455554>
        # for further information.
        envCommon.Tool('mssdk')

        # This is a temporary workaround for SCons bug <http://scons.tigris.org/issues/show_bug.cgi?id=2663>.
        # It should be removed again as soon as the bug is fixed in SCons.
        if envCommon["TARGET_ARCH"] in ["x86_64", "amd64", "emt64"]:
            envCommon["ENV"]["LIB"] = envCommon["ENV"]["LIB"].replace("C:\\Program Files\\Microsoft SDKs\\Windows\\v7.0\\lib;", "C:\\Program Files\\Microsoft SDKs\\Windows\\v7.0\\lib\\x64;");
            envCommon["ENV"]["LIBPATH"] = envCommon["ENV"]["LIBPATH"].replace("C:\\Program Files\\Microsoft SDKs\\Windows\\v7.0\\lib;", "C:\\Program Files\\Microsoft SDKs\\Windows\\v7.0\\lib\\x64;");

        # Environment for debug builds:
        envDebug=envCommon.Clone();
        envDebug.Append(CCFLAGS=Split("/MTd /Od /Z7 /RTC1"));
        envDebug.Append(LINKFLAGS=["/debug"]);

        # Environment for release builds:
        envRelease=envCommon.Clone();
        envRelease.Append(CCFLAGS=Split("/MT /O2 /Ob2"));

        # Environment for profile builds:
        envProfile=envCommon.Clone();
        envProfile.Append(CCFLAGS=Split("/MT /O2 /Ob2 /Z7"));
        envProfile.Append(LINKFLAGS=["/fixed:no", "/debug"]);

    elif envCommon["MSVC_VERSION"] in ["10.0", "10.0Exp"]:
        ##############################
        ### Win32, Visual C++ 2010 ###
        ##############################

        compiler="vc10"

        # Reference of commonly used compiler switches:
        # Identical to the compiler switches for Visual C++ 2005, see there for more details.

        # Begin with an environment with settings that are common for debug, release and profile builds.
        envCommon.Append(CCFLAGS = Split("/GR /EHsc"))   # CCFLAGS is also taken as the default value for CXXFLAGS.
        envCommon.Append(CPPDEFINES = ["_CRT_SECURE_NO_DEPRECATE", "_CRT_NONSTDC_NO_DEPRECATE"])
        envCommon.Append(LINKFLAGS = Split("/incremental:no"))

        # Explicitly instruct SCons to detect and use the Microsoft Platform SDK, as it is not among the default tools.
        # See thread "Scons 2010/01/17 doesn't look for MS SDK?" at <http://scons.tigris.org/ds/viewMessage.do?dsForumId=1272&dsMessageId=2455554>
        # for further information.
        envCommon.Tool('mssdk')

        # Environment for debug builds:
        envDebug=envCommon.Clone();
        envDebug.Append(CCFLAGS=Split("/MTd /Od /Z7 /RTC1"));
        envDebug.Append(LINKFLAGS=["/debug"]);

        # Environment for release builds:
        envRelease=envCommon.Clone();
        envRelease.Append(CCFLAGS=Split("/MT /O2 /Ob2"));

        # Environment for profile builds:
        envProfile=envCommon.Clone();
        envProfile.Append(CCFLAGS=Split("/MT /O2 /Ob2 /Z7"));
        envProfile.Append(LINKFLAGS=["/fixed:no", "/debug"]);

    else:
        ###############################
        ### Win32, unknown compiler ###
        ###############################

        print "Unknown compiler on platform " + sys.platform + "."
        exit

elif sys.platform=="linux2":
    if envCommon["CXX"]=="g++":
        ##################
        ### Linux, g++ ###
        ##################

        compiler="g++"

        # Begin with an environment with settings that are common for debug, release and profile builds.
        envCommon.Append(CCFLAGS = Split(""))   # CCFLAGS is also taken as the default value for CXXFLAGS.

        # Environment for debug builds:
        envDebug=envCommon.Clone();
        envDebug.Append(CCFLAGS=["-g"]);

        # Environment for release builds:
        envRelease=envCommon.Clone();
        envRelease.Append(CCFLAGS=["-O3"]);
        envRelease.Append(LINKFLAGS=["-Wl,--strip-all"]);  # Reduce file size, and keep nosy people from printing our symbol names (e.g. with strings -a).

        # Environment for profile builds:
        envProfile=envCommon.Clone();
        envProfile.Append(CCFLAGS=["-O3", "-g"]);

    else:
        ###############################
        ### Linux, unknown compiler ###
        ###############################

        print "Unknown compiler " + envCommon["CXX"] + " on platform " + sys.platform + "."
        exit

else:
    print "Unknown platform '" + sys.platform + "'."
    exit

envDebug  .Append(CPPDEFINES=["DEBUG"]);
envRelease.Append(CPPDEFINES=["NDEBUG"]);   # Need NDEBUG to have the assert macro generate no runtime code.
envProfile.Append(CPPDEFINES=["NDEBUG"]);   # Need NDEBUG to have the assert macro generate no runtime code.


####################################
### Build all external libraries ###
####################################

# Set the list of variants (debug, profile, release) that should be built.
BVs=ARGUMENTS.get("bv", CompilerSetup.buildVariants)

# Set the build directories.
my_build_dir="build/"+sys.platform+"/"+compiler
if envCommon["TARGET_ARCH"]: my_build_dir += "/"+envCommon["TARGET_ARCH"];

my_build_dir_dbg=my_build_dir+"/debug"
my_build_dir_rel=my_build_dir+"/release"
my_build_dir_prf=my_build_dir+"/profile"


for lib_name in ["bullet", "freealut", "freetype", "jpeg", "libogg", "libpng", "libvorbis", "lwo", "lua", "mpg123", "noise", "openal-soft", "zlib"]:
    s_name=lib_name

    if lib_name=="openal-soft" and sys.platform=="win32": continue;     # OpenAL-Soft is not built on Windows, use the OpenAL Windows SDK there.
    if lib_name=="bullet": s_name+="/src";
    if lib_name=="lua":    s_name+="/src";
    if lib_name=="mpg123": s_name+="/src/libmpg123";

    if "d" in BVs: SConscript("ExtLibs/"+s_name+"/SConscript", exports={ "env": envDebug },   variant_dir="ExtLibs/"+lib_name+"/"+my_build_dir_dbg, duplicate=0)
    if "r" in BVs: SConscript("ExtLibs/"+s_name+"/SConscript", exports={ "env": envRelease }, variant_dir="ExtLibs/"+lib_name+"/"+my_build_dir_rel, duplicate=0)
    if "p" in BVs: SConscript("ExtLibs/"+s_name+"/SConscript", exports={ "env": envProfile }, variant_dir="ExtLibs/"+lib_name+"/"+my_build_dir_prf, duplicate=0)


# Compile wxWidgets for the current platform.
if sys.platform=="win32":
    if compiler.startswith("vc"):
        # If we target a non-x86 architecture, the Makefiles automatically append a suffix to the directory names (no need to tweak COMPILER_PREFIX).
        if   envCommon["TARGET_ARCH"] in ["x86_64", "amd64", "emt64"]: target_cpu=" TARGET_CPU=AMD64"
        elif envCommon["TARGET_ARCH"] in ["ia64"]:                     target_cpu=" TARGET_CPU=IA64"
        else:                                                          target_cpu=""

        result=envDebug.  Execute("nmake /nologo /f makefile.vc BUILD=debug   SHARED=0 USE_OPENGL=1 RUNTIME_LIBS=static COMPILER_PREFIX="+compiler+target_cpu, chdir="ExtLibs/wxWidgets/build/msw");
        if (result!=0): envDebug.  Exit(result);
        result=envRelease.Execute("nmake /nologo /f makefile.vc BUILD=release SHARED=0 USE_OPENGL=1 RUNTIME_LIBS=static COMPILER_PREFIX="+compiler+target_cpu, chdir="ExtLibs/wxWidgets/build/msw");
        if (result!=0): envRelease.Exit(result);
        print "";   # Print just another empty line for better visual separation.

elif sys.platform=="linux2":
    # This automatic compilation of wxGTK requires that some library packages have been installed already,
    # e.g. libgtk2.0-dev, libgl1-mesa-dev and libglu1-mesa-dev under Ubuntu 8.04.
    # See the documentation at http://www.cafu.de/wiki/ for more details!
    if not os.path.exists("ExtLibs/wxWidgets/build-gtk_d"):
        envDebug.Execute(Mkdir("ExtLibs/wxWidgets/build-gtk_d"));
        result=envDebug.Execute("../configure --with-gtk --enable-debug --disable-shared --without-libtiff --disable-pnm --disable-pcx --disable-iff --with-opengl --with-regex=builtin --disable-compat26 --disable-compat28", chdir="ExtLibs/wxWidgets/build-gtk_d");
        if (result!=0): envDebug.Exit(result);
        result=envDebug.Execute("make", chdir="ExtLibs/wxWidgets/build-gtk_d");
        if (result!=0): envDebug.Exit(result);

    if not os.path.exists("ExtLibs/wxWidgets/build-gtk_r"):
        envRelease.Execute(Mkdir("ExtLibs/wxWidgets/build-gtk_r"));
        result=envRelease.Execute("../configure --with-gtk --disable-shared --without-libtiff --disable-pnm --disable-pcx --disable-iff --with-opengl --with-regex=builtin --disable-compat26 --disable-compat28", chdir="ExtLibs/wxWidgets/build-gtk_r");
        if (result!=0): envRelease.Exit(result);
        result=envRelease.Execute("make", chdir="ExtLibs/wxWidgets/build-gtk_r");
        if (result!=0): envRelease.Exit(result);


######################################
### Install the external libraries ###
######################################

# "Install" (copy) all DLLs that are required at run-time into the main Cafu directory.
# Some of them are self-built, others are provided ready-to-use by the vendor.
#
# They should work for the respective OS (.dll on Windows, .so on Linux) with all supported
# compilers and build variants (debug, profile, release): All interfaces are pure C, no C++,
# with the proper calling conventions etc. Therefore, matching the precise DLL built variant
# with that of the main Cafu executable is not necessary, and overwriting (some of) these
# DLLs with their debug build variants for debugging should not be a problem.
#
# We use release builds whenever we can, and assume that if the user has disabled them,
# debug builds are available (i.e. the code below fails if neither "d" nor "r" is in BVs).
if sys.platform=="win32":
    envRelease.Install(".", ["#/ExtLibs/Cg/bin/cg.dll", "#/ExtLibs/Cg/bin/cgGL.dll"]);
    envRelease.Install(".", ["#/ExtLibs/fmod/api/fmod.dll"]);
    envRelease.Install(".", ["#/ExtLibs/openal-win/OpenAL32.dll", "#/ExtLibs/openal-win/wrap_oal.dll"]);

    if "r" in BVs:
        envRelease.Install(".", ["#/ExtLibs/freealut/"+my_build_dir_rel+"/alut.dll"]);
        envRelease.Install(".", ["#/ExtLibs/mpg123/"+my_build_dir_rel+"/mpg123.dll"]);

    else:
        envRelease.Install(".", ["#/ExtLibs/freealut/"+my_build_dir_dbg+"/alut.dll"]);
        envRelease.Install(".", ["#/ExtLibs/mpg123/"+my_build_dir_dbg+"/mpg123.dll"]);

elif sys.platform=="linux2":
    envRelease.Install(".", ["#/ExtLibs/Cg/lib/libCg.so", "#/ExtLibs/Cg/lib/libCgGL.so"]);
    envRelease.Install(".", ["#/ExtLibs/fmod/api/libfmod-3.75.so"]);

    if "r" in BVs:
        envRelease.Install(".", ["#/ExtLibs/freealut/"+my_build_dir_rel+"/libalut.so"]);
        envRelease.Install(".", ["#/ExtLibs/mpg123/"+my_build_dir_rel+"/libmpg123.so"]);
        envRelease.Install(".", ["#/ExtLibs/openal-soft/"+my_build_dir_rel+"/libopenal.so"]);

    else:
        envRelease.Install(".", ["#/ExtLibs/freealut/"+my_build_dir_dbg+"/libalut.so"]);
        envRelease.Install(".", ["#/ExtLibs/mpg123/"+my_build_dir_dbg+"/libmpg123.so"]);
        envRelease.Install(".", ["#/ExtLibs/openal-soft/"+my_build_dir_dbg+"/libopenal.so"]);


############################################
### Update the construction environments ###
############################################

CommonLibPaths=["#/ExtLibs/bullet/",
                "#/ExtLibs/freealut/",
                "#/ExtLibs/jpeg/",
                "#/ExtLibs/libogg/",
                "#/ExtLibs/libpng/",
                "#/ExtLibs/libvorbis/",
                "#/ExtLibs/lwo/",
                "#/ExtLibs/lua/",
                "#/ExtLibs/mpg123/",
                "#/ExtLibs/noise/",
                "#/ExtLibs/openal-soft/",
                "#/ExtLibs/zlib/",
                "#/Libs/"];

if sys.platform=="win32":
    # Only for Windows, as under Linux we must link to the systems freetype library.
    CommonLibPaths.append("#/ExtLibs/freetype/");

CommonLibPaths_dbg=[x+my_build_dir_dbg for x in CommonLibPaths];
CommonLibPaths_rel=[x+my_build_dir_rel for x in CommonLibPaths];
CommonLibPaths_prf=[x+my_build_dir_prf for x in CommonLibPaths];


# Note that modifying the original environments here affects the build of the external libraries above!
envDebug_Cafu  =envDebug.Clone();
envRelease_Cafu=envRelease.Clone();
envProfile_Cafu=envProfile.Clone();

envDebug_Cafu  .Append(CPPDEFINES=[("SCONS_BUILD_DIR", my_build_dir_dbg)]);
envRelease_Cafu.Append(CPPDEFINES=[("SCONS_BUILD_DIR", my_build_dir_rel)]);
envProfile_Cafu.Append(CPPDEFINES=[("SCONS_BUILD_DIR", my_build_dir_prf)]);

envDebug_Cafu  .Append(CPPPATH=["#/Libs", "#/ExtLibs"]);
envRelease_Cafu.Append(CPPPATH=["#/Libs", "#/ExtLibs"]);
envProfile_Cafu.Append(CPPPATH=["#/Libs", "#/ExtLibs"]);

envDebug_Cafu  .Append(LIBPATH=CommonLibPaths_dbg);
envRelease_Cafu.Append(LIBPATH=CommonLibPaths_rel);
envProfile_Cafu.Append(LIBPATH=CommonLibPaths_prf);

if compiler=="vc8":
    envDebug_Cafu  .Append(CCFLAGS=Split("/J /W3 /WX"));
    envRelease_Cafu.Append(CCFLAGS=Split("/J /W3 /WX"));
    envProfile_Cafu.Append(CCFLAGS=Split("/J /W3 /WX"));

elif compiler=="vc9":
    envDebug_Cafu  .Append(CCFLAGS=Split("/J /W3 /WX"));
    envRelease_Cafu.Append(CCFLAGS=Split("/J /W3 /WX"));
    envProfile_Cafu.Append(CCFLAGS=Split("/J /W3 /WX"));

elif compiler=="vc10":
    envDebug_Cafu  .Append(CCFLAGS=Split("/J /W3 /WX"));
    envRelease_Cafu.Append(CCFLAGS=Split("/J /W3 /WX"));
    envProfile_Cafu.Append(CCFLAGS=Split("/J /W3 /WX"));

elif compiler=="g++":
    envDebug_Cafu  .Append(CCFLAGS=Split("-funsigned-char -Wall -Werror -Wno-char-subscripts"));
    envRelease_Cafu.Append(CCFLAGS=Split("-funsigned-char -Wall -Werror -Wno-char-subscripts -fno-strict-aliasing"));
    envProfile_Cafu.Append(CCFLAGS=Split("-funsigned-char -Wall -Werror -Wno-char-subscripts -fno-strict-aliasing"));


###########################
### Build all Cafu code ###
###########################

if os.path.exists("Libs/SConscript"):
    # Build everything in the Libs/ directory.
    if "d" in BVs: buildMode = "dbg"; SConscript('Libs/SConscript', exports=[{'env':envDebug_Cafu},   'buildMode'], variant_dir="Libs/"+my_build_dir_dbg, duplicate=0)
    if "r" in BVs: buildMode = "rel"; SConscript('Libs/SConscript', exports=[{'env':envRelease_Cafu}, 'buildMode'], variant_dir="Libs/"+my_build_dir_rel, duplicate=0)
    if "p" in BVs: buildMode = "prf"; SConscript('Libs/SConscript', exports=[{'env':envProfile_Cafu}, 'buildMode'], variant_dir="Libs/"+my_build_dir_prf, duplicate=0)

if os.path.exists("SConscript"):
    # Build the Cafu executables.
    if "d" in BVs: buildMode = "dbg"; SConscript('SConscript', exports=[{'env':envDebug_Cafu},   'buildMode', 'compiler'], variant_dir=""+my_build_dir_dbg, duplicate=0)
    if "r" in BVs: buildMode = "rel"; SConscript('SConscript', exports=[{'env':envRelease_Cafu}, 'buildMode', 'compiler'], variant_dir=""+my_build_dir_rel, duplicate=0)
    if "p" in BVs: buildMode = "prf"; SConscript('SConscript', exports=[{'env':envProfile_Cafu}, 'buildMode', 'compiler'], variant_dir=""+my_build_dir_prf, duplicate=0)

if os.path.exists("Games/DeathMatch/Code/SConscript"):
    # Build the DeathMatch DLL.
    if "d" in BVs: buildMode = "dbg"; SConscript('Games/DeathMatch/Code/SConscript', exports=[{'env':envDebug_Cafu},   'buildMode'], variant_dir="Games/DeathMatch/Code/"+my_build_dir_dbg, duplicate=0)
    if "r" in BVs: buildMode = "rel"; SConscript('Games/DeathMatch/Code/SConscript', exports=[{'env':envRelease_Cafu}, 'buildMode'], variant_dir="Games/DeathMatch/Code/"+my_build_dir_rel, duplicate=0)
    if "p" in BVs: buildMode = "prf"; SConscript('Games/DeathMatch/Code/SConscript', exports=[{'env':envProfile_Cafu}, 'buildMode'], variant_dir="Games/DeathMatch/Code/"+my_build_dir_prf, duplicate=0)

if os.path.exists("Games/VSWM/Code/SConscript"):
    # Build the VSWM DLL.
    if "d" in BVs: buildMode = "dbg"; SConscript('Games/VSWM/Code/SConscript', exports=[{'env':envDebug_Cafu},   'buildMode'], variant_dir="Games/VSWM/Code/"+my_build_dir_dbg, duplicate=0)
    if "r" in BVs: buildMode = "rel"; SConscript('Games/VSWM/Code/SConscript', exports=[{'env':envRelease_Cafu}, 'buildMode'], variant_dir="Games/VSWM/Code/"+my_build_dir_rel, duplicate=0)
    if "p" in BVs: buildMode = "prf"; SConscript('Games/VSWM/Code/SConscript', exports=[{'env':envProfile_Cafu}, 'buildMode'], variant_dir="Games/VSWM/Code/"+my_build_dir_prf, duplicate=0)
