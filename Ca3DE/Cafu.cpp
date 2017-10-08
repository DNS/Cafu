/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

/**************************************/
/***                                ***/
/***          Cafu Engine           ***/
/***                                ***/
/*** Dass ich erkenne, was die Welt ***/
/*** im Innersten zusammenhält.     ***/
/***   -- Goethe, Faust             ***/
/***                                ***/
/**************************************/

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
#include "MainWindow/glfwLibrary.hpp"
#include "MainWindow/glfwMainWindow.hpp"
#include "MainWindow/glfwMonitor.hpp"
#include "MaterialSystem/MaterialManagerImpl.hpp"
#include "Network/Network.hpp"
#include "SoundSystem/SoundShaderManagerImpl.hpp"
#include "SoundSystem/SoundSys.hpp"
#include "PlatformAux.hpp"
#include "String.hpp"

#include "ClientMainWindow.hpp"
#include "GameInfo.hpp"

#include "GLFW/glfw3.h"
#include "tclap/CmdLine.h"
#include "tclap/StdOutput.h"


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


class ConsolesResourceT
{
    public:

    ConsolesResourceT()
        : m_ConFile(NULL)
    {
        s_CompositeConsole.Attach(&m_ConStdout);
        s_CompositeConsole.Attach(&m_ConBuffer);
    }

    ~ConsolesResourceT()
    {
        s_CompositeConsole.Detach(m_ConFile);
        s_CompositeConsole.Detach(&m_ConBuffer);
        s_CompositeConsole.Detach(&m_ConStdout);

        delete m_ConFile;
    }

    void AddFileConsole(const std::string& filename)
    {
        if (m_ConFile)
        {
            Console->Warning("A file console is already attached, not adding another one.\n");
            return;
        }

        m_ConFile = new cf::ConsoleFileT(filename);

        m_ConFile->SetAutoFlush(true);
        m_ConFile->Print(m_ConBuffer.GetBuffer());

        s_CompositeConsole.Attach(m_ConFile);
    }

    /// Returns the composite console that is also available via the global Console pointer.
    cf::CompositeConsoleT& GetConComposite() const { return s_CompositeConsole; }

    /// Returns the console that buffers all output.
    cf::ConsoleStringBufferT& GetConBuffer() { return m_ConBuffer; }

    // /// Returns the console that logs all output into a file (can be NULL if not used).
    // cf::ConsoleFileT& GetConFile() const { return *m_ConFile; }


    private:

    ConsolesResourceT(const ConsolesResourceT&);    ///< Use of the Copy    Constructor is not allowed.
    void operator = (const ConsolesResourceT&);     ///< Use of the Assignment Operator is not allowed.

    cf::ConsoleStdoutT       m_ConStdout;
    cf::ConsoleStringBufferT m_ConBuffer;   ///< The console that buffers all output in a string.
    cf::ConsoleFileT*        m_ConFile;     ///< The console that logs all output into a file (can be NULL if not used).
};


class WinSockResourceT
{
    public:

    WinSockResourceT()
    {
        try
        {
            g_WinSock = new WinSockT;
        }
        catch (const WinSockT::InitFailure&) { throw std::runtime_error("Unable to initialize WinSock 2.0." ); }
        catch (const WinSockT::BadVersion& ) { throw std::runtime_error("WinSock version 2.0 not supported."); }
    }

    ~WinSockResourceT()
    {
        delete g_WinSock;
        g_WinSock = NULL;
    }


    private:

    WinSockResourceT(const WinSockResourceT&);  ///< Use of the Copy    Constructor is not allowed.
    void operator = (const WinSockResourceT&);  ///< Use of the Assignment Operator is not allowed.
};


class ConInterpreterResourceT
{
    public:

    ConInterpreterResourceT(const char* command=NULL)
    {
        if (command)
            ConsoleInterpreter->RunCommand(command);
    }

    ~ConInterpreterResourceT()
    {
        // Setting the ConsoleInterpreter to NULL before the end of main() is important
        // to make sure that no ConFuncT or ConVarT dtor accesses the ConsoleInterpreter
        // that might already have been destroyed by then.
        ConsoleInterpreter = NULL;
    }
};


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


class CommandLineArgumentsT
{
    public:

    CommandLineArgumentsT(int argc, char* argv[], ConsolesResourceT& ConsolesRes, GameInfosT& GameInfos)
        : m_QuitNormally(false)
    {
        // Parse the command line.
        // Possible alternatives to TCLAP:
        //   - https://github.com/jarro2783/cxxopts
        //   - search for "python argparse c++"
        std::ostringstream consoleOutputStream;
        TCLAP::StdOutput   stdOutput(consoleOutputStream, consoleOutputStream);
        TCLAP::CmdLine     cmd("Cafu Engine", stdOutput, ' ', "'" __DATE__ "'");

        try
        {
            // These may throw e.g. SpecificationException, but such exceptions are easily fixed permanently.
            const TCLAP::ValueArg<std::string> argLog     ("l", "log",            "Logs all console messages into the specified file.", false, "", "filename", cmd);
            const TCLAP::ValueArg<std::string> argConsole ("c", "console",        "Runs the given commands in the console, as if appended to the config.lua file.", false, "", "lua-script", cmd);
            const TCLAP::ValueArg<std::string> argSvGame  ("g", "sv-game",        "Name of the game (MOD) that the server should run. Available: " + GameInfos.getList() + ".", false, GameInfos.getCurrentGameInfo().GetName(), "string", cmd);
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

            if (argLog.getValue() != "")
            {
                ConsolesRes.AddFileConsole(argLog.getValue());
            }

            if (!GameInfos.setGame(argSvGame.getValue()))
                throw TCLAP::ArgParseException("Unknown game \"" + argSvGame.getValue() + "\"", "sv-game");

            Options_ServerWorldName     = argSvWorld .getValue();
            Options_ServerPortNr        = argSvPort  .getValue();
            Options_ClientFullScreen    = !argClNoFS .getValue();   // TODO: --cl-no-fs is a temporary override, not a permanent setting.
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
            Console->Print(consoleOutputStream.str());
            m_QuitNormally = true;
        }
        catch (const TCLAP::ArgException& ae)
        {
            cmd.getOutput().failure(cmd, ae, true);
            throw std::runtime_error(consoleOutputStream.str());
        }
    }

    bool quitNormally() const { return m_QuitNormally; }


    private:

    bool m_QuitNormally;
};


int main(int argc, char* argv[])
{
    ConsolesResourceT ConsolesRes;

    Console->Print("Cafu Engine, " __DATE__ "\n");

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

    // SetAppName("Cafu");
    // SetAppDisplayName("Cafu Engine");
    // SetVendorName("Carsten Fuchs Software");

    try
    {
        GameInfosT GameInfos;
        ConInterpreterResourceT ConInterpreterRes("dofile('config.lua');");
        CommandLineArgumentsT CommandLineArguments(argc, argv, ConsolesRes, GameInfos);

        if (CommandLineArguments.quitNormally())    // --help or --version
            return 0;

        const std::string& gn = GameInfos.getCurrentGameInfo().GetName();

        cf::FileSys::FileMan->MountFileSystem(cf::FileSys::FS_TYPE_LOCAL_PATH, "./", "");
        // cf::FileSys::FileMan->MountFileSystem(cf::FileSys::FS_TYPE_LOCAL_PATH, "Games/" + gn + "/", "");
        // cf::FileSys::FileMan->MountFileSystem(cf::FileSys::FS_TYPE_ZIP_ARCHIVE, "Games/" + gn + "/Textures/TechDemo.zip", "Games/" + gn + "/Textures/TechDemo/", "Ca3DE");
        // cf::FileSys::FileMan->MountFileSystem(cf::FileSys::FS_TYPE_ZIP_ARCHIVE, "Games/" + gn + "/Textures/SkyDomes.zip", "Games/" + gn + "/Textures/SkyDomes/", "Ca3DE");

        MaterialManager->RegisterMaterialScriptsInDir("Games/" + gn + "/Materials", "Games/" + gn + "/");
        SoundShaderManager->RegisterSoundShaderScriptsInDir("Games/" + gn + "/SoundShader", "Games/" + gn + "/");

        WinSockResourceT WinSockRes;
        cf::glfwLibraryT glfwLib;

#if 0
        int num_monitors = 0;
        GLFWmonitor** monitors = glfwGetMonitors(&num_monitors);

        Console->Print("Monitors:\n");
        for (int i = 0; i < num_monitors; i++)
        {
            cf::glfwMonitorT mon(monitors[i]);
            mon.printAllModes();
        }
#endif

        // Note that the format of the VideoModes string is fixed - it is parsed by the Main Menu GUI in order to populate the choice box.
        const std::string vid_modes = cf::glfwMonitorT(glfwGetPrimaryMonitor()).getUserChoiceList();
        static ConVarT VideoModes("VideoModes", vid_modes, ConVarT::FLAG_MAIN_EXE | ConVarT::FLAG_READ_ONLY, "The list of video modes that are available on your system.");

        // The default values for the window creations hints look just right for our purposes,
        // see http://www.glfw.org/docs/latest/window_guide.html#window_hints_values for details.
        ClientMainWindowT win(GameInfos.getCurrentGameInfo(),
                              Options_ClientWindowSizeX.GetValueInt(),
                              Options_ClientWindowSizeY.GetValueInt(),
                              "Cafu Engine",
                              Options_ClientFullScreen.GetValueBool() ? glfwGetPrimaryMonitor() : NULL);
        glfwMainWindowT   MainWin(win, ClientMainWindowT::getGlfwKey);

        win.makeContextCurrent();
        win.Init(ConsolesRes.GetConComposite(), ConsolesRes.GetConBuffer().GetBuffer(), MainWin);
        win.triggerFramebufferSizeEvent();

        glfwSwapInterval(1);   // enable v-sync

        while (!win.shouldClose())
        {
            win.runFrame();
            glfwPollEvents();
        }
    }
    catch (const std::runtime_error& re)
    {
        // Exceptions from CommandLineArgumentsT already start with "\nError".
        if (!cf::String::startsWith(re.what(), "\nError"))
            Console->Print("ERROR: ");

        // Print to stderr instead? Into a message box?
        Console->Print(re.what());
        return -1;
    }

    return 0;
}
