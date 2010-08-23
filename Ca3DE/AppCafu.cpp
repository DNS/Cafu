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

#include "AppCafu.hpp"
#include "MainFrame.hpp"

#include "ClipSys/CollisionModelMan_impl.hpp"
#include "ConsoleCommands/ConsoleInterpreterImpl.hpp"
#include "ConsoleCommands/ConsoleStdout.hpp"
#include "ConsoleCommands/ConVar.hpp"
#include "ConsoleCommands/ConFunc.hpp"
#include "FileSys/FileManImpl.hpp"
#include "GuiSys/GuiMan.hpp"
#include "GuiSys/Window.hpp"
#include "MaterialSystem/MaterialManagerImpl.hpp"
#include "Network/Network.hpp"
#include "SoundSystem/SoundShaderManagerImpl.hpp"
#include "SoundSystem/SoundSys.hpp"
#include "TypeSys.hpp"
#include "../Games/Game.hpp"

#include "wx/cmdline.h"
#include "wx/filename.h"
#include "wx/msgdlg.h"
#include "wx/stdpaths.h"


// For each interface that is globally available to the application,
// provide a definition for the pointer instance and have it point to an implementation.
static cf::ConsoleStdoutT ConsoleStdout;
cf::ConsoleI* Console=&ConsoleStdout;

// static cf::ConsoleFileT ConsoleFile("info.log");
// cf::ConsoleI* Console=&ConsoleFile;

static cf::FileSys::FileManImplT FileManImpl;
cf::FileSys::FileManI* cf::FileSys::FileMan=&FileManImpl;

static cf::ClipSys::CollModelManImplT CCM;
cf::ClipSys::CollModelManI* cf::ClipSys::CollModelMan=&CCM;

static ConsoleInterpreterImplT ConInterpreterImpl;
ConsoleInterpreterI* ConsoleInterpreter=&ConInterpreterImpl;        // TODO: Put it into a proper namespace.

static MaterialManagerImplT MaterialManagerImpl;
MaterialManagerI* MaterialManager=&MaterialManagerImpl;             // TODO: Put it into a proper namespace.

static SoundShaderManagerImplT SoundShaderManagerImpl;
SoundShaderManagerI* SoundShaderManager=&SoundShaderManagerImpl;    // TODO: Put it into a proper namespace.

// static WinSockT WinSock;     // This unfortunately can throw.
WinSockT* g_WinSock=NULL;


// Implementations for these interfaces are obtained later at run-time.
// MatSys::RendererI* MatSys::Renderer=NULL;                    // TODO: Don't have it predefine the global pointer instance.
// MatSys::TextureMapManagerI* MatSys::TextureMapManager=NULL;  // TODO: Don't have it predefine the global pointer instance.
SoundSysI* SoundSystem=NULL;
cf::GuiSys::GuiManI* cf::GuiSys::GuiMan=NULL;
cf::GameSys::GameI* cf::GameSys::Game=NULL;


IMPLEMENT_APP(AppCafu)


AppCafu::AppCafu()
    : wxApp(),
      m_IsFullScreen(false),
      m_MainFrame(NULL)
{
    // All global convars and confuncs have registered themselves in linked lists.
    // Register them with the console interpreter now.
    ConFuncT::RegisterStaticList();
    ConVarT ::RegisterStaticList();

    // The one-time init of the GuiSys' windows type info manager.
    cf::GuiSys::GetWindowTIM().Init();

    SetAppName("Cafu");
    SetAppDisplayName("Cafu Engine");
    SetVendorName("Carsten Fuchs Software");
    wxStandardPaths::Get().UseAppInfo(wxStandardPaths::AppInfo_VendorName | wxStandardPaths::AppInfo_AppName);

    Console->Print("Cafu Engine, "__DATE__"\n");
}


bool AppCafu::OnInit()
{
    const wxString UserDataDir=wxStandardPaths::Get().GetUserDataDir();

    if (!wxFileName::Mkdir(UserDataDir, 0777, wxPATH_MKDIR_FULL))
        wxMessageBox(wxString("Config file storage path \n")+UserDataDir+"\n doesn't exist, and it could not be created, either.", "Warning!");

    // Undo the wx locale initialization, as we want to be sure to use the same (default) locale "C" always and everywhere.
    // Using other locales introduces a lot of subtle errors. E.g. reading floating point numbers from anywhere
    // (like map files!) fails because e.g. "1.4" is no proper floating point string in the German locale (but "1,4" is).
    setlocale(LC_ALL, "C");

    ConsoleInterpreter->RunCommand("dofile('config.lua');");

    // Parse the command line.
    if (!wxApp::OnInit()) return false;

    // Insert legacy dialog here...

    try
    {
        g_WinSock=new WinSockT;
    }
    catch (const WinSockT::InitFailure& /*E*/) { wxMessageBox("Unable to initialize WinSock 2.0." ); OnExit(); return false; }
    catch (const WinSockT::BadVersion&  /*E*/) { wxMessageBox("WinSock version 2.0 not supported."); OnExit(); return false; }

    cf::FileSys::FileMan->MountFileSystem(cf::FileSys::FS_TYPE_LOCAL_PATH, "./", "");
 // cf::FileSys::FileMan->MountFileSystem(cf::FileSys::FS_TYPE_LOCAL_PATH, "Games/DeathMatch/", "");
    cf::FileSys::FileMan->MountFileSystem(cf::FileSys::FS_TYPE_ZIP_ARCHIVE, "Games/DeathMatch/Textures/TechDemo.zip", "Games/DeathMatch/Textures/TechDemo/", "Ca3DE");
    cf::FileSys::FileMan->MountFileSystem(cf::FileSys::FS_TYPE_ZIP_ARCHIVE, "Games/DeathMatch/Textures/SkyDomes.zip", "Games/DeathMatch/Textures/SkyDomes/", "Ca3DE");

    extern ConVarT     Options_ServerGameName;
    const std::string& GameName=Options_ServerGameName.GetValueString();

    MaterialManager->RegisterMaterialScriptsInDir("Games/"+GameName+"/Materials", "Games/"+GameName+"/");
    SoundShaderManager->RegisterSoundShaderScriptsInDir("Games/"+GameName+"/SoundShader", "Games/"+GameName+"/");

    // Change display mode.
    wxDisplay Display;

    m_DesktopMode=Display.GetCurrentMode();

    if (m_DesktopMode.w==0) m_DesktopMode.w=wxGetDisplaySize().x;
    if (m_DesktopMode.h==0) m_DesktopMode.h=wxGetDisplaySize().y;

    m_CurrentMode=m_DesktopMode;

    extern ConVarT Options_ClientWindowSizeX;
    extern ConVarT Options_ClientWindowSizeY;
    extern ConVarT Options_ClientDisplayBPP;
    extern ConVarT Options_ClientDisplayRefresh;
    extern ConVarT Options_ClientFullScreen;

    if (Options_ClientFullScreen.GetValueBool())
    {
        const wxVideoMode VideoMode(Options_ClientWindowSizeX.GetValueInt(),
                                    Options_ClientWindowSizeY.GetValueInt(),
                                    Options_ClientDisplayBPP.GetValueInt(),
                                    Options_ClientDisplayRefresh.GetValueInt());

        if (Display.ChangeMode(VideoMode))
        {
            m_CurrentMode=Display.GetCurrentMode();
            m_IsFullScreen=true;
        }
        else
        {
            wxMessageBox("Cafu tried to change the video mode to\n"+
                wxString::Format("        %i x %i, %i BPP at %i Hz,\n", VideoMode.w, VideoMode.h, VideoMode.bpp, VideoMode.refresh)+
                "but it didn't work out (zero values mean system defaults).\n"+
                "We will use the currently active video mode instead and continue.\n\n"+
                "Alternatively, you can set a different video mode at the Options menu,\n"+
                "or tweak the video mode details via the console variables.\n",
                "Could not change the video mode", wxOK | wxICON_EXCLAMATION);
        }
    }

    // Create the main frame.
    m_MainFrame=new MainFrameT();
    SetTopWindow(m_MainFrame);

    return true;
}


int AppCafu::OnExit()
{
    if (m_CurrentMode!=m_DesktopMode)
    {
        wxDisplay Display;

        Display.ChangeMode(m_DesktopMode);
    }

    delete g_WinSock;
    g_WinSock=NULL;

    // Setting the ConsoleInterpreter to NULL is very important, to make sure that no ConFuncT
    // or ConVarT dtor accesses the ConsoleInterpreter that might already have been destroyed then.
    ConsoleInterpreter=NULL;

    return wxApp::OnExit();
}


extern ConVarT Options_ServerGameName;
extern ConVarT Options_ServerWorldName;
extern ConVarT Options_ServerPortNr;
extern ConVarT Options_ClientPortNr;
extern ConVarT Options_ClientRemoteName;
extern ConVarT Options_ClientRemotePortNr;

extern ConVarT Options_ClientWindowSizeX;
extern ConVarT Options_ClientWindowSizeY;
extern ConVarT Options_ClientDisplayBPP;        // TODO
extern ConVarT Options_ClientDisplayRefresh;    // TODO
extern ConVarT Options_ClientFullScreen;

extern ConVarT Options_ClientDesiredRenderer;
extern ConVarT Options_ClientDesiredSoundSystem;
extern ConVarT Options_ClientTextureDetail;
extern ConVarT Options_DeathMatchPlayerName;
extern ConVarT Options_DeathMatchModelName;


void AppCafu::OnInitCmdLine(wxCmdLineParser& Parser)
{
    Parser.AddOption("con",            "", "Runs the given commands in the console. -con \"x=3;\" is equivalent to appending x=3; to the end of the config.lua file.", wxCMD_LINE_VAL_STRING);
    Parser.AddOption("svGame",         "", "Name of the MOD / game the server should run ["+Options_ServerGameName.GetValueString()+"]. Case sensitive!", wxCMD_LINE_VAL_STRING);
    Parser.AddOption("svWorld",        "", "Name of the world the server should run ["+Options_ServerWorldName.GetValueString()+"]. Case sensitive!", wxCMD_LINE_VAL_STRING);
    Parser.AddOption("svPort",         "", wxString::Format("Server port number [%i].", Options_ServerPortNr.GetValueInt()), wxCMD_LINE_VAL_NUMBER);
    Parser.AddSwitch("noFS", "clNoFullscreen", "Don't run full-screen, but rather in a window.");
    Parser.AddOption("clPort",         "", wxString::Format("Client port number [%i].", Options_ClientPortNr.GetValueInt()), wxCMD_LINE_VAL_NUMBER);
    Parser.AddOption("clRemoteName",   "", "Name or IP of the server we connect to ["+Options_ClientRemoteName.GetValueString()+"].", wxCMD_LINE_VAL_STRING);
    Parser.AddOption("clRemotePort",   "", wxString::Format("Port number of the remote server [%i].", Options_ClientRemotePortNr.GetValueInt()), wxCMD_LINE_VAL_NUMBER);
    Parser.AddOption("clTexDetail",    "", wxString::Format("0 high detail, 1 medium detail, 2 low detail [%i].", Options_ClientTextureDetail.GetValueInt()), wxCMD_LINE_VAL_NUMBER);
    Parser.AddOption("clWinSizeX",     "", wxString::Format("If not full-screen, this is the width  of the window [%i].", Options_ClientWindowSizeX.GetValueInt()), wxCMD_LINE_VAL_NUMBER);
    Parser.AddOption("clWinSizeY",     "", wxString::Format("If not full-screen, this is the height of the window [%i].", Options_ClientWindowSizeY.GetValueInt()), wxCMD_LINE_VAL_NUMBER);
    Parser.AddOption("clRenderer",     "", "Override the auto-selection of the best available renderer [auto].", wxCMD_LINE_VAL_STRING);
    Parser.AddOption("clSoundSystem",  "", "Override the auto-selection of the best available sound system [auto].", wxCMD_LINE_VAL_STRING);
    Parser.AddOption("clPlayerName",   "", "Player name ["+Options_DeathMatchPlayerName.GetValueString()+"].", wxCMD_LINE_VAL_STRING);
    Parser.AddOption("clModelName",    "", "Name of the players model ["+Options_DeathMatchModelName.GetValueString()+"].", wxCMD_LINE_VAL_STRING);

    wxApp::OnInitCmdLine(Parser);
    Parser.AddUsageText("\nDefault values are enclosed in [brackets].");
    Parser.AddUsageText("It is also ok to omit the \"-svWorld\" option for specifying a world name: \"Cafu MyWorld\" is the same as \"Cafu -svWorld MyWorld\".\n");
    Parser.SetSwitchChars("-");
}


bool AppCafu::OnCmdLineParsed(wxCmdLineParser& Parser)
{
    long int i=0;
    wxString s="";

    if (Parser.Found("con",           &s)) ConsoleInterpreter->RunCommand(s.ToStdString());
    if (Parser.Found("svGame",        &s)) Options_ServerGameName=s;
    if (Parser.Found("svWorld",       &s)) Options_ServerWorldName=s;
    if (Parser.Found("svPort",        &i)) Options_ServerPortNr=i;
    if (Parser.Found("noFS")             ) Options_ClientFullScreen=false;
    if (Parser.Found("clPort",        &i)) Options_ClientPortNr=i;
    if (Parser.Found("clRemoteName",  &s)) Options_ClientRemoteName=s;
    if (Parser.Found("clRemotePort",  &i)) Options_ClientRemotePortNr=i;
    if (Parser.Found("clTexDetail",   &i)) Options_ClientTextureDetail=i % 3;
    if (Parser.Found("clWinSizeX",    &i)) Options_ClientWindowSizeX=i;
    if (Parser.Found("clWinSizeY",    &i)) Options_ClientWindowSizeY=i;
    if (Parser.Found("clRenderer",    &s)) Options_ClientDesiredRenderer=s;
    if (Parser.Found("clSoundSystem", &s)) Options_ClientDesiredSoundSystem=s;
    if (Parser.Found("clPlayerName",  &s)) Options_DeathMatchPlayerName=s;
    if (Parser.Found("clModelName",   &s)) Options_DeathMatchModelName=s;

    if (!Parser.Found("svWorld") && Parser.GetParamCount()==1)
        Options_ServerWorldName=Parser.GetParam(0);

    return wxApp::OnCmdLineParsed(Parser);
}
