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
