/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_FILESYS_FILEMAN_HPP_INCLUDED
#define CAFU_FILESYS_FILEMAN_HPP_INCLUDED

#include <string>


namespace cf
{
    namespace FileSys
    {
        class FileSystemT;
        class FileI;
        class InFileI;

        enum FileSystemTypeT
        {
            FS_TYPE_LOCAL_PATH,
            FS_TYPE_ZIP_ARCHIVE
        };


        /// This class provides an interface to a file manager.
        /// The interface is specified as an ABC in order to be able to share it across exe/dll boundaries.
        class FileManI
        {
            public:

            /// The destructor.
            /// This ABC does neither have nor need a destructor, because no implementation will ever be deleted via a pointer to a FileManI.
            /// (The implementations are singletons after all.)  See the Singleton pattern and the C++ FAQ 21.05 (the "precise rule") for more information.
            /// g++ however issues a warning with no such destructor, so I provide one anyway and am safe.
            virtual ~FileManI() { }

            /// "Mounts" (or "registers") a new file system into the scope of the file manager.
            /// For example, you can register paths on a local hard-disk, compressed archives and HTTP connections as file systems.
            /// @param Type         The type of the file system to be mounted.
            /// @param Descr        A description/specification of the file system to be mounted, for example "Games/DeathMatch/", "MyArchive.zip", etc.
            /// @param MountPoint   The mount point of the file system, i.e. the path where the contents of the file system is attached to.
            /// @param Password     The password string that might be required to access the specified file system (e.g. encrypted zip archives).
            /// @returns a handle to the mounted file system, or NULL on error. Note that the handle is "opaque" for the user code.
            ///     That is, it can do nothing with it but check it for NULL in order to learn if an error occurred, and pass it to Unregister() later.
            ///     Also note that a mount failure (getting a NULL result) here is not necessarily reason enough to terminate the entire application -
            ///     the file manager is robust enough to deal with the situation (i.e. you can pass NULL to Unregister()), but of course opening files
            ///     that are supposed to be in the file system that failed to mount will not succeed.
            virtual FileSystemT* MountFileSystem(FileSystemTypeT Type, const std::string& Descr, const std::string& MountPoint, const std::string& Password="")=0;

            /// "Unmounts" (or "unregisters") a file system from the file manager.
            /// @param FileSystem   The handle to the file system that was obtained by the call to MountFileSystem.
            virtual void Unmount(FileSystemT* FileSystem)=0;

            /// Opens the file with the given name for reading.
            /// The registered file systems are probed in opposite order in which they have been registered for opening the file.
            /// @param FileName   The name of the file to be opened within one of the previously registered file systems.
            /// @returns The pointer to the file or NULL if the file could not be opened.
            virtual InFileI* OpenRead(const std::string& FileName)=0;

            /// Closes the given file.
            virtual void Close(FileI* File)=0;
        };


        /// A global pointer to an implementation of the FileManI interface.
        ///
        /// Each module (exe or dll) that uses this pointer must somewhere provide exactly one definition for it (none is provided by the FileSys library).
        /// That is, typically the main.cpp or similar file of each exe and dll must contain a line like
        ///     cf::FileSys::FileManI* cf::FileSys::FileMan=NULL;
        /// or else the module will not link successfully due to an undefined symbol.
        ///
        /// Exe files will then want to reset this pointer to an instance of a FileManImplT during their initialization
        /// e.g. by code like:   cf::FileSys::FileMan=new cf::FileSys::FileManImplT;
        /// Note that the FileManImplT ctor may require that other interfaces (e.g. the MatSys) have been inited first.
        ///
        /// Dlls typically get one of their init functions called immediately after they have been loaded.
        /// By doing so, the exe passes a pointer to its above instance to the dll, which in turn copies it to its FileMan variable.
        extern FileManI* FileMan;
    }
}

#endif
