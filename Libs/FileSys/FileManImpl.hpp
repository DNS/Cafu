/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_FILESYS_FILEMAN_IMPL_HPP_INCLUDED
#define CAFU_FILESYS_FILEMAN_IMPL_HPP_INCLUDED

#include "FileMan.hpp"
#include "Templates/Array.hpp"


namespace cf
{
    namespace FileSys
    {
        /// This class implements the FileManI interface.
        class FileManImplT : public FileManI
        {
            public:

            /// The constructor.
            FileManImplT();

            /// The destructor.
            ~FileManImplT();


            // Implement all the (pure) virtual methods of the FileManI interface.
            FileSystemT* MountFileSystem(FileSystemTypeT Type, const std::string& Descr, const std::string& MountPoint, const std::string& Password="");
            void         Unmount(FileSystemT* FileSystem);
            InFileI*     OpenRead(const std::string& FileName);
            void         Close(FileI* File);


            private:

            FileManImplT(const FileManImplT&);          ///< Use of the Copy Constructor    is not allowed.
            void operator = (const FileManImplT&);      ///< Use of the Assignment Operator is not allowed.

            ArrayT<FileSystemT*> FileSystems;   ///< The stack of file systems (local paths, archives, ...) where files are loaded from (and saved to).
        };
    }
}

#endif
