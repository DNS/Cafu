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

#ifndef _CF_FILESYS_FILESYSTEM_ZIPARCHIVE_AS_HPP_
#define _CF_FILESYS_FILESYSTEM_ZIPARCHIVE_AS_HPP_

#include "FileSys.hpp"
#include "ZipArchive.h"


namespace cf
{
    namespace FileSys
    {
        /// This class implements file systems that are ZIP archives, employing the ZipArchive library by Artpol Software.
        /// The ZipArchive library supports AES encryption in accordance with the WinZip format,
        /// but it cannot be redistributed with the Cafu source code due to its licensing conditions.
        /// Note however that employing AES encryption for Cafu asset files is not useful anyway,
        /// as any determined hacker could reverse-engineer the Cafu executable or access e.g. the texture
        /// images by a modified graphics driver anyway.
        /// Therefore, we now use the other implementation FileSystemZipArchiveGST by Gilles Vollant now,
        /// which is under the liberal zlib license with which redistribution is no problem.
        class FileSystemZipArchiveAST : public FileSystemT
        {
            public:

            /// Constructor.
            /// @throws FileSystemExceptionT   when there is a problem with initializing this file system.
            FileSystemZipArchiveAST(const std::string& ArchiveName_, const std::string& MountPoint_, const std::string& Password_="");

            /// Destructor. Destroys the file system.
            ~FileSystemZipArchiveAST();

            // Implement all the (pure) virtual methods of the FileSystemT class.
            InFileI* OpenRead(const std::string& FileName);


            private:

            const std::string                     ArchiveName;   ///< The name of the archive.
            CZipArchive                           Archive;       ///< The zip archive itself.
            const std::string                     MountPoint;    ///< The mount point of the archive contents.
            const std::string::size_type          MountPointLen; ///< The length of the MountPoint string.
            std::map<std::string, ZIP_INDEX_TYPE> NameToFileNr;  ///< Maps a file name to the related index in the zip archive.
        };
    }
}

#endif
