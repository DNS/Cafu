/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "SoundStream.hpp"
#include "MP3Stream.hpp"
#include "OggVorbisStream.hpp"
#include "../SoundSysOpenAL/CaptureStream.hpp"

#include "String.hpp"


SoundStreamT* SoundStreamT::Create(const std::string& ResName)
{
    if (cf::String::EndsWith(ResName, ".mp3")) return new MP3StreamT(ResName);
    if (cf::String::EndsWith(ResName, ".ogg")) return new OggVorbisStreamT(ResName);

    return new CaptureStreamT(ResName);
}
