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

/**************************************/
/***                                ***/
/***          Cafu Engine           ***/
/***                                ***/
/*** Dass ich erkenne, was die Welt ***/
/*** im Innersten zusammenhält.     ***/
/*** (Faust)                        ***/
/***                                ***/
/**************************************/

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <conio.h>
#include <windows.h>                // Header File für Win32
#include <direct.h>
#else
#include <time.h>
#include <errno.h>
#include <dlfcn.h>
#define WSAEWOULDBLOCK EWOULDBLOCK
#define WSAEMSGSIZE    EMSGSIZE
#define getch getchar
#define __stdcall
#define GetProcAddress dlsym
#define FreeLibrary dlclose
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <fstream>
#include <iostream>

extern "C"
{
    #include <lua.h>
 // #include <lualib.h>
    #include <lauxlib.h>
}

#include "Bitmap/Bitmap.hpp"
#include "Util/Util.hpp"
#include "MaterialSystem/Material.hpp"
#include "MaterialSystem/MaterialManagerImpl.hpp"
#include "MaterialSystem/Renderer.hpp"
#include "MaterialSystem/TextureMap.hpp"
#include "SoundSystem/Sound.hpp"    // For playing background music from the console.
#include "SoundSystem/SoundSys.hpp"
#include "SoundSystem/SoundShaderManagerImpl.hpp"
#include "OpenGL/OpenGLWindow.hpp"  // OpenGL Window
#include "ClipSys/CollisionModelMan_impl.hpp"
#include "FileSys/FileMan.hpp"
#include "FileSys/FileManImpl.hpp"
#include "FileSys/File.hpp"
#include "GuiSys/GuiMan.hpp"
#include "GuiSys/GuiManImpl.hpp"
#include "GuiSys/Gui.hpp"
#include "GuiSys/GuiImpl.hpp"
#include "GuiSys/ConsoleByWindow.hpp"
#include "GuiSys/Window.hpp"    // For the one-time init of the Window TypeInfoMan.
#include "PlatformAux.hpp"
#include "ConsoleCommands/ConsoleInterpreter.hpp"
#include "ConsoleCommands/ConsoleInterpreterImpl.hpp"
#include "ConsoleCommands/ConVar.hpp"
#include "ConsoleCommands/ConFunc.hpp"
#include "ConsoleCommands/Console.hpp"
#include "ConsoleCommands/ConsoleStdout.hpp"
#include "TypeSys.hpp"          // For the one-time init of the Window TypeInfoMan.
#include "../Games/Game.hpp"

#include "Network/Network.hpp"      // Header File für die Network-Library
#ifdef CA3DE_DEDICATED_SERVER
SingleOpenGLWindowT* SingleOpenGLWindow=NULL;   // Dedicated servers don't link OpenGLWindow.cpp in.
#else
#include "Client/Client.hpp"        // Header File für die Client-Library
#include "Client/ClientWindow.hpp"
#endif
#include "Server/Server.hpp"        // Header File für die Server-Library


// These convars replace the legacy "Ca3DE Options" that were kept in the Ca3DE_OptionsT struct, defined in "Dialog.cpp".
// This is done so that these options are also available and accessible via the "config.lua" file.
const int CLIENT_RUNMODE=1;
const int SERVER_RUNMODE=2;

#ifdef CA3DE_DEDICATED_SERVER
    const int DefaultRunMode=SERVER_RUNMODE;
#else
    const int DefaultRunMode=CLIENT_RUNMODE | SERVER_RUNMODE;
#endif

/*static*/ ConVarT Options_RunMode                 ("dlg_RunMode",     DefaultRunMode, ConVarT::FLAG_MAIN_EXE,                            "1 is client-only, 2 is server-only, 3 is both.", 1, 3);
/*static*/ ConVarT Options_DeathMatchPlayerName    ("dlg_dmPlayerName",      "Player", ConVarT::FLAG_MAIN_EXE | ConVarT::FLAG_PERSISTENT, "Player name for the DeathMatch game.");
/*static*/ ConVarT Options_DeathMatchModelName     ("dlg_dmModelName",        "James", ConVarT::FLAG_MAIN_EXE | ConVarT::FLAG_PERSISTENT, "Name of the model to use for the DeathMatch game.");
/*static*/ ConVarT Options_ClientFullScreen        ("dlg_clFullScreen",          true, ConVarT::FLAG_MAIN_EXE | ConVarT::FLAG_PERSISTENT, "If the clients runs in full-screen or windowed mode.");
/*static*/ ConVarT Options_ClientWindowSizeX       ("dlg_clWindowSizeX",         1024, ConVarT::FLAG_MAIN_EXE | ConVarT::FLAG_PERSISTENT, "The size of the client window in X direction.");
/*static*/ ConVarT Options_ClientWindowSizeY       ("dlg_clWindowSizeY",          768, ConVarT::FLAG_MAIN_EXE | ConVarT::FLAG_PERSISTENT, "The size of the client window in Y direction.");
/*static*/ ConVarT Options_ClientDesiredRenderer   ("dlg_clDesiredRenderer",       "", ConVarT::FLAG_MAIN_EXE,                            "If set, overrides the auto-selection of the renderer and determines which renderer is to be used instead.");
/*static*/ ConVarT Options_ClientDesiredSoundSystem("dlg_clDesiredSoundSystem",    "", ConVarT::FLAG_MAIN_EXE,                            "If set, overrides the auto-selection of the sound system and determines which sound system is to be used instead.");
/*static*/ ConVarT Options_ClientPortNr            ("dlg_clPortNr",             33000, ConVarT::FLAG_MAIN_EXE,                            "The client port number.", 0, 0xFFFF);
/*static*/ ConVarT Options_ClientRemoteName        ("dlg_clRemoteName", "192.168.1.1", ConVarT::FLAG_MAIN_EXE | ConVarT::FLAG_PERSISTENT, "Name or IP of the server the client connects to.");
/*static*/ ConVarT Options_ClientRemotePortNr      ("dlg_clRemotePort",         30000, ConVarT::FLAG_MAIN_EXE | ConVarT::FLAG_PERSISTENT, "Port number of the remote server.", 0, 0xFFFF);
/*static*/ ConVarT Options_ClientDisplayBPP        ("dlg_clDisplayBPP",             1, ConVarT::FLAG_MAIN_EXE,                            "Use 0 for 16 BPP, 1 for 32 BPP.", 0, 1);
/*static*/ ConVarT Options_ClientTextureDetail     ("dlg_clTextureDetail",          0, ConVarT::FLAG_MAIN_EXE | ConVarT::FLAG_PERSISTENT, "0 high detail, 1 medium detail, 2 low detail", 0, 2);
/*static*/ ConVarT Options_ServerGameName          ("dlg_svGameName",    "DeathMatch", ConVarT::FLAG_MAIN_EXE,                            "Name of the game (MOD) to load.");
/*static*/ ConVarT Options_ServerWorldName         ("dlg_svWorldName",     "TechDemo", ConVarT::FLAG_MAIN_EXE,                            "Name of the world to load.");
/*static*/ ConVarT Options_ServerPortNr            ("dlg_svPortNr",             30000, ConVarT::FLAG_MAIN_EXE,                            "Server port number.", 0, 0xFFFF);
#include "Dialog1.h"
#include "Dialog1.cpp"


#include "NetConst.hpp"             // Network-Protocol Constants


static cf::ConsoleStdoutT ConsoleStdout;
cf::ConsoleI* Console=&ConsoleStdout;

// Init the Collision Model Manager.
// Note that it is not really necessary to init it after the FileSys, in fact the time of CMM init is pretty flexible.
// Also note that instantiating the CCM as a "global" like all the other system (Console, FileSys, GuiMan, etc.)
// seems pretty weird at first (I always thought of it as a member of the world, near the entities).
// But if you think about it, having the CCM "global" makes a lot of sense:
// Now the entire subsequent application can go crazy with loading and unloading collision models,
// (e.g. client loads the same world as the server, server loads another world before freeing the previous one, etc.),
// and they are all optimally served, with no resource duplication ever.
static cf::ClipSys::CollModelManImplT CCM;
cf::ClipSys::CollModelManI* cf::ClipSys::CollModelMan=&CCM;


cf::FileSys::FileManI* cf::FileSys::FileMan=NULL;   // Define the global FileMan pointer instance -- see FileMan.hpp for more details.
cf::GuiSys::GuiManI*   cf::GuiSys::GuiMan  =NULL;   // Define the global GuiMan  pointer instance -- see GuiMan.hpp  for more details.

// Provide a definition for Game, the global (Ca3DE.exe-wide) pointer to a GameI implementation.
// The implementation in turn will be provided by a dynamically loaded game DLL.
// This is analogous to the Material System, where Renderer DLLs provide renderer and texture manager implementations.
cf::GameSys::GameI* cf::GameSys::Game=NULL;

// Provide a definition for the sound system.
SoundSysI* SoundSystem=NULL;


void HandleButton_OK();
void HandleButton_Configure();
void HandleButton_GetServerInfo();
void HandleButton_Ping();
void HandleButton_SendNOP();


static std::string Usage()
{
    std::string UsgStr;

    UsgStr+=cf::va("Ca3DE command line overview:\n");
    UsgStr+=cf::va("Default values are enclosed in [brackets].\n");
    UsgStr+=cf::va("  -help            Print this message, and quit.\n");
    UsgStr+=cf::va("  -con             Runs the given commands in the console.\n");
    UsgStr+=cf::va("                   -con \"x=3;\" is equivalent to appending x=3; to the end of the config.lua file.\n");
    UsgStr+=cf::va("  -svGame          Name of the MOD / game the server should run [%s]. Case sensitive!\n", Options_ServerGameName.GetValueString().c_str());
    UsgStr+=cf::va("  -svWorld         Name of the world the server should run [%s]. Case sensitive!\n", Options_ServerWorldName.GetValueString().c_str());
    UsgStr+=cf::va("  -svPort          Server port number [%i].\n", Options_ServerPortNr.GetValueInt());
#ifndef CA3DE_DEDICATED_SERVER
    UsgStr+=cf::va("  -showDialog      Show the obsolete Options dialog.\n");
    UsgStr+=cf::va("  -runMode         1 is client-only, 2 is server-only, 3 is both [%i].\n", Options_RunMode.GetValueInt());
    UsgStr+=cf::va("  -noFS            Abbreviation for -clNoFullscreen.\n");
    UsgStr+=cf::va("  -clNoFullscreen  Don't run full-screen, but rather in a window.\n");
    UsgStr+=cf::va("  -clPort          Client port number [%i].\n", Options_ClientPortNr.GetValueInt());
    UsgStr+=cf::va("  -clRemoteName    Name or IP of the server we connect to [%s].\n", Options_ClientRemoteName.GetValueString().c_str());
    UsgStr+=cf::va("  -clRemotePort    Port number of the remote server [%i].\n", Options_ClientRemotePortNr.GetValueInt());
    UsgStr+=cf::va("  -clTexDetail     0 high detail, 1 medium detail, 2 low detail [%i].\n", Options_ClientTextureDetail.GetValueInt());
    UsgStr+=cf::va("  -clWinSizeX      If not full-screen, this is the width  of the window [%i].\n", Options_ClientWindowSizeX.GetValueInt());
    UsgStr+=cf::va("  -clWinSizeY      If not full-screen, this is the height of the window [%i].\n", Options_ClientWindowSizeY.GetValueInt());
    UsgStr+=cf::va("  -clRenderer      Override the auto-selection of the best available renderer (e.g. \"RendererOpenGL12\") [auto].\n");
    UsgStr+=cf::va("  -clPlayerName    Player name [%s].\n", Options_DeathMatchPlayerName.GetValueString().c_str());
    UsgStr+=cf::va("  -clModelName     Name of the players model [%s].\n", Options_DeathMatchModelName.GetValueString().c_str());
#endif
    UsgStr+=cf::va("\n");
    UsgStr+=cf::va("It is also legal to OMIT the \"-svWorld\" keyword for specifying a world name!\n");
    UsgStr+=cf::va("That is, \"Ca3DE MyWorld\" instead of \"Ca3DE -svWorld MyWorld\" is fine.\n");

    return UsgStr;
}


static bool ParseCommandLine(const int ArgC, char* ArgV[], bool& SkipDialog)
{
    bool SvWorldNameHasBeenGiven=false;

    for (int ArgNr=1; ArgNr<ArgC; ArgNr++)
    {
             if (_stricmp(ArgV[ArgNr], "-help"          )==0) return false;
        else if (_stricmp(ArgV[ArgNr], ""               )==0) { }    // The argument is "", the empty string. This can happen under Linux, when Ca3DE is called via wxExecute() with white-space trailing the command string.
        else if (_stricmp(ArgV[ArgNr], "-con"           )==0) { ArgNr++; if (ArgNr>=ArgC) return false; ConsoleInterpreter->RunCommand(ArgV[ArgNr]); }
        else if (_stricmp(ArgV[ArgNr], "-svGame"        )==0) { ArgNr++; if (ArgNr>=ArgC) return false; Options_ServerGameName=std::string(ArgV[ArgNr]); }
        else if (_stricmp(ArgV[ArgNr], "-svWorld"       )==0) { ArgNr++; if (ArgNr>=ArgC) return false; Options_ServerWorldName=std::string(ArgV[ArgNr]); SvWorldNameHasBeenGiven=true; }
        else if (_stricmp(ArgV[ArgNr], "-svPort"        )==0) { ArgNr++; if (ArgNr>=ArgC) return false; Options_ServerPortNr=atoi(ArgV[ArgNr]); }
#ifndef CA3DE_DEDICATED_SERVER
        else if (_stricmp(ArgV[ArgNr], "-showDialog"    )==0) SkipDialog=false;
        else if (_stricmp(ArgV[ArgNr], "-runMode"       )==0) { ArgNr++; if (ArgNr>=ArgC) return false; Options_RunMode=((atoi(ArgV[ArgNr])-1) % 3)+1; }
        else if (_stricmp(ArgV[ArgNr], "-clNoFullscreen")==0) Options_ClientFullScreen=false;
        else if (_stricmp(ArgV[ArgNr], "-noFS"          )==0) Options_ClientFullScreen=false;
        else if (_stricmp(ArgV[ArgNr], "-clPort"        )==0) { ArgNr++; if (ArgNr>=ArgC) return false; Options_ClientPortNr=atoi(ArgV[ArgNr]); }
        else if (_stricmp(ArgV[ArgNr], "-clRemoteName"  )==0) { ArgNr++; if (ArgNr>=ArgC) return false; Options_ClientRemoteName=std::string(ArgV[ArgNr]); }
        else if (_stricmp(ArgV[ArgNr], "-clRemotePort"  )==0) { ArgNr++; if (ArgNr>=ArgC) return false; Options_ClientRemotePortNr=atoi(ArgV[ArgNr]); }
        else if (_stricmp(ArgV[ArgNr], "-clTexDetail"   )==0) { ArgNr++; if (ArgNr>=ArgC) return false; Options_ClientTextureDetail=atoi(ArgV[ArgNr]) % 3; }
        else if (_stricmp(ArgV[ArgNr], "-clWinSizeX"    )==0) { ArgNr++; if (ArgNr>=ArgC) return false; Options_ClientWindowSizeX=atoi(ArgV[ArgNr]); }
        else if (_stricmp(ArgV[ArgNr], "-clWinSizeY"    )==0) { ArgNr++; if (ArgNr>=ArgC) return false; Options_ClientWindowSizeY=atoi(ArgV[ArgNr]); }
        else if (_stricmp(ArgV[ArgNr], "-clRenderer"    )==0) { ArgNr++; if (ArgNr>=ArgC) return false; Options_ClientDesiredRenderer=std::string(ArgV[ArgNr]); }
        else if (_stricmp(ArgV[ArgNr], "-clSoundSystem" )==0) { ArgNr++; if (ArgNr>=ArgC) return false; Options_ClientDesiredSoundSystem=std::string(ArgV[ArgNr]); }
        else if (_stricmp(ArgV[ArgNr], "-clPlayerName"  )==0) { ArgNr++; if (ArgNr>=ArgC) return false; Options_DeathMatchPlayerName=std::string(ArgV[ArgNr]); }
        else if (_stricmp(ArgV[ArgNr], "-clModelName"   )==0) { ArgNr++; if (ArgNr>=ArgC) return false; Options_DeathMatchModelName=std::string(ArgV[ArgNr]); }
#endif
        else
        {
            if (SvWorldNameHasBeenGiven)
            {
                std::cout << "Sorry, I don't know what \"" << ArgV[ArgNr] << "\" means.\n"
                          << "It might be a typo elsewhere, because when I tried to interprete it as world name,\n"
                          << "I found that you already specified \"" << Options_ServerWorldName.GetValueString() << "\" as world name.\n";
                return false;
            }
            else
            {
                Options_ServerWorldName=std::string(ArgV[ArgNr]);
                SvWorldNameHasBeenGiven=true;
            }
        }
    }

    return true;
}


// Notes about the GlobalTime convar:
// 1) It's declared for use with "extern ConVarT GlobalTime;" elsewhere, so we cannot use "static" here.
// 2) It's intentionally not defined very precisely (except for that it is in seconds and ever increases),
//    because I especially don't want to define where "0" is (at program start, at start of first frame, at new map start, ...?).
ConVarT GlobalTime("time", 0.0, ConVarT::FLAG_MAIN_EXE | ConVarT::FLAG_READ_ONLY, "The ever proceeding time, in seconds.");


static int ConFunc_CleanupPersistentConfig_Callback(lua_State* LuaState)
{
    const char*                        CfgFileName=luaL_checkstring(LuaState, 1);
    TextParserT                        TP(CfgFileName, "=");
    std::map<std::string, std::string> Pairs;

    if (TP.IsAtEOF())
    {
        // This function must not cause a Lua error, or otherwise the calling config.lua script is aborted early!
        Console->Warning(cf::va("Unable to open file %s.\n", CfgFileName));
        return 0;
    }

    try
    {
        while (!TP.IsAtEOF())
        {
            std::string Key  =TP.GetNextToken(); TP.AssertAndSkipToken("=");
            std::string Value=TP.GetNextToken();

            if (TP.WasLastTokenQuoted()) Value="\""+Value+"\"";

            Pairs[Key]=Value;
        }
    }
    catch (const TextParserT::ParseError&)
    {
        // This function must not cause a Lua error, or otherwise the calling config.lua script is aborted early!
        Console->Warning(cf::va("Problem parsing the config file near byte %lu (%.3f%%).", TP.GetReadPosByte(), TP.GetReadPosPercent()*100.0));
        return 0;
    }


    // Ok, all the right keys and values are now in Pairs, now rewrite the config file.
    std::ofstream CfgFile(CfgFileName, std::ios::out);

    if (!CfgFile.bad())
    {
        for (std::map<std::string, std::string>::const_iterator It=Pairs.begin(); It!=Pairs.end(); ++It)
        {
            CfgFile << It->first << " = " << It->second << "\n";
        }

        CfgFile << "\n";
    }

    return 0;
}

static ConFuncT ConFunc_CleanupPersistentConfig("CleanupPersistentConfig", ConFunc_CleanupPersistentConfig_Callback, ConFuncT::FLAG_MAIN_EXE, "");


static int ConFunc_glInfo_Callback(lua_State* LuaState)
{
    Console->Print(cf::va("OpenGL Window: %u * %u, %u bpp, %s.\n", SingleOpenGLWindow->GetWidth(), SingleOpenGLWindow->GetHeight(), SingleOpenGLWindow->GetBPP(), SingleOpenGLWindow->GetFullScreen() ? "full-screen" : "windowed"));
    Console->Print(cf::va("Renderer Info: %s\n", MatSys::Renderer ? MatSys::Renderer->GetDescription() : "[No renderer active.]"));

    return 0;
}

static ConFuncT ConFunc_glInfo("glInfo", ConFunc_glInfo_Callback, ConFuncT::FLAG_MAIN_EXE, "Prints some information about the OpenGL window and renderer.");


static int ConFunc_VideoGetModes_Callback(lua_State* LuaState)
{
#ifdef _WIN32
    for (unsigned long ModeNr=0; true; ModeNr++)
    {
        DEVMODE DeviceMode;

        memset(&DeviceMode, 0, sizeof(DeviceMode));
        DeviceMode.dmSize=sizeof(DeviceMode);

        if (EnumDisplaySettings(NULL, ModeNr, &DeviceMode)==0) break;
        if (DeviceMode.dmBitsPerPel!=32 || DeviceMode.dmDisplayFlags!=0 /*|| DeviceMode.dmDisplayFrequency!=DefaultFrequency*/) continue;

        Console->Print(cf::va("%3u,    %4u x %u\n", ModeNr, DeviceMode.dmPelsWidth, DeviceMode.dmPelsHeight));
    }

    // Doppelte ausfiltern, 1024*768 garantieren, sortieren(?),
    // Vorzugsweise DisplayFrequence verwenden, die der des Desfault-Desktops entspricht.
    // Was macht Q3?

    return 0;
#else
    return luaL_error(LuaState, "Sorry, this function is not yet implemented on this platform.");
#endif
}

static ConFuncT ConFunc_VideoGetModes("VideoGetModes", ConFunc_VideoGetModes_Callback, ConFuncT::FLAG_MAIN_EXE, "Prints some information about the OpenGL window and renderer.");


static int ConFunc_forceRM_Callback(lua_State* LuaState)
{
    static MatSys::RenderMaterialT* ForceRM=NULL;

    // First check if the MatSys::Renderer is available, we need it below in all usage-cases of this function.
    if (MatSys::Renderer==NULL)
    {
        ForceRM=NULL;   // The ForceRM was either never assigned, or automatically freed when the MatSys::Renderer last went down.
        return luaL_error(LuaState, "MatSys::Renderer is not available.");
    }

    // If a ForceRM was set in a previous call to this function, free it.
    if (ForceRM!=NULL)
    {
        if (MatSys::Renderer->GetCurrentMaterial()==ForceRM)
        {
            MatSys::Renderer->FreeMaterial(ForceRM);
        }
        else
        {
            // Getting here is highly unusual, because either there is code somewhere else that improperly broke our material lock,
            // or the MatSys::Renderer was shutdown and re-instantiated since the previous call to this function.
            // In the latter case, we must not try to free the ForceRM, so the above if-test is a nice protection.
            Console->Warning("The previously forced material has been unforced elsewhere!\n");
        }

        ForceRM=NULL;
    }

    // If no parameters were given, remove any lock to restore normal operation.
    if (lua_gettop(LuaState)==0)
    {
        MatSys::Renderer->LockCurrentMaterial(false);
        return 0;
    }

    // Establish the lock for the given material name.
    if (MaterialManager==NULL) return luaL_error(LuaState, "The MaterialManager is not available.");

    const char* MatName=luaL_checkstring(LuaState, 1);
    MaterialT*  Mat    =MaterialManager->GetMaterial(MatName);

    if (Mat==NULL) return luaL_error(LuaState, "Unknown material \"%s\".", MatName);

    ForceRM=MatSys::Renderer->RegisterMaterial(Mat);

    if (ForceRM==NULL) return luaL_error(LuaState, "Could not register material \"%s\".", MatName);

    MatSys::Renderer->SetCurrentMaterial(ForceRM);
    MatSys::Renderer->LockCurrentMaterial(true);

    return 0;
}

static ConFuncT ConFunc_forceRM("forceRM", ConFunc_forceRM_Callback, ConFuncT::FLAG_MAIN_EXE,
    "Enforces the use of the given material for all rendering. Restores normal rendering when no parameter is given.");




static SoundI* ConMusic=NULL;   // For playing background music from the console.

static int ConFunc_MusicLoad_Callback(lua_State* LuaState)
{
    if (SoundSystem==NULL)
    {
        return luaL_error(LuaState, "SoundSystem is not available.");
    }

    if (ConMusic!=NULL)
    {
        SoundSystem->DeleteSound(ConMusic);
        ConMusic=NULL;
    }

    ConMusic=SoundSystem->CreateSound2D(SoundShaderManager->GetSoundShader(luaL_checkstring(LuaState, 1)));
    return 0;
}

static ConFuncT ConFunc_MusicLoad("MusicLoad", ConFunc_MusicLoad_Callback, ConFuncT::FLAG_MAIN_EXE, "");


static int ConFunc_MusicPlay_Callback(lua_State* LuaState)
{
    if (SoundSystem==NULL) return luaL_error(LuaState, "SoundSystem is not available.");
    if (ConMusic   ==NULL) return luaL_error(LuaState, "No sound shader loaded.");

    ConMusic->Play();
    return 0;
}

static ConFuncT ConFunc_MusicPlay("MusicPlay", ConFunc_MusicPlay_Callback, ConFuncT::FLAG_MAIN_EXE, "");


static int ConFunc_MusicIsPlaying_Callback(lua_State* LuaState)
{
    if (SoundSystem==NULL) return luaL_error(LuaState, "SoundSystem is not available.");

    lua_pushboolean(LuaState, ConMusic!=NULL && ConMusic->IsPlaying());
    return 1;
}

static ConFuncT ConFunc_MusicIsPlaying("MusicIsPlaying", ConFunc_MusicIsPlaying_Callback, ConFuncT::FLAG_MAIN_EXE, "");


static int ConFunc_MusicStop_Callback(lua_State* LuaState)
{
    if (SoundSystem==NULL) return luaL_error(LuaState, "SoundSystem is not available.");
    if (ConMusic   ==NULL) return luaL_error(LuaState, "No sound shader loaded.");

    ConMusic->Stop();
    return 0;
}

static ConFuncT ConFunc_MusicStop("MusicStop", ConFunc_MusicStop_Callback, ConFuncT::FLAG_MAIN_EXE, "");


static int ConFunc_MusicSetVol_Callback(lua_State* LuaState)
{
    if (SoundSystem==NULL) return luaL_error(LuaState, "SoundSystem is not available.");
    if (ConMusic   ==NULL) return luaL_error(LuaState, "No sound shader loaded.");

    ConMusic->SetInnerVolume(float(lua_tonumber(LuaState, 1)));
    return 0;
}

static ConFuncT ConFunc_MusicSetVol("MusicSetVolume", ConFunc_MusicSetVol_Callback, ConFuncT::FLAG_MAIN_EXE, "");


static int ConFunc_GetMasterVolume_Callback(lua_State* LuaState)
{
    if (SoundSystem==NULL) return luaL_error(LuaState, "SoundSystem is not available.");

    lua_pushnumber(LuaState, SoundSystem->GetMasterVolume());
    return 1;
}

static ConFuncT ConFunc_GetMasterVolume("GetMasterVolume", ConFunc_GetMasterVolume_Callback, ConFuncT::FLAG_MAIN_EXE, "");


static int ConFunc_SetMasterVolume_Callback(lua_State* LuaState)
{
    if (SoundSystem==NULL) return luaL_error(LuaState, "SoundSystem is not available.");

    SoundSystem->SetMasterVolume(float(lua_tonumber(LuaState, 1)));
    return 0;
}

static ConFuncT ConFunc_SetMasterVolume("SetMasterVolume", ConFunc_SetMasterVolume_Callback, ConFuncT::FLAG_MAIN_EXE, "");




WinSockT* g_WinSock=NULL;


#ifdef _WIN32
void WinError(const char* ErrorText)
{
    MessageBox(NULL, ErrorText, "Cafu Engine", MB_OK | MB_ICONERROR | MB_TOPMOST);
    PostQuitMessage(1);
}


int main(int, char*[])
// int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int)
{
    cf::GuiSys::GetWindowTIM().Init();    // The one-time init of the Window type info manager.

    // Initialize the global interfaces.
    Console->Print("Cafu Engine, "__DATE__"\n");

    static ConsoleInterpreterImplT ConInterpreterImpl;
    ConsoleInterpreter=&ConInterpreterImpl;

    // All global convars and confuncs have registered themselves in linked lists.
    // Register them with the console interpreter now.
    ConFuncT::RegisterStaticList();
    ConVarT ::RegisterStaticList();

    ConsoleInterpreter->RunCommand("dofile('config.lua');");


    bool SkipDialog=true;
    if (!ParseCommandLine(__argc, __argv, SkipDialog))
    {
        std::string UsageString=Usage();

        MessageBox(NULL, UsageString.c_str(), "Cafu Engine", MB_OK | MB_ICONINFORMATION | MB_TOPMOST);
        ConsoleInterpreter=NULL;     // This is very important, to make sure that no ConFuncT or ConVarT dtor accesses the ConsoleInterpreter that might already have been destroyed then.
        return 1;
    }


    try
    {
        g_WinSock=new WinSockT;
    }
    catch (const WinSockT::InitFailure& /*E*/) { WinError("Unable to initialize WinSock 2.0." ); return 1; }
    catch (const WinSockT::BadVersion&  /*E*/) { WinError("WinSock version 2.0 not supported."); return 1; }


    bool QuitProgram=false;

    if (SkipDialog)
    {
        HandleButton_OK();
        ConsoleInterpreter=NULL;     // This is very important, to make sure that no ConFuncT or ConVarT dtor accesses the ConsoleInterpreter that might already have been destroyed then.
        return 0;
    }

    while (!QuitProgram)
    {
        MSG Message;

        // Owner handle is set to NULL, which means that the dialog box has no owner.
        switch (DialogBox(NULL, "MainDialog", NULL, (DLGPROC)Ca3DEMainDialogProcedure))
        {
            case -4: WinError("Could not fill list with world names!"); break;
            case -3: WinError("Could not fill list with game  names!"); break;
            case -2: WinError("Could not fill list with model names!"); break;
            case -1: WinError("DialogBox() failed!");                   break;

            case  0: QuitProgram=true; break;

            case  1: HandleButton_OK();             break;
            case  2: HandleButton_Configure();      break;
            case  3: HandleButton_GetServerInfo();  break;
            case  4: HandleButton_Ping();           break;
            case  5: HandleButton_SendNOP();        break;
        }

        // Diese Funktion ist hier nur von begrenztem Wert, aber WinError() ruft z.B. PostQuitMessage() auf.
        // Damit wird sichergestellt, daß wir auch noch einen ordentliche Shut-Down durchführen können (Destruktoren)!
        // Wenn ein Client läuft, ruft dieser PeekMessage() selbst auf, ein Server dagegen niemals.
        while (PeekMessage(&Message, NULL, 0, 0, PM_REMOVE))
        {
            if (Message.message==WM_QUIT)   // Programm-Ende?
            {
                QuitProgram=true;
                break;
            }

            TranslateMessage(&Message);     // Tastencodes übersetzen
            DispatchMessage (&Message);     // WindowProzedur aufrufen
        }
    }

    delete g_WinSock;
    g_WinSock=NULL;
    ConsoleInterpreter=NULL;     // This is very important, to make sure that no ConFuncT or ConVarT dtor accesses the ConsoleInterpreter that might already have been destroyed then.
    return 0;
}
#else
#include <dirent.h>

int MB_OK=1;
int MB_ICONERROR=2;
int MB_ICONINFORMATION=3;

void MessageBox(void*, const char* Text, const char* Title, int)
{
    std::cout << Title << ": " << Text << "\n";
}

int Sleep(unsigned long msec)
{
    timespec ts;

    ts.tv_sec =msec / 1000;
    ts.tv_nsec=(msec % 1000)*1000000;

    return nanosleep(&ts, 0);
}

int main(int ArgC, char* ArgV[])
{
    cf::GuiSys::GetWindowTIM().Init();    // The one-time init of the Window type info manager.

    static ConsoleInterpreterImplT ConCmdManImpl;
    ConsoleInterpreter=&ConCmdManImpl;

    // All global convars and confuncs have registered themselves in linked lists.
    // Register them with the console interpreter now.
    ConFuncT::RegisterStaticList();
    ConVarT ::RegisterStaticList();

    // Override the Windows defaults.
    Options_ClientWindowSizeX=800;
    Options_ClientWindowSizeY=600;

    ConsoleInterpreter->RunCommand("dofile('config.lua');");


    // See if "Games/DeathMatch/Worlds/'Options_ServerWorldName'.cw" exists.
    // If it does - fine. Otherwise, search the directory for a CW file, and use that instead.
    static char TempName[1024];

    sprintf(TempName, "Games/DeathMatch/Worlds/%s.cw", Options_ServerWorldName.GetValueString().c_str());
    FILE* TempFile=fopen(TempName, "r");

    if (TempFile)
    {
        fclose(TempFile);
        // printf("Dev-Info: The default file %s does exist.\n", TempName);
    }
    else
    {
        // printf("Dev-Info: The default file %s does NOT exist.\n", TempName);

        // Scan "Games/DeathMatch/Worlds" for any other world.
        DIR* Dir=opendir("Games/DeathMatch/Worlds");

        if (Dir)
        {
            for (dirent* DirEnt=readdir(Dir); DirEnt!=NULL; DirEnt=readdir(Dir))
            {
                const unsigned long DirNameLength=strlen(DirEnt->d_name);

                if (DirNameLength>=4)
                    if (_stricmp(&DirEnt->d_name[DirNameLength-3], ".cw")==0)
                    {
                        // Copy the 'DirEnt->d_name' into 'Ca3DE_Options.ServerWorldName',
                        // but cut off the ".cw" suffix.
                        Options_ServerWorldName=std::string(DirEnt->d_name, 0, DirNameLength-3);
                        break;
                    }
            }

            closedir(Dir);
        }

        // printf("Dev-Info: Using %s instead.\n", Ca3DE_Options.ServerWorldName);
    }


    bool SkipDialog=true;
    if (!ParseCommandLine(ArgC, ArgV, SkipDialog))
    {
        std::cout << Usage();
        return 1;
    }

    WinSockT WinSock;
    g_WinSock=&WinSock;
    HandleButton_OK();

    g_WinSock=NULL;
    ConsoleInterpreter=NULL;     // This is very important, to make sure that no ConFuncT or ConVarT dtor accesses the ConsoleInterpreter that might already have been destroyed then.
    return 0;
}
#endif


static const char* ScreenShotSuffixTypes[] = { "jpg", "png", "bmp", NULL };
static ConVarT ScreenShotSuffix("screenSuffix", "jpg", ConVarT::FLAG_MAIN_EXE, "The suffix to be used for screen-shot image files. Only \"jpg\", \"png\" and \"bmp\" are allowed.", ScreenShotSuffixTypes);

void TakeScreenshot()
{
    unsigned int Width;
    unsigned int Height;
    uint32_t*    FrameBuffer=SingleOpenGLWindow->GetFrameBuffer(Width, Height);

    if (FrameBuffer==NULL) return;

    char FileName[256];

    for (unsigned int Nr=0; Nr<10000; Nr++)
    {
        sprintf(FileName, "pic%04u.%s", Nr, ScreenShotSuffix.GetValueString().c_str());

        FILE* TestFile=fopen(FileName, "r");
        if (!TestFile) break;
        fclose(TestFile);
    }

    if (BitmapT(Width, Height, FrameBuffer).SaveToDisk(FileName))
    {
        Console->Print(std::string("Screenshot \"")+FileName+"\" saved.\n");
    }
    else
    {
        Console->Warning(std::string("Screenshot \"")+FileName+"\" could not be saved.\n");
    }
}


// A helper function modelled analogous to the PlatformAux::GetRenderer() function.
static cf::GameSys::GameI* LoadGameDLL(const std::string& GameDllName, HMODULE& GameDLL)
{
#ifdef SCONS_BUILD_DIR
    #define QUOTE(str) QUOTE_HELPER(str)
    #define QUOTE_HELPER(str) #str

    #ifdef _WIN32
    const std::string GameDllPathName=std::string("Games/")+GameDllName+"/Code/"+QUOTE(SCONS_BUILD_DIR)+"/"+GameDllName+".dll";
    #else
    const std::string GameDllPathName=std::string("Games/")+GameDllName+"/Code/"+QUOTE(SCONS_BUILD_DIR)+"/lib"+GameDllName+".so";
    #endif

    #undef QUOTE
    #undef QUOTE_HELPER
#else
    const std::string GameDllPathName=std::string("Games/")+GameDllName+"/Code/"+GameDllName+PlatformAux::GetEnvFileSuffix()+".dll";
#endif


    #ifdef _WIN32
        GameDLL=LoadLibrary(GameDllPathName.c_str());
        if (!GameDLL) { Console->Warning(cf::va("Could not load the game DLL at %s.\n", GameDllPathName.c_str())); return NULL; }
    #else
        // Note that RTLD_GLOBAL must *not* be passed-in here, or else we get in trouble with subsequently loaded libraries.
        // (E.g. it causes dlsym(GameDLL, "GetGame") to return identical results for different GameDLLs.)
        // Please refer to the man page of dlopen for more details.
        GameDLL=dlopen(GameDllPathName.c_str(), RTLD_NOW);
        if (!GameDLL) { Console->Warning(cf::va("Could not load the game DLL at %s (%s).\n", GameDllPathName.c_str(), dlerror())); return NULL; }
    #endif


    typedef cf::GameSys::GameI* (__stdcall *GetGameFuncT)(SingleOpenGLWindowT* SingleWin, MatSys::RendererI* Renderer,
        MatSys::TextureMapManagerI* TexMapMan, MaterialManagerI* MatMan, cf::GuiSys::GuiManI* GuiMan_, cf::ConsoleI* Console_,
        ConsoleInterpreterI* ConInterpreter_, cf::ClipSys::CollModelManI* CollModelMan_, SoundSysI* SoundSystem_,
        SoundShaderManagerI* SoundShaderManager_);

    #ifdef _WIN32
        GetGameFuncT GetGameFunc=(GetGameFuncT)GetProcAddress(GameDLL, "_GetGame@40");
    #else
        GetGameFuncT GetGameFunc=(GetGameFuncT)GetProcAddress(GameDLL, "GetGame");
    #endif

    if (!GetGameFunc) { Console->Warning("Could not get the address of the GetGame() function.\n"); FreeLibrary(GameDLL); GameDLL=NULL; return NULL; }


    // When we get here, the other interfaces must already have been implementations assigned.
    assert(SingleOpenGLWindow!=NULL);
    assert(MatSys::Renderer!=NULL);
    assert(MatSys::TextureMapManager!=NULL);
    assert(MaterialManager!=NULL);
    assert(cf::GuiSys::GuiMan!=NULL);
    assert(Console!=NULL);
    assert(ConsoleInterpreter!=NULL);
    assert(cf::ClipSys::CollModelMan!=NULL);

    cf::GameSys::GameI* Game=GetGameFunc(SingleOpenGLWindow, MatSys::Renderer, MatSys::TextureMapManager, MaterialManager, cf::GuiSys::GuiMan, Console, ConsoleInterpreter, cf::ClipSys::CollModelMan, SoundSystem, SoundShaderManager);

    if (!Game) { Console->Warning("Could not get the game implementation.\n"); FreeLibrary(GameDLL); GameDLL=NULL; return NULL; }

    return Game;
}


// Unique packet ID for connection-less packets
static unsigned long PacketID=0;
static MaterialManagerImplT    MaterialManagerImpl;
static SoundShaderManagerImplT SoundShaderManagerImpl;


class NoGuiCallbackT : public ServerT::GuiCallbackI
{
    public:

    void OnServerStateChanged(const char* /*NewState*/) const
    {
    }
};

static const NoGuiCallbackT NoGuiCallback;


class ClientGuiCallbackT : public ServerT::GuiCallbackI
{
    public:

    ClientGuiCallbackT()
        : MainMenuGui(NULL)
    {
    }

    void OnServerStateChanged(const char* NewState) const
    {
        if (!MainMenuGui) return;

        MainMenuGui->CallLuaFunc("OnServerStateChanged", "s", NewState);
    }

    cf::GuiSys::GuiI* MainMenuGui;
};

static ClientGuiCallbackT ClientGuiCallback;


void HandleButton_OK()
{
    // Init the FileSys. Note that the output Console should already be available at this point.
    cf::FileSys::FileMan=new cf::FileSys::FileManImplT;

    cf::FileSys::FileMan->MountFileSystem(cf::FileSys::FS_TYPE_LOCAL_PATH, "./", "");
 // cf::FileSys::FileMan->MountFileSystem(cf::FileSys::FS_TYPE_LOCAL_PATH, "Games/DeathMatch/", "");
    cf::FileSys::FileMan->MountFileSystem(cf::FileSys::FS_TYPE_ZIP_ARCHIVE, "Games/DeathMatch/Textures/TechDemo.zip", "Games/DeathMatch/Textures/TechDemo/", "Ca3DE");
    cf::FileSys::FileMan->MountFileSystem(cf::FileSys::FS_TYPE_ZIP_ARCHIVE, "Games/DeathMatch/Textures/SkyDomes.zip", "Games/DeathMatch/Textures/SkyDomes/", "Ca3DE");

    if (Options_RunMode.GetValueInt()==SERVER_RUNMODE)      // Server only
    {
        HMODULE RendererDLL;

        // Provide an implementation for the MaterialManager interface.
        MaterialManager=&MaterialManagerImpl;

        // Provide an implementation for the SoundShaderManager interface.
        SoundShaderManager=&SoundShaderManagerImpl;

        // For the Server-only mode, we need the Null renderer.
#ifdef SCONS_BUILD_DIR
        #define QUOTE(str) QUOTE_HELPER(str)
        #define QUOTE_HELPER(str) #str
#ifdef _WIN32
        MatSys::Renderer=PlatformAux::GetRenderer(std::string("Libs/")+QUOTE(SCONS_BUILD_DIR)+"/MaterialSystem/RendererNull.dll", RendererDLL);
#else
        MatSys::Renderer=PlatformAux::GetRenderer(std::string("Libs/")+QUOTE(SCONS_BUILD_DIR)+"/MaterialSystem/libRendererNull.so", RendererDLL);
#endif
        #undef QUOTE
        #undef QUOTE_HELPER
#endif

        if (MatSys::Renderer==NULL || RendererDLL==NULL)
        {
            MessageBox(NULL, "Sorry, was not able to find any renderer that is supported on your system.", "Cafu Engine", MB_OK | MB_ICONERROR);

            delete cf::FileSys::FileMan;
            cf::FileSys::FileMan=NULL;
            return;
        }

        MatSys::Renderer->Initialize();


        // Get the TextureMapManager from the RendererDLL.
        MatSys::TextureMapManager=PlatformAux::GetTextureMapManager(RendererDLL);

        if (MatSys::TextureMapManager==NULL)
        {
            MatSys::Renderer->Release();
            MatSys::Renderer=NULL;
            MatSys::TextureMapManager=NULL;
            FreeLibrary(RendererDLL);
            MessageBox(NULL, "Could not get the TextureMapManager from the RendererDLL.", "Cafu Engine", MB_OK | MB_ICONERROR);

            delete cf::FileSys::FileMan;
            cf::FileSys::FileMan=NULL;
            return;
        }

        switch (Options_ClientTextureDetail.GetValueInt())
        {
            case 1: MatSys::TextureMapManager->SetMaxTextureSize(256); break;
            case 2: MatSys::TextureMapManager->SetMaxTextureSize(128); break;
        }


     // MaterialManager->ClearAllMaterials();
        MaterialManager->RegisterMaterialScriptsInDir("Games/"+Options_ServerGameName.GetValueString()+"/Materials", "Games/"+Options_ServerGameName.GetValueString()+"/");
        SoundShaderManager->RegisterSoundShaderScriptsInDir("Games/"+Options_ServerGameName.GetValueString()+"/SoundShader", "Games/"+Options_ServerGameName.GetValueString()+"/");


        HMODULE SoundDLL;

        // For the Server-only mode, we need the Null sound system.
#ifdef SCONS_BUILD_DIR
        #define QUOTE(str) QUOTE_HELPER(str)
        #define QUOTE_HELPER(str) #str
#ifdef _WIN32
        SoundSystem=PlatformAux::GetSoundSys(std::string("Libs/")+QUOTE(SCONS_BUILD_DIR)+"/SoundSystem/SoundSysNull.dll", SoundDLL);
#else
        SoundSystem=PlatformAux::GetSoundSys(std::string("Libs/")+QUOTE(SCONS_BUILD_DIR)+"/SoundSystem/libSoundSysNull.so", SoundDLL);
#endif
        #undef QUOTE
        #undef QUOTE_HELPER
#endif

        if (SoundSystem==NULL || SoundDLL==NULL)
        {
            Console->Print("Sorry, could not load the null sound system dll.\n");

            MatSys::Renderer->Release();
            MatSys::Renderer=NULL;
            MatSys::TextureMapManager=NULL;
            FreeLibrary(RendererDLL);
         // SingleOpenGLWindow->Close();

            delete cf::FileSys::FileMan;
            cf::FileSys::FileMan=NULL;
            return;
        }

        // Init the GuiSys.
        // We must initialize the cf::GuiSys::GuiMan pointer early, because even the server needs access to it
        // when it loads static detail model entities that have world/entity-GUIs.
     // cf::GuiSys::GetWindowTIM().Init();      // This is done near the top of main(), to make sure it's really done only once.
        cf::GuiSys::GuiMan=new cf::GuiSys::GuiManImplT;


        // Load the Game DLL, that is, set the cf::GameSys::Game pointer to an implementation, analogous to the MatSys::Renderer.
        HMODULE GameDLL;    // Handle to the loaded game DLL. (Only used for the call to FreeLibrary() when closing.)
        cf::GameSys::Game=LoadGameDLL(Options_ServerGameName.GetValueString(), GameDLL);

        if (cf::GameSys::Game==NULL || GameDLL==NULL)
        {
            Console->Print("Sorry, could not load the game dll.\n");

            delete cf::GuiSys::GuiMan;
            cf::GuiSys::GuiMan=NULL;

            SoundSystem->Release();
            SoundSystem=NULL;
            FreeLibrary(SoundDLL);

            MatSys::Renderer->Release();
            MatSys::Renderer=NULL;
            MatSys::TextureMapManager=NULL;
            FreeLibrary(RendererDLL);
         // SingleOpenGLWindow->Close();

            delete cf::FileSys::FileMan;
            cf::FileSys::FileMan=NULL;
            return;
        }

        cf::GameSys::Game->Initialize(false, true);


        try
        {
            ServerT Server(Options_ServerGameName.GetValueString(), NoGuiCallback);

            if (!ConsoleInterpreter->RunCommand("changeLevel('"+Options_ServerWorldName.GetValueString()+"')"))
                throw ServerT::InitFailureT("Initial changeLevel() failed.");

            while (true)
            {
                static ConVarT quit("quit", false, ConVarT::FLAG_MAIN_EXE, "The program quits if this variable is set to 1 (true).");
                if (quit.GetValueBool())
                {
                    quit.SetValue(false);   // Immediately reset the value, so that we're able to restart the game from a loop that governs the master loop...
                    break;
                }

                Server.MainLoop();
            }
        }
        catch (const ServerT::InitFailureT& E) { MessageBox(NULL, E.Msg, "Cafu Engine  (server init)", MB_OK | MB_ICONERROR); }


        if (cf::GameSys::Game!=NULL) cf::GameSys::Game->Release();
        cf::GameSys::Game=NULL;
        FreeLibrary(GameDLL);

        delete cf::GuiSys::GuiMan;
        cf::GuiSys::GuiMan=NULL;

        SoundSystem->Release();
        SoundSystem=NULL;
        FreeLibrary(SoundDLL);

        MatSys::Renderer->Release();
        MatSys::Renderer=NULL;
        MatSys::TextureMapManager=NULL;
        FreeLibrary(RendererDLL);

        delete cf::FileSys::FileMan;
        cf::FileSys::FileMan=NULL;
        return;
    }


#ifndef CA3DE_DEDICATED_SERVER
    /******************/
    /*** Initialize ***/
    /******************/

    HMODULE RendererDLL;

    // Provide an implementation for the MaterialManager interface.
    MaterialManager=&MaterialManagerImpl;

    // Provide an implementation for the SoundShaderManager interface.
    SoundShaderManager=&SoundShaderManagerImpl;

 // MaterialManager->ClearAllMaterials();
    MaterialManager->RegisterMaterialScriptsInDir("Games/"+Options_ServerGameName.GetValueString()+"/Materials", "Games/"+Options_ServerGameName.GetValueString()+"/");
    SoundShaderManager->RegisterSoundShaderScriptsInDir("Games/"+Options_ServerGameName.GetValueString()+"/SoundShader", "Games/"+Options_ServerGameName.GetValueString()+"/");


    // MainWindow öffnen.
    const char* ErrorMsg=SingleOpenGLWindow->Open("Cafu Engine", Options_ClientWindowSizeX.GetValueInt(), Options_ClientWindowSizeY.GetValueInt(), Options_ClientDisplayBPP.GetValueInt()==0 ? 16 : 32, Options_ClientFullScreen.GetValueBool());

    if (ErrorMsg)
    {
        Console->Print(ErrorMsg);
        Console->Print("\nTrying again with fail-safe settings...\n");

        // Try again with settings that hopefully should work everywhere
        // (this assumes that the first attempt failed due to weird settings).
        Options_ClientWindowSizeX=1024;
        Options_ClientWindowSizeY=768;
        Options_ClientDisplayBPP =1;

        ErrorMsg=SingleOpenGLWindow->Open("Cafu Engine", Options_ClientWindowSizeX.GetValueInt(), Options_ClientWindowSizeY.GetValueInt(), Options_ClientDisplayBPP.GetValueInt()==0 ? 16 : 32, Options_ClientFullScreen.GetValueBool());

        if (ErrorMsg)
        {
            Console->Print(ErrorMsg);

            delete cf::FileSys::FileMan;
            cf::FileSys::FileMan=NULL;
            return;
        }
    }


    // Test the available renderers for support and pick the best.
    // Get the renderer with the highest preference number that is supported.
    MatSys::Renderer=(Options_ClientDesiredRenderer.GetValueString()=="") ? PlatformAux::GetBestRenderer(RendererDLL) : PlatformAux::GetRenderer(Options_ClientDesiredRenderer.GetValueString(), RendererDLL);

    if (MatSys::Renderer==NULL || RendererDLL==NULL)
    {
        Console->Print("Sorry, was not able to find any renderer that is supported on your system.\n");
        SingleOpenGLWindow->Close();

        delete cf::FileSys::FileMan;
        cf::FileSys::FileMan=NULL;
        return;
    }

    MatSys::Renderer->Initialize();


    // Get the TextureMapManager from the RendererDLL.
    MatSys::TextureMapManager=PlatformAux::GetTextureMapManager(RendererDLL);

    if (MatSys::TextureMapManager==NULL)
    {
        Console->Print("Could not get the TextureMapManager from the RendererDLL.\n");
        MatSys::Renderer->Release();
        MatSys::Renderer=NULL;
        MatSys::TextureMapManager=NULL;
        FreeLibrary(RendererDLL);
        SingleOpenGLWindow->Close();

        delete cf::FileSys::FileMan;
        cf::FileSys::FileMan=NULL;
        return;
    }

    switch (Options_ClientTextureDetail.GetValueInt())
    {
        case 1: MatSys::TextureMapManager->SetMaxTextureSize(256); break;
        case 2: MatSys::TextureMapManager->SetMaxTextureSize(128); break;
    }


    // Initialize the sound system.
    HMODULE SoundDLL;
    SoundSystem=(Options_ClientDesiredSoundSystem.GetValueString()=="") ? PlatformAux::GetBestSoundSys(SoundDLL) : PlatformAux::GetSoundSys(Options_ClientDesiredSoundSystem.GetValueString(), SoundDLL);

    if (SoundSystem==NULL || SoundDLL==NULL)
    {
        Console->Print("Sorry, was not able to find any sound system that is supported on your system.\n");
        MatSys::Renderer->Release();
        MatSys::Renderer=NULL;
        MatSys::TextureMapManager=NULL;
        FreeLibrary(RendererDLL);
        SingleOpenGLWindow->Close();

        delete cf::FileSys::FileMan;
        cf::FileSys::FileMan=NULL;
        return;
    }

    if (!SoundSystem->Initialize())
    {
        Console->Print("WARNING: Sound system failed to initialize!\n");
        Console->Print("I'm sorry, but this is probably due to sound driver problems with your computer.\n");
        Console->Print("We'll proceed anyway, with sound effects disabled.\n");

        SoundSystem=NULL;
        FreeLibrary(SoundDLL);

#ifdef SCONS_BUILD_DIR
        #define QUOTE(str) QUOTE_HELPER(str)
        #define QUOTE_HELPER(str) #str
#ifdef _WIN32
        SoundSystem=PlatformAux::GetSoundSys(std::string("Libs/")+QUOTE(SCONS_BUILD_DIR)+"/SoundSystem/SoundSysNull.dll", SoundDLL);
#else
        SoundSystem=PlatformAux::GetSoundSys(std::string("Libs/")+QUOTE(SCONS_BUILD_DIR)+"/SoundSystem/libSoundSysNull.so", SoundDLL);
#endif
        #undef QUOTE
        #undef QUOTE_HELPER
#endif

        // Null sound system should always be there and loadable.
        assert(SoundSystem!=NULL && SoundDLL!=NULL);

        SoundSystem->Initialize();    // Init of Null sound system always succeeds.
    }

    static ConVarT InitialMasterVolume("snd_InitialMasterVolume", 1.0, ConVarT::FLAG_MAIN_EXE | ConVarT::FLAG_PERSISTENT, "The master volume with which the sound system is initialized.", 0.0, 1.0);
    SoundSystem->SetMasterVolume(float(InitialMasterVolume.GetValueDouble()));


    // Init the GuiSys.
    // We must initialize the cf::GuiSys::GuiMan pointer early, because even the server needs access to it
    // when it loads static detail model entities that have world/entity-GUIs.
    cf::GuiSys::GuiMan=new cf::GuiSys::GuiManImplT;


    // Load the Game DLL, that is, set the cf::GameSys::Game pointer to an implementation, analogous to the MatSys::Renderer.
    HMODULE GameDLL;    // Handle to the loaded game DLL. (Only used for the call to FreeLibrary() when closing.)
    cf::GameSys::Game=LoadGameDLL(Options_ServerGameName.GetValueString(), GameDLL);

    if (cf::GameSys::Game==NULL || GameDLL==NULL)
    {
        Console->Print("Sorry, could not load the game dll.\n");

        delete cf::GuiSys::GuiMan;
        cf::GuiSys::GuiMan=NULL;

        SoundSystem->Release();
        FreeLibrary(SoundDLL);

        MatSys::Renderer->Release();
        MatSys::Renderer=NULL;
        MatSys::TextureMapManager=NULL;
        FreeLibrary(RendererDLL);
        SingleOpenGLWindow->Close();

        delete cf::FileSys::FileMan;
        cf::FileSys::FileMan=NULL;
        return;
    }

    cf::GameSys::Game->Initialize((Options_RunMode.GetValueInt() & CLIENT_RUNMODE)>0, (Options_RunMode.GetValueInt() & SERVER_RUNMODE)>0);


    // Create the client and server instances.
    ServerT* Server=new ServerT(Options_ServerGameName.GetValueString(), ClientGuiCallback);
    ClientT* Client=new ClientT();  // The client initializes in IDLE state.


    // Finish the initialization of the GuiSys.
    // Note that in the line below, the call to gui:setMousePos() is important, because it sets "MouseOverWindow" in the GUI properly to "Cl".
    // Without this, a left mouse button click that was not preceeded by a mouse movement would erroneously remove the input focus from "Cl".
    cf::GuiSys::GuiImplT* ClientGui   =new cf::GuiSys::GuiImplT("Cl=gui:new('ClientWindowT'); gui:SetRootWindow(Cl); gui:showMouse(false); gui:setMousePos(320, 240); gui:setFocus(Cl); Cl:SetName('Client');", true);
    cf::GuiSys::WindowT*  ClientWindow=ClientGui->GetRootWindow()->Find("Client");
    ClientWindowT*        ClWin       =dynamic_cast<ClientWindowT*>(ClientWindow);

    assert(ClWin!=NULL);
    if (ClWin) ClWin->SetClient(Client);
    cf::GuiSys::GuiMan->Register(ClientGui);


    cf::GuiSys::GuiI* MainMenuGui=cf::GuiSys::GuiMan->Find("Games/"+Options_ServerGameName.GetValueString()+"/GUIs/MainMenu/MainMenu_main.cgui", true);
    if (MainMenuGui==NULL)
    {
        MainMenuGui=new cf::GuiSys::GuiImplT("Err=gui:new('WindowT'); gui:SetRootWindow(Err); gui:activate(true); gui:setInteractive(true); gui:showMouse(true); Err:set('rect', 0, 0, 640, 480); Err:set('text', 'Error loading MainMenu_main.cgui,\\nsee console <F1> for details.');", true);
        cf::GuiSys::GuiMan->Register(MainMenuGui);
    }
    Client->SetMainMenuGui(MainMenuGui);
    ClientGuiCallback.MainMenuGui=MainMenuGui;      // This is the callback for the server, so that it can let the MainMenuGui know about its state changes.
    ClientGuiCallback.OnServerStateChanged("idle"); // Argh, this is a HACK for setting the initial state... can we move this / do it better?

    cf::GuiSys::GuiI*             ConsoleGui   =cf::GuiSys::GuiMan->Find("Games/"+Options_ServerGameName.GetValueString()+"/GUIs/Console.cgui", true);
    cf::GuiSys::WindowT*          ConsoleWindow=ConsoleGui ? ConsoleGui->GetRootWindow()->Find("ConsoleOutput") : NULL;
    cf::GuiSys::ConsoleByWindowT* ConByGuiWin  =new cf::GuiSys::ConsoleByWindowT(ConsoleWindow);


    /*******************/
    /*** Master loop ***/
    /*******************/

    cf::ConsoleI* PrevConsole=Console;
    Console=ConByGuiWin;

    TimerT Timer;
    double TotalTime=0.0;

    while (true)
    {
        // Rufe die Nachrichten der Windows-Nachrichtenschlange ab.
        if (SingleOpenGLWindow->HandleWindowMessages()) break;

        // Beende das Programm, wenn das letzte aktive GUI geschlossen wurde.
        // if (cf::GuiSys::GuiMan->GetNumberOfActiveGUIs()==0) break;

        static ConVarT quit("quit", false, ConVarT::FLAG_MAIN_EXE, "The program quits if this variable is set to 1 (true).");
        if (quit.GetValueBool())
        {
            quit.SetValue(false);   // Immediately reset the value, so that we're able to restart the game from a loop that governs the master loop...
            break;
        }


        const double FrameTimeD=Timer.GetSecondsSinceLastCall();
        const float  FrameTimeF=float(FrameTimeD);

        TotalTime+=FrameTimeD;
        GlobalTime.SetValue(TotalTime);


        // Forward the mouse and keyboard events from the "device" to the GuiMan.
        CaKeyboardEventT KE;
        while (SingleOpenGLWindow->GetNextKeyboardEvent(KE)>0)
        {
            if (KE.Type==CaKeyboardEventT::CKE_KEYDOWN)
            {
                if (KE.Key==CaKeyboardEventT::CK_F1)
                {
                    // Activate the in-game console GUI (it's "F1" now, not CK_GRAVE ("^", accent grave) any longer).
                    // a) ConsoleGui could be NULL on file not found, parse error, etc.
                    // b) Only handle the key if the console is not yet active, otherwise let it deal with it itself (e.g. for closing again).
                    if (ConsoleGui!=NULL && !ConsoleGui->GetIsActive())
                    {
                        ConsoleGui->Activate();
                        cf::GuiSys::GuiMan->BringToFront(ConsoleGui);


                        static bool InitialHelpMsgPrinted=false;

                        if (!InitialHelpMsgPrinted)
                        {
                            Console->Print("\n");
                            Console->Print("Welcome to the Ca3DE console!\n");
                            Console->Print("Enter   help()   to obtain more information.\n");
                            Console->Print("\n");
                            InitialHelpMsgPrinted=true;
                        }

                        continue;
                    }
                }

                if (KE.Key==CaKeyboardEventT::CK_F5)
                {
                    TakeScreenshot();
                    continue;
                }
            }

            // Console->Print(cf::va("KeyEvent: Type %lu, Key %lu == 0x%X == (%c)\n", KE.Type, KE.Key, KE.Key, char(KE.Key)));
            cf::GuiSys::GuiMan->ProcessDeviceEvent(KE);
        }

        CaMouseEventT ME;
        while (SingleOpenGLWindow->GetNextMouseEvent(ME)>0)
            cf::GuiSys::GuiMan->ProcessDeviceEvent(ME);

        cf::GuiSys::GuiMan->DistributeClockTickEvents(FrameTimeF);


        if (!SingleOpenGLWindow->GetIsMinimized())
        {
            // Render all the GUIs.
            MatSys::Renderer->BeginFrame(TotalTime);

            cf::GuiSys::GuiMan->RenderAll();

            MatSys::Renderer->EndFrame();
            SingleOpenGLWindow->SwapBuffers();
        }
        else
        {
            // The main window is minimized - give other applications a chance to run.
            Sleep(5);
        }


        // Update the sound system (Reposition sound sources, update streams).
        SoundSystem->Update();


        // Run a client and a server frame.
        Client->MainLoop(FrameTimeF);
        if (Server) Server->MainLoop();
    }

    Console=PrevConsole;

    delete ConByGuiWin;
    ConByGuiWin=NULL;

    delete Client;
    delete Server;

    ClientGuiCallback.MainMenuGui=NULL;


    /****************/
    /*** Clean-up ***/
    /****************/

    cf::GameSys::Game->Release();
    cf::GameSys::Game=NULL;
    FreeLibrary(GameDLL);

    // The latest when the game has been unloaded, no collision models must be left in the collision model manager.
    assert(cf::ClipSys::CollModelMan->GetUniqueCMCount()==0);

    delete cf::GuiSys::GuiMan;
    cf::GuiSys::GuiMan=NULL;

    SoundSystem->Release();
    FreeLibrary(SoundDLL);

    MatSys::Renderer->Release();
    MatSys::Renderer=NULL;
    MatSys::TextureMapManager=NULL;
    FreeLibrary(RendererDLL);
    SingleOpenGLWindow->Close();
#endif

    delete cf::FileSys::FileMan;
    cf::FileSys::FileMan=NULL;
}


void HandleButton_Configure()
{
    Console->Print("Sorry, this is not yet implemented.\n");
}


void HandleButton_GetServerInfo()
{
    Console->Print("Sorry, server information is not yet implemented.\n");
}


void HandleButton_Ping()
{
    NetSocketT ClientSocket(g_WinSock->GetUDPSocket(Options_ClientPortNr.GetValueInt()));

    if (ClientSocket==INVALID_SOCKET)
    {
        MessageBox(NULL, "Unable to obtain a UDP socket on client port.", "Cafu Engine", MB_OK | MB_ICONERROR);
        return;
    }


    try
    {
        NetDataT    InData;
        NetDataT    OutData;
        NetAddressT ServerAddress(Options_ClientRemoteName.GetValueString().c_str(), Options_ClientRemotePortNr.GetValueInt());

        OutData.WriteLong(0xFFFFFFFF);
        OutData.WriteLong(PacketID++);
        OutData.WriteByte(CS0_Ping);
        OutData.Send(ClientSocket, ServerAddress);
        Console->Print(cf::va("Sent ping to %s (%s)\n(awaiting acknowledgement within 5.0 seconds)...\n\n", Options_ClientRemoteName.GetValueString().c_str(), ServerAddress.ToString()));

        TimerT Timer;

        // Wait until ping packet gets acknowledged or times out.
        while (true)
        {
            if (Timer.GetSecondsSinceCtor()>5.0)
            {
                Console->Print("Ping expired after 5.0 seconds!\n");
                break;
            }

            try
            {
                NetAddressT RemoteAddress=InData.Receive(ClientSocket);

                if (RemoteAddress    !=ServerAddress) { Console->Print("WARNING: Foreign server answer!\n");    continue; }
                if (InData.ReadLong()!=0xFFFFFFFF   ) { Console->Print("WARNING: Ignoring alien packet!\n");    continue; }
                if (InData.ReadLong()!=PacketID-1   ) { Console->Print("WARNING: Ignoring wrong ID packet!\n"); continue; }
                if (InData.ReadByte()!=SC0_ACK      ) { Console->Print("WARNING: Ping was not properly ACKed! Packet ignored!\n"); continue; }

                // Connection attempt was successful
                Console->Print(cf::va("Ping time was %lu msec!\n", (unsigned long)(Timer.GetSecondsSinceCtor()*1000.0)));
                break;
            }
            catch (const NetDataT::WinSockAPIError& E)
            {
                switch (E.Error)
                {
                    case WSAEWOULDBLOCK: Sleep(10); break;
                    case WSAEMSGSIZE   : Console->Print("WARNING: A response exceeds buffer size! Packet ignored!\n"); break;
                    default            : Console->Print(cf::va("InData.Receive() returned WSA fail code %u.\n", E.Error));
                }
            }
        }

        Console->Print("\nPlease press any key to continue.\n");
        _getch();
        //FreeConsole();
    }
    catch (const NetAddressT::BadHostName& /*E*/) { MessageBox(NULL, "Unable to resolve server name.", "Cafu Engine", MB_OK | MB_ICONERROR); }
}


void HandleButton_SendNOP()
{
    NetSocketT ClientSocket(g_WinSock->GetUDPSocket(Options_ClientPortNr.GetValueInt()));

    if (ClientSocket==INVALID_SOCKET)
    {
        MessageBox(NULL, "Unable to obtain a UDP socket on client port.", "Cafu Engine", MB_OK | MB_ICONERROR);
        return;
    }


    try
    {
        NetDataT    OutData;
        NetAddressT ServerAddress(Options_ClientRemoteName.GetValueString().c_str(), Options_ClientRemotePortNr.GetValueInt());

        OutData.WriteLong(0xFFFFFFFF);
        OutData.WriteLong(PacketID++);
        OutData.WriteByte(CS0_NoOperation);
        OutData.Send(ClientSocket, ServerAddress);

        char PrintString[256];
        sprintf(PrintString, "Sent CS0_NoOperation packet to %s (%s)\n", Options_ClientRemoteName.GetValueString().c_str(), ServerAddress.ToString());
        MessageBox(NULL, PrintString, "Cafu Engine", MB_OK | MB_ICONINFORMATION);
    }
    catch (const NetAddressT::BadHostName& /*E*/) { MessageBox(NULL, "Unable to resolve server name.", "Cafu Engine", MB_OK | MB_ICONERROR); }
}
