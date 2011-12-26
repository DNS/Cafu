from subprocess import call
import os, sys

# Note that this script *must* be run from the Cafu top-level directory,
# not from Cafu/Games/DeathMatch/, or else the tools cannot find e.g. the Textures/*.zip files.

Maps = [
    ("Test1",            [], [], ["-StopUE", "0.01"]),
    ("TestPatches",      [], [], ["-StopUE", "0.01"]),
    ("AEonsCanyonTower", [], [], ["-StopUE", "0.1"]),
    ("AEonsCube",        [], [], ["-StopUE", "0.1"]),
    ("BPRockB",          [], [], ["-StopUE", "0.1"]),
    ("BPWxBeta",         [], [], ["-StopUE", "0.1"]),
    ("JrBaseHQ",         [], [], ["-StopUE", "0.1"]),
    ("Kidney",           [], [], ["-StopUE", "0.1"]),
    ("ReNoEcho",         [], [], ["-StopUE", "0.1"]),
    ("ReNoElixir",       [], [], ["-StopUE", "0.1"]),
    ("TechDemo",         [], [], ["-StopUE", "0.1"]),
    ("Gotham",           [], [], ["-fast"])     # Gotham is, at the moment, a really bad case that can take CaLight very long to complete.
]


def FindTools():
    for compiler in ["vc10", "vc9", "vc8"]:
        for arch in ["x64", "x86"]:
            path = 'build/' + sys.platform + '/' + compiler + "/" + arch + "/release"
            if os.path.isfile(path + "/CaBSP/CaBSP.exe"):
                return path
    raise Exception("Could not find the Cafu map compile tools.")


ToolPath = FindTools()
print "Using tools in " + ToolPath


for Map in Maps:
    MapName = Map[0]

    if len(sys.argv) > 1:
        if MapName.lower() not in [arg.lower() for arg in sys.argv[1:]]:
            continue

    call([ToolPath + '/CaBSP/CaBSP.exe', 'Games/DeathMatch/Maps/%s.cmap' % MapName, 'Games/DeathMatch/Worlds/%s.cw' % MapName] + Map[1])
    call([ToolPath + '/CaPVS/CaPVS.exe', 'Games/DeathMatch/Worlds/%s.cw' % MapName] + Map[2])
    call([ToolPath + '/CaLight/CaLight.exe', 'Games/DeathMatch/Worlds/%s.cw' % MapName, '-gd=Games/DeathMatch'] + Map[3])


# Finally shutdown the computer.
#   -c   Allow the shutdown to be aborted by the interactive user.
#   -f   Forces running applications to close.
#   -k   Poweroff the computer (reboot if poweroff is not supported).
#   -t   Specifies countdown in seconds until shutdown (default is 20) or the time of shutdown (in 24 hour notation).
if "--shutdown" in sys.argv[1:]:
    if sys.platform == "win32":
        shutdown_app = "d:\Programme\PsTools\psshutdown.exe"
        if os.path.isfile(shutdown_app):
            call([shutdown_app, "-c", "-f", "-k", "-t", "300"])
