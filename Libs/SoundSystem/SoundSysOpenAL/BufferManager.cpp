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

#include "BufferManager.hpp"
#include "CaptureBuffer.hpp"
#include "StaticBuffer.hpp"
#include "StreamingBuffer.hpp"

#include "FileSys/FileMan.hpp"
#include "FileSys/File.hpp"
#include "String.hpp"

#include <iostream>


static const unsigned long StreamFileSize=50000; ///< Filesize (in bytes) at which GetBuffer uses streams instead of static files.


BufferManagerT* BufferManagerT::GetInstance()
{
    static BufferManagerT BufferManagerInstance;

    return &BufferManagerInstance;
}


BufferManagerT::BufferManagerT()
    : m_Buffers()
{
}


BufferManagerT::~BufferManagerT()
{
    ReleaseAll();
}


BufferT* BufferManagerT::GetBuffer(const std::string& AudioFile, SoundShaderT::LoadTypeE LoadType, bool Is3DSound)
{
    if (AudioFile.find("capture")==0 && AudioFile.length()>8)
    {
        const unsigned int WantedDeviceNum=atoi(AudioFile.substr(8).c_str());

        const char*  DeviceNames=alcGetString(NULL, ALC_CAPTURE_DEVICE_SPECIFIER);
        unsigned int Offset     =0;
        unsigned int DeviceNum  =0;
        std::string  DeviceName ="";

        while (DeviceNames[Offset]!='\0' && DeviceNum<=WantedDeviceNum)
        {
            DeviceName=&DeviceNames[Offset];

            Offset+=DeviceName.length()+1;   // Jump to next device name.
            DeviceNum++;
        }

        BufferT* Buf=new CaptureBufferT(DeviceName, Is3DSound);

        Buf->References++;
        m_Buffers.PushBack(Buf);

        return Buf;
    }

    switch (LoadType)
    {
        case SoundShaderT::AUTO:
        {
            if (cf::String::EndsWith(AudioFile, ".wav"))
                return GetBuffer(AudioFile, SoundShaderT::STATIC, Is3DSound);   // Always use a static buffer for .wav files.

            cf::FileSys::InFileI* FileHandle=cf::FileSys::FileMan->OpenRead(AudioFile);
            if (FileHandle!=NULL && FileHandle->GetSize()>StreamFileSize)
            {
                cf::FileSys::FileMan->Close(FileHandle);
                return GetBuffer(AudioFile, SoundShaderT::STREAM, Is3DSound);
            }
            cf::FileSys::FileMan->Close(FileHandle);

            return GetBuffer(AudioFile, SoundShaderT::STATIC, Is3DSound);   // Non-existent files are handled in the StaticBufferT constructor.
        }

        case SoundShaderT::STATIC:  // Intentional fall-through.
        case SoundShaderT::STREAM:
        {
            // If we have a static buffer with the same name and same number of channels already,
            // just increase its reference count and return it.
            for (unsigned long BufNr=0; BufNr<m_Buffers.Size(); BufNr++)
            {
                BufferT* Buf=m_Buffers[BufNr];

                if (!Buf->IsStream() && Buf->GetName()==AudioFile && Buf->Is3D()==Is3DSound)
                {
                    Buf->References++;
                    return Buf;
                }
            }

            BufferT* NewBuffer=NULL;

            if (LoadType==SoundShaderT::STATIC) NewBuffer=new StaticBufferT(AudioFile, Is3DSound);
                                           else NewBuffer=new StreamingBufferT(AudioFile, Is3DSound);

            NewBuffer->References++;
            m_Buffers.PushBack(NewBuffer);

            return NewBuffer;
        }

        case SoundShaderT::COMPRESSED:
        {
            std::cout << "OpenAL: COMPRESSED creation is not yet supported, switching to AUTO\n";
            return GetBuffer(AudioFile, SoundShaderT::AUTO, Is3DSound);
        }
    }

    return NULL;
}


void BufferManagerT::ReleaseBuffer(BufferT* Buffer)
{
    Buffer->References--;

    if (Buffer->References<1)
        CleanUp();
}


void BufferManagerT::CleanUp()
{
    for (unsigned long BufNr=0; BufNr<m_Buffers.Size(); BufNr++)
    {
        if (m_Buffers[BufNr]->References<1)
        {
            delete m_Buffers[BufNr];
            m_Buffers.RemoveAt(BufNr);
            BufNr--;
        }
    }
}


void BufferManagerT::ReleaseAll()
{
    for (unsigned long BufNr=0; BufNr<m_Buffers.Size(); BufNr++)
        delete m_Buffers[BufNr];

    m_Buffers.Overwrite();
}
