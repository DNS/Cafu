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

#ifndef _CF_FILESYS_FILE_LOCAL_HPP_
#define _CF_FILESYS_FILE_LOCAL_HPP_

#include <fstream>

#include "File.hpp"


namespace cf
{
    namespace FileSys
    {
        class LocalInFileT : public InFileI
        {
            public:

            /// Constructor.
            LocalInFileT(const std::string& BaseName_, const std::string& FullName_);

            // Implement all the (pure) virtual methods of the FileI and InFileI interfaces.
            bool               IsOpen() const;
            const std::string& GetBaseName() const;
            const std::string& GetFullName() const;
            unsigned long      GetPos() const;
            bool               Seek(int Offset, SeekFromT SeekFrom);
            unsigned long      Read(char* Buffer, unsigned long Size);
            unsigned long      GetSize() const;


            private:

            std::string           BaseName;     ///< The relative name of the file, agnostic of the local path.
            std::string           FullName;     ///< The BaseName prepended by the local path.
            mutable std::ifstream ifs;          ///< The input file stream.
        };


        class LocalOutFileT : public OutFileI
        {
            public:

            /// Constructor.
            LocalOutFileT(const std::string& BaseName_, const std::string& FullName_);

            // Implement all the (pure) virtual methods of the FileI and OutFileI interfaces.
            bool               IsOpen() const;
            const std::string& GetBaseName() const;
            const std::string& GetFullName() const;
            unsigned long      GetPos() const;
            bool               Seek(int Offset, SeekFromT SeekFrom);
            void               Write(const char* Buffer, unsigned long Size);


            private:

            std::string           BaseName;     ///< The relative name of the file, agnostic of the local path.
            std::string           FullName;     ///< The BaseName prepended by the local path.
            mutable std::ofstream ofs;          ///< The output file stream.
        };
    }
}

#endif
