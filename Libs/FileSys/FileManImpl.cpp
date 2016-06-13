/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "FileManImpl.hpp"
#include "FileSys_LocalPath.hpp"
#include "FileSys_ZipArchive_GV.hpp"
#include "File_local.hpp"
#include "Password.hpp"
#include "ConsoleCommands/Console.hpp"

using namespace cf::FileSys;


FileManImplT::FileManImplT()
{
}


FileManImplT::~FileManImplT()
{
    for (unsigned long FileSysNr=0; FileSysNr<FileSystems.Size(); FileSysNr++)
        delete FileSystems[FileSysNr];
}


FileSystemT* FileManImplT::MountFileSystem(FileSystemTypeT Type, const std::string& Descr, const std::string& MountPoint, const std::string& Password)
{
    try
    {
        switch (Type)
        {
            case FS_TYPE_LOCAL_PATH:
                // TODO:
                // Untersuche den FileSystemDescr String, um welches File Sys es sich handelt (.zip, .7z Suffix?, http: prefix?)
                // Bei lokalem Pfad: Make sure it's only / slashes and that the trailing / is present.
                FileSystems.PushBack(new FileSystemLocalPathT(Descr, MountPoint));
                break;

            case FS_TYPE_ZIP_ARCHIVE:
                if (Password=="Ca3DE")
                {
                    const unsigned long PasswordLength=32;
                    unsigned char ObfusPassword[32]={ 74, 34, 195, 0, 16, 66, 28, 16, 101, 92, 6, 32, 74, 17, 194, 40, 150, 59, 76, 50, 65, 225, 109, 236, 132, 153, 156, 219, 48, 164, 45, 94 };
                    ArrayT<unsigned char> ObfusStr=cf::Password::GenerateObfuscationString(PasswordLength);
                    ArrayT<unsigned char> ObfusPwd; for (unsigned long i=0; i<PasswordLength; i++) ObfusPwd.PushBack(ObfusPassword[i]);
                    ArrayT<unsigned char> Password_=cf::Password::XOR(ObfusPwd, ObfusStr);
                    std::string           ZipPassword=cf::Password::ToString(Password_);

                    // Console->Print(std::string("Zip key is: ")+ZipPassword+"\n");
                    FileSystems.PushBack(new FileSystemZipArchiveGVT(Descr, MountPoint, ZipPassword));

                    for (unsigned long i=0; i<PasswordLength; i++)
                    {
                        ObfusPassword[i]=0;
                        ObfusStr[i]=0;
                        ObfusPwd[i]=0;
                        Password_[i]=0;
                        ZipPassword[i]='0';
                    }

                    break;
                }

                FileSystems.PushBack(new FileSystemZipArchiveGVT(Descr, MountPoint, Password));
                break;

            default:
                Console->Warning("Tried to register filesystem of unknown type.\n");
                break;
        }
    }
    catch (const FileSystemExceptionT& /*FileSysEx*/)
    {
        // Assume that the filesystem that threw the exception printed a message to the console already.
    }

    return NULL;
}


void FileManImplT::Unmount(FileSystemT* FileSystem)
{
    if (FileSystem==NULL) return;

    if (FileSystems.Size()==0)
    {
        Console->Warning(cf::va("No file systems mounted, but FileManImplT::Unmount(%p); was called.\n", FileSystem));
        return;
    }

    if (FileSystems[FileSystems.Size()-1]!=FileSystem)
    {
        // I don't think that unmouning in opposite mount order is not a reasonable requirement...
        // For example, consider CaWE, where each game configuration mounts its own file systems,
        // and the livetime of a game config is entirely user controlled (i.e. he may add or delete them at will at runtime).
        Console->DevPrint("Note: File systems not unmounted in reverse mount order.\n");
    }

    const int FileSysNr=FileSystems.Find(FileSystem);

    if (FileSysNr<0)
    {
        Console->Warning(cf::va("Tried to unmount unknown file system %p.\n", FileSystem));
        return;
    }

    FileSystems.RemoveAtAndKeepOrder(FileSysNr);
}


InFileI* FileManImplT::OpenRead(const std::string& FileName)
{
    // Try to load the file from the local file system first.
    // This way, the user is not required to register any file system at all before using the file manager,
    // and the behavior is just as expected (principle of "least surprise").
    {
        LocalInFileT* InFile=new LocalInFileT(FileName, FileName);

        if (InFile->IsOpen()) return InFile;
        delete InFile;
    }

    // Now try the registered file systems in turn.
    for (unsigned long FileSysNr=0; FileSysNr<FileSystems.Size(); FileSysNr++)
    {
        InFileI* InFile=FileSystems[FileSystems.Size()-1-FileSysNr]->OpenRead(FileName);

        if (InFile!=NULL) return InFile;
    }

    return NULL;
}


void FileManImplT::Close(FileI* File)
{
    delete File;
}
