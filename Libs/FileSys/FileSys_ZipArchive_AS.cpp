/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "FileSys_ZipArchive_AS.hpp"
#include "File_memory.hpp"
#include "ConsoleCommands/Console.hpp"

using namespace cf::FileSys;


FileSystemZipArchiveAST::FileSystemZipArchiveAST(const std::string& ArchiveName_, const std::string& MountPoint_, const std::string& Password_)
    : ArchiveName(ArchiveName_),
      Archive(),
      MountPoint(MountPoint_),
      MountPointLen(MountPoint.length()),
      NameToFileNr()
{
    try
    {
        Archive.Open(ArchiveName.c_str(), CZipArchive::zipOpenReadOnly);
        Archive.SetPassword(Password_.c_str());
    }
    catch (CZipException& ZipEx)
    {
        Console->Warning(ArchiveName+": "+ZipEx.GetErrorDescription()+"\n");
        throw FileSystemExceptionT(FileSystemExceptionT::InitFailure);
    }

    const ZIP_INDEX_TYPE NrOfFiles=Archive.GetCount();

    Console->Print(std::string("Registering archive \"")+ArchiveName+"\".\n");
    for (ZIP_INDEX_TYPE FileNr=0; FileNr<NrOfFiles; FileNr++)
    {
        CZipFileHeader* FileHeader=Archive[FileNr];

        if (FileHeader==NULL) continue;
        if (FileHeader->IsDirectory()) continue;

        std::string                  FileName   =FileHeader->GetFileName();
        const std::string::size_type FileNameLen=FileName.length();

        // Flip all '\' slashes into '/' slashes.
        for (unsigned long i=0; i<FileNameLen; i++)
            if (FileName[i]=='\\') FileName[i]='/';

        NameToFileNr[FileName]=FileNr;
        // Console->Print(cf::va("    %4lu ", FileNr)+FileName+"\n");
    }
}


FileSystemZipArchiveAST::~FileSystemZipArchiveAST()
{
    try
    {
        Archive.Close();
    }
    catch (CZipException& ZipEx)
    {
        Console->Warning(ArchiveName+": "+ZipEx.GetErrorDescription()+"\n");
    }
}


InFileI* FileSystemZipArchiveAST::OpenRead(const std::string& FileName)
{
    // Remove the MountPoint portion from FileName.
    if (FileName.compare(0, MountPointLen, MountPoint)!=0) return NULL;     // See if FileName begins with MountPoint.

#if defined(_WIN32) && defined(_MSC_VER) && (_MSC_VER<1300)
    std::string   RelFileName=FileName.c_str()+MountPointLen;   // RelFileName is FileName without the MountPoint prefix.
#else
    std::string RelFileName(FileName, MountPointLen);   // RelFileName is FileName without the MountPoint prefix.
#endif

    // Determine at which index, if any, the file is.
    std::map<std::string, ZIP_INDEX_TYPE>::const_iterator It=NameToFileNr.find(RelFileName);

    // Check if we have a file with such a name in the zip archive.
    if (It==NameToFileNr.end()) return NULL;

    const ZIP_INDEX_TYPE FileNr    =It->second;
    CZipFileHeader*      FileHeader=Archive[FileNr];

    if (FileHeader==NULL) return NULL;

    MemoryInFileT* InFile=new MemoryInFileT(FileName);
    ArrayT<char>&  Buffer=InFile->GetBuffer();

    Buffer.PushBackEmpty(FileHeader->m_uUncomprSize);

    try
    {
        // Note that we need the try-catch block even though the called methods return error values;
        // under some conditions they also throw exceptions instead (for example when the open-source
        // GPL version of the ZipArchive library is employed and Archive.OpenFile() is called with an
        // encrypted archive file).
        if (!Archive.OpenFile(FileNr) ||
            Archive.ReadFile(&Buffer[0], Buffer.Size())!=Buffer.Size() ||
         // Archive.ReadFile(&Test, 1)!=0 ||
            Archive.CloseFile()!=1)
        {
            CZipException::Throw();
        }
    }
    catch (CZipException& ZipEx)
    {
        Console->Warning(ArchiveName+"/"+RelFileName+": "+ZipEx.GetErrorDescription()+"\n");
        Archive.CloseFile(NULL, true);
        delete InFile;
        return NULL;
    }

    return InFile;
}
