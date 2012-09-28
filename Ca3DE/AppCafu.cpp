/*
=================================================================================
This file is part of Cafu, the open-source game engine and graphics engine
for multiplayer, cross-platform, real-time 3D action.
Copyright (C) 2002-2012 Carsten Fuchs Software.

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
#include "ConsoleCommands/ConsoleComposite.hpp"
#include "ConsoleCommands/ConsoleFile.hpp"
#include "ConsoleCommands/ConsoleStdout.hpp"
#include "ConsoleCommands/ConsoleStringBuffer.hpp"
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
#include "../Games/GameInfo.hpp"

#include "wx/cmdline.h"
#include "wx/filename.h"
#include "wx/msgdlg.h"
#include "wx/stdpaths.h"


// For each interface that is globally available to the application,
// provide a definition for the pointer instance and have it point to an implementation.
static cf::CompositeConsoleT s_CompositeConsole;
cf::ConsoleI* Console=&s_CompositeConsole;

static cf::FileSys::FileManImplT s_FileManImpl;
cf::FileSys::FileManI* cf::FileSys::FileMan=&s_FileManImpl;

static cf::ClipSys::CollModelManImplT s_CCM;
cf::ClipSys::CollModelManI* cf::ClipSys::CollModelMan=&s_CCM;

static ConsoleInterpreterImplT s_ConInterpreterImpl;
ConsoleInterpreterI* ConsoleInterpreter=&s_ConInterpreterImpl;      // TODO: Put it into a proper namespace.

static MaterialManagerImplT s_MaterialManagerImpl;
MaterialManagerI* MaterialManager=&s_MaterialManagerImpl;           // TODO: Put it into a proper namespace.

static SoundShaderManagerImplT s_SoundShaderManagerImpl;
SoundShaderManagerI* SoundShaderManager=&s_SoundShaderManagerImpl;  // TODO: Put it into a proper namespace.

// static WinSockT WinSock;     // This unfortunately can throw.
WinSockT* g_WinSock=NULL;


// Implementations for these interfaces are obtained later at run-time.
// MatSys::RendererI* MatSys::Renderer=NULL;                    // TODO: Don't have it predefine the global pointer instance.
// MatSys::TextureMapManagerI* MatSys::TextureMapManager=NULL;  // TODO: Don't have it predefine the global pointer instance.
SoundSysI* SoundSystem=NULL;
cf::GuiSys::GuiManI* cf::GuiSys::GuiMan=NULL;


static bool CompareModes(const wxVideoMode& Mode1, const wxVideoMode& Mode2)
{
    // Compare the widths.
    if (Mode1.w < Mode2.w) return true;
    if (Mode1.w > Mode2.w) return false;

    // The widths are equal, now compare the heights.
    if (Mode1.h < Mode2.h) return true;
    if (Mode1.h > Mode2.h) return false;

    // The widths and heights are equal, now compare the BPP.
    if (Mode1.bpp < Mode2.bpp) return true;
    if (Mode1.bpp > Mode2.bpp) return false;

    // The widths, heights and BPPs are equal, now compare the refresh rate.
    if (Mode1.refresh < Mode2.refresh) return true;
    if (Mode1.refresh > Mode2.refresh) return false;

    // The modes are equal.
    return false;
}


static std::string GetVideoModes()
{
    ArrayT<wxVideoMode> Modes;

    {
        wxDisplay         Display;
        wxArrayVideoModes wxModes=Display.GetModes();

        for (size_t ModeNr=0; ModeNr<wxModes.GetCount(); ModeNr++)
            Modes.PushBack(wxModes[ModeNr]);
    }

    // Remove modes according to certain filter criteria, cutting excessively long mode lists.
    for (unsigned long ModeNr=0; ModeNr<Modes.Size(); ModeNr++)
    {
        const wxVideoMode& Mode=Modes[ModeNr];

        if (Mode.w==0 || Mode.h==0 || Mode.bpp<15)
        {
            Modes.RemoveAt(ModeNr);
            ModeNr--;
            continue;
        }

        for (unsigned long OtherNr=0; OtherNr<Modes.Size(); OtherNr++)
        {
            if (OtherNr==ModeNr) continue;

            const wxVideoMode& Other=Modes[OtherNr];

            if (Mode==Other || (Mode.w==Other.w && Mode.h==Other.h && Mode.bpp<32 && Mode.bpp<Other.bpp))
            {
                Modes.RemoveAt(ModeNr);
                ModeNr--;
                break;
            }
        }

        // Note that the above loop is written in a way that allows no additional statements here.
    }

    // Sort the modes by increasing width, height, BPP and refresh rate.
    Modes.QuickSort(CompareModes);

    // Build the result string.
    wxString List;

    for (unsigned long ModeNr=0; ModeNr<Modes.Size(); ModeNr++)
    {
        const wxVideoMode& Mode=Modes[ModeNr];

        List+=wxString::Format("%i x %i, %i bpp, %i Hz\n", Mode.w, Mode.h, Mode.bpp, Mode.refresh);
    }

    return List.ToStdString();
}


IMPLEMENT_APP(AppCafuT)


AppCafuT::AppCafuT()
    : wxApp(),
      m_Locale(NULL),
      m_ConBuffer(new cf::ConsoleStringBufferT()),
      m_ConFile(NULL),
      m_AllGameInfos(),
      m_GameInfo(NULL),
      m_IsCustomVideoMode(false),
      m_MainFrame(NULL)
{
    s_CompositeConsole.Attach(m_ConBuffer);

    #ifdef __WXGTK__
    {
        static cf::ConsoleStdoutT s_ConStdout;
        s_CompositeConsole.Attach(&s_ConStdout);
    }
    #endif

    // All global convars and confuncs have registered themselves in linked lists.
    // Register them with the console interpreter now.
    ConFuncT::RegisterStaticList();
    ConVarT ::RegisterStaticList();

    // The one-time init of the GuiSys' windows type info manager.
    cf::GuiSys::GetWindowTIM().Init();

    // Gather infos about all available games.
    InitGameInfos();

    SetAppName("Cafu");
    SetAppDisplayName("Cafu Engine");
    SetVendorName("Carsten Fuchs Software");
    wxStandardPaths::Get().UseAppInfo(wxStandardPaths::AppInfo_VendorName | wxStandardPaths::AppInfo_AppName);

    Console->Print("Cafu Engine, "__DATE__"\n");
}


AppCafuT::~AppCafuT()
{
    for (unsigned int i = 0; i < m_AllGameInfos.Size(); i++)
        delete m_AllGameInfos[i];

    s_CompositeConsole.Detach(m_ConFile);
    delete m_ConFile;

    s_CompositeConsole.Detach(m_ConBuffer);
    delete m_ConBuffer;
}


cf::CompositeConsoleT& AppCafuT::GetConComposite() const
{
    return s_CompositeConsole;
}


bool AppCafuT::OnInit()
{
    const wxString UserDataDir=wxStandardPaths::Get().GetUserDataDir();

    if (!wxFileName::Mkdir(UserDataDir, 0777, wxPATH_MKDIR_FULL))
        wxMessageBox(wxString("Config file storage path \n")+UserDataDir+"\n doesn't exist, and it could not be created, either.", "Warning!");

    // Undo the wx locale initialization, as we want to be sure to use the same (default) locale "C" always and everywhere.
    // Using other locales introduces a lot of subtle errors. E.g. reading floating point numbers from anywhere
    // (like map files!) fails because e.g. "1.4" is no proper floating point string in the German locale (but "1,4" is).
    // setlocale(LC_ALL, "C");      // This alone is not enough, see http://trac.wxwidgets.org/ticket/12970 for details.
    for (int LangNr=wxLANGUAGE_ENGLISH; LangNr<=wxLANGUAGE_ENGLISH_ZIMBABWE; LangNr++)
    {
        if (wxLocale::IsAvailable(LangNr))
        {
            m_Locale=new wxLocale(LangNr, wxLOCALE_DONT_LOAD_DEFAULT);

            wxLogDebug("Program locale set to %s (%s, %s).", m_Locale->GetName(), m_Locale->GetCanonicalName(), m_Locale->GetLocale());
            break;
        }
    }

    if (!m_Locale)
    {
        // If the above for some reason didn't work, set at least the CRT to the "C" locale.
        setlocale(LC_ALL, "C");
        wxLogDebug("Program locale set to \"C\".");
    }

    ConsoleInterpreter->RunCommand("dofile('config.lua');");

    // Parse the command line.
    if (!wxApp::OnInit()) { OnExit(); return false; }

    try
    {
        g_WinSock=new WinSockT;
    }
    catch (const WinSockT::InitFailure& /*E*/) { wxMessageBox("Unable to initialize WinSock 2.0." ); OnExit(); return false; }
    catch (const WinSockT::BadVersion&  /*E*/) { wxMessageBox("WinSock version 2.0 not supported."); OnExit(); return false; }


    assert(m_GameInfo);

    cf::FileSys::FileMan->MountFileSystem(cf::FileSys::FS_TYPE_LOCAL_PATH, "./", "");
 // cf::FileSys::FileMan->MountFileSystem(cf::FileSys::FS_TYPE_LOCAL_PATH, "Games/" + m_GameInfo->GetName() + "/", "");
    cf::FileSys::FileMan->MountFileSystem(cf::FileSys::FS_TYPE_ZIP_ARCHIVE, "Games/" + m_GameInfo->GetName() + "/Textures/TechDemo.zip", "Games/" + m_GameInfo->GetName() + "/Textures/TechDemo/", "Ca3DE");
    cf::FileSys::FileMan->MountFileSystem(cf::FileSys::FS_TYPE_ZIP_ARCHIVE, "Games/" + m_GameInfo->GetName() + "/Textures/SkyDomes.zip", "Games/" + m_GameInfo->GetName() + "/Textures/SkyDomes/", "Ca3DE");

    MaterialManager->RegisterMaterialScriptsInDir("Games/" + m_GameInfo->GetName() + "/Materials", "Games/" + m_GameInfo->GetName() + "/");
    SoundShaderManager->RegisterSoundShaderScriptsInDir("Games/" + m_GameInfo->GetName() + "/SoundShader", "Games/" + m_GameInfo->GetName() + "/");


    // The console variable VideoModes is initialized here, because under wxGTK, using wxDisplay requires
    // that the wxWidgets library (and thus GTK) is initialized first.
    // Note that the format of the VideoModes string is fixed - it is parsed by the Main Menu GUI in order to populate the choice box.
    static ConVarT VideoModes("VideoModes", GetVideoModes(), ConVarT::FLAG_MAIN_EXE | ConVarT::FLAG_READ_ONLY, "The list of video modes that are available on your system.");


    // Change the video mode. Although the two actions
    //
    //     (a) change the screen resolution (video mode) and
    //     (b) show the Cafu window full-screen
    //
    // are theoretically independent of each other, case "(a) but not (b)" does not make sense at all;
    // case "not (a) but (b)" makes more sense but is atypical as well, and is easily switched from case
    // "neither (a) nor (b)" by pressing F11. Thus, we are effectively only concerned by two cases:
    //
    //     (1) windowed mode:    "neither (a) nor (b)"
    //     (2) full-screen mode: "both (a) and (b)"
    //
    // For case (1), we simply open a normal application window with dimensions Options_ClientWindowSize[X/Y].
    // For case (2), we change the video mode as specified by Options_Client* and show the application
    // window full-screen.
    wxDisplay      Display;
    extern ConVarT Options_ClientFullScreen;

    if (Options_ClientFullScreen.GetValueBool())
    {
        extern ConVarT Options_ClientWindowSizeX;
        extern ConVarT Options_ClientWindowSizeY;
        extern ConVarT Options_ClientDisplayBPP;
        extern ConVarT Options_ClientDisplayRefresh;

        const wxVideoMode VideoMode1(Options_ClientWindowSizeX.GetValueInt(),
                                     Options_ClientWindowSizeY.GetValueInt(),
                                     Options_ClientDisplayBPP.GetValueInt(),
                                     Options_ClientDisplayRefresh.GetValueInt());

        const wxVideoMode VideoMode2(Options_ClientWindowSizeX.GetValueInt(),
                                     Options_ClientWindowSizeY.GetValueInt(),
                                     Options_ClientDisplayBPP.GetValueInt(), 0);

        const wxVideoMode VideoMode3(Options_ClientWindowSizeX.GetValueInt(),
                                     Options_ClientWindowSizeY.GetValueInt(), 0, 0);

        if (Display.ChangeMode(VideoMode1))
        {
            m_IsCustomVideoMode=true;
        }
        else if (Display.ChangeMode(VideoMode2))
        {
            Options_ClientDisplayRefresh.SetValue(Display.GetCurrentMode().refresh);
            m_IsCustomVideoMode=true;
        }
        else if (Display.ChangeMode(VideoMode3))
        {
            Options_ClientDisplayBPP.SetValue(Display.GetCurrentMode().bpp);
            Options_ClientDisplayRefresh.SetValue(Display.GetCurrentMode().refresh);
            m_IsCustomVideoMode=true;
        }
        else
        {
            wxMessageBox("Cafu tried to change the video mode to\n"+
                wxString::Format("        %i x %i, %i bpp at %i Hz,\n", VideoMode1.w, VideoMode1.h, VideoMode1.bpp, VideoMode1.refresh)+
                wxString::Format("        %i x %i, %i bpp at any refresh rate,\n", VideoMode2.w, VideoMode2.h, VideoMode2.bpp)+
                wxString::Format("        %i x %i at any color depth and refresh rate,\n", VideoMode3.w, VideoMode3.h)+
                "but it didn't work out (zero values mean system defaults).\n"+
                "We will continue with the currently active video mode instead, where you can press F11 to toggle full-screen mode.\n\n"+
                "Alternatively, you can set a different video mode at the Options menu, or tweak the video mode details via the console variables.\n",
                "Could not change the video mode", wxOK | wxICON_EXCLAMATION);

            Options_ClientFullScreen.SetValue(false);
        }
    }

    m_CurrentMode=Display.GetCurrentMode();

    if (m_CurrentMode.w==0) { m_CurrentMode.w=wxGetDisplaySize().x; wxLogDebug("Set m_CurrentMode.w from 0 to %i", m_CurrentMode.w); }
    if (m_CurrentMode.h==0) { m_CurrentMode.h=wxGetDisplaySize().y; wxLogDebug("Set m_CurrentMode.h from 0 to %i", m_CurrentMode.h); }

    if (m_CurrentMode.w<200) { m_CurrentMode.w=1024; wxLogDebug("Set m_CurrentMode.w from <200 to %i", m_CurrentMode.w); }
    if (m_CurrentMode.h<150) { m_CurrentMode.h= 768; wxLogDebug("Set m_CurrentMode.h from <150 to %i", m_CurrentMode.h); }


    // Create the main frame.
    m_MainFrame=new MainFrameT(m_GameInfo);
    SetTopWindow(m_MainFrame);

    return true;
}


int AppCafuT::OnExit()
{
    if (m_IsCustomVideoMode)
    {
        wxDisplay Display;

        // Reset the display to default (desktop) video mode.
        Display.ChangeMode();
    }

    delete g_WinSock;
    g_WinSock=NULL;

    // Setting the ConsoleInterpreter to NULL is very important, to make sure that no ConFuncT
    // or ConVarT dtor accesses the ConsoleInterpreter that might already have been destroyed then.
    ConsoleInterpreter=NULL;

    delete m_Locale;
    m_Locale=NULL;

    return wxApp::OnExit();
}


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


#define GAME_NAME DeathMatch
#include "../Games/DeathMatch/Code/GameInfo.hpp"
#undef GAME_NAME

void AppCafuT::InitGameInfos()
{
    m_AllGameInfos.PushBack(new DeathMatch::GameInfoT());

    // TODO: Make sure that m_AllGameInfos.Size() > 0.
    // TODO: Sort m_AllGameInfos by "priority"?
}


void AppCafuT::OnInitCmdLine(wxCmdLineParser& Parser)
{
    std::string GamesList;

    for (unsigned int i = 0; i < m_AllGameInfos.Size(); i++)
    {
        if (i > 0) GamesList += ", ";
        GamesList += m_AllGameInfos[i]->GetName();
    }

    Parser.AddOption("con",            "", "Runs the given commands in the console, as if appended to the config.lua file.", wxCMD_LINE_VAL_STRING);
    Parser.AddOption("svGame",         "", "Name of the game (MOD) that the server should run [" + m_AllGameInfos[0]->GetName() + "]. Available: " + GamesList + ".", wxCMD_LINE_VAL_STRING);
    Parser.AddOption("svWorld",        "", "Name of the world that the server should run ["+Options_ServerWorldName.GetValueString()+"]. Case sensitive!", wxCMD_LINE_VAL_STRING);
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
    Parser.AddOption("log",            "", "Logs all console message into the specified file.");
    Parser.AddParam("worldname", wxCMD_LINE_VAL_STRING, wxCMD_LINE_PARAM_OPTIONAL);

    wxApp::OnInitCmdLine(Parser);
    Parser.AddUsageText("\nDefault values are enclosed in [brackets].");
    Parser.AddUsageText("It is also ok to omit the \"-svWorld\" option for specifying a world name: \"Cafu MyWorld\" is the same as \"Cafu -svWorld MyWorld\".\n");
    Parser.SetSwitchChars("-");
}


bool AppCafuT::OnCmdLineParsed(wxCmdLineParser& Parser)
{
    long int i=0;
    wxString s="";

    m_GameInfo = m_AllGameInfos[0];

    if (Parser.Found("con",           &s)) ConsoleInterpreter->RunCommand(s.ToStdString());
    if (Parser.Found("svWorld",       &s)) Options_ServerWorldName=s.ToStdString();
    if (Parser.Found("svPort",        &i)) Options_ServerPortNr=int(i);
    if (Parser.Found("noFS")             ) Options_ClientFullScreen=false;
    if (Parser.Found("clPort",        &i)) Options_ClientPortNr=int(i);
    if (Parser.Found("clRemoteName",  &s)) Options_ClientRemoteName=s.ToStdString();
    if (Parser.Found("clRemotePort",  &i)) Options_ClientRemotePortNr=int(i);
    if (Parser.Found("clTexDetail",   &i)) Options_ClientTextureDetail=int(i % 3);
    if (Parser.Found("clWinSizeX",    &i)) Options_ClientWindowSizeX=int(i);
    if (Parser.Found("clWinSizeY",    &i)) Options_ClientWindowSizeY=int(i);
    if (Parser.Found("clRenderer",    &s)) Options_ClientDesiredRenderer=s.ToStdString();
    if (Parser.Found("clSoundSystem", &s)) Options_ClientDesiredSoundSystem=s.ToStdString();
    if (Parser.Found("clPlayerName",  &s)) Options_DeathMatchPlayerName=s.ToStdString();
    if (Parser.Found("clModelName",   &s)) Options_DeathMatchModelName=s.ToStdString();

    if (Parser.Found("svGame", &s))
    {
        unsigned int i = 0;

        for (i = 0; i < m_AllGameInfos.Size(); i++)
            if (s == m_AllGameInfos[i]->GetName())
            {
                m_GameInfo = m_AllGameInfos[i];
                break;
            }

        if (i >= m_AllGameInfos.Size())
        {
            // Game not found, but for clarity, *don't* silently use the default instead.
            wxMessageBox("svGame: Unknown game \"" + s.ToStdString() + "\".\n\nUse --help to see a list of available games.\n", "Unknown game", wxOK | wxICON_EXCLAMATION);
            return false;
        }
    }

    if (Parser.Found("log", &s) && m_ConFile==NULL)
    {
        m_ConFile=new cf::ConsoleFileT(s.ToStdString());
        m_ConFile->SetAutoFlush(true);
        m_ConFile->Print(m_ConBuffer->GetBuffer());

        s_CompositeConsole.Attach(m_ConFile);
    }

    if (!Parser.Found("svWorld") && Parser.GetParamCount()==1)
        Options_ServerWorldName=Parser.GetParam(0).ToStdString();

    return wxApp::OnCmdLineParsed(Parser);
}
