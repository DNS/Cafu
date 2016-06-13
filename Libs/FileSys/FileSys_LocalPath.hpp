/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_FILESYS_FILESYSTEM_LOCALPATH_HPP_INCLUDED
#define CAFU_FILESYS_FILESYSTEM_LOCALPATH_HPP_INCLUDED

#include "FileSys.hpp"


namespace cf
{
    namespace FileSys
    {
        class FileSystemLocalPathT : public FileSystemT
        {
            public:

            /// Constructor.
            /// @throws FileSystemExceptionT   when there is a problem with initializing this file system.
            FileSystemLocalPathT(const std::string& LocalPath_, const std::string& MountPoint_);

            /// Destructor. Destroys the file system.
         // ~FileSystemLocalPathT();

            // Implement all the (pure) virtual methods of the FileSystemT class.
            InFileI* OpenRead(const std::string& FileName);


            private:

            const std::string            LocalPath;     ///< The local path to be mounted.
            const std::string            MountPoint;    ///< The mount point of the path contents.
            const std::string::size_type MountPointLen; ///< The length of the MountPoint string.
        };
    }
}

#endif
