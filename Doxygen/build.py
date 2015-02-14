#!/usr/bin/env python
# -*- coding: utf-8 -*-
import argparse
import os
import subprocess

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


def Build(SourceDir):
    """
    Builds the C++ and Lua reference documentation in a Cafu source code repository `SourceDir`.
    """

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
    parser = argparse.ArgumentParser(description='Builds and optionally uploads the C++ and Lua reference documentation.')
    parser.add_argument('--username', action="store", help='the FTP username for uploading the files')
    parser.add_argument('--password', action="store", help='the FTP password for uploading the files')

    args         = parser.parse_args()
    ftp_username = args.username or raw_input("FTP username: ")
    ftp_password = args.password or raw_input("FTP password: ")

    CafuRepo = "."
    Build(CafuRepo)

    if ftp_username and ftp_password:
        ssh = paramiko.SSHClient()

        ssh.set_missing_host_key_policy(paramiko.AutoAddPolicy())
        ssh.connect('ftp.cafu.de', username=ftp_username, password=ftp_password)

        sftp = ssh.open_sftp()

        Upload(sftp, CafuRepo + "/Doxygen/cpp/out/html", "cafu/docs/c++")
        Upload(sftp, CafuRepo + "/Doxygen/scripting/out/html", "cafu/docs/lua")

        sftp.close()