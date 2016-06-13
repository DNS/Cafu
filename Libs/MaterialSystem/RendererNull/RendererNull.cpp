/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

/*************************/
/*** The Null Renderer ***/
/*************************/

#include "RendererImpl.hpp"
#include "TextureMapImpl.hpp"
#include "ConsoleCommands/Console.hpp"
#include "FileSys/FileMan.hpp"


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


cf::ConsoleI*          Console=NULL;
cf::FileSys::FileManI* cf::FileSys::FileMan=NULL;


// WARNING: When the signature of GetRenderer() is changed here (e.g. by adding more interface pointer parameters),
// grep all C++ source code files for "GetRenderer@", because the number of parameter bytes must be updated there!
DLL_EXPORT MatSys::RendererI* __stdcall GetRenderer(cf::ConsoleI* Console_, cf::FileSys::FileManI* FileMan_)
{
    Console             =Console_;
    cf::FileSys::FileMan=FileMan_;

    return &RendererImplT::GetInstance();
}


DLL_EXPORT MatSys::TextureMapManagerI* __stdcall GetTextureMapManager()
{
    return &TextureMapManagerImplT::Get();
}
