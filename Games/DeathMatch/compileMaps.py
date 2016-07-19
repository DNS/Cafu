#!/usr/bin/env python
# -*- coding: utf-8 -*-
from subprocess import call
import os, sys

# Note that this script *must* be run from the Cafu top-level directory,
# not from Cafu/Games/DeathMatch/, or else the tools cannot find e.g. the Textures/*.zip files.

MapSettings = {
    "Test1":            ([], [], ["-StopUE", "0.01"],  1),
    "TestPatches":      ([], [], ["-StopUE", "0.01"],  2),
    "TestPhysics":      ([], [], ["-StopUE", "0.01"],  3),
    "AEonsCanyonTower": ([], [], ["-StopUE", "0.1"],   4),
    "AEonsCube":        ([], [], ["-StopUE", "0.1"],   5),
    "BPRockB":          ([], [], ["-StopUE", "0.1"],   6),
    "BPWxBeta":         ([], [], ["-StopUE", "0.1"],   7),
    "JrBaseHQ":         ([], [], ["-StopUE", "0.1"],   8),
    "Kidney":           ([], [], ["-StopUE", "0.1"],   9),
    "ReNoEcho":         ([], [], ["-StopUE", "0.1"],  10),
    "ReNoElixir":       ([], [], ["-StopUE", "0.1"],  11),
    "TechDemo":         ([], [], ["-StopUE", "0.1"],  12),
    "Gotham":           ([], [], ["-fast"],           13),  # Gotham is, at this time, a really bad case that can take CaLight very long to complete.
}


def FindTools():
    if sys.platform == "win32":
        for compiler in ["vc14", "vc12", "vc11", "vc10", "vc9", "vc8"]:
            for arch in ["x64", "x86"]:
                path = 'build/' + sys.platform + '/' + compiler + "/" + arch + "/release"
                if os.path.isfile(path + "/CaBSP/CaBSP.exe"):
                    return path
    else:
        for compiler in ["g++"]:
            path = 'build/' + sys.platform + '/' + compiler + "/release"
            if os.path.isfile(path + "/CaBSP/CaBSP"):
                return path
    raise Exception("Could not find the Cafu map compile tools.")


ToolPath = FindTools()
print "Using tools in:", ToolPath

MapList = sys.argv[1:] or [x[0] for x in sorted(MapSettings.items(), key=lambda SettingItem: SettingItem[1][3])]
print "Compiling maps:", MapList


for MapName in MapList:
    if not os.path.isfile("Games/DeathMatch/Maps/{0}.cmap". format(MapName)):
        print "ERROR: Input map file \"Games/DeathMatch/Maps/{0}.cmap\" not found!". format(MapName)
        continue

    if MapName in MapSettings:
        Params = MapSettings[MapName]
    else:
        print "Default settings will be used for map \"{0}\".".format(MapName)
        Params = ([], [], ["-StopUE", "0.1"])

    exeSuffix = ".exe" if sys.platform == "win32" else ""

    if call([ToolPath + '/CaBSP/CaBSP' + exeSuffix, 'Games/DeathMatch/Maps/%s.cmap' % MapName, 'Games/DeathMatch/Worlds/%s.cw' % MapName] + Params[0]):
        continue

    if call([ToolPath + '/CaPVS/CaPVS' + exeSuffix, 'Games/DeathMatch/Worlds/%s.cw' % MapName] + Params[1]):
        continue

    if call([ToolPath + '/CaLight/CaLight' + exeSuffix, 'Games/DeathMatch/Worlds/%s.cw' % MapName, '-gd=Games/DeathMatch'] + Params[2]):
        pass


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
