/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "Resources.hpp"
#include "GameInfo.hpp"

#include "Client/Client.hpp"
#include "Client/CompClient.hpp"
#include "Server/Server.hpp"

#include "ClipSys/CollisionModelMan.hpp"    // Only needed for an assert() below.
#include "ConsoleCommands/Console.hpp"
// #include "ConsoleCommands/ConsoleInterpreter.hpp"
#include "ConsoleCommands/ConsoleComposite.hpp"
// #include "ConsoleCommands/ConsoleStringBuffer.hpp"
#include "ConsoleCommands/ConVar.hpp"
#include "GuiSys/CompText.hpp"
#include "GuiSys/ConsoleByWindow.hpp"
#include "GuiSys/GuiImpl.hpp"
#include "GuiSys/GuiManImpl.hpp"
#include "GuiSys/GuiResources.hpp"
#include "GuiSys/Window.hpp"
// #include "MaterialSystem/MaterialManager.hpp"
#include "MaterialSystem/Renderer.hpp"
#include "MaterialSystem/TextureMap.hpp"
#include "Models/ModelManager.hpp"
// #include "SoundSystem/SoundShaderManager.hpp"
#include "SoundSystem/SoundSys.hpp"
#include "PlatformAux.hpp"
#include "UniScriptState.hpp"
#include "String.hpp"

#ifndef _WIN32
#include <dlfcn.h>
// #define __stdcall
// #define GetProcAddress dlsym
#define FreeLibrary dlclose
#endif


class SvGuiCallbT : public ServerT::GuiCallbackI
{
    public:

    SvGuiCallbT()
        : MainMenuGui(NULL)
    {
    }

    void OnServerStateChanged(const char* NewState) const
    {
        if (MainMenuGui.IsNull()) return;

        MainMenuGui->GetScriptState().Call("OnServerStateChanged", "s", NewState);
    }

    IntrusivePtrT<cf::GuiSys::GuiImplT> MainMenuGui;
};


ResourcesT::ResourcesT(cf::CompositeConsoleT& CC, const std::string& ConsoleText, const GameInfoT& GameInfo, MainWindowT& MainWin)
    : m_CompositeConsole(CC),
      m_RendererDLL(NULL),
      m_ModelManager(NULL),
      m_GuiResources(NULL),
      m_SoundSysDLL(NULL),
      m_Client(NULL),
      m_Server(NULL),
      m_SvGuiCallback(NULL),
      m_ConByGuiWin(NULL)
{
    extern ConVarT Options_ClientDesiredRenderer;
    extern ConVarT Options_ClientTextureDetail;
    extern ConVarT Options_ClientDesiredSoundSystem;

    // Obtain the specified MatSys renderer (or if none is specified, automatically find the "best").
    const std::string& RendererName = Options_ClientDesiredRenderer.GetValueString();

    if (RendererName != "" && !cf::String::startsWith(RendererName, "#"))
        MatSys::Renderer = PlatformAux::GetRenderer(RendererName, m_RendererDLL);

    if (MatSys::Renderer==NULL || m_RendererDLL==NULL)
        MatSys::Renderer=PlatformAux::GetBestRenderer(m_RendererDLL);

    if (MatSys::Renderer==NULL || m_RendererDLL==NULL)
    {
        Cleanup();
        throw std::runtime_error("Could not find a renderer that is supported on your system.");
    }

    MatSys::Renderer->Initialize();


    // Obtain the texture map manager from the previously loaded renderer DLL.
    MatSys::TextureMapManager=PlatformAux::GetTextureMapManager(m_RendererDLL);

    if (MatSys::TextureMapManager==NULL)
    {
        Cleanup();
        throw std::runtime_error("Could not get the TextureMapManager from the renderer DLL.");
    }

    switch (Options_ClientTextureDetail.GetValueInt())
    {
        case 1: MatSys::TextureMapManager->SetMaxTextureSize(256); break;
        case 2: MatSys::TextureMapManager->SetMaxTextureSize(128); break;
    }


    // Initialize the model manager and the GUI resources.
    m_ModelManager=new ModelManagerT();
    m_GuiResources=new cf::GuiSys::GuiResourcesT(*m_ModelManager);


    // Initialize the sound system.
    const std::string& SoundSysName = Options_ClientDesiredSoundSystem.GetValueString();

    if (SoundSysName != "" && !cf::String::startsWith(SoundSysName, "#"))
        SoundSystem=PlatformAux::GetSoundSys(SoundSysName, m_SoundSysDLL);

    if (SoundSystem==NULL || m_SoundSysDLL==NULL)
        SoundSystem=PlatformAux::GetBestSoundSys(m_SoundSysDLL);

    if (SoundSystem==NULL || m_SoundSysDLL==NULL)
    {
        Cleanup();
        throw std::runtime_error("Could not find a sound system that is supported on your system.");
    }

    if (!SoundSystem->Initialize())
    {
        Console->Print("WARNING: Sound system failed to initialize!\n");
        Console->Print("Sorry, but this is probably due to sound driver problems with your computer.\n");
        Console->Print("We'll proceed anyway, with sound effects disabled.\n");

        SoundSystem=NULL;
        FreeLibrary(m_SoundSysDLL);

#ifdef SCONS_BUILD_DIR
        #define QUOTE(str) QUOTE_HELPER(str)
        #define QUOTE_HELPER(str) #str
#ifdef _WIN32
        const std::string NullPaths[]={ std::string("Libs/")+QUOTE(SCONS_BUILD_DIR)+"/SoundSystem/SoundSysNull.dll", "./SoundSysNull.dll" };
#else
        const std::string NullPaths[]={ std::string("Libs/")+QUOTE(SCONS_BUILD_DIR)+"/SoundSystem/libSoundSysNull.so", "./libSoundSysNull.so" };
#endif
        #undef QUOTE
        #undef QUOTE_HELPER
#endif

        SoundSystem=PlatformAux::GetSoundSys(NullPaths[0], m_SoundSysDLL);

        if (SoundSystem==NULL || m_SoundSysDLL==NULL)
            SoundSystem=PlatformAux::GetSoundSys(NullPaths[1], m_SoundSysDLL);

        // Null sound system should always be there and loadable...
        if (SoundSystem==NULL || m_SoundSysDLL==NULL)
        {
            Cleanup();
            throw std::runtime_error("Could not load the Null sound system.");
        }

        SoundSystem->Initialize();    // Init of Null sound system always succeeds.
    }

    static ConVarT InitialMasterVolume("snd_InitialMasterVolume", 1.0, ConVarT::FLAG_MAIN_EXE | ConVarT::FLAG_PERSISTENT, "The master volume with which the sound system is initialized.", 0.0, 1.0);
    SoundSystem->SetMasterVolume(float(InitialMasterVolume.GetValueDouble()));


    // Initialize the GUI systems GUI manager.
    //   - This has to be done *after* all materials are loaded (AppCafuT::OnInit()) and after the MatSys::Renderer
    //     has been initialized, so that the GuiMan finds its default material and can register it for rendering.
    //     (This is no longer exactly true: each GUI has now its own local material manager! See r359 from 2011-08-29 for details.)
    //   - It has to be done *before* the game is initialized, because even the server needs access to it
    //     when it loads static detail model entities that have world/entity-GUIs.
    cf::GuiSys::GuiMan=new cf::GuiSys::GuiManImplT(*m_GuiResources);


    // Create the client and server instances.
    m_SvGuiCallback=new SvGuiCallbT();
    m_Server=new ServerT(GameInfo, *m_SvGuiCallback, *m_ModelManager, *m_GuiResources);
    m_Client=new ClientT(MainWin, GameInfo, *m_ModelManager, *m_GuiResources);   // The client initializes in IDLE state.


    // Finish the initialization of the GuiSys.
    // Note that in the line below, the call to gui:setMousePos() is important, because it sets "MouseOverWindow" in the GUI properly to "Cl".
    // Without this, a left mouse button click that was not preceeded by a mouse movement would erroneously remove the input focus from "Cl".
    IntrusivePtrT<cf::GuiSys::GuiImplT> ClientGui = new cf::GuiSys::GuiImplT(*m_GuiResources);

    ClientGui->LoadScript(
        "local gui = ...\n"
        "local Cl = gui:new('WindowT', 'Client')\n"
        "\n"
        "Cl:GetTransform():set('Pos', 0, 0)\n"
        "Cl:GetTransform():set('Size', 640, 480)\n"   // This must be done before gui:setMousePos() is called.
        "\n"
        "gui:SetRootWindow(Cl)\n"
        "gui:showMouse(false)\n"
        "gui:setMousePos(320, 240)\n"
        "gui:setFocus(Cl)\n",
        cf::GuiSys::GuiImplT::InitFlag_InlineCode);

    IntrusivePtrT<cf::GuiSys::WindowT> ClientWindow = ClientGui->GetRootWindow()->Find("Client");
    IntrusivePtrT<ComponentClientT>    CompClient   = new ComponentClientT;

    assert(ClientWindow != NULL);
    CompClient->SetClient(m_Client);
    ClientWindow->AddComponent(CompClient);

    cf::GuiSys::GuiMan->Register(ClientGui);


    IntrusivePtrT<cf::GuiSys::GuiImplT> MainMenuGui = cf::GuiSys::GuiMan->Find("Games/" + GameInfo.GetName() + "/GUIs/MainMenu/MainMenu_main.cgui", true);

    if (MainMenuGui==NULL)
    {
        MainMenuGui = new cf::GuiSys::GuiImplT(*m_GuiResources);

        MainMenuGui->LoadScript(
            "local gui = ...\n"
            "local Err = gui:new('WindowT')\n"
            "\n"
            "gui:SetRootWindow(Err)\n"
            "gui:activate(true)\n"
            "gui:setInteractive(true)\n"
            "gui:showMouse(false)\n"
            "\n"
            "Err:GetTransform():set('Pos', 0, 0)\n"
            "Err:GetTransform():set('Size', 640, 480)\n"
            "\n"
            "local c1 = gui:new('ComponentTextT')\n"
            "c1:set('Text', 'Error loading MainMenu_main.cgui,\\nsee console <F1> for details.')\n"
            "c1:set('Scale', 0.8)\n"
            "Err:AddComponent(c1)\n",
            cf::GuiSys::GuiImplT::InitFlag_InlineCode);

        cf::GuiSys::GuiMan->Register(MainMenuGui);
    }

    m_Client->SetMainMenuGui(MainMenuGui);
    m_SvGuiCallback->MainMenuGui=MainMenuGui;       // This is the callback for the server, so that it can let the MainMenuGui know about its state changes.
    m_SvGuiCallback->OnServerStateChanged("idle");  // Argh, this is a HACK for setting the initial state... can we move this / do it better?

    IntrusivePtrT<cf::GuiSys::GuiImplT> ConsoleGui    = cf::GuiSys::GuiMan->Find("Games/"+ GameInfo.GetName() +"/GUIs/Console_main.cgui", true);
    IntrusivePtrT<cf::GuiSys::WindowT>  ConsoleWindow = ConsoleGui != NULL ? ConsoleGui->GetRootWindow()->Find("ConsoleOutput") : NULL;

    // Copy the previous console output to the new graphical console and print all new messages also there.
    m_ConByGuiWin=new cf::GuiSys::ConsoleByWindowT(ConsoleWindow);
    m_ConByGuiWin->Print(ConsoleText);
    m_CompositeConsole.Attach(m_ConByGuiWin);
}


ResourcesT::~ResourcesT()
{
    Cleanup();
}


void ResourcesT::Cleanup()
{
    m_CompositeConsole.Detach(m_ConByGuiWin);

    delete m_ConByGuiWin;
    m_ConByGuiWin=NULL;

    delete m_Client; m_Client=NULL;
    delete m_Server; m_Server=NULL;

    if (m_SvGuiCallback)
    {
        m_SvGuiCallback->MainMenuGui=NULL;
        delete m_SvGuiCallback; m_SvGuiCallback=NULL;
    }


    // When the game has been unloaded, no collision models must be left in the collision model manager.
    assert(cf::ClipSys::CollModelMan->GetUniqueCMCount() == 0);


    // Release the GuiManager (*before* the renderer).
    if (cf::GuiSys::GuiMan)
    {
        delete cf::GuiSys::GuiMan;
        cf::GuiSys::GuiMan=NULL;
    }


    // Release the Cafu Sound System.
    if (SoundSystem)
    {
        SoundSystem->Release();
        SoundSystem=NULL;
    }

    if (m_SoundSysDLL)
    {
        FreeLibrary(m_SoundSysDLL);
        m_SoundSysDLL=NULL;
    }


    // Release the GUI resources.
    if (m_GuiResources)
    {
        delete m_GuiResources;
        m_GuiResources=NULL;
    }


    // Release the model manager.
    if (m_ModelManager)
    {
        delete m_ModelManager;
        m_ModelManager=NULL;
    }


    // Release the Cafu Material System.
    if (MatSys::TextureMapManager)
    {
        // MatSys::TextureMapManager->FreeTextureMap(m_WhiteTexture);
        MatSys::TextureMapManager=NULL;
    }

    if (MatSys::Renderer)
    {
        MatSys::Renderer->Release();
        MatSys::Renderer=NULL;
    }

    if (m_RendererDLL)
    {
        FreeLibrary(m_RendererDLL);
        m_RendererDLL=NULL;
    }
}


void ResourcesT::runFrame(float FrameTimeF)
{
    m_Client->MainLoop(FrameTimeF);
    if (m_Server) m_Server->MainLoop();
}
