/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "ClientMainWindow.hpp"
#include "Client/Client.hpp"
#include "Server/Server.hpp"
#include "GameInfo.hpp"

#include "Bitmap/Bitmap.hpp"
#include "ConsoleCommands/ConsoleComposite.hpp"
#include "ConsoleCommands/ConsoleStringBuffer.hpp"
#include "ConsoleCommands/ConVar.hpp"
#include "GuiSys/ConsoleByWindow.hpp"
#include "GuiSys/GuiImpl.hpp"
#include "GuiSys/GuiManImpl.hpp"
#include "GuiSys/Window.hpp"
#include "MaterialSystem/Renderer.hpp"
#include "OpenGL/OpenGLWindow.hpp"  // For CaMouseEventT and CaKeyboardEventT.
#include "SoundSystem/SoundSys.hpp"

#include "GLFW/glfw3.h"

#include <chrono>
#include <thread>

//#ifndef _WIN32
//#include <dlfcn.h>
//#define __stdcall
//#define GetProcAddress dlsym
//#define FreeLibrary dlclose
//#endif


ClientMainWindowT::ClientMainWindowT(const GameInfoT& GameInfo, int width, int height, const char* title, GLFWmonitor* monitor)
    : glfwWindowT(width, height, title, monitor),
      m_GameInfo(GameInfo),
      m_Resources(NULL),
      m_Timer(),
      m_TotalTime(0.0),
      m_LastMousePos(IN_OTHER_2D_GUI),
      m_LastMousePosX(0.0),
      m_LastMousePosY(0.0)
{
    setMouseCursorMode(GLFW_CURSOR_HIDDEN);
}


ClientMainWindowT::~ClientMainWindowT()
{
    delete m_Resources;
}


void ClientMainWindowT::Init(cf::CompositeConsoleT& CC, const std::string& ConsoleText, MainWindowT& MainWin)
{
    if (!m_Resources)
    {
        try
        {
            m_Resources = new ResourcesT(CC, ConsoleText, m_GameInfo, MainWin);
        }
        catch (const std::runtime_error& /*RE*/)
        {
            // TODO: Make m_Resources fail gracefully!
//            wxMessageDialog Msg(NULL,
//                wxString("While initializing Cafu, the following error occurred:\n\n") + typeid(RE).name() + "\n" + RE.what(),
//                "Cafu Initialization Error",
//                wxOK | wxCANCEL | wxCANCEL_DEFAULT | wxICON_ERROR);
//
//            Msg.SetExtendedMessage("Sorry it didn't work out.\nTo get help, please post this error at the Cafu forums or mailing-list.");
//            Msg.SetOKCancelLabels("Open www.cafu.de", "Close");
//
//            while (Msg.ShowModal()==wxID_OK)
//            {
//                wxLaunchDefaultBrowser("http://www.cafu.de");
//            }
//
//            m_Parent->Destroy();
//            return;
            throw;
        }
    }
}


void ClientMainWindowT::runFrame()
{
    // Beende das Programm, wenn das letzte aktive GUI geschlossen wurde.
    // if (cf::GuiSys::GuiMan->GetNumberOfActiveGUIs()==0) m_Parent->Close();

    static ConVarT quit("quit", false, ConVarT::FLAG_MAIN_EXE, "The program quits if this variable is set to 1 (true).");
    if (quit.GetValueBool())
    {
        quit.SetValue(false);   // Immediately reset the value, so that we're able to restart the game from a loop that governs the master loop...
        setShouldClose(true);
    }


    const double FrameTimeD=m_Timer.GetSecondsSinceLastCall();
    const float  FrameTimeF=float(FrameTimeD);

    m_TotalTime+=FrameTimeD;

    extern ConVarT GlobalTime;
    GlobalTime.SetValue(m_TotalTime);


    if (!m_Resources) return;

    cf::GuiSys::GuiMan->DistributeClockTickEvents(FrameTimeF);

    if (!getWindowAttrib(GLFW_ICONIFIED))
    {
        // Render all the GUIs.
        MatSys::Renderer->BeginFrame(m_TotalTime);

        cf::GuiSys::GuiMan->RenderAll();

        MatSys::Renderer->EndFrame();
        swapBuffers();
    }
    else
    {
        // The main window is minimized - give other applications a chance to run.
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

    // Update the sound system (Reposition sound sources, update streams).
    SoundSystem->Update();

    // Run a client and a server frame.
    m_Resources->runFrame(FrameTimeF);
}


void ClientMainWindowT::FramebufferSizeEvent(int width, int height)
{
    if (!m_Resources) return;

    // The framebuffer's width and height are given in pixels (not in screen coordinates).
    MatSys::Renderer->SetViewport(0, 0, width, height);
}


void ClientMainWindowT::KeyEvent(int key, int scancode, int action, int mods)
{
    if (!m_Resources) return;

    if (action == GLFW_PRESS)
    {
        switch (key)
        {
            case GLFW_KEY_F1:
            {
                // Activate the in-game console GUI (it's "F1" now, not CK_GRAVE ("^", accent grave) any longer).
                IntrusivePtrT<cf::GuiSys::GuiImplT> ConsoleGui = cf::GuiSys::GuiMan->Find("Games/"+ m_GameInfo.GetName() +"/GUIs/Console_main.cgui");

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

            case GLFW_KEY_F5:
            {
                TakeScreenshot();
                return;
            }

            case GLFW_KEY_F11:
            {
                // In full screen mode, F11 switches to windowed mode.
                // In windowed mode, F11 switches to full screen mode at desktop resolution,
                // Ctrl+F11 switches to full screen mode at the previously used resolution.
                toggleFullScreen(mods == GLFW_MOD_CONTROL);
                return;
            }
        }
    }

    // There was no special-case treatment for this key above.
    // Now translate the GLFW keycode to a CaKeyboardEventT keycode and handle it normally.
    const int caKey = getCaKey(key);

    if (caKey)
    {
        CaKeyboardEventT KeyboardEvent;

        if (action == GLFW_PRESS || action == GLFW_REPEAT)
            KeyboardEvent.Type = CaKeyboardEventT::CKE_KEYDOWN;
        else
            KeyboardEvent.Type = CaKeyboardEventT::CKE_KEYUP;

        KeyboardEvent.Key = caKey;

        cf::GuiSys::GuiMan->ProcessDeviceEvent(KeyboardEvent);
    }
}


void ClientMainWindowT::CharEvent(unsigned int codepoint)
{
    if (!m_Resources) return;

    CaKeyboardEventT KeyboardEvent;

    KeyboardEvent.Type = CaKeyboardEventT::CKE_CHAR;
    KeyboardEvent.Key  = codepoint;

    cf::GuiSys::GuiMan->ProcessDeviceEvent(KeyboardEvent);
}


void ClientMainWindowT::MouseMoveEvent(double xpos, double ypos)
{
    const double diffX = xpos - m_LastMousePosX;
    const double diffY = ypos - m_LastMousePosY;

    m_LastMousePosX = xpos;
    m_LastMousePosY = ypos;

    if (!m_Resources) return;

    IntrusivePtrT<cf::GuiSys::GuiImplT> ActiveGui = cf::GuiSys::GuiMan->GetTopmostActiveAndInteractive();

    if (ActiveGui == NULL)
        return;

    // If the active GUI is the client GUI, see how far the mouse cursor is off center,
    // derive a mouse event from it, then recenter the mouse cursor.
    if (ActiveGui->GetRootWindow()->GetBasics()->GetWindowName() == "Client")
    {
        if (m_LastMousePos != IN_CLIENT_3D_GUI)
        {
            setMouseCursorMode(GLFW_CURSOR_DISABLED);
            m_LastMousePos = IN_CLIENT_3D_GUI;

            // Disabling the mouse cursor captures it at the center of the window, implicitly moving it.
            // Therefore we must forego this mouse event and anticipate the next.
            // (This exploits an implementation detail, maybe we should just skip this event and the next.)
            unsigned int sizeX, sizeY;

            getWindowSize(sizeX, sizeY);
            m_LastMousePosX = sizeX / 2;
            m_LastMousePosY = sizeY / 2;
            return;
        }

        CaMouseEventT ME;

        ME.Type   = CaMouseEventT::CM_MOVE_X;
        ME.Amount = int(diffX);
        if (ME.Amount != 0) ActiveGui->ProcessDeviceEvent(ME);

        ME.Type   = CaMouseEventT::CM_MOVE_Y;
        ME.Amount = int(diffY);
        if (ME.Amount != 0) ActiveGui->ProcessDeviceEvent(ME);
    }
    else
    {
        if (m_LastMousePos != IN_OTHER_2D_GUI)
        {
            setMouseCursorMode(GLFW_CURSOR_HIDDEN);
            m_LastMousePos = IN_OTHER_2D_GUI;
        }

        // This is equivalent to calling cf::GuiSys::GuiMan->ProcessDeviceEvent(MouseEvent),
        // but computing the amount of mouse movement is much easier (and more precise) like this.
        float OldGuiMousePosX;
        float OldGuiMousePosY;
        unsigned int WinSizeX;
        unsigned int WinSizeY;

        ActiveGui->GetMousePos(OldGuiMousePosX, OldGuiMousePosY);
        getWindowSize(WinSizeX, WinSizeY);

        const float NewGuiMousePosX = float(xpos) * (cf::GuiSys::VIRTUAL_SCREEN_SIZE_X / WinSizeX);
        const float NewGuiMousePosY = float(ypos) * (cf::GuiSys::VIRTUAL_SCREEN_SIZE_Y / WinSizeY);

        // This works, but doesn't forward the mouse event to the windows.
        // ActiveGui->SetMousePos(NewGuiMousePosX, NewGuiMousePosY);

        CaMouseEventT ME;

        ME.Type   = CaMouseEventT::CM_MOVE_X;
        ME.Amount = int(NewGuiMousePosX - OldGuiMousePosX);
        if (ME.Amount != 0) ActiveGui->ProcessDeviceEvent(ME);

        ME.Type   = CaMouseEventT::CM_MOVE_Y;
        ME.Amount = int(NewGuiMousePosY - OldGuiMousePosY);
        if (ME.Amount != 0) ActiveGui->ProcessDeviceEvent(ME);
    }
}


void ClientMainWindowT::MouseButtonEvent(int button, int action, int mods)
{
    if (!m_Resources) return;

    CaMouseEventT MouseEvent;

    MouseEvent.Type   = button == GLFW_MOUSE_BUTTON_LEFT ? CaMouseEventT::CM_BUTTON0 : CaMouseEventT::CM_BUTTON1;
    MouseEvent.Amount = action == GLFW_PRESS ? 1 : 0;

    cf::GuiSys::GuiMan->ProcessDeviceEvent(MouseEvent);
}


void ClientMainWindowT::MouseScrollEvent(double xoffset, double yoffset)
{
    if (!m_Resources) return;

    CaMouseEventT MouseEvent;

    MouseEvent.Type   = CaMouseEventT::CM_MOVE_Z;
    MouseEvent.Amount = int(yoffset);

    cf::GuiSys::GuiMan->ProcessDeviceEvent(MouseEvent);
}


static const char* ScreenShotSuffixTypes[] = { "jpg", "png", "bmp", NULL };
static ConVarT ScreenShotSuffix("screenSuffix", "jpg", ConVarT::FLAG_MAIN_EXE, "The suffix to be used for screen-shot image files. Only \"jpg\", \"png\" and \"bmp\" are allowed.", ScreenShotSuffixTypes);


void ClientMainWindowT::TakeScreenshot() const
{
    unsigned int Width;
    unsigned int Height;
    static ArrayT<uint32_t> FrameBuffer;

    getFramebufferSize(Width, Height);

    FrameBuffer.Overwrite();
    FrameBuffer.PushBackEmpty(Width*Height);

    // Read the pixels from the OpenGL back buffer into FrameBuffer.
    // Note that the first two parameters (0, 0) specify the left BOTTOM corner of the desired rectangle!
    glReadPixels(0, 0, Width, Height, GL_RGBA, GL_UNSIGNED_BYTE, &FrameBuffer[0]);

    // As mentioned above is the data in the FrameBuffer bottom-up.
    // Thus now swap all lines (vertical mirroring).
    for (unsigned int y = 0; y < Height/2; y++)
    {
        uint32_t* UpperRow = &FrameBuffer[0] +         y    * Width;
        uint32_t* LowerRow = &FrameBuffer[0] + (Height-y-1) * Width;

        for (unsigned int x = 0; x < Width; x++)
        {
            const uint32_t Swap = *UpperRow;

            *UpperRow = *LowerRow;
            *LowerRow = Swap;

            UpperRow++;
            LowerRow++;
        }
    }

    // Find the next free image name and save the file.
    for (unsigned int Nr = 0; Nr < 10000; Nr++)
    {
        char fn[16];
        snprintf(fn, 16, "cafu_%04u.%s", Nr, ScreenShotSuffix.GetValueString().c_str());

        if (FILE* file = fopen(fn, "r"))
        {
            // A file with that name already exists, try the next.
            fclose(file);
            continue;
        }

        if (BitmapT(Width, Height, &FrameBuffer[0]).SaveToDisk(fn))
        {
            Console->Print(std::string("Screenshot \"") + fn + "\" saved.\n");
        }
        else
        {
            Console->Warning(std::string("Screenshot \"") + fn + "\" could not be saved.\n");
        }

        break;
    }
}


struct KeyCodePairT
{
    int                    glfwKey;
    CaKeyboardEventT::KeyT CaKC;
};


static const KeyCodePairT KeyCodes[]=
{
    { GLFW_KEY_SPACE,       CaKeyboardEventT::CK_SPACE },
    { GLFW_KEY_APOSTROPHE,  CaKeyboardEventT::CK_APOSTROPHE },
    { GLFW_KEY_COMMA,       CaKeyboardEventT::CK_COMMA },
    { GLFW_KEY_MINUS,       CaKeyboardEventT::CK_MINUS },
    { GLFW_KEY_PERIOD,      CaKeyboardEventT::CK_PERIOD },
    { GLFW_KEY_SLASH,       CaKeyboardEventT::CK_SLASH },
    { GLFW_KEY_0,           CaKeyboardEventT::CK_0 },
    { GLFW_KEY_1,           CaKeyboardEventT::CK_1 },
    { GLFW_KEY_2,           CaKeyboardEventT::CK_2 },
    { GLFW_KEY_3,           CaKeyboardEventT::CK_3 },
    { GLFW_KEY_4,           CaKeyboardEventT::CK_4 },
    { GLFW_KEY_5,           CaKeyboardEventT::CK_5 },
    { GLFW_KEY_6,           CaKeyboardEventT::CK_6 },
    { GLFW_KEY_7,           CaKeyboardEventT::CK_7 },
    { GLFW_KEY_8,           CaKeyboardEventT::CK_8 },
    { GLFW_KEY_9,           CaKeyboardEventT::CK_9 },
    { GLFW_KEY_SEMICOLON,   CaKeyboardEventT::CK_SEMICOLON },
    { GLFW_KEY_EQUAL,       CaKeyboardEventT::CK_EQUALS },
    { GLFW_KEY_A,           CaKeyboardEventT::CK_A },
    { GLFW_KEY_B,           CaKeyboardEventT::CK_B },
    { GLFW_KEY_C,           CaKeyboardEventT::CK_C },
    { GLFW_KEY_D,           CaKeyboardEventT::CK_D },
    { GLFW_KEY_E,           CaKeyboardEventT::CK_E },
    { GLFW_KEY_F,           CaKeyboardEventT::CK_F },
    { GLFW_KEY_G,           CaKeyboardEventT::CK_G },
    { GLFW_KEY_H,           CaKeyboardEventT::CK_H },
    { GLFW_KEY_I,           CaKeyboardEventT::CK_I },
    { GLFW_KEY_J,           CaKeyboardEventT::CK_J },
    { GLFW_KEY_K,           CaKeyboardEventT::CK_K },
    { GLFW_KEY_L,           CaKeyboardEventT::CK_L },
    { GLFW_KEY_M,           CaKeyboardEventT::CK_M },
    { GLFW_KEY_N,           CaKeyboardEventT::CK_N },
    { GLFW_KEY_O,           CaKeyboardEventT::CK_O },
    { GLFW_KEY_P,           CaKeyboardEventT::CK_P },
    { GLFW_KEY_Q,           CaKeyboardEventT::CK_Q },
    { GLFW_KEY_R,           CaKeyboardEventT::CK_R },
    { GLFW_KEY_S,           CaKeyboardEventT::CK_S },
    { GLFW_KEY_T,           CaKeyboardEventT::CK_T },
    { GLFW_KEY_U,           CaKeyboardEventT::CK_U },
    { GLFW_KEY_V,           CaKeyboardEventT::CK_V },
    { GLFW_KEY_W,           CaKeyboardEventT::CK_W },
    { GLFW_KEY_X,           CaKeyboardEventT::CK_X },
    { GLFW_KEY_Y,           CaKeyboardEventT::CK_Y },
    { GLFW_KEY_Z,           CaKeyboardEventT::CK_Z },

    { GLFW_KEY_ESCAPE,      CaKeyboardEventT::CK_ESCAPE },
    { GLFW_KEY_ENTER,       CaKeyboardEventT::CK_RETURN },
    { GLFW_KEY_TAB,         CaKeyboardEventT::CK_TAB },
    { GLFW_KEY_BACKSPACE,   CaKeyboardEventT::CK_BACKSPACE },
    { GLFW_KEY_INSERT,      CaKeyboardEventT::CK_INSERT },
    { GLFW_KEY_DELETE,      CaKeyboardEventT::CK_DELETE },

    { GLFW_KEY_RIGHT,       CaKeyboardEventT::CK_RIGHT },
    { GLFW_KEY_LEFT,        CaKeyboardEventT::CK_LEFT },
    { GLFW_KEY_DOWN,        CaKeyboardEventT::CK_DOWN },
    { GLFW_KEY_UP,          CaKeyboardEventT::CK_UP },
    { GLFW_KEY_PAGE_UP,     CaKeyboardEventT::CK_PGUP },
    { GLFW_KEY_PAGE_DOWN,   CaKeyboardEventT::CK_PGDN },
    { GLFW_KEY_HOME,        CaKeyboardEventT::CK_HOME },
    { GLFW_KEY_END,         CaKeyboardEventT::CK_END },
    { GLFW_KEY_CAPS_LOCK,   CaKeyboardEventT::CK_CAPITAL },
    { GLFW_KEY_SCROLL_LOCK, CaKeyboardEventT::CK_SCROLL },
    { GLFW_KEY_NUM_LOCK,    CaKeyboardEventT::CK_NUMLOCK },
 // { GLFW_KEY_PRINT,       CaKeyboardEventT:: },
    { GLFW_KEY_PAUSE,       CaKeyboardEventT::CK_PAUSE },

    { GLFW_KEY_F1,          CaKeyboardEventT::CK_F1 },
    { GLFW_KEY_F2,          CaKeyboardEventT::CK_F2 },
    { GLFW_KEY_F3,          CaKeyboardEventT::CK_F3 },
    { GLFW_KEY_F4,          CaKeyboardEventT::CK_F4 },
    { GLFW_KEY_F5,          CaKeyboardEventT::CK_F5 },
    { GLFW_KEY_F6,          CaKeyboardEventT::CK_F6 },
    { GLFW_KEY_F7,          CaKeyboardEventT::CK_F7 },
    { GLFW_KEY_F8,          CaKeyboardEventT::CK_F8 },
    { GLFW_KEY_F9,          CaKeyboardEventT::CK_F9 },
    { GLFW_KEY_F10,         CaKeyboardEventT::CK_F10 },
    { GLFW_KEY_F11,         CaKeyboardEventT::CK_F11 },
    { GLFW_KEY_F12,         CaKeyboardEventT::CK_F12 },
    { GLFW_KEY_F13,         CaKeyboardEventT::CK_F13 },
    { GLFW_KEY_F14,         CaKeyboardEventT::CK_F14 },
    { GLFW_KEY_F15,         CaKeyboardEventT::CK_F15 },

    { GLFW_KEY_KP_0,        CaKeyboardEventT::CK_NUMPAD0 },
    { GLFW_KEY_KP_1,        CaKeyboardEventT::CK_NUMPAD1 },
    { GLFW_KEY_KP_2,        CaKeyboardEventT::CK_NUMPAD2 },
    { GLFW_KEY_KP_3,        CaKeyboardEventT::CK_NUMPAD3 },
    { GLFW_KEY_KP_4,        CaKeyboardEventT::CK_NUMPAD4 },
    { GLFW_KEY_KP_5,        CaKeyboardEventT::CK_NUMPAD5 },
    { GLFW_KEY_KP_6,        CaKeyboardEventT::CK_NUMPAD6 },
    { GLFW_KEY_KP_7,        CaKeyboardEventT::CK_NUMPAD7 },
    { GLFW_KEY_KP_8,        CaKeyboardEventT::CK_NUMPAD8 },
    { GLFW_KEY_KP_9,        CaKeyboardEventT::CK_NUMPAD9 },
    { GLFW_KEY_KP_DECIMAL,  CaKeyboardEventT::CK_DECIMAL },
    { GLFW_KEY_KP_DIVIDE,   CaKeyboardEventT::CK_DIVIDE },
    { GLFW_KEY_KP_MULTIPLY, CaKeyboardEventT::CK_MULTIPLY },
    { GLFW_KEY_KP_SUBTRACT, CaKeyboardEventT::CK_SUBTRACT },
    { GLFW_KEY_KP_ADD,      CaKeyboardEventT::CK_ADD },
    { GLFW_KEY_KP_ENTER,    CaKeyboardEventT::CK_NUMPADENTER },
 // { GLFW_KEY_KP_EQUAL,    CaKeyboardEventT:: },

    { GLFW_KEY_LEFT_SHIFT,      CaKeyboardEventT::CK_LSHIFT },
    { GLFW_KEY_LEFT_CONTROL,    CaKeyboardEventT::CK_LCONTROL },
    { GLFW_KEY_LEFT_ALT,        CaKeyboardEventT::CK_LMENU },   // CK_LALT ??
    { GLFW_KEY_LEFT_SUPER,      CaKeyboardEventT::CK_LWIN },
    { GLFW_KEY_RIGHT_SHIFT,     CaKeyboardEventT::CK_RSHIFT },
    { GLFW_KEY_RIGHT_CONTROL,   CaKeyboardEventT::CK_RCONTROL },
    { GLFW_KEY_RIGHT_ALT,       CaKeyboardEventT::CK_RMENU },   // CK_RALT ??
    { GLFW_KEY_RIGHT_SUPER,     CaKeyboardEventT::CK_RWIN },
 // { GLFW_KEY_MENU,            CaKeyboardEventT:: },
    { 0,                        CaKeyboardEventT::KeyT(0) }     // end-of-array marker
};


/*static*/
int ClientMainWindowT::getCaKey(int glfwKey)
{
    for (unsigned int i = 0; KeyCodes[i].glfwKey != 0; i++)
        if (KeyCodes[i].glfwKey == glfwKey)
            return KeyCodes[i].CaKC;

    return 0;
}


/*static*/
int ClientMainWindowT::getGlfwKey(int caKey)
{
    for (unsigned int i = 0; KeyCodes[i].glfwKey != 0; i++)
        if (KeyCodes[i].CaKC == caKey)
            return KeyCodes[i].glfwKey;

    return 0;
}
