/*
=================================================================================
This file is part of Cafu, the open-source game engine and graphics engine
for multiplayer, cross-platform, real-time 3D action.
Copyright (C) 2002-2011 Carsten Fuchs Software.

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

#ifndef _CF_FILESYS_FILESYSTEM_ZIPARCHIVE_GV_HPP_
#define _CF_FILESYS_FILESYSTEM_ZIPARCHIVE_GV_HPP_

#include "FileSys.hpp"
#include "minizip/unzip.h"
#include <map>


namespace cf
{
    namespace FileSys
    {
        /// This class implements file systems that are ZIP archives, employing the unzip library by Gilles Vollant.
        /// The unzip library is part of the minizip contribution to the zlib, and comes under the same liberal zlib license.
        class FileSystemZipArchiveGVT : public FileSystemT
        {
            public:

            /// Constructor.
            /// @throws FileSystemExceptionT   when there is a problem with initializing this file system.
            FileSystemZipArchiveGVT(const std::string& ArchiveName, const std::string& MountPoint, const std::string& Password="");

            /// Destructor. Destroys the file system.
            ~FileSystemZipArchiveGVT();

            // Implement all the (pure) virtual methods of the FileSystemT class.
            InFileI* OpenRead(const std::string& FileName);


            private:

            const std::string                   m_ArchiveName;      ///< The name of the zip archive.
            const std::string                   m_ArchivePassword;  ///< The password for decrypting the contents of the zip archive.
            void*                               m_ArchiveHandle;    ///< The file handle to the opened zip archive itself, for use with all the unzip functions.
            const std::string                   m_MountPoint;       ///< The mount point of the archive contents.
            const std::string::size_type        m_MountPointLen;    ///< The length of the m_MountPoint string.
            std::map<std::string, unz_file_pos> m_NameToFilePos;    ///< Maps a file name to the related position/index in the zip archive.
        };
    }
}

#endif
