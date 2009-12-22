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
{
}


BufferManagerT::~BufferManagerT()
{
    ReleaseAll();
}


BufferT* BufferManagerT::GetBuffer(const std::string& AudioFile, SoundShaderT::LoadTypeE LoadType, bool Is3DSound)
{
    // We need to differentiate between 2D and 3D sounds since they are created differently.
    std::string BufferName=AudioFile;

    if (Is3DSound) BufferName.append("3D");
    else           BufferName.append("2D");

    switch (LoadType)
    {
        case SoundShaderT::AUTO:
        {
            if (cf::String::EndsWith(AudioFile, ".wav"))
                return GetBuffer(AudioFile, SoundShaderT::STATIC, Is3DSound); // Always use static buffer for .wav files.

            cf::FileSys::InFileI* FileHandle=cf::FileSys::FileMan->OpenRead(AudioFile);

            if (FileHandle!=NULL && FileHandle->GetSize()>StreamFileSize)
            {
                cf::FileSys::FileMan->Close(FileHandle);
                return GetBuffer(AudioFile, SoundShaderT::STREAM, Is3DSound);
            }

            cf::FileSys::FileMan->Close(FileHandle);
            return GetBuffer(AudioFile, SoundShaderT::STATIC, Is3DSound); // Note: non existent files are handled in the StaticBufferT constructor.
        }

        case SoundShaderT::STATIC:

            // If audio file has not yet been loaded into a buffer.
            if (StaticBuffers.find(BufferName)==StaticBuffers.end())
                StaticBuffers[BufferName]=new StaticBufferT(AudioFile, Is3DSound);

            StaticBuffers.find(BufferName)->second->References++;

            return StaticBuffers.find(BufferName)->second;


        case SoundShaderT::STREAM:
        {
            // If audio file has already been loaded into a static buffer we don't need to create a
            // streaming buffer anymore and just use the static one.
            if (StaticBuffers.find(BufferName)!=StaticBuffers.end())
                return StaticBuffers.find(BufferName)->second;

            StreamingBufferT* NewBuffer=new StreamingBufferT(AudioFile, Is3DSound);
            NewBuffer->References++;

            StreamingBuffers.PushBack(NewBuffer);

            return NewBuffer;
        }


        case SoundShaderT::COMPRESSED:
            std::cout << "OpenAL: COMPRESSED creation is not yet supported, switching to AUTO\n";
            return GetBuffer(AudioFile, SoundShaderT::AUTO, Is3DSound);
    }

    return NULL;
}


void BufferManagerT::CleanUp()
{
    for (std::map<std::string, StaticBufferT*>::iterator It=StaticBuffers.begin(); It!=StaticBuffers.end();)
    {
        if (It->second->References<1)
        {
            // Store current position and increment BEFORE erasing the iterator position.
            std::map<std::string, StaticBufferT*>::iterator Current=It;
            It++;

            delete Current->second;
            StaticBuffers.erase(Current);
            continue;
        }

        It++;
    }

    for (unsigned long i=0; i<StreamingBuffers.Size();)
    {
        if (StreamingBuffers[i]->References<1)
        {
            delete StreamingBuffers[i];
            StreamingBuffers.RemoveAtAndKeepOrder(i);
            continue;
        }

        i++;
    }
}


void BufferManagerT::ReleaseBuffer(BufferT* Buffer)
{
    Buffer->References--;

    if (Buffer->References<1)
        CleanUp();
}


void BufferManagerT::ReleaseAll()
{
    for (std::map<std::string, StaticBufferT*>::iterator It=StaticBuffers.begin(); It!=StaticBuffers.end(); It++)
        delete It->second;

    StaticBuffers.clear();

    for (unsigned long i=0; i<StreamingBuffers.Size(); i++)
        delete StreamingBuffers[i];

    StreamingBuffers.Clear();
}
