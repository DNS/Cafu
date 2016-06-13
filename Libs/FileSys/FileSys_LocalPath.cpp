/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
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
