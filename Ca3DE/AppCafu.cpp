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

    // Create the main frame.
    m_MainFrame=new MainFrameT();
    SetTopWindow(m_MainFrame);

    return wxApp::OnInit();
}


int AppCafu::OnExit()
{
    delete g_WinSock;
    g_WinSock=NULL;

    // Setting the ConsoleInterpreter to NULL is very important, to make sure that no ConFuncT
    // or ConVarT dtor accesses the ConsoleInterpreter that might already have been destroyed then.
    ConsoleInterpreter=NULL;

    return wxApp::OnExit();
}
