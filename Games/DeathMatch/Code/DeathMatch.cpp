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

#include "GameImpl.hpp"
#include "Libs/LookupTables.hpp"
#include "../../BaseEntity.hpp"     // Just for the one-time init of the TypeInfoMan.

#include "ClipSys/CollisionModelMan.hpp"
#include "ConsoleCommands/ConsoleInterpreter.hpp"
#include "ConsoleCommands/ConVar.hpp"
#include "ConsoleCommands/ConFunc.hpp"
#include "ConsoleCommands/Console.hpp"
#include "FileSys/FileMan.hpp"
#include "GuiSys/GuiMan.hpp"
#include "MaterialSystem/MaterialManager.hpp"
#include "MaterialSystem/Renderer.hpp"
#include "MaterialSystem/TextureMap.hpp"
#include "TypeSys.hpp"
#include "SoundSystem/SoundShaderManager.hpp"


#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>

    // Specifying the DllMain() function is mandatory in order to have the compiler choose the proper startup code.
    // An alternative (when DllMain() is not needed) is to specify the compilers -bd command line option.
    int __stdcall DllMain(void* /*hInstance*/, unsigned long /*Reason*/, void* /*Reserved*/)
    {
        // Remember: When the same DLL is loaded more than once (multiple calls to LoadLibrary()),
        // only a reference counter is increased; all "global" data still exists only once, and the DLL_PROCESS_ATTACH
        // reason occurs only on the very first load and the DLL_PROCESS_DETACH reason only occurs on the very last unload.
        return TRUE;
    }

    // This is for creating a DLL that is later loaded either statically via an import lib, or dynamically via LoadLibrary().
    // If instead an "old-fashioned" static library (obj/lib) was desired, just remove the __declspec(dllexport), and have user code have
    //    #include "Game.hpp"
    //    extern "C" cf::GameSys::GameI* GetGame(... <signature as below>);
    // somewhere for declaration. If the DLL is to be used with an import lib, leave the __declspec(dllexport) in, but change
    // the above line to: extern "C" __declspec(dllimport) cf::GameSys::GameI* GetGame(... <signature as below>);
    // User code that loads this DLL via LoadLibrary() does only need to declare a proper function pointer for GetProcAddress().
    #define DLL_EXPORT extern "C" __declspec(dllexport)
#else
    #define DLL_EXPORT extern "C"
    #define __stdcall
#endif


class SoundSysI;


// Exported DLL Functions
// **********************


// This is the first function that is called by both the client and the server after they have loaded this DLL.
// Its purpose is to point us to the shared implementation of the relevant interfaces (the MatSys etc.),
// so that we can access the same implementation of the interfaces as the engine.
//
// The fact that DLLs that are loaded multiple times cause only a reference counter to be increased rather than separate copies
// of the DLL to be created (the global state exists only once), and the way how clients and servers change worlds (client deletes
// the old world first, then loads the new, server loads new world first and only then deletes the old one), and the fact that in
// a single Cafu.exe instance, the client only, the server only, or both can be running, means that a *single* instance of this
// DLL may live over several world changes of a client and server, because at least one of them keeps referring to it at all times.
//
// Therefore, it may happen that GetGame() is called *many* times, namely on each world change once by the server and once
// by the client. The parameters to this function however are always non-volatile, they don't change over multiple calls.
// In future implementations I'll possibly change this and load and init the DLL only once, even before the client or server gets instantiated.

#ifdef _WIN32
// Under Linux (where DLLs are shared objects), these all resolve to their counterparts in the main executable.
cf::GuiSys::GuiManI*   cf::GuiSys::GuiMan=NULL;     // Define the global GuiMan pointer instance -- see GuiMan.hpp for more details.
MaterialManagerI*      MaterialManager   =NULL;
cf::ConsoleI*          Console=NULL;
ConsoleInterpreterI*   ConsoleInterpreter=NULL;
cf::FileSys::FileManI* cf::FileSys::FileMan=NULL;
cf::ClipSys::CollModelManI* cf::ClipSys::CollModelMan=NULL;
SoundSysI*             SoundSystem=NULL;
SoundShaderManagerI*   SoundShaderManager=NULL;
#endif


// WARNING: When the signature of GetGame() is changed here (e.g. by adding more interface pointer parameters),
// grep all C++ source code files for "GetGame@", because the number of parameter bytes must be updated there!
DLL_EXPORT cf::GameSys::GameI* __stdcall GetGame(MatSys::RendererI* Renderer, MatSys::TextureMapManagerI* TexMapMan, MaterialManagerI* MatMan, cf::GuiSys::GuiManI* GuiMan_, cf::ConsoleI* Console_, ConsoleInterpreterI* ConInterpreter_, cf::ClipSys::CollModelManI* CollModelMan_, SoundSysI* SoundSystem_, SoundShaderManagerI* SoundShaderManager_)
{
#ifdef _WIN32
    // Under Linux (where DLLs are shared objects), these all resolve to their counterparts in the main executable.
    MatSys::Renderer         =Renderer;
    MatSys::TextureMapManager=TexMapMan;
    MaterialManager          =MatMan;
    cf::GuiSys::GuiMan       =GuiMan_;
    Console                  =Console_;
    ConsoleInterpreter       =ConInterpreter_;
    cf::ClipSys::CollModelMan=CollModelMan_;
    SoundSystem              =SoundSystem_;
    SoundShaderManager       =SoundShaderManager_;
 // cf::FileSys::FileMan     =TODO!!!;
#endif

    // This won't be possible here:
    //     cf::GameSys::GameWorld=GameWorld_;
    // because each entity needs an individual pointer (as is now)!
    // This is because entities may "live" in a client world or in a server world, and accordingly need pointers to respective, different implementations.
    // Moreover, during world changes, entities that exist in *different* worlds may shortly exist simultaneously - even more
    // need to provide each with a pointer to its appropriate engine interface.

    // Console->Print("Hello from GetGame()!\n");


    static bool ConVarsAreRegistered=false;

    if (!ConVarsAreRegistered)
    {
#ifdef _WIN32
        // Under Linux (where DLLs are shared objects), there is only one global "static list" instance for the executable and the shared objects
        // (unlike Windows DLLs, who have their own static list instance). Thus, calling the RegisterStaticList() functions here again is like
        // calling them again in the main executable.
        //
        // This (currently, December 2006) accesses a NULL pointer if called more than once...
        ConFuncT::RegisterStaticList();
        ConVarT ::RegisterStaticList();
#endif
        ConVarsAreRegistered=true;


        // More "random" init-once stuff follows...  TODO: Move this into GameI::Initialize() ??????
        LookupTables::Initialize();

        GetBaseEntTIM().Init();     // Mis-use the ConVarsAreRegistered var to init the TIM exactly once...
    }

    return &cf::GameSys::GameImplT::GetInstance();
}
