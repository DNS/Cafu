/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "BufferManager.hpp"
#include "StaticBuffer.hpp"
#include "StreamingBuffer.hpp"

#include "FileSys/FileMan.hpp"
#include "FileSys/File.hpp"
#include "String.hpp"

#include <iostream>
#include <stdexcept>


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


BufferT* BufferManagerT::GetBuffer(const std::string& ResName, bool ForceMono, SoundShaderT::LoadTypeE LoadType)
{
    if (ResName.find("capture")==0 && ResName.length()>8)
    {
        const unsigned int WantedDeviceNum=atoi(ResName.substr(8).c_str());

        const char*  DeviceNames=alcGetString(NULL, ALC_CAPTURE_DEVICE_SPECIFIER);
        size_t       Offset     =0;
        unsigned int DeviceNum  =0;
        std::string  DeviceName ="";

        while (DeviceNames[Offset]!='\0' && DeviceNum<=WantedDeviceNum)
        {
            DeviceName=&DeviceNames[Offset];

            Offset+=DeviceName.length()+1;  // Jump to next device name.
            DeviceNum++;
        }

        BufferT* Buf=new StreamingBufferT(DeviceName, ForceMono);

        Buf->References++;
        m_Buffers.PushBack(Buf);

        return Buf;
    }

    switch (LoadType)
    {
        case SoundShaderT::AUTO:
        {
            if (cf::String::EndsWith(ResName, ".wav"))
                return GetBuffer(ResName, ForceMono, SoundShaderT::STATIC);   // Always use a static buffer for .wav files.

            cf::FileSys::InFileI* FileHandle=cf::FileSys::FileMan->OpenRead(ResName);
            if (FileHandle!=NULL && FileHandle->GetSize()>StreamFileSize)
            {
                cf::FileSys::FileMan->Close(FileHandle);
                return GetBuffer(ResName, ForceMono, SoundShaderT::STREAM);
            }
            cf::FileSys::FileMan->Close(FileHandle);

            return GetBuffer(ResName, ForceMono, SoundShaderT::STATIC);       // Non-existent files are handled in the StaticBufferT constructor.
        }

        case SoundShaderT::STATIC:  // Intentional fall-through.
        case SoundShaderT::STREAM:
        {
            // If we have a static buffer with the same resource name and a suitable number of channels already,
            // just increase its reference count and return it.
            for (unsigned long BufNr=0; BufNr<m_Buffers.Size(); BufNr++)
            {
                BufferT* Buf=m_Buffers[BufNr];

                if (Buf->CanShare() && Buf->GetName()==ResName && (!ForceMono || Buf->GetChannels()==1))
                {
                    Buf->References++;
                    return Buf;
                }
            }

            BufferT* NewBuffer=NULL;

            if (LoadType==SoundShaderT::STATIC) NewBuffer=new StaticBufferT(ResName, ForceMono);
                                           else NewBuffer=new StreamingBufferT(ResName, ForceMono);

            NewBuffer->References++;
            m_Buffers.PushBack(NewBuffer);

            return NewBuffer;
        }

        case SoundShaderT::COMPRESSED:
        {
            std::cout << "OpenAL: COMPRESSED creation is not yet supported, switching to AUTO\n";
            return GetBuffer(ResName, ForceMono, SoundShaderT::AUTO);
        }
    }

    throw std::runtime_error("Unknown LoadType.");
    return NULL;
}


void BufferManagerT::ReleaseBuffer(BufferT* Buffer)
{
    Buffer->References--;

    if (Buffer->References<1)
        CleanUp();
}


void BufferManagerT::UpdateAll()
{
    for (unsigned long BufNr=0; BufNr<m_Buffers.Size(); BufNr++)
        m_Buffers[BufNr]->Update();
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
