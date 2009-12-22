/*
=================================================================================
This file is part of Cafu, the open-source game and graphics engine for
multiplayer, cross-platform, real-time 3D action.
$Id$

Copyright (C) 2002-2010 Carsten Fuchs Software.

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

#ifndef _CF_FILESYS_FILESYSTEM_HPP_
#define _CF_FILESYS_FILESYSTEM_HPP_

#include <string>


namespace cf
{
    namespace FileSys
    {
        class InFileI;


        /// A class for exceptions of the FileSystemT class.
        /// This is mostly used when new FileSystemT objects are to be created, but the creation is not possible,
        /// for example when a zip archive file cannot be opened, a local path doesn't exist, etc.
        class FileSystemExceptionT : public std::exception
        {
            public:

            enum ReasonT
            {
                Generic,
                InitFailure
            };


            /// The constructor.
            FileSystemExceptionT(ReasonT R=Generic) : Reason(R) { }

            /// The reason for the exception.
            ReasonT Reason;
        };


        class FileSystemT
        {
            public:

            /// The virtual destructor, so that derived classes can safely be deleted via a FileSystemT (base class) pointer.
            virtual ~FileSystemT() { }

            /// Opens the file with the given name for reading.
            /// @param FileName   The name of the file to be opened within this file system.
            /// @returns The pointer to the file or NULL if the file could not be opened.
            virtual InFileI* OpenRead(const std::string& FileName)=0;
        };
    }
}

#endif
