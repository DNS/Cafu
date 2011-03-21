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

#ifndef _CF_FILESYS_FILE_HPP_
#define _CF_FILESYS_FILE_HPP_

#include <string>

#if defined(_WIN32) && _MSC_VER<1600
#include "pstdint.h"            // Paul Hsieh's portable implementation of the stdint.h header.
#else
#include <stdint.h>
#endif


namespace cf
{
    namespace FileSys
    {
        class FileI
        {
            public:

            /// The values of this enumeration define from where the seek operation applies the offset.
            enum SeekFromT { FROM_BEGINNING, FROM_CURRENT_POS, FROM_END };


            /// The virtual destructor, so that derived classes can safely be deleted via a FileI (base class) pointer.
            virtual ~FileI() { }

            /// Returns whether the file has successfully been opened and is still open.
            virtual bool IsOpen() const=0;

            /// Returns the base name of this file. The base name is relative to and agnostic of the file system this file is in.
            virtual const std::string& GetBaseName() const=0;

            /// Returns the full name of this file. The full name is the base name prepended by the file system specific path/URL.
            /// TODO: Have a GetFileSys() method instead???
            virtual const std::string& GetFullName() const=0;

            /// Returns the current read/write position in the file.
            virtual uint64_t GetPos() const=0;

            /// Modifies the position of the read/write pointer in the file.
            /// @param Offset     How much to move the pointer.
            /// @param SeekFrom   Defines from where the offset is applied, see SeekFromT for possible values.
            /// @returns true if the seek operation was successful, false otherwise.
            virtual bool Seek(int32_t Offset, SeekFromT SeekFrom)=0;
        };


        class InFileI : public FileI
        {
            public:

            /// Reads Size bytes into the Buffer.
            /// @returns How many bytes have actually been read.
            virtual uint32_t Read(char* Buffer, uint32_t Size)=0;

            /// Returns the size of the file.
            virtual uint64_t GetSize() const=0;
        };


        class OutFileI : public FileI
        {
            public:

            /// Writes the contents of Buffer, which has size Size, into the file.
            virtual void Write(const char* Buffer, uint32_t Size)=0;
        };
    }
}

#endif
