/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_SOUNDSYS_OPENAL_INCL_HPP_INCLUDED
#define CAFU_SOUNDSYS_OPENAL_INCL_HPP_INCLUDED

// Include the OpenAL related header files like "alut" does:
// apparently the subdirectory is not standardized across the platforms yet.
#if defined(_WIN32)
#include <alc.h>
#include <al.h>
#elif defined(__APPLE__)
#include <OpenAL/alc.h>
#include <OpenAL/al.h>
#else
#include <AL/alc.h>
#include <AL/al.h>
#include <AL/alext.h>
#endif

#include <string>


/// A helper function to tranlate OpenAL error codes into a string.
inline std::string TranslateErrorCode(ALenum ErrorCode)
{
    switch (ErrorCode)
    {
        case AL_INVALID_NAME:
            return "A bad name (ID) was passed to an OpenAL function.";

        case AL_INVALID_ENUM:
            return "An invalid enum value was passed to an OpenAL function.";

        case AL_INVALID_VALUE:
            return "An invalid value was passed to an OpenAL function.";

        case AL_INVALID_OPERATION:
            return "The requested operation is not valid.";

        case AL_OUT_OF_MEMORY:
            return "The requested operation resulted in OpenAL running out of memory.";
    }

    return "There is currently no error.";
}

#endif
