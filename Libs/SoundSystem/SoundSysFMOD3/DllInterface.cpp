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

#include "SoundSysImpl.hpp"


namespace cf { class ConsoleI; }
namespace cf { namespace FileSys { class FileManI; } }


#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>

    // Specifying the DllMain() function is mandatory in order to have the compiler choose the proper startup code.
    // An alternative (when DllMain() is not needed) is to specify the compilers -bd command line option.
    int __stdcall DllMain(void* /*hInstance*/, unsigned long /*Reason*/, void* /*Reserved*/)
    {
        return TRUE;
    }

    // This is for creating a DLL that is later loaded either statically via an import lib, or dynamically via LoadLibrary().
    // If instead an "old-fashioned" static library (obj/lib) was desired, just remove the __declspec(dllexport), and have user code have
    //    #include "Renderer.hpp"
    //    extern "C" RendererI* GetRendererImpl();
    // somewhere for declaration. If the DLL is to be used with an import lib, leave the __declspec(dllexport) in, but change
    // the above line to: extern "C" __declspec(dllimport) RendererI* GetRendererImpl();
    // User code that loads this DLL via LoadLibrary() does only need to declare a proper function pointer for GetProcAddress().
    #define DLL_EXPORT extern "C" __declspec(dllexport)
#else
    #define DLL_EXPORT extern "C"
    #define __stdcall
#endif


cf::ConsoleI* Console=NULL;

namespace cf
{
    namespace FileSys
    {
        FileManI* FileMan=NULL;
    }
}


// WARNING: When the signature of GetSoundSys() is changed here (e.g. by adding more interface pointer parameters),
// grep all C++ source code files for "GetSoundSys@", because the number of parameter bytes must be updated there!
DLL_EXPORT SoundSysI* __stdcall GetSoundSys(cf::ConsoleI* Console_, cf::FileSys::FileManI* FileMan_)
{
    Console             =Console_;
    cf::FileSys::FileMan=FileMan_;

    return &SoundSysImplT::GetInstance();
}
