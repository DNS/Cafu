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

#ifndef CAFU_FILESYS_FILE_MEMORY_HPP_INCLUDED
#define CAFU_FILESYS_FILE_MEMORY_HPP_INCLUDED

#include "File.hpp"
#include "Templates/Array.hpp"


namespace cf
{
    namespace FileSys
    {
        class MemoryInFileT : public InFileI
        {
            public:

            /// Constructor. Creates an empty memory file for reading.
            /// Note that because the file is empty, the code that created the file should use the GetBuffer() method to set the initial contents.
            MemoryInFileT(const std::string& FileName_);

            /// Returns a reference to the buffer that keeps the contents of this file.
            ArrayT<char>& GetBuffer() { return Buffer; }

            /// Returns a const reference to the buffer that keeps the contents of this file.
            const ArrayT<char>& GetBuffer() const { return Buffer; }


            // Implement all the (pure) virtual methods of the FileI and InFileI interfaces.
            bool               IsOpen() const;
            const std::string& GetBaseName() const;
            const std::string& GetFullName() const;
            uint64_t           GetPos() const;
            bool               Seek(int32_t Offset, SeekFromT SeekFrom);
            uint32_t           Read(char* Buffer, uint32_t Size);
            uint64_t           GetSize() const;


            private:

            std::string   FileName;     ///< The name of the file.
            ArrayT<char>  Buffer;       ///< The contents of the file.
            unsigned long ReadPos;      ///< The position from which the next read operation starts.
        };


        /* class MemoryOutFileT : public OutFileI
        {
            public:

            /// Constructor.
            MemoryOutFileT(const std::string& FileName_);

            // Implement all the (pure) virtual methods of the FileI and OutFileI interfaces.
            bool               IsOpen() const;
            const std::string& GetBaseName() const;
            const std::string& GetFullName() const;
            uint64_t           GetPos() const;
            bool               Seek(int32_t Offset, SeekFromT SeekFrom);
            void               Write(const char* Buffer, uint32_t Size);


            private:

            // ...
        }; */
    }
}

#endif
