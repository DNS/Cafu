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

#include "File_local.hpp"

using namespace cf::FileSys;


LocalInFileT::LocalInFileT(const std::string& BaseName_, const std::string& FullName_)
    : BaseName(BaseName_),
      FullName(FullName_),
      ifs(FullName.c_str(), std::ios::in | std::ios::binary)
{
}


bool LocalInFileT::IsOpen() const
{
#if __linux__
    // See post "Should basic_*stream::is_open() be const?" in the mail archive of the libstdc++@gcc.gnu.org
    // mailing list for the libstdc++ project at http://gcc.gnu.org/ml/libstdc++/2004-08/threads.html#00108
    // for why we need to cast away const-ness here...
    return const_cast<std::ifstream&>(ifs).is_open();
#else
    // Seems to work just fine everywhere else...
    return ifs.is_open();
#endif
}


const std::string& LocalInFileT::GetBaseName() const
{
    return BaseName;
}


const std::string& LocalInFileT::GetFullName() const
{
    return FullName;
}


uint64_t LocalInFileT::GetPos() const
{
    return ifs.tellg();
}


bool LocalInFileT::Seek(int32_t Offset, SeekFromT SeekFrom)
{
    // Reason for call to ifs.clear(): If the end of a filestream is reached the eofbit is set and prevents further calls
    // to seek (e.g. to rewind the stream) to succeed. To make sure a call to seek doesn't fail becuase of the eofbit, we
    // reset the state of the stream here.
    ifs.clear();

    switch (SeekFrom)
    {
        case FROM_BEGINNING:   ifs.seekg(Offset, std::ios::beg); break;
        case FROM_CURRENT_POS: ifs.seekg(Offset, std::ios::cur); break;
        case FROM_END:         ifs.seekg(Offset, std::ios::end); break;
    }

    return !ifs.fail();
}


uint32_t LocalInFileT::Read(char* Buffer, uint32_t Size)
{
    ifs.read(Buffer, Size);

    return uint32_t(ifs.gcount());
}


uint64_t LocalInFileT::GetSize() const
{
    const std::streamoff CurPos=ifs.tellg();
    ifs.seekg(0, std::ios::end);
    const std::streamoff EndPos=ifs.tellg();
    ifs.seekg(CurPos, std::ios::beg);

    return EndPos;
}




LocalOutFileT::LocalOutFileT(const std::string& BaseName_, const std::string& FullName_)
    : BaseName(BaseName_),
      FullName(FullName_),
      ofs(FullName.c_str(), std::ios::out | std::ios::binary)
{
}


bool LocalOutFileT::IsOpen() const
{
#if __linux__
    // See post "Should basic_*stream::is_open() be const?" in the mail archive of the libstdc++@gcc.gnu.org
    // mailing list for the libstdc++ project at http://gcc.gnu.org/ml/libstdc++/2004-08/threads.html#00108
    // for why we need to cast away const-ness here...
    return const_cast<std::ofstream&>(ofs).is_open();
#else
    // Seems to work just fine everywhere else...
    return ofs.is_open();
#endif
}


const std::string& LocalOutFileT::GetBaseName() const
{
    return BaseName;
}


const std::string& LocalOutFileT::GetFullName() const
{
    return FullName;
}


uint64_t LocalOutFileT::GetPos() const
{
    return ofs.tellp();
}


bool LocalOutFileT::Seek(int32_t Offset, SeekFromT SeekFrom)
{
    switch (SeekFrom)
    {
        case FROM_BEGINNING:   ofs.seekp(Offset, std::ios::beg); break;
        case FROM_CURRENT_POS: ofs.seekp(Offset, std::ios::cur); break;
        case FROM_END:         ofs.seekp(Offset, std::ios::end); break;
    }

    return !ofs.fail();
}


void LocalOutFileT::Write(const char* Buffer, uint32_t Size)
{
    ofs.write(Buffer, Size);
}
