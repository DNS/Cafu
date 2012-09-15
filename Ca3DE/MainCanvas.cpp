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

#include "MainCanvas.hpp"
#include "AppCafu.hpp"
#include "MainFrame.hpp"
#include "Client/Client.hpp"
#include "Client/ClientWindow.hpp"
#include "Server/Server.hpp"

#include "Bitmap/Bitmap.hpp"
#include "ClipSys/CollisionModelMan.hpp"
#include "ConsoleCommands/Console.hpp"
#include "ConsoleCommands/ConsoleInterpreter.hpp"
#include "ConsoleCommands/ConsoleComposite.hpp"
#include "ConsoleCommands/ConsoleStringBuffer.hpp"
#include "ConsoleCommands/ConVar.hpp"
#include "GuiSys/ConsoleByWindow.hpp"
#include "GuiSys/GuiImpl.hpp"
#include "GuiSys/GuiManImpl.hpp"
#include "GuiSys/GuiResources.hpp"
#include "GuiSys/Window.hpp"
#include "MaterialSystem/MaterialManager.hpp"
#include "MaterialSystem/Renderer.hpp"
#include "MaterialSystem/TextureMap.hpp"
#include "Models/ModelManager.hpp"
#include "OpenGL/OpenGLWindow.hpp"  // For CaMouseEventT and CaKeyboardEventT.
#include "SoundSystem/SoundShaderManager.hpp"
#include "SoundSystem/SoundSys.hpp"
#include "../Games/Game.hpp"
#define GAME_NAME DeathMatch
#include "../Games/DeathMatch/Code/GameImpl.hpp"
#undef GAME_NAME
#include "PlatformAux.hpp"

#ifndef _WIN32
#include <dlfcn.h>
#define __stdcall
#define GetProcAddress dlsym
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
        if (!MainMenuGui) return;

        MainMenuGui->GetScriptState().Call("OnServerStateChanged", "s", NewState);
    }

    cf::GuiSys::GuiI* MainMenuGui;
};


BEGIN_EVENT_TABLE(MainCanvasT, wxGLCanvas)
    EVT_PAINT(MainCanvasT::OnPaint)
    EVT_IDLE(MainCanvasT::OnIdle)
    EVT_SIZE(MainCanvasT::OnSize)

    EVT_MOTION    (MainCanvasT::OnMouseMove )
    EVT_MOUSEWHEEL(MainCanvasT::OnMouseWheel)
    EVT_LEFT_DOWN (MainCanvasT::OnLMouseDown)
    EVT_LEFT_UP   (MainCanvasT::OnLMouseUp  )
    EVT_RIGHT_DOWN(MainCanvasT::OnRMouseDown)
    EVT_RIGHT_UP  (MainCanvasT::OnRMouseUp  )

    EVT_KEY_DOWN(MainCanvasT::OnKeyDown)
    EVT_KEY_UP  (MainCanvasT::OnKeyUp  )
    EVT_CHAR    (MainCanvasT::OnKeyChar)
END_EVENT_TABLE()


static int OpenGLAttributeList[]=
{
    WX_GL_RGBA,
    WX_GL_DOUBLEBUFFER,
    WX_GL_MIN_RED,       8,
    WX_GL_MIN_GREEN,     8,
    WX_GL_MIN_BLUE,      8,
    WX_GL_MIN_ALPHA,     8,
    WX_GL_DEPTH_SIZE,   16,
    WX_GL_STENCIL_SIZE,  8,
    0   // Zero indicates the end of the array.
};


MainCanvasT::MainCanvasT(MainFrameT* Parent)
    : wxGLCanvas(Parent, wxID_ANY, OpenGLAttributeList, wxDefaultPosition, wxDefaultSize, wxWANTS_CHARS, "CafuMainCanvas"),
      m_Parent(Parent),
      m_InitState(INIT_REQUIRED),
      m_GLContext(NULL),
      m_RendererDLL(NULL),
      m_ModelManager(NULL),
      m_GuiResources(NULL),
      m_SoundSysDLL(NULL),
      m_Game(NULL),
      m_Client(NULL),
      m_Server(NULL),
      m_SvGuiCallback(NULL),
      m_ConByGuiWin(NULL),
      m_Timer(),
      m_TotalTime(0.0),
      m_LastMousePos(IN_OTHER_2D_GUI)
{
    m_GLContext=new wxGLContext(this);

    SetCursor(wxCURSOR_BLANK);
    SetBackgroundStyle(wxBG_STYLE_PAINT);
}


MainCanvasT::~MainCanvasT()
{
    m_InitState=INIT_REQUIRED;

    wxGetApp().GetConComposite().Detach(m_ConByGuiWin);
    delete m_ConByGuiWin;
    m_ConByGuiWin=NULL;

    delete m_Client; m_Client=NULL;
    delete m_Server; m_Server=NULL;

    if (m_SvGuiCallback)
    {
        m_SvGuiCallback->MainMenuGui=NULL;
        delete m_SvGuiCallback; m_SvGuiCallback=NULL;
    }


    // Release the Game.
    if (m_Game)
    {
        m_Game->Release();
        m_Game=NULL;
    }


    // When the game has been unloaded, no collision models must be left in the collision model manager.
    wxASSERT(cf::ClipSys::CollModelMan->GetUniqueCMCount()==0);


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


void MainCanvasT::Initialize()
{
    extern ConVarT Options_ClientDesiredRenderer;
    extern ConVarT Options_ClientTextureDetail;
    extern ConVarT Options_ClientDesiredSoundSystem;
    extern ConVarT Options_ServerGameName;

    m_InitState=INIT_FAILED;

    // Initialize the Material System.
    wxASSERT(this->IsShownOnScreen());

    // If this call was in the ctor, it would trigger an assertion in debug build and yield an invalid (unusable)
    // OpenGL context in release builds (the GL code in the MatSys::Renderer->IsSupported() methods would fail).
    this->SetCurrent(*m_GLContext);


    try
    {
        // Obtain the specified MatSys renderer (or if none is specified, automatically find the "best").
        const wxString RendererName=wxString(Options_ClientDesiredRenderer.GetValueString()).Trim();

        if (RendererName!="" && !RendererName.StartsWith("#"))
            MatSys::Renderer=PlatformAux::GetRenderer(std::string(RendererName), m_RendererDLL);

        if (MatSys::Renderer==NULL || m_RendererDLL==NULL)
            MatSys::Renderer=PlatformAux::GetBestRenderer(m_RendererDLL);

        if (MatSys::Renderer==NULL || m_RendererDLL==NULL)
            throw std::runtime_error("Could not find a renderer that is supported on your system.");

        MatSys::Renderer->Initialize();


        // Obtain the texture map manager from the previously loaded renderer DLL.
        MatSys::TextureMapManager=PlatformAux::GetTextureMapManager(m_RendererDLL);

        if (MatSys::TextureMapManager==NULL)
            throw std::runtime_error("Could not get the TextureMapManager from the renderer DLL.");

        switch (Options_ClientTextureDetail.GetValueInt())
        {
            case 1: MatSys::TextureMapManager->SetMaxTextureSize(256); break;
            case 2: MatSys::TextureMapManager->SetMaxTextureSize(128); break;
        }


        // Initialize the model manager and the GUI resources.
        m_ModelManager=new ModelManagerT();
        m_GuiResources=new cf::GuiSys::GuiResourcesT(*m_ModelManager);


        // Initialize the sound system.
        const wxString SoundSysName=wxString(Options_ClientDesiredSoundSystem.GetValueString()).Trim();

        if (SoundSysName!="" && !SoundSysName.StartsWith("#"))
            SoundSystem=PlatformAux::GetSoundSys(std::string(SoundSysName), m_SoundSysDLL);

        if (SoundSystem==NULL || m_SoundSysDLL==NULL)
            SoundSystem=PlatformAux::GetBestSoundSys(m_SoundSysDLL);

        if (SoundSystem==NULL || m_SoundSysDLL==NULL)
            throw std::runtime_error("Could not find a sound system that is supported on your system.");

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
                throw std::runtime_error("Could not load the Null sound system.");

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


        // Provide a definition for m_Game, the cf::GameSys::Game pointer
        // to a GameI implementation that is provided by a dynamically loaded game DLL.
        // This is analogous to the Material System, where Renderer DLLs provide renderer and texture manager implementations.
        m_Game=&DeathMatch::GameImplT::GetInstance();

        m_Game->Initialize(true /*(Options_RunMode.GetValueInt() & CLIENT_RUNMODE)>0*/,
                           true /*(Options_RunMode.GetValueInt() & SERVER_RUNMODE)>0*/,
                           *m_ModelManager);


        // Create the client and server instances.
        m_SvGuiCallback=new SvGuiCallbT();
        m_Server=new ServerT(m_Game, Options_ServerGameName.GetValueString(), *m_SvGuiCallback, *m_ModelManager);
        m_Client=new ClientT(m_Game, *m_ModelManager);  // The client initializes in IDLE state.


        // Finish the initialization of the GuiSys.
        // Note that in the line below, the call to gui:setMousePos() is important, because it sets "MouseOverWindow" in the GUI properly to "Cl".
        // Without this, a left mouse button click that was not preceeded by a mouse movement would erroneously remove the input focus from "Cl".
        cf::GuiSys::GuiImplT*              ClientGui   =new cf::GuiSys::GuiImplT(*m_GuiResources, "Cl=gui:new('ClientWindowT'); gui:SetRootWindow(Cl); gui:showMouse(false); gui:setMousePos(320, 240); gui:setFocus(Cl); Cl:SetName('Client');", true);
        IntrusivePtrT<cf::GuiSys::WindowT> ClientWindow=ClientGui->GetRootWindow()->Find("Client");
        IntrusivePtrT<ClientWindowT>       ClWin       =dynamic_pointer_cast<ClientWindowT>(ClientWindow);

        assert(ClWin!=NULL);
        if (!ClWin.IsNull()) ClWin->SetClient(m_Client);
        cf::GuiSys::GuiMan->Register(ClientGui);


        cf::GuiSys::GuiI* MainMenuGui=cf::GuiSys::GuiMan->Find("Games/"+Options_ServerGameName.GetValueString()+"/GUIs/MainMenu/MainMenu_main.cgui", true);
        if (MainMenuGui==NULL)
        {
            MainMenuGui=new cf::GuiSys::GuiImplT(*m_GuiResources, "Err=gui:new('WindowT'); gui:SetRootWindow(Err); gui:activate(true); gui:setInteractive(true); gui:showMouse(false); Err:set('rect', 0, 0, 640, 480); Err:set('textScale', 0.8); Err:set('text', 'Error loading MainMenu_main.cgui,\\nsee console <F1> for details.');", true);
            cf::GuiSys::GuiMan->Register(MainMenuGui);
        }
        m_Client->SetMainMenuGui(MainMenuGui);
        m_SvGuiCallback->MainMenuGui=MainMenuGui;       // This is the callback for the server, so that it can let the MainMenuGui know about its state changes.
        m_SvGuiCallback->OnServerStateChanged("idle");  // Argh, this is a HACK for setting the initial state... can we move this / do it better?

        cf::GuiSys::GuiI*                  ConsoleGui   =cf::GuiSys::GuiMan->Find("Games/"+Options_ServerGameName.GetValueString()+"/GUIs/Console.cgui", true);
        IntrusivePtrT<cf::GuiSys::WindowT> ConsoleWindow=ConsoleGui ? ConsoleGui->GetRootWindow()->Find("ConsoleOutput") : NULL;

        // Copy the previously collected console output to the new graphical console.
        m_ConByGuiWin=new cf::GuiSys::ConsoleByWindowT(ConsoleWindow);
        m_ConByGuiWin->Print(wxGetApp().GetConBuffer().GetBuffer());
        wxGetApp().GetConComposite().Attach(m_ConByGuiWin);

        m_InitState=INIT_SUCCESS;
    }
    catch (const std::runtime_error& RE)
    {
        wxMessageDialog Msg(NULL,
            wxString("While initializing Cafu, the following error occurred:\n\n") + typeid(RE).name() + "\n" + RE.what(),
            "Cafu Initialization Error",
            wxOK | wxCANCEL | wxCANCEL_DEFAULT | wxICON_ERROR);

        Msg.SetExtendedMessage("Sorry it didn't work out.\nTo get help, please post this error at the Cafu forums or mailing-list.");
        Msg.SetOKCancelLabels("Open www.cafu.de", "Close");

        while (Msg.ShowModal()==wxID_OK)
        {
            wxLaunchDefaultBrowser("http://www.cafu.de");
        }

        m_Parent->Destroy();
    }
}


static const char* ScreenShotSuffixTypes[] = { "jpg", "png", "bmp", NULL };
static ConVarT ScreenShotSuffix("screenSuffix", "jpg", ConVarT::FLAG_MAIN_EXE, "The suffix to be used for screen-shot image files. Only \"jpg\", \"png\" and \"bmp\" are allowed.", ScreenShotSuffixTypes);


void MainCanvasT::TakeScreenshot() const
{
    const unsigned int      Width =GetClientSize().GetWidth();
    const unsigned int      Height=GetClientSize().GetHeight();
    static ArrayT<uint32_t> FrameBuffer;

    FrameBuffer.Overwrite();
    FrameBuffer.PushBackEmpty(Width*Height);

    // Read the pixels from the OpenGL back buffer into FrameBuffer.
    // Note that the first two parameters (0, 0) specify the left BOTTOM corner of the desired rectangle!
    glReadPixels(0, 0, Width, Height, GL_RGBA, GL_UNSIGNED_BYTE, &FrameBuffer[0]);

    // As mentioned above is the data in the FrameBuffer bottom-up.
    // Thus now swap all lines (vertical mirroring).
    for (unsigned int y=0; y<Height/2; y++)
    {
        uint32_t* UpperRow=&FrameBuffer[0]+        y   *Width;
        uint32_t* LowerRow=&FrameBuffer[0]+(Height-y-1)*Width;

        for (unsigned int x=0; x<Width; x++)
        {
            const uint32_t Swap=*UpperRow;

            *UpperRow=*LowerRow;
            *LowerRow=Swap;

            UpperRow++;
            LowerRow++;
        }
    }

    // Find the next free image name and save the file.
    for (unsigned int Nr=0; Nr<100000; Nr++)
    {
        const wxString FileName=wxString::Format("pic%04u", Nr) + "." + ScreenShotSuffix.GetValueString();

        if (!wxFileExists(FileName))
        {
            if (BitmapT(Width, Height, &FrameBuffer[0]).SaveToDisk(FileName))
            {
                Console->Print(std::string("Screenshot \"")+FileName.ToStdString()+"\" saved.\n");
            }
            else
            {
                Console->Warning(std::string("Screenshot \"")+FileName.ToStdString()+"\" could not be saved.\n");
            }
            return;
        }
    }
}


void MainCanvasT::OnPaint(wxPaintEvent& PE)
{
    wxPaintDC dc(this);     // It is VERY important not to omit this, or otherwise everything goes havoc.

    if (m_InitState==INIT_REQUIRED)
    {
        dc.SetBackground(*wxBLACK_BRUSH);
        dc.Clear();
        dc.SetBackgroundMode(wxTRANSPARENT);
        dc.SetTextForeground(wxColour(255, 200, 0));
        dc.DrawText("Initializing Cafu...", 10, 40);

        // This code is in this place due to a few peculiarities of OpenGL under GTK that do not exist under MSW:
        //   - An OpenGL context can only be made current with a canvas that is shown on the screen.
        //   - Relying on EVT_SHOW however is not a good approach, see the discussions at
        //     <http://thread.gmane.org/gmane.comp.lib.wxwidgets.general/68490> and
        //     <http://thread.gmane.org/gmane.comp.lib.wxwidgets.general/70607> for details.
        // Consequently, the first and best opportunity for initialization is here.
        Initialize();
    }
}


void MainCanvasT::OnSize(wxSizeEvent& SE)
{
    if (m_InitState!=INIT_SUCCESS) return;

    const wxSize Size=SE.GetSize();

    if (Size.x>0 && Size.y>0)
        MatSys::Renderer->SetViewport(0, 0, Size.x, Size.y);
}


void MainCanvasT::OnIdle(wxIdleEvent& IE)
{
    IE.RequestMore();

    // Beende das Programm, wenn das letzte aktive GUI geschlossen wurde.
    // if (cf::GuiSys::GuiMan->GetNumberOfActiveGUIs()==0) m_Parent->Close();

    static ConVarT quit("quit", false, ConVarT::FLAG_MAIN_EXE, "The program quits if this variable is set to 1 (true).");
    if (quit.GetValueBool())
    {
        quit.SetValue(false);   // Immediately reset the value, so that we're able to restart the game from a loop that governs the master loop...
        m_Parent->Close();
    }


    const double FrameTimeD=m_Timer.GetSecondsSinceLastCall();
    const float  FrameTimeF=float(FrameTimeD);

    m_TotalTime+=FrameTimeD;

    extern ConVarT GlobalTime;
    GlobalTime.SetValue(m_TotalTime);


    if (m_InitState!=INIT_SUCCESS) return;

    cf::GuiSys::GuiMan->DistributeClockTickEvents(FrameTimeF);

    // If the active GUI is the client GUI, see how far the mouse cursor is off center,
    // derive a mouse event from it, then recenter the mouse cursor.
    cf::GuiSys::GuiI* ActiveGui=cf::GuiSys::GuiMan->GetTopmostActiveAndInteractive();

    if (ActiveGui && ActiveGui->GetRootWindow()->Name=="Client")
    {
        const wxPoint MousePos  =ScreenToClient(wxGetMousePosition());  // Note: ScreenToClient() is a method of wxWindow.
        const wxSize  WinCenter =GetClientSize()/2;
        const wxPoint MouseDelta=MousePos-WinCenter;

        if (m_LastMousePos==IN_CLIENT_3D_GUI)
        {
            CaMouseEventT ME;

            ME.Type  =CaMouseEventT::CM_MOVE_X;
            ME.Amount=MouseDelta.x;
            if (ME.Amount!=0) ActiveGui->ProcessDeviceEvent(ME);

            ME.Type  =CaMouseEventT::CM_MOVE_Y;
            ME.Amount=MouseDelta.y;
            if (ME.Amount!=0) ActiveGui->ProcessDeviceEvent(ME);
        }

        if (MouseDelta.x || MouseDelta.y) WarpPointer(WinCenter.x, WinCenter.y);
        m_LastMousePos=IN_CLIENT_3D_GUI;
    }

    if (!m_Parent->IsIconized())
    {
        // Render all the GUIs.
        MatSys::Renderer->BeginFrame(m_TotalTime);

        cf::GuiSys::GuiMan->RenderAll();

        MatSys::Renderer->EndFrame();
        this->SwapBuffers();
    }
    else
    {
        // The main window is minimized - give other applications a chance to run.
        wxMilliSleep(5);
    }

    // Update the sound system (Reposition sound sources, update streams).
    SoundSystem->Update();

    // Run a client and a server frame.
    m_Client->MainLoop(FrameTimeF);
    if (m_Server) m_Server->MainLoop();
}


void MainCanvasT::OnMouseMove(wxMouseEvent& ME)
{
    if (m_InitState!=INIT_SUCCESS) return;

    cf::GuiSys::GuiI* Gui=cf::GuiSys::GuiMan->GetTopmostActiveAndInteractive();

    // This is equivalent to calling cf::GuiSys::GuiMan->ProcessDeviceEvent(MouseEvent),
    // but computing the amount of mouse movement is much easier (and more precise) like this.
    if (Gui && Gui->GetRootWindow()->Name!="Client")
    {
        float OldMousePosX;
        float OldMousePosY;

        Gui->GetMousePos(OldMousePosX, OldMousePosY);

        const float NewMousePosX=ME.GetX()*(cf::GuiSys::VIRTUAL_SCREEN_SIZE_X/GetSize().x);
        const float NewMousePosY=ME.GetY()*(cf::GuiSys::VIRTUAL_SCREEN_SIZE_Y/GetSize().y);

        // This works, but doesn't forward the mouse event to the windows.
        // Gui->SetMousePos(NewMousePosX, NewMousePosY);

        CaMouseEventT ME;

        ME.Type  =CaMouseEventT::CM_MOVE_X;
        ME.Amount=NewMousePosX-OldMousePosX;
        if (ME.Amount!=0) Gui->ProcessDeviceEvent(ME);

        ME.Type  =CaMouseEventT::CM_MOVE_Y;
        ME.Amount=NewMousePosY-OldMousePosY;
        if (ME.Amount!=0) Gui->ProcessDeviceEvent(ME);

        m_LastMousePos=IN_OTHER_2D_GUI;
    }
}


void MainCanvasT::OnMouseWheel(wxMouseEvent& ME)
{
    if (m_InitState!=INIT_SUCCESS) return;

    CaMouseEventT MouseEvent;

    MouseEvent.Type  =CaMouseEventT::CM_MOVE_Z;
    MouseEvent.Amount=ME.GetWheelDelta();

    cf::GuiSys::GuiMan->ProcessDeviceEvent(MouseEvent);
}


void MainCanvasT::OnLMouseDown(wxMouseEvent& ME)
{
    if (m_InitState!=INIT_SUCCESS) return;

    CaMouseEventT MouseEvent;

    MouseEvent.Type  =CaMouseEventT::CM_BUTTON0;
    MouseEvent.Amount=1;

    cf::GuiSys::GuiMan->ProcessDeviceEvent(MouseEvent);
}


void MainCanvasT::OnLMouseUp(wxMouseEvent& ME)
{
    if (m_InitState!=INIT_SUCCESS) return;

    CaMouseEventT MouseEvent;

    MouseEvent.Type=CaMouseEventT::CM_BUTTON0;
    MouseEvent.Amount=0;

    cf::GuiSys::GuiMan->ProcessDeviceEvent(MouseEvent);
}


void MainCanvasT::OnRMouseDown(wxMouseEvent& ME)
{
    if (m_InitState!=INIT_SUCCESS) return;

    CaMouseEventT MouseEvent;

    MouseEvent.Type  =CaMouseEventT::CM_BUTTON1;
    MouseEvent.Amount=1;

    cf::GuiSys::GuiMan->ProcessDeviceEvent(MouseEvent);
}


void MainCanvasT::OnRMouseUp(wxMouseEvent& ME)
{
    if (m_InitState!=INIT_SUCCESS) return;

    CaMouseEventT MouseEvent;

    MouseEvent.Type  =CaMouseEventT::CM_BUTTON1;
    MouseEvent.Amount=0;

    cf::GuiSys::GuiMan->ProcessDeviceEvent(MouseEvent);
}


struct KeyCodePairT
{
    wxKeyCode              wxKC;
    CaKeyboardEventT::KeyT CaKC;
};


static const KeyCodePairT KeyCodes[]=
{
    { WXK_BACK,             CaKeyboardEventT::CK_BACKSPACE },
    { WXK_TAB,              CaKeyboardEventT::CK_TAB },
    { WXK_RETURN,           CaKeyboardEventT::CK_RETURN },
    { WXK_ESCAPE,           CaKeyboardEventT::CK_ESCAPE },
    { WXK_SPACE,            CaKeyboardEventT::CK_SPACE },
    { (wxKeyCode)48,        CaKeyboardEventT::CK_0 },
    { (wxKeyCode)49,        CaKeyboardEventT::CK_1 },
    { (wxKeyCode)50,        CaKeyboardEventT::CK_2 },
    { (wxKeyCode)51,        CaKeyboardEventT::CK_3 },
    { (wxKeyCode)52,        CaKeyboardEventT::CK_4 },
    { (wxKeyCode)53,        CaKeyboardEventT::CK_5 },
    { (wxKeyCode)54,        CaKeyboardEventT::CK_6 },
    { (wxKeyCode)55,        CaKeyboardEventT::CK_7 },
    { (wxKeyCode)56,        CaKeyboardEventT::CK_8 },
    { (wxKeyCode)57,        CaKeyboardEventT::CK_9 },
    { (wxKeyCode)65,        CaKeyboardEventT::CK_A },
    { (wxKeyCode)66,        CaKeyboardEventT::CK_B },
    { (wxKeyCode)67,        CaKeyboardEventT::CK_C },
    { (wxKeyCode)68,        CaKeyboardEventT::CK_D },
    { (wxKeyCode)69,        CaKeyboardEventT::CK_E },
    { (wxKeyCode)70,        CaKeyboardEventT::CK_F },
    { (wxKeyCode)71,        CaKeyboardEventT::CK_G },
    { (wxKeyCode)72,        CaKeyboardEventT::CK_H },
    { (wxKeyCode)73,        CaKeyboardEventT::CK_I },
    { (wxKeyCode)74,        CaKeyboardEventT::CK_J },
    { (wxKeyCode)75,        CaKeyboardEventT::CK_K },
    { (wxKeyCode)76,        CaKeyboardEventT::CK_L },
    { (wxKeyCode)77,        CaKeyboardEventT::CK_M },
    { (wxKeyCode)78,        CaKeyboardEventT::CK_N },
    { (wxKeyCode)79,        CaKeyboardEventT::CK_O },
    { (wxKeyCode)80,        CaKeyboardEventT::CK_P },
    { (wxKeyCode)81,        CaKeyboardEventT::CK_Q },
    { (wxKeyCode)82,        CaKeyboardEventT::CK_R },
    { (wxKeyCode)83,        CaKeyboardEventT::CK_S },
    { (wxKeyCode)84,        CaKeyboardEventT::CK_T },
    { (wxKeyCode)85,        CaKeyboardEventT::CK_U },
    { (wxKeyCode)86,        CaKeyboardEventT::CK_V },
    { (wxKeyCode)87,        CaKeyboardEventT::CK_W },
    { (wxKeyCode)88,        CaKeyboardEventT::CK_X },
    { (wxKeyCode)89,        CaKeyboardEventT::CK_Y },
    { (wxKeyCode)90,        CaKeyboardEventT::CK_Z },
    { WXK_DELETE,           CaKeyboardEventT::CK_DELETE },
    //{ WXK_START,            CaKeyboardEventT:: },
    //{ WXK_LBUTTON,          CaKeyboardEventT:: },
    //{ WXK_RBUTTON,          CaKeyboardEventT:: },
    //{ WXK_CANCEL,           CaKeyboardEventT:: },
    //{ WXK_MBUTTON,          CaKeyboardEventT:: },
    //{ WXK_CLEAR,            CaKeyboardEventT:: },
    { WXK_SHIFT,            CaKeyboardEventT::CK_LSHIFT },
    { WXK_ALT,              CaKeyboardEventT::CK_LMENU },
    { WXK_CONTROL,          CaKeyboardEventT::CK_LCONTROL },
    //{ WXK_MENU,             CaKeyboardEventT:: },
    { WXK_PAUSE,            CaKeyboardEventT::CK_PAUSE },
    { WXK_CAPITAL,          CaKeyboardEventT::CK_CAPITAL },
    { WXK_END,              CaKeyboardEventT::CK_END },
    { WXK_HOME,             CaKeyboardEventT::CK_HOME },
    { WXK_LEFT,             CaKeyboardEventT::CK_LEFT },
    { WXK_UP,               CaKeyboardEventT::CK_UP },
    { WXK_RIGHT,            CaKeyboardEventT::CK_RIGHT },
    { WXK_DOWN,             CaKeyboardEventT::CK_DOWN },
    //{ WXK_SELECT,           CaKeyboardEventT:: },
    //{ WXK_PRINT,            CaKeyboardEventT:: },
    //{ WXK_EXECUTE,          CaKeyboardEventT:: },
    //{ WXK_SNAPSHOT,         CaKeyboardEventT:: },
    { WXK_INSERT,           CaKeyboardEventT::CK_INSERT },
    //{ WXK_HELP,             CaKeyboardEventT:: },
    { WXK_NUMPAD0,          CaKeyboardEventT::CK_NUMPAD0 },
    { WXK_NUMPAD1,          CaKeyboardEventT::CK_NUMPAD1 },
    { WXK_NUMPAD2,          CaKeyboardEventT::CK_NUMPAD2 },
    { WXK_NUMPAD3,          CaKeyboardEventT::CK_NUMPAD3 },
    { WXK_NUMPAD4,          CaKeyboardEventT::CK_NUMPAD4 },
    { WXK_NUMPAD5,          CaKeyboardEventT::CK_NUMPAD5 },
    { WXK_NUMPAD6,          CaKeyboardEventT::CK_NUMPAD6 },
    { WXK_NUMPAD7,          CaKeyboardEventT::CK_NUMPAD7 },
    { WXK_NUMPAD8,          CaKeyboardEventT::CK_NUMPAD8 },
    { WXK_NUMPAD9,          CaKeyboardEventT::CK_NUMPAD9 },
    //{ WXK_MULTIPLY,         CaKeyboardEventT:: },
    //{ WXK_ADD,              CaKeyboardEventT:: },
    { WXK_SEPARATOR,        CaKeyboardEventT::CK_COMMA },
    { WXK_SUBTRACT,         CaKeyboardEventT::CK_MINUS },
    { WXK_DECIMAL,          CaKeyboardEventT::CK_PERIOD },
    { WXK_DIVIDE,           CaKeyboardEventT::CK_SLASH },
    { WXK_F1,               CaKeyboardEventT::CK_F1 },
    { WXK_F2,               CaKeyboardEventT::CK_F2 },
    { WXK_F3,               CaKeyboardEventT::CK_F3 },
    { WXK_F4,               CaKeyboardEventT::CK_F4 },
    { WXK_F5,               CaKeyboardEventT::CK_F5 },
    { WXK_F6,               CaKeyboardEventT::CK_F6 },
    { WXK_F7,               CaKeyboardEventT::CK_F7 },
    { WXK_F8,               CaKeyboardEventT::CK_F8 },
    { WXK_F9,               CaKeyboardEventT::CK_F9 },
    { WXK_F10,              CaKeyboardEventT::CK_F10 },
    { WXK_F11,              CaKeyboardEventT::CK_F11 },
    { WXK_F12,              CaKeyboardEventT::CK_F12 },
    { WXK_F13,              CaKeyboardEventT::CK_F13 },
    { WXK_F14,              CaKeyboardEventT::CK_F14 },
    { WXK_F15,              CaKeyboardEventT::CK_F15 },
    { WXK_NUMLOCK,          CaKeyboardEventT::CK_NUMLOCK },
    { WXK_SCROLL,           CaKeyboardEventT::CK_SCROLL },
    { WXK_PAGEUP,           CaKeyboardEventT::CK_PGUP },
    { WXK_PAGEDOWN,         CaKeyboardEventT::CK_PGDN },
    //{ WXK_NUMPAD_SPACE,     CaKeyboardEventT:: },
    //{ WXK_NUMPAD_TAB,       CaKeyboardEventT:: },
    { WXK_NUMPAD_ENTER,     CaKeyboardEventT::CK_NUMPADENTER },
    //{ WXK_NUMPAD_F1,        CaKeyboardEventT:: },
    //{ WXK_NUMPAD_F2,        CaKeyboardEventT:: },
    //{ WXK_NUMPAD_F3,        CaKeyboardEventT:: },
    //{ WXK_NUMPAD_F4,        CaKeyboardEventT:: },
    //{ WXK_NUMPAD_HOME,      CaKeyboardEventT:: },
    //{ WXK_NUMPAD_LEFT,      CaKeyboardEventT:: },
    //{ WXK_NUMPAD_UP,        CaKeyboardEventT:: },
    //{ WXK_NUMPAD_RIGHT,     CaKeyboardEventT:: },
    //{ WXK_NUMPAD_DOWN,      CaKeyboardEventT:: },
    //{ WXK_NUMPAD_PAGEUP,    CaKeyboardEventT:: },
    //{ WXK_NUMPAD_PAGEDOWN,  CaKeyboardEventT:: },
    //{ WXK_NUMPAD_END,       CaKeyboardEventT:: },
    //{ WXK_NUMPAD_BEGIN,     CaKeyboardEventT:: },
    //{ WXK_NUMPAD_INSERT,    CaKeyboardEventT:: },
    //{ WXK_NUMPAD_DELETE,    CaKeyboardEventT:: },
    //{ WXK_NUMPAD_EQUAL,     CaKeyboardEventT:: },
    { WXK_NUMPAD_MULTIPLY,  CaKeyboardEventT::CK_MULTIPLY },
    { WXK_NUMPAD_ADD,       CaKeyboardEventT::CK_ADD },
    { WXK_NUMPAD_SEPARATOR, CaKeyboardEventT::CK_NUMPADCOMMA },
    { WXK_NUMPAD_SUBTRACT,  CaKeyboardEventT::CK_SUBTRACT },
    { WXK_NUMPAD_DECIMAL,   CaKeyboardEventT::CK_DECIMAL },
    { WXK_NUMPAD_DIVIDE,    CaKeyboardEventT::CK_DIVIDE },
    { WXK_WINDOWS_LEFT,     CaKeyboardEventT::CK_LWIN },
    { WXK_WINDOWS_RIGHT,    CaKeyboardEventT::CK_RWIN },
    //{ WXK_WINDOWS_MENU ,    CaKeyboardEventT:: },
    //{ WXK_COMMAND,          CaKeyboardEventT:: },
    //{ (wxKeyCode)0, (CaKeyboardEventT::KeyT)0 }
};


void MainCanvasT::OnKeyDown(wxKeyEvent& KE)
{
    if (m_InitState!=INIT_SUCCESS) return;

    switch (KE.GetKeyCode())
    {
        case WXK_F1:
        {
            // Activate the in-game console GUI (it's "F1" now, not CK_GRAVE ("^", accent grave) any longer).
            extern ConVarT    Options_ServerGameName;
            cf::GuiSys::GuiI* ConsoleGui=cf::GuiSys::GuiMan->Find("Games/"+Options_ServerGameName.GetValueString()+"/GUIs/Console.cgui");

            // ConsoleGui should be the same as in Initialize(), but could be NULL on file not found, parse error, etc.
            if (ConsoleGui!=NULL && !ConsoleGui->GetIsActive())
            {
                ConsoleGui->Activate();
                cf::GuiSys::GuiMan->BringToFront(ConsoleGui);

                static bool InitialHelpMsgPrinted=false;

                if (!InitialHelpMsgPrinted)
                {
                    Console->Print("\n");
                    Console->Print("Welcome to the Cafu console!\n");
                    Console->Print("Enter   help()   to obtain more information.\n");
                    Console->Print("\n");
                    InitialHelpMsgPrinted=true;
                }

                // Handled the key.
                return;
            }

            // Let the active GUI handle the key below (e.g. for closing the console again).
            break;
        }

        case WXK_F5:
        {
            TakeScreenshot();
            return;
        }

        case WXK_F11:
        {
            if (!wxGetApp().IsCustomVideoMode())
            {
                // Switching full-screen mode with F11 only makes sense if we didn't set a custom video mode (screen resolution).
                // See AppCafuT::OnInit() for more details.
                m_Parent->ShowFullScreen(!m_Parent->IsFullScreen());
            }
            return;
        }
    }

    // Look for the pressed keys keycode in the table and translate it to a CaKeyCode if found.
    for (int KeyCodeNr=0; KeyCodes[KeyCodeNr].wxKC!=0; KeyCodeNr++)
    {
        if (KeyCodes[KeyCodeNr].wxKC==KE.GetKeyCode())
        {
            CaKeyboardEventT KeyboardEvent;

            KeyboardEvent.Type=CaKeyboardEventT::CKE_KEYDOWN;
            KeyboardEvent.Key =KeyCodes[KeyCodeNr].CaKC;

            cf::GuiSys::GuiMan->ProcessDeviceEvent(KeyboardEvent);
            // We should probably return here if the GuiMan handled the key, break (only when unhandled) for calling KE.Skip() otherwise.
            break;
        }
    }

    KE.Skip();
}


void MainCanvasT::OnKeyUp(wxKeyEvent& KE)
{
    if (m_InitState!=INIT_SUCCESS) return;

    // Look for the released keys keycode in the table and translate it to a CaKeyCode if found.
    for (int KeyCodeNr=0; KeyCodes[KeyCodeNr].wxKC!=0; KeyCodeNr++)
    {
        if (KeyCodes[KeyCodeNr].wxKC==KE.GetKeyCode())
        {
            CaKeyboardEventT KeyboardEvent;

            KeyboardEvent.Type=CaKeyboardEventT::CKE_KEYUP;
            KeyboardEvent.Key =KeyCodes[KeyCodeNr].CaKC;

            cf::GuiSys::GuiMan->ProcessDeviceEvent(KeyboardEvent);
            return;
        }
    }

    KE.Skip();
}


void MainCanvasT::OnKeyChar(wxKeyEvent& KE)
{
    if (m_InitState!=INIT_SUCCESS) return;

    CaKeyboardEventT KeyboardEvent;

    KeyboardEvent.Type=CaKeyboardEventT::CKE_CHAR;
    KeyboardEvent.Key =KE.GetKeyCode();

    cf::GuiSys::GuiMan->ProcessDeviceEvent(KeyboardEvent);
}
