/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "ConDefs.hpp"
#include "ConsoleCommands/Console.hpp"
#include "ConsoleCommands/ConVar.hpp"
#include "ConsoleCommands/ConFunc.hpp"
#include "MaterialSystem/MaterialManager.hpp"
#include "MaterialSystem/Renderer.hpp"
#include "SoundSystem/Sound.hpp"
#include "SoundSystem/SoundSys.hpp"
#include "SoundSystem/SoundShaderManager.hpp"
#include "TextParser/TextParser.hpp"

extern "C"
{
    #include <lua.h>
    #include <lualib.h>
    #include <lauxlib.h>
}

#include <fstream>
#include <map>


static const int CLIENT_RUNMODE=1;
static const int SERVER_RUNMODE=2;
static const int DefaultRunMode=CLIENT_RUNMODE | SERVER_RUNMODE;

/*static*/ ConVarT Options_RunMode                 ("dlg_RunMode",     DefaultRunMode, ConVarT::FLAG_MAIN_EXE,                            "1 is client-only, 2 is server-only, 3 is both.", 1, 3);
/*static*/ ConVarT Options_PlayerName              ("dlg_dmPlayerName",      "Player", ConVarT::FLAG_MAIN_EXE | ConVarT::FLAG_PERSISTENT, "Player name.");
/*static*/ ConVarT Options_PlayerModelName         ("dlg_dmModelName",        "James", ConVarT::FLAG_MAIN_EXE | ConVarT::FLAG_PERSISTENT, "Name of the player model.");
/*static*/ ConVarT Options_ClientFullScreen        ("dlg_clFullScreen",          true, ConVarT::FLAG_MAIN_EXE | ConVarT::FLAG_PERSISTENT, "When true, the video mode is changed on startup and fullscreen display is used. Otherwise, Cafu runs in an application window on the desktop.");
/*static*/ ConVarT Options_ClientWindowSizeX       ("dlg_clWindowSizeX",         1024, ConVarT::FLAG_MAIN_EXE | ConVarT::FLAG_PERSISTENT, "The size of the client window in X direction.");
/*static*/ ConVarT Options_ClientWindowSizeY       ("dlg_clWindowSizeY",          768, ConVarT::FLAG_MAIN_EXE | ConVarT::FLAG_PERSISTENT, "The size of the client window in Y direction.");
/*static*/ ConVarT Options_ClientDesiredRenderer   ("dlg_clDesiredRenderer",       "", ConVarT::FLAG_MAIN_EXE,                            "If set, overrides the auto-selection of the renderer and determines which renderer is to be used instead.");
/*static*/ ConVarT Options_ClientDesiredSoundSystem("dlg_clDesiredSoundSystem",    "", ConVarT::FLAG_MAIN_EXE,                            "If set, overrides the auto-selection of the sound system and determines which sound system is to be used instead.");
/*static*/ ConVarT Options_ClientPortNr            ("dlg_clPortNr",             33000, ConVarT::FLAG_MAIN_EXE,                            "The client port number.", 0, 0xFFFF);
/*static*/ ConVarT Options_ClientRemoteName        ("dlg_clRemoteName", "192.168.1.1", ConVarT::FLAG_MAIN_EXE | ConVarT::FLAG_PERSISTENT, "Name or IP of the server the client connects to.");
/*static*/ ConVarT Options_ClientRemotePortNr      ("dlg_clRemotePort",         30000, ConVarT::FLAG_MAIN_EXE | ConVarT::FLAG_PERSISTENT, "Port number of the remote server.", 0, 0xFFFF);
/*static*/ ConVarT Options_ClientDisplayBPP        ("dlg_clDisplayBPP",            32, ConVarT::FLAG_MAIN_EXE | ConVarT::FLAG_PERSISTENT, "The display depth in bits-per-pixel. Normally use 32, or 0 for system default.", 0, 64);
/*static*/ ConVarT Options_ClientDisplayRefresh    ("dlg_clDisplayRefresh",         0, ConVarT::FLAG_MAIN_EXE | ConVarT::FLAG_PERSISTENT, "The display refresh rate. Use 0 for system default, check with your monitor specs for any other value.");
/*static*/ ConVarT Options_ClientTextureDetail     ("dlg_clTextureDetail",          0, ConVarT::FLAG_MAIN_EXE | ConVarT::FLAG_PERSISTENT, "0 high detail, 1 medium detail, 2 low detail", 0, 2);
/*static*/ ConVarT Options_ServerWorldName         ("dlg_svWorldName",     "TechDemo", ConVarT::FLAG_MAIN_EXE,                            "Name of the world to load.");
/*static*/ ConVarT Options_ServerPortNr            ("dlg_svPortNr",             30000, ConVarT::FLAG_MAIN_EXE,                            "Server port number.", 0, 0xFFFF);


// Notes about the GlobalTime convar:
// 1) It's declared for use with "extern ConVarT GlobalTime;" elsewhere, so we don't use "static" here.
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


static int ConFunc_VideoInfo_Callback(lua_State* LuaState)
{
    // Console->Print(GetVideoModes());
    Console->Print(cf::va("Renderer Info: %s\n", MatSys::Renderer ? MatSys::Renderer->GetDescription() : "[No renderer active.]"));
    return 0;
}

static ConFuncT ConFunc_VideoInfo("VideoInfo", ConFunc_VideoInfo_Callback, ConFuncT::FLAG_MAIN_EXE, "Prints some information about the OpenGL window and renderer.");


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
