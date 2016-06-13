/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "FileSys_ZipArchive_GV.hpp"
#include "File_memory.hpp"
#include "ConsoleCommands/Console.hpp"

using namespace cf::FileSys;


FileSystemZipArchiveGVT::FileSystemZipArchiveGVT(const std::string& ArchiveName, const std::string& MountPoint, const std::string& Password)
    : m_ArchiveName(ArchiveName),
      m_ArchivePassword(Password),
      m_ArchiveHandle(NULL),
      m_MountPoint(MountPoint),
      m_MountPointLen(MountPoint.length()),
      m_NameToFilePos()
{
    m_ArchiveHandle=unzOpen(m_ArchiveName.c_str());

    if (m_ArchiveHandle==NULL)
    {
        Console->Warning("Failed to open ZIP archive "+m_ArchiveName+"\n");
        throw FileSystemExceptionT(FileSystemExceptionT::InitFailure);
    }

/*  unz_global_info ZipInfo;
    if (unzGetGlobalInfo(m_ArchiveHandle, &ZipInfo)!=UNZ_OK)
    {
        Console->Warning("Couldn't get global info for ZIP archive "+m_ArchiveName+"\n");
        ZipInfo.number_entry=0;
    } */

    Console->Print(std::string("Registering archive \"")+m_ArchiveName+"\".\n");

    for (int Result=unzGoToFirstFile(m_ArchiveHandle); Result==UNZ_OK; Result=unzGoToNextFile(m_ArchiveHandle))
    {
        char FileName[4096];

        FileName[0]=0;
        if (unzGetCurrentFileInfo(m_ArchiveHandle, NULL, FileName, 4096, NULL, 0, NULL, 0)!=UNZ_OK) continue;
        FileName[4095]=0;

        // FIXME / TODO: if (current file is a directory) continue;

        // Flip all '\' slashes into '/' slashes.
        for (unsigned long i=0; FileName[i]; i++)
            if (FileName[i]=='\\') FileName[i]='/';

        unz_file_pos FilePos;
        if (unzGetFilePos(m_ArchiveHandle, &FilePos)!=UNZ_OK) continue;

        m_NameToFilePos[std::string(FileName)]=FilePos;
        // Console->Print(cf::va("    %4lu ", FileNr)+FileName+"\n");
    }
}


FileSystemZipArchiveGVT::~FileSystemZipArchiveGVT()
{
    const int Result=unzClose(m_ArchiveHandle);

    assert(Result==UNZ_OK);
    (void)Result;       // Have g++ not warn about Result being an unused variable.
}


InFileI* FileSystemZipArchiveGVT::OpenRead(const std::string& FileName)
{
    // Remove the m_MountPoint portion from FileName.
    if (FileName.compare(0, m_MountPointLen, m_MountPoint)!=0) return NULL;     // Determine if FileName begins with m_MountPoint.

#if defined(_WIN32) && defined(_MSC_VER) && (_MSC_VER<1300)
    const std::string RelFileName=FileName.c_str()+m_MountPointLen; // RelFileName is FileName without the m_MountPoint prefix.
#else
    const std::string RelFileName(FileName, m_MountPointLen);       // RelFileName is FileName without the m_MountPoint prefix.
#endif

    // Determine at which index, if any, the file is, i.e. check if we have a file with such a name in the zip archive.
    std::map<std::string, unz_file_pos>::const_iterator It=m_NameToFilePos.find(RelFileName);
    if (It==m_NameToFilePos.end())
    {
        Console->Warning(RelFileName+" not found in "+m_ArchiveName+"\n");
        return NULL;
    }

    unz_file_pos FilePos=It->second;
    if (unzGoToFilePos(m_ArchiveHandle, &FilePos)!=UNZ_OK)
    {
        Console->Warning("unzGoToFilePos() failed for "+RelFileName+" in "+m_ArchiveName+"\n");
        return NULL;
    }

    unz_file_info FileInfo;
    if (unzGetCurrentFileInfo(m_ArchiveHandle, &FileInfo, NULL, 0, NULL, 0, NULL, 0)!=UNZ_OK)
    {
        Console->Warning("Could not get file info for "+RelFileName+" in "+m_ArchiveName+"\n");
        return NULL;
    }

    if (unzOpenCurrentFilePassword(m_ArchiveHandle, m_ArchivePassword!="" ? m_ArchivePassword.c_str() : NULL)!=UNZ_OK)
    {
        Console->Warning("Could not open "+RelFileName+" for reading from "+m_ArchiveName+"\n");
        return NULL;
    }

    MemoryInFileT* InFile=new MemoryInFileT(FileName);
    ArrayT<char>&  Buffer=InFile->GetBuffer();

    Buffer.PushBackEmpty(FileInfo.uncompressed_size);

    const int ReadResult=unzReadCurrentFile(m_ArchiveHandle, &Buffer[0], Buffer.Size());
    unzCloseCurrentFile(m_ArchiveHandle);

    if (ReadResult!=int(Buffer.Size()))
    {
        Console->Warning("Couldn't read "+m_ArchiveName+"/"+RelFileName+cf::va(" (%i)", ReadResult)+"\n");
        delete InFile;
        InFile=NULL;
    }

    return InFile;
}
