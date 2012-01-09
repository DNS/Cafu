/*
=================================================================================
This file is part of Cafu, the open-source game engine and graphics engine
for multiplayer, cross-platform, real-time 3D action.
Copyright (C) 2002-2012 Carsten Fuchs Software.

Cafu is free software: you can redistribute it and/or modify it under the terms
of the GNU General Public License as published by the Free Software Foundation,
either version 3 of the License, or (at your option) any later version.

Cafu is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Cafu. If not, see <http://www.gnu.org/licenses/>.

For support and more information about Cafu, visit us at <http://www.cafu.de>.
=================================================================================
*/

#include "FileSys_LocalPath.hpp"
#include "File_local.hpp"

using namespace cf::FileSys;


FileSystemLocalPathT::FileSystemLocalPathT(const std::string& LocalPath_, const std::string& MountPoint_)
    : LocalPath(LocalPath_),
      MountPoint(MountPoint_),
      MountPointLen(MountPoint.length())
{
}


InFileI* FileSystemLocalPathT::OpenRead(const std::string& FileName)
{
    // Remove the MountPoint portion from FileName.
    if (FileName.compare(0, MountPointLen, MountPoint)!=0) return NULL;     // See if FileName begins with MountPoint.

#if defined(_WIN32) && defined(_MSC_VER) && (_MSC_VER<1300)
    std::string   RelFileName=FileName.c_str()+MountPointLen;   // RelFileName is FileName without the MountPoint prefix.
#else
    std::string   RelFileName(FileName, MountPointLen);     // RelFileName is FileName without the MountPoint prefix.
#endif
    LocalInFileT* InFile=new LocalInFileT(FileName, LocalPath+RelFileName);

    if (InFile->IsOpen()) return InFile;

    delete InFile;
    return NULL;
}
