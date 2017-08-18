/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
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
#include "GameSys/AllComponents.hpp"
#include "GameSys/Entity.hpp"
#include "GameSys/World.hpp"
#include "GuiSys/AllComponents.hpp"
#include "GuiSys/GuiManImpl.hpp"
#include "GuiSys/GuiImpl.hpp"
#include "GuiSys/Window.hpp"
#include "MaterialSystem/MaterialManagerImpl.hpp"
#include "Network/Network.hpp"
#include "PlatformAux.hpp"
#include "SoundSystem/SoundShaderManagerImpl.hpp"
#include "SoundSystem/SoundSys.hpp"
#include "String.hpp"
#include "TypeSys.hpp"

#include "tclap/CmdLine.h"
#include "tclap/StdOutput.h"
#include "wx/msgdlg.h"


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
cf::GuiSys::GuiManImplT* cf::GuiSys::GuiMan=NULL;


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
      m_GameInfos(),    // This might throw an std::runtime_error that we don't catch anywhere. Also see CafuTest.cxx for the intended approach.
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

    cf::GameSys::GetComponentTIM().Init();      // The one-time init of the GameSys components type info manager.
    cf::GameSys::GetGameSysEntityTIM().Init();  // The one-time init of the GameSys entity type info manager.
    cf::GameSys::GetWorldTIM().Init();          // The one-time init of the GameSys world type info manager.

    cf::GuiSys::GetComponentTIM().Init();       // The one-time init of the GuiSys components type info manager.
    cf::GuiSys::GetWindowTIM().Init();          // The one-time init of the GuiSys window type info manager.
    cf::GuiSys::GetGuiTIM().Init();             // The one-time init of the GuiSys GUI type info manager.

    SetAppName("Cafu");
    SetAppDisplayName("Cafu Engine");
    SetVendorName("Carsten Fuchs Software");

    Console->Print("Cafu Engine, " __DATE__ "\n");
}


AppCafuT::~AppCafuT()
{
    s_CompositeConsole.Detach(m_ConFile);
    delete m_ConFile;

    s_CompositeConsole.Detach(m_ConBuffer);
    delete m_ConBuffer;
}


cf::CompositeConsoleT& AppCafuT::GetConComposite() const
{
    return s_CompositeConsole;
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
extern ConVarT Options_PlayerName;
extern ConVarT Options_PlayerModelName;


bool AppCafuT::OnInit()
{
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
    std::ostringstream consoleOutputStream;
    TCLAP::StdOutput   stdOutput(consoleOutputStream, consoleOutputStream);
    TCLAP::CmdLine     cmd("Cafu Engine", stdOutput, ' ', "'" __DATE__ "'");
    bool               forceWindowMode = false;

    try
    {
        // These may throw e.g. SpecificationException, but such exceptions are easily fixed permanently.
        const TCLAP::ValueArg<std::string> argLog     ("l", "log",            "Logs all console messages into the specified file.", false, "", "filename", cmd);
        const TCLAP::ValueArg<std::string> argConsole ("c", "console",        "Runs the given commands in the console, as if appended to the config.lua file.", false, "", "lua-script", cmd);
        const TCLAP::ValueArg<std::string> argSvGame  ("g", "sv-game",        "Name of the game (MOD) that the server should run. Available: " + m_GameInfos.getList() + ".", false, m_GameInfos.getCurrentGameInfo().GetName(), "string", cmd);
        const TCLAP::ValueArg<std::string> argSvWorld ("w", "sv-world",       "Name of the world that the server should run. Case sensitive!", false, Options_ServerWorldName.GetValueString(), "string", cmd);
        const TCLAP::ValueArg<int>         argSvPort  ("o", "sv-port",        "Server port number.", false, Options_ServerPortNr.GetValueInt(), "number", cmd);
        const TCLAP::SwitchArg             argClNoFS  ("n", "cl-no-fs",       "Don't switch to full-screen, use a plain window instead.", cmd);
        const TCLAP::ValueArg<int>         argClPort  ("",  "cl-port",        "Client port number.", false, Options_ClientPortNr.GetValueInt(), "number", cmd);
        const TCLAP::ValueArg<std::string> argClRmName("",  "cl-remote-name", "Name or IP of the server to connect to.", false, Options_ClientRemoteName.GetValueString(), "string", cmd);
        const TCLAP::ValueArg<int>         argClRmPort("",  "cl-remote-port", "Port number of the remote server.", false, Options_ClientRemotePortNr.GetValueInt(), "number", cmd);
        const TCLAP::ValueArg<int>         argClTexDt ("d", "cl-tex-detail",  "0 for high detail, 1 for medium detail, 2 for low detail.", false, Options_ClientTextureDetail.GetValueInt(), "number", cmd);
        const TCLAP::ValueArg<int>         argClWinX  ("x", "cl-win-x",       "If not full-screen, this is the width  of the window.", false, Options_ClientWindowSizeX.GetValueInt(), "number", cmd);
        const TCLAP::ValueArg<int>         argClWinY  ("y", "cl-win-y",       "If not full-screen, this is the height of the window.", false, Options_ClientWindowSizeY.GetValueInt(), "number", cmd);
        const TCLAP::ValueArg<std::string> argClRend  ("r", "cl-renderer",    "Overrides the auto-selection of the best available renderer.", false, "[auto]", "string", cmd);
        const TCLAP::ValueArg<std::string> argClSound ("s", "cl-soundsystem", "Overrides the auto-selection of the best available sound system.", false, "[auto]", "string", cmd);
        const TCLAP::ValueArg<std::string> argClPlayer("p", "cl-playername",  "Player name.", false, Options_PlayerName.GetValueString(), "string", cmd);
        const TCLAP::ValueArg<std::string> argClModel ("m", "cl-modelname",   "Name of the player's model.", false, Options_PlayerModelName.GetValueString(), "string", cmd);

        TCLAP::VersionVisitor vv(&cmd, stdOutput);
        const TCLAP::SwitchArg argVersion("",  "version", "Displays version information and exits.", cmd, false, &vv);

        TCLAP::HelpVisitor hv(&cmd, stdOutput);
        const TCLAP::SwitchArg argHelp("h", "help", "Displays usage information and exits.", cmd, false, &hv);

        cmd.parse(argc, argv);

        if (argConsole.getValue() != "")
        {
            ConsoleInterpreter->RunCommand(argConsole.getValue());
        }

        if (argLog.getValue() != "" && m_ConFile == NULL)
        {
            m_ConFile = new cf::ConsoleFileT(argLog.getValue());
            m_ConFile->SetAutoFlush(true);
            m_ConFile->Print(m_ConBuffer->GetBuffer());

            s_CompositeConsole.Attach(m_ConFile);
        }

        if (true)
        {
            if (!m_GameInfos.setGame(argSvGame.getValue()))
                throw TCLAP::ArgParseException("Unknown game \"" + argSvGame.getValue() + "\"", "sv-game");
        }

        Options_ServerWorldName     = argSvWorld .getValue();
        Options_ServerPortNr        = argSvPort  .getValue();
        forceWindowMode             = argClNoFS  .getValue();   // --cl-no-fs is a temporary override, not a permanent setting.
        Options_ClientPortNr        = argClPort  .getValue();
        Options_ClientRemoteName    = argClRmName.getValue();
        Options_ClientRemotePortNr  = argClRmPort.getValue();
        Options_ClientTextureDetail = argClTexDt .getValue() % 3;
        Options_ClientWindowSizeX   = argClWinX  .getValue();
        Options_ClientWindowSizeY   = argClWinY  .getValue();
        if (argClRend.getValue()  != "[auto]") Options_ClientDesiredRenderer    = argClRend.getValue();
        if (argClSound.getValue() != "[auto]") Options_ClientDesiredSoundSystem = argClSound.getValue();
        Options_PlayerName          = argClPlayer.getValue();
        Options_PlayerModelName     = argClModel .getValue();
    }
    catch (const TCLAP::ExitException&)
    {
        //  ExitException is thrown after --help or --version was handled.
        std::string s = consoleOutputStream.str();
        s = cf::String::Replace(s, "\nUsage:", "Usage:");   // Hack: Reduce the output's height.
        s = cf::String::Replace(s, "\n\n", "\n");

        wxMessageBox(s, "Cafu Engine", wxOK);
        // exit(ee.getExitStatus());
        OnExit();
        return false;
    }
    catch (const TCLAP::ArgException& ae)
    {
        cmd.getOutput().failure(cmd, ae, true);
        std::string s = consoleOutputStream.str();
        s = cf::String::Replace(s, "\nError:", "Error:");   // Hack: Reduce the output's height.

        wxMessageBox(s, "Cafu Engine", wxOK | wxICON_EXCLAMATION);
        // exit(-1);
        OnExit();
        return false;
    }

    try
    {
        g_WinSock=new WinSockT;
    }
    catch (const WinSockT::InitFailure& /*E*/) { wxMessageBox("Unable to initialize WinSock 2.0." ); OnExit(); return false; }
    catch (const WinSockT::BadVersion&  /*E*/) { wxMessageBox("WinSock version 2.0 not supported."); OnExit(); return false; }


    const std::string& gn = m_GameInfos.getCurrentGameInfo().GetName();

    cf::FileSys::FileMan->MountFileSystem(cf::FileSys::FS_TYPE_LOCAL_PATH, "./", "");
 // cf::FileSys::FileMan->MountFileSystem(cf::FileSys::FS_TYPE_LOCAL_PATH, "Games/" + gn + "/", "");
    // cf::FileSys::FileMan->MountFileSystem(cf::FileSys::FS_TYPE_ZIP_ARCHIVE, "Games/" + gn + "/Textures/TechDemo.zip", "Games/" + gn + "/Textures/TechDemo/", "Ca3DE");
    // cf::FileSys::FileMan->MountFileSystem(cf::FileSys::FS_TYPE_ZIP_ARCHIVE, "Games/" + gn + "/Textures/SkyDomes.zip", "Games/" + gn + "/Textures/SkyDomes/", "Ca3DE");

    MaterialManager->RegisterMaterialScriptsInDir("Games/" + gn + "/Materials", "Games/" + gn + "/");
    SoundShaderManager->RegisterSoundShaderScriptsInDir("Games/" + gn + "/SoundShader", "Games/" + gn + "/");


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

    if (Options_ClientFullScreen.GetValueBool() && !forceWindowMode)
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
    m_MainFrame=new MainFrameT(m_GameInfos.getCurrentGameInfo());
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
