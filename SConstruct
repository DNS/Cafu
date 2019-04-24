import codecs, os, platform, shutil, sys, time


# See the SCons manual, http://www.scons.org/wiki/GoFastButton and the man page for more information about the next two lines.
Decider("MD5-timestamp")
SetOption("implicit_cache", 1)

# Print an empty line, so that there is a better visual separation at the command line between successive calls.
print("")


try:
    import CompilerSetup
except ImportError:
    # In order to make getting started with the Cafu source code more convenient, install the
    # CompilerSetup.py file from the related template file (which is under version control) automatically.
    shutil.copy("CompilerSetup.py.tmpl", "CompilerSetup.py")
    import CompilerSetup


def FixFormatting(Filename, Lines):
    with codecs.open(Filename, encoding='utf-8', errors='strict', mode='wb') as f:
        for l in Lines:
            f.write(l.replace("\t", "    ").rstrip() + "\n")


def CheckFormatting(start_dir, QuickMode):
    """
    This function checks for common problems such as trailing whitespace, TABs,
    improper encoding (non-UTF8), no EOL at end of file, and mixed(!) EOL styles
    within a single file.
    Files whose newline style does not match the operating system's default are
    accounted for statistics, but not counted as errors.
    """
    t1 = time.time()
    CheckedCount  = 0
    ProblemsCount = 0
    c_NewL_Dos    = 0
    c_NewL_Unix   = 0

    QuickCount  = 0
    QuickRefCO  = 0                             # For excluding newly checked-out files.
    QuickRefOld = time.time() - 3600 * 24 * 7   # For excluding too old files.

    if QuickMode:
        try:
            QuickRefCO = os.path.getmtime(".gitattributes") + 3600
        except:
            print('Could not determine mtime for ".gitattributes".')

    for root, dirs, files in os.walk(start_dir):
        # print root, dirs

        if not dirs and not files:
            print(root, "is empty")

        for filename in files:
            if os.path.splitext(filename)[1] in [".h", ".hpp", ".c", ".cpp", ".lua", ".py", ".cmat", ".cgui", ""] or filename in ["CMakeLists.txt"]:
                pathname = os.path.join(root, filename)

                if QuickMode:
                    ts = os.path.getmtime(pathname)
                    if ts < QuickRefOld or ts < QuickRefCO:
                        QuickCount += 1
                        continue

                CheckedCount += 1

                try:
                    with codecs.open(pathname, encoding='utf-8', errors='strict') as f:
                        Lines = f.readlines()

                    c = {
                        "tabs": 0,
                        "NewL_Dos": 0,
                        "NewL_Unix": 0,
                        "NewL_Mac": 0,
                        "NewL_Other": 0,
                        "trailing_ws": 0,
                    }

                    for line in Lines:
                        if "\t" in line:
                            c["tabs"] += 1

                        if line.endswith("\r\n"):
                            c["NewL_Dos"] += 1
                        elif line.endswith("\n"):
                            c["NewL_Unix"] += 1
                        elif line.endswith("\r"):
                            c["NewL_Mac"] += 1
                        else:
                            # Note that "no newline at end of file" is a special case of this!
                            c["NewL_Other"] += 1

                        if line.rstrip("\r\n").endswith(" "):
                            c["trailing_ws"] += 1

                    if c["NewL_Dos" ]: c_NewL_Dos  += 1
                    if c["NewL_Unix"]: c_NewL_Unix += 1

                    if c["tabs"] or c["trailing_ws"] or c["NewL_Mac"] or c["NewL_Other"] or (c["NewL_Dos"] and c["NewL_Unix"]):
                        print(pathname, c)
                        if False:
                            FixFormatting(pathname, Lines)
                        ProblemsCount += 1

                except UnicodeDecodeError:
                    if filename in ["MainMenu_init.cgui", "MainMenu_main.cgui"]:
                        continue

                    print(pathname, "is not properly UTF-8 encoded")
                    ProblemsCount += 1
                    # subprocess.Popen(["iconv", "-f", "iso-8859-1", "-t", "utf-8", pathname, "-o", "xy.tmp"]).wait()
                    # subprocess.Popen(["mv", "-f", "xy.tmp", pathname]).wait()

        for excl in ["FontWizard", "wxExt", "wxFB"]:
            if "CaWE" in root and excl in dirs:
                dirs.remove(excl)

        for excl in [".git", ".svn", ".sconf_temp", "build", "docs", "ExtLibs", "out", ".vs"]:
            if excl in dirs:
                dirs.remove(excl)

    t2 = time.time()

    if ProblemsCount:
        print("")

    if QuickCount:
        print("{} source files checked in {:.2f}s ({} files skipped due to quick mode).".format(CheckedCount, t2 - t1, QuickCount))
    else:
        print("{} source files checked in {:.2f}s.".format(CheckedCount, t2 - t1))

    if c_NewL_Dos and c_NewL_Unix:
        print("Files with DOS-style  newlines:", c_NewL_Dos)
        print("Files with Unix-style newlines:", c_NewL_Unix)

    if ProblemsCount:
        print("\nError: The formatting of the above {} indicated source files is unexpected.".format(ProblemsCount))
        Exit(1)

    print("")


if hasattr(CompilerSetup, "checkSourceFormatting") and CompilerSetup.checkSourceFormatting:
    CheckFormatting(".", CompilerSetup.checkSourceFormatting == "quick")


# Import the (user-configured) base environment from the setup file.
# The base environment is evaluated and further refined (e.g. with compiler-specific settings) below.
envCommon = CompilerSetup.envCommon


# Add a Builder to envCommon that expects a Value node as its source, and builds the target from the contents
# of the source node: If the source value changes, the target file is re-built. See the section about Value()
# in the SCons man page and http://thread.gmane.org/gmane.comp.programming.tools.scons.user/23752 for details.
def BuildFileFromValue(env, target, source):
    with open(str(target[0]), 'w') as f:
        f.write(source[0].get_contents())

envCommon['BUILDERS']['FileFromValue'] = envCommon.Builder(action=BuildFileFromValue)


# A custom check to determine if the C++ compiler supports the C++11 `override` identifier.
def CheckOverrideIdentifier(context):
    source = """
    class A
    {
        public:

        virtual int f() const { return 1; }
    };

    class B : public A
    {
        public:

        int f() const override { return 2; }
    };

    int main()
    {
        return 0;
    }
    """

    context.Message("Checking for C++11 `override` identifier... ")
    result = context.TryLink(source, '.cpp')
    context.Result(result)
    return result


# This big if-else tree has a branch for each supported platform and each supported compiler.
# For the chosen combination of platform and compiler, it prepares the environments envDebug, envRelease and envProfile.
if sys.platform=="win32":
    # Under Windows, there are no system copies of these libraries, so instead we use our own local copies.
    # Setting these paths in envCommon means that they are "globally" available everywhere, but this is ok:
    # Under Linux, all library headers are globally available (e.g. at /usr/include/) as well.
    envCommon.Append(CPPPATH=["#/ExtLibs/freetype/include"])
    envCommon.Append(CPPPATH=["#/ExtLibs/libpng"])
    envCommon.Append(CPPPATH=["#/ExtLibs/zlib"])

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
        envDebug.Append(CCFLAGS=Split("/MDd /Od /Z7 /RTC1"));
        envDebug.Append(LINKFLAGS=["/debug"]);

        # Environment for release builds:
        envRelease=envCommon.Clone();
        envRelease.Append(CCFLAGS=Split("/MD /O2 /Ob2"));

        # Environment for profile builds:
        envProfile=envCommon.Clone();
        envProfile.Append(CCFLAGS=Split("/MD /O2 /Ob2 /Z7"));
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
        envDebug.Append(CCFLAGS=Split("/MDd /Od /Z7 /RTC1"));
        envDebug.Append(LINKFLAGS=["/debug"]);

        # Environment for release builds:
        envRelease=envCommon.Clone();
        envRelease.Append(CCFLAGS=Split("/MD /O2 /Ob2"));

        # Environment for profile builds:
        envProfile=envCommon.Clone();
        envProfile.Append(CCFLAGS=Split("/MD /O2 /Ob2 /Z7"));
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
        envDebug.Append(CCFLAGS=Split("/MDd /Od /Z7 /RTC1"));
        envDebug.Append(LINKFLAGS=["/debug"]);

        # Environment for release builds:
        envRelease=envCommon.Clone();
        envRelease.Append(CCFLAGS=Split("/MD /O2 /Ob2"));

        # Environment for profile builds:
        envProfile=envCommon.Clone();
        envProfile.Append(CCFLAGS=Split("/MD /O2 /Ob2 /Z7"));
        envProfile.Append(LINKFLAGS=["/fixed:no", "/debug"]);

    elif envCommon["MSVC_VERSION"] in ["11.0", "11.0Exp"]:
        ##############################
        ### Win32, Visual C++ 2012 ###
        ##############################

        compiler="vc11"

        # Reference of commonly used compiler switches:
        # Identical to the compiler switches for Visual C++ 2005, see there for more details.

        # Begin with an environment with settings that are common for debug, release and profile builds.
        envCommon.Append(CCFLAGS = Split("/GR /EHsc"))   # CCFLAGS is also taken as the default value for CXXFLAGS.
        envCommon.Append(CPPDEFINES = ["_CRT_SECURE_NO_DEPRECATE", "_CRT_NONSTDC_NO_DEPRECATE"])
        envCommon.Append(LINKFLAGS = Split("/incremental:no"))

        # Environment for debug builds:
        envDebug=envCommon.Clone();
        envDebug.Append(CCFLAGS=Split("/MDd /Od /Z7 /RTC1"));
        envDebug.Append(LINKFLAGS=["/debug"]);

        # Environment for release builds:
        envRelease=envCommon.Clone();
        envRelease.Append(CCFLAGS=Split("/MD /O2 /Ob2"));

        # Environment for profile builds:
        envProfile=envCommon.Clone();
        envProfile.Append(CCFLAGS=Split("/MD /O2 /Ob2 /Z7"));
        envProfile.Append(LINKFLAGS=["/fixed:no", "/debug"]);

    elif envCommon["MSVC_VERSION"] in ["12.0", "12.0Exp", "14.0", "14.0Exp"]:
        #######################################
        ### Win32, Visual C++ 2013 and 2015 ###
        #######################################

        compiler = "vc" + envCommon["MSVC_VERSION"][:2]    # "vc12" or "vc14"
        assert compiler in ["vc12", "vc14"]

        # Reference of commonly used compiler switches:
        # Identical to the compiler switches for Visual C++ 2005, see there for more details.

        # Begin with an environment with settings that are common for debug, release and profile builds.
        envCommon.Append(CCFLAGS = Split("/GR /EHsc"))   # CCFLAGS is also taken as the default value for CXXFLAGS.
        envCommon.Append(CPPDEFINES = ["_CRT_SECURE_NO_DEPRECATE", "_CRT_NONSTDC_NO_DEPRECATE", "_WINSOCK_DEPRECATED_NO_WARNINGS"])
        envCommon.Append(LINKFLAGS = Split("/incremental:no"))

        # Environment for debug builds:
        envDebug=envCommon.Clone();
        envDebug.Append(CCFLAGS=Split("/MDd /Od /Z7 /RTC1"));
        envDebug.Append(LINKFLAGS=["/debug"]);

        # Environment for release builds:
        envRelease=envCommon.Clone();
        envRelease.Append(CCFLAGS=Split("/MD /O2 /Ob2 /GL"));

        # Environment for profile builds:
        envProfile=envCommon.Clone();
        envProfile.Append(CCFLAGS=Split("/MD /O2 /Ob2 /Z7"));
        envProfile.Append(LINKFLAGS=["/fixed:no", "/debug"]);

    else:
        ###############################
        ### Win32, unknown compiler ###
        ###############################

        print("Unknown compiler " + envCommon["MSVC_VERSION"] + " on platform " + sys.platform + ".")
        Exit(1)

elif sys.platform.startswith("linux"):
    ErrorMsg = "Please install the %s library (development files)!\nOn many systems, the required package is named %s (possibly with a different version number)."
    conf = Configure(envCommon)

    # conf.CheckLib(...)    # See http://www.cafu.de/wiki/cppdev:gettingstarted#linux_packages for additional libraries and headers that should be checked here.

    #if not conf.CheckLibWithHeader("freetype", "ft2build.h", "c"):     # TODO: What is the proper way to check for freetype?
        #print ErrorMsg % ("freetype", "libfreetype6-dev")
        #Exit(1)

    if not conf.CheckLibWithHeader("png", "png.h", "c"):
        print(ErrorMsg % ("png", "libpng12-dev"))
        Exit(1)

    if not conf.CheckLibWithHeader("z", "zlib.h", "c"):
        print(ErrorMsg % ("zlib", "zlib1g-dev"))
        Exit(1)

    envCommon = conf.Finish()

    if envCommon["CXX"]=="g++":
        ##################
        ### Linux, g++ ###
        ##################

        compiler="g++"

        # The initial environment has settings that are common for debug, release and profile builds.
        envCommon.Append(CCFLAGS = [])    # CCFLAGS is also taken as the default value for CXXFLAGS.
        envCommon.Append(CXXFLAGS = ["-std=c++0x"])

        # Run additional, compiler-specific checks.
        conf = Configure(envCommon, custom_tests = {'CheckOverrideIdentifier' : CheckOverrideIdentifier})

        if not conf.CheckOverrideIdentifier():
            print("C++11 `override` identifier is not supported (defining an empty macro as a work-around).")
            conf.env.Append(CPPDEFINES = [("override", "")])

        envCommon = conf.Finish()

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

        print("Unknown compiler " + envCommon["CXX"] + " on platform " + sys.platform + ".")
        Exit(1)

else:
    print("Unknown platform '" + sys.platform + "'.")
    Exit(1)

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


ExtLibsList = [
    "bullet",
    "freealut",
    "freetype",
    "glfw",
    "jpeg",
    "libogg",
    "libpng",
    "libvorbis",
    "lwo",
    "lua",
    "minizip",
    "mpg123",
    "noise",
    "openal-soft",
    "zlib",
]

if sys.platform=="win32":
    ExtLibsList.remove("openal-soft")   # OpenAL-Soft is not built on Windows, use the OpenAL Windows SDK there.
else:   # sys.platform.startswith("linux")
    ExtLibsList.remove("freetype")      # We use the system copies of these libraries.
    ExtLibsList.remove("libpng")
    ExtLibsList.remove("zlib")

for lib_name in ExtLibsList:
    s_name=lib_name

    if lib_name=="bullet": s_name+="/src";
    if lib_name=="lua":    s_name+="/src";
    if lib_name=="mpg123": s_name+="/src/libmpg123";

    if "d" in BVs: SConscript("ExtLibs/"+s_name+"/SConscript", exports={ "env": envDebug },   variant_dir="ExtLibs/"+lib_name+"/"+my_build_dir_dbg, duplicate=0)
    if "r" in BVs: SConscript("ExtLibs/"+s_name+"/SConscript", exports={ "env": envRelease }, variant_dir="ExtLibs/"+lib_name+"/"+my_build_dir_rel, duplicate=0)
    if "p" in BVs: SConscript("ExtLibs/"+s_name+"/SConscript", exports={ "env": envProfile }, variant_dir="ExtLibs/"+lib_name+"/"+my_build_dir_prf, duplicate=0)


# Compile wxWidgets for the current platform.
if not envCommon.GetOption('clean'):
    if sys.platform=="win32":
        if compiler.startswith("vc"):
            # If we target a non-x86 architecture, the Makefiles automatically append a suffix to the directory names (no need to tweak COMPILER_PREFIX).
            if   envCommon["TARGET_ARCH"] in ["x86_64", "amd64", "emt64"]: target_cpu=" TARGET_CPU=AMD64"
            elif envCommon["TARGET_ARCH"] in ["ia64"]:                     target_cpu=" TARGET_CPU=IA64"
            else:                                                          target_cpu=""

            result=envDebug.  Execute("nmake /nologo /f makefile.vc BUILD=debug   SHARED=0 COMPILER_PREFIX="+compiler+target_cpu, chdir="ExtLibs/wxWidgets/build/msw");
            if (result!=0): envDebug.  Exit(result);
            result=envRelease.Execute("nmake /nologo /f makefile.vc BUILD=release SHARED=0 COMPILER_PREFIX="+compiler+target_cpu, chdir="ExtLibs/wxWidgets/build/msw");
            if (result!=0): envRelease.Exit(result);
            print("");   # Print just another empty line for better visual separation.

    elif sys.platform.startswith("linux"):
        # This automatic compilation of wxGTK requires that some library packages have been installed already,
        # e.g. libgtk2.0-dev, libgl1-mesa-dev and libglu1-mesa-dev under Ubuntu 8.04.
        # See the documentation at http://www.cafu.de/wiki/ for more details!
        if not os.path.exists("ExtLibs/wxWidgets/build-gtk/make-ok-flag"):
            envRelease.Execute(Mkdir("ExtLibs/wxWidgets/build-gtk"));
            result=envRelease.Execute("../configure --with-gtk --disable-shared --without-libtiff --disable-pnm --disable-pcx --disable-iff --with-opengl --with-regex=builtin --disable-compat26 --disable-compat28", chdir="ExtLibs/wxWidgets/build-gtk");
            if (result!=0): envRelease.Exit(result);
            result=envRelease.Execute("make && touch make-ok-flag", chdir="ExtLibs/wxWidgets/build-gtk");
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
    if envCommon["TARGET_ARCH"]=="x86":
        envRelease.Install(".", ["#/ExtLibs/Cg/bin/cg.dll", "#/ExtLibs/Cg/bin/cgGL.dll"]);
        envRelease.Install(".", ["#/ExtLibs/openal-win/libs/Win32/OpenAL32.dll", "#/ExtLibs/openal-win/libs/Win32/wrap_oal.dll"]);
        envRelease.Install(".", ["#/ExtLibs/fmod/api/fmod.dll"]);
    else:
        envRelease.Install(".", ["#/ExtLibs/Cg/bin.x64/cg.dll", "#/ExtLibs/Cg/bin.x64/cgGL.dll"]);
        envRelease.Install(".", ["#/ExtLibs/openal-win/libs/Win64/OpenAL32.dll", "#/ExtLibs/openal-win/libs/Win64/wrap_oal.dll"]);

elif sys.platform.startswith("linux"):
    if platform.machine()!="x86_64":
        envRelease.Install(".", ["#/ExtLibs/Cg/lib/libCg.so", "#/ExtLibs/Cg/lib/libCgGL.so"]);
        envRelease.Install(".", ["#/ExtLibs/fmod/api/libfmod-3.75.so"]);
    else:
        envRelease.Install(".", ["#/ExtLibs/Cg/lib.x64/libCg.so", "#/ExtLibs/Cg/lib.x64/libCgGL.so"]);


############################################
### Update the construction environments ###
############################################

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

envDebug_Cafu  .Append(LIBPATH=["#/ExtLibs/"+lib_name+"/"+my_build_dir_dbg for lib_name in ExtLibsList] + ["#/Libs/"+my_build_dir_dbg]);
envRelease_Cafu.Append(LIBPATH=["#/ExtLibs/"+lib_name+"/"+my_build_dir_rel for lib_name in ExtLibsList] + ["#/Libs/"+my_build_dir_rel]);
envProfile_Cafu.Append(LIBPATH=["#/ExtLibs/"+lib_name+"/"+my_build_dir_prf for lib_name in ExtLibsList] + ["#/Libs/"+my_build_dir_prf]);

if compiler in ["vc8", "vc9", "vc10"]:
    envDebug_Cafu  .Append(CCFLAGS=Split("/J /W3 /WX"));
    envRelease_Cafu.Append(CCFLAGS=Split("/J /W3 /WX"));
    envProfile_Cafu.Append(CCFLAGS=Split("/J /W3 /WX"));

elif compiler in ["vc11", "vc12", "vc14"]:
    envDebug_Cafu  .Append(CCFLAGS=Split("/J /W3 /WX"))     # /analyze /analyze:stacksize 80000
    envRelease_Cafu.Append(CCFLAGS=Split("/J /W3 /WX"))
    envProfile_Cafu.Append(CCFLAGS=Split("/J /W3 /WX"))

elif compiler=="g++":
    envDebug_Cafu  .Append(CCFLAGS=Split("-funsigned-char -Wall -Werror -Wno-char-subscripts"));
    envRelease_Cafu.Append(CCFLAGS=Split("-funsigned-char -Wall -Werror -Wno-char-subscripts -fno-strict-aliasing"));
    envProfile_Cafu.Append(CCFLAGS=Split("-funsigned-char -Wall -Werror -Wno-char-subscripts -fno-strict-aliasing"));

else:
    print("Unknown compiler " + compiler + " when updating the build environments.")
    Exit(1)


###########################
### Build all Cafu code ###
###########################

if not os.path.exists(Dir("#/ExtLibs/fbx/lib").abspath):
    print("Note: The FBX SDK is not present.\n")

if os.path.exists("Libs/SConscript"):
    # Build everything in the Libs/ directory.
    if "d" in BVs: buildMode = "dbg"; SConscript('Libs/SConscript', exports=[{'env':envDebug_Cafu},   'buildMode', 'compiler'], variant_dir="Libs/"+my_build_dir_dbg, duplicate=0)
    if "r" in BVs: buildMode = "rel"; SConscript('Libs/SConscript', exports=[{'env':envRelease_Cafu}, 'buildMode', 'compiler'], variant_dir="Libs/"+my_build_dir_rel, duplicate=0)
    if "p" in BVs: buildMode = "prf"; SConscript('Libs/SConscript', exports=[{'env':envProfile_Cafu}, 'buildMode', 'compiler'], variant_dir="Libs/"+my_build_dir_prf, duplicate=0)

if os.path.exists("SConscript"):
    # Build the Cafu executables.
    if "d" in BVs: buildMode = "dbg"; SConscript('SConscript', exports=[{'env':envDebug_Cafu},   'buildMode', 'compiler'], variant_dir=""+my_build_dir_dbg, duplicate=0)
    if "r" in BVs: buildMode = "rel"; SConscript('SConscript', exports=[{'env':envRelease_Cafu}, 'buildMode', 'compiler'], variant_dir=""+my_build_dir_rel, duplicate=0)
    if "p" in BVs: buildMode = "prf"; SConscript('SConscript', exports=[{'env':envProfile_Cafu}, 'buildMode', 'compiler'], variant_dir=""+my_build_dir_prf, duplicate=0)
