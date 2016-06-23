#!/usr/bin/env python
# -*- coding: utf-8 -*-
import argparse
import os
import shutil
import subprocess
import sys

# In order to install module "paramiko", follow these steps:
#
#   1. As a prerequisite under Windows, install a prebuilt binary of PyCrypto
#      from http://www.voidspace.org.uk/python/modules.shtml#pycrypto
#      As we use 32-bit versions of Python 2.7 (as explained at
#      http://www.cafu.de/wiki/cppdev:gettingstarted#python_and_scons), usually
#      the right choice is
#      http://www.voidspace.org.uk/downloads/pycrypto26/pycrypto-2.6.win32-py2.7.exe
#
#   2. Both under Windows and Linux, install Paramiko at the command line:
#          pip install paramiko
#      If pip is not yet installed on your system, install it as explained at
#      http://www.pip-installer.org/en/latest/installing.html
import paramiko


def FindSConsBuildDir(root):
    """
    Starting at the given Cafu root directory, this method guesses where all the program
    files are that were built by a previously successful run of SCons.
    """
    if sys.platform == "win32":
        for compiler in ["vc15", "vc14", "vc13", "vc12", "vc11", "vc10", "vc9", "vc8"]:
            for arch in ["amd64", "x64", "x86"]:
                path = "build/" + sys.platform + "/" + compiler + "/" + arch + "/release"
                if os.path.isfile(root + "/" + path + "/CaBSP/CaBSP.exe"):
                    return path
    else:
        for compiler in ["g++"]:
            path = "build/" + sys.platform + "/" + compiler + "/release"
            if os.path.isfile(root + "/" + path + "/CaBSP/CaBSP"):
                return path

    raise Exception("Could not find the SCons build directory.")


def BuildDoxygenDocs(SourceDir):
    """
    Builds the C++ and Lua reference documentation in a Cafu source code repository `SourceDir`.

    The directory `SourceDir` must be the root directory of a Cafu source code repository checkout
    in which in which all programs and libraries have already successfully been built.

    The actions of the function modify the contents of the `cpp/out`,  `scripting/tmpl` and
    `scripting/out` directories in `SourceDir`.
    """
    PathCaWE = FindSConsBuildDir(SourceDir) + "/CaWE/CaWE"

    subprocess.call([SourceDir + "/" + PathCaWE, "--update-doxygen"], cwd=SourceDir)

    # For all files in the Doxygen/scripting/tmpl directory, make sure that they
    #   - don't contain the string "// WARNING" (e.g. about mismatches),
    #   - don't contain any lines that the related files in ../src don't have.
    for root, dirs, files in os.walk(SourceDir + "/Doxygen/scripting/tmpl"):
        for filename in files:
            if filename == "README.txt":
                continue

            with open(root + "/" + filename, 'r') as tmplFile:
                with open(root + "/../src/" + filename, 'r') as srcFile:
                    srcLines  = srcFile.readlines()
                    srcLineNr = 0

                    for tmplLine in tmplFile:
                        if "// WARNING" in tmplLine:
                            raise Exception('Found a "// WARNING ..." comment in file "{0}".'.format(filename))

                        while tmplLine != srcLines[srcLineNr]:
                            srcLineNr += 1
                            if srcLineNr >= len(srcLines):
                                raise Exception('A line in template file "{0}" does not exist in the related source '
                                                'file:\n\n{1}\nUse e.g. BeyondCompare to review the situation.'.format(filename, tmplLine))

    # TODO: See http://marc.info/?l=doxygen-users&m=126909030104387&w=2 about Doxygen exit codes.
    subprocess.call(["doxygen", "Doxygen/cpp/Doxyfile"], cwd=SourceDir)
    subprocess.call(["doxygen", "Doxygen/scripting/Doxyfile"], cwd=SourceDir)


def Upload(sftp, LocalDir, RemoteDir):
    """
    Recursively uploads the contents of `LocalDir` to `RemoteDir`,
    using the already established (opened) FTP connection `sftp`.
    """
    for root, dirs, files in os.walk(LocalDir):
        relpath = os.path.relpath(root, LocalDir)
        # print "root =", root, "relpath =", relpath, "dirs =", dirs, "#files =", len(files), "files[0] =", files[0]

        for filename in files:
            if relpath == ".":
                sftp.put(LocalDir  + "/" + filename,
                         RemoteDir + "/" + filename)
            else:
                # TODO: This will fail if `relpath` does not yet exist in `RemoteDir`.
                sftp.put(LocalDir  + "/" + relpath + "/" + filename,
                         RemoteDir + "/" + relpath + "/" + filename)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Builds and uploads the C++ and Lua reference documentation.")
    parser.add_argument("-u", "--upload", action="store_true", help="upload via FTP")
    args = parser.parse_args()

    CafuRoot = "."
    BuildDoxygenDocs(CafuRoot)

    if args.upload:
        try:
            from ftp_login import GetFtpLogin
        except ImportError:
            # For convenience, install the file from the related template file
            # (which is under version control) automatically.
            d = os.path.dirname(__file__)
            shutil.copy(
                os.path.join(d, "ftp_login.py.tmpl"),
                os.path.join(d, "ftp_login.py"))
            from ftp_login import GetFtpLogin

        ftp_username, ftp_password = GetFtpLogin()

        if ftp_username and ftp_password:
            print "Uploading files..."
            ssh = paramiko.SSHClient()

            ssh.set_missing_host_key_policy(paramiko.AutoAddPolicy())
            ssh.connect('ftp.cafu.de', username=ftp_username, password=ftp_password)

            sftp = ssh.open_sftp()

            Upload(sftp, CafuRoot + "/Doxygen/cpp/out/html", "cafu/api/c++")
            Upload(sftp, CafuRoot + "/Doxygen/scripting/out/html", "cafu/api/lua")

            sftp.close()
        else:
            print "No FTP login details given, not uploading."
