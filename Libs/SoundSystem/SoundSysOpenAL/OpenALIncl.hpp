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

#ifndef _SOUNDSYS_OPENAL_INCL_HPP_
#define _SOUNDSYS_OPENAL_INCL_HPP_

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
