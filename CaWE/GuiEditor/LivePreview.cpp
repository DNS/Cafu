/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "LivePreview.hpp"
#include "../ParentFrame.hpp"
#include "../AppCaWE.hpp"

#include "GuiSys/GuiImpl.hpp"
#include "GuiSys/Window.hpp"
#include "OpenGL/OpenGLWindow.hpp"  // For CaMouseEventT and CaKeyboardEventT.
#include "MaterialSystem/Renderer.hpp"
#include "Math3D/Matrix.hpp"
#include "Math3D/Vector3.hpp"
#include "UniScriptState.hpp"

#include "wx/glcanvas.h"


using namespace GuiEditor;


namespace GuiEditor
{
    class PreviewCanvasT : public wxGLCanvas
    {
        public:

        PreviewCanvasT(LivePreviewT* Parent);


        private:

        LivePreviewT* m_Parent;
        unsigned long m_TimeLastFrame;
        wxPoint       m_LastMousePos;

        void OnIdle      (wxIdleEvent&  IE);
        void OnPaint     (wxPaintEvent& PE);
        void OnMouseWheel(wxMouseEvent& ME);
        void OnMouseMove (wxMouseEvent& ME);
        void OnLMouseDown(wxMouseEvent& ME);
        void OnLMouseUp  (wxMouseEvent& ME);
        void OnRMouseDown(wxMouseEvent& ME);
        void OnRMouseUp  (wxMouseEvent& ME);
        void OnMouseEnter(wxMouseEvent& ME);
        void OnKeyDown   (wxKeyEvent&   KE);
        void OnKeyUp     (wxKeyEvent&   KE);
        void OnChar      (wxKeyEvent&   CE);

        DECLARE_EVENT_TABLE()
    };
}


BEGIN_EVENT_TABLE(PreviewCanvasT, wxGLCanvas)
    EVT_IDLE        (PreviewCanvasT::OnIdle      )
    EVT_PAINT       (PreviewCanvasT::OnPaint     )
    EVT_MOUSEWHEEL  (PreviewCanvasT::OnMouseWheel)
    EVT_MOTION      (PreviewCanvasT::OnMouseMove )
    EVT_LEFT_DOWN   (PreviewCanvasT::OnLMouseDown)
    EVT_LEFT_UP     (PreviewCanvasT::OnLMouseUp  )
    EVT_RIGHT_DOWN  (PreviewCanvasT::OnRMouseDown)
    EVT_RIGHT_UP    (PreviewCanvasT::OnRMouseUp  )
    EVT_ENTER_WINDOW(PreviewCanvasT::OnMouseEnter)
    EVT_KEY_DOWN    (PreviewCanvasT::OnKeyDown   )
    EVT_KEY_UP      (PreviewCanvasT::OnKeyUp     )
    EVT_CHAR        (PreviewCanvasT::OnChar      )
END_EVENT_TABLE()


PreviewCanvasT::PreviewCanvasT(LivePreviewT* Parent)
    : wxGLCanvas(Parent, wxID_ANY, ParentFrameT::OpenGLAttributeList, wxDefaultPosition, wxSize(640, 480), wxWANTS_CHARS, "GuiPreviewWindow"),
      m_Parent(Parent),
      m_TimeLastFrame(0),
      m_LastMousePos(0, 0)
{
}


void PreviewCanvasT::OnIdle(wxIdleEvent& IE)
{
    this->Refresh(false);
    IE.RequestMore();
}


void PreviewCanvasT::OnPaint(wxPaintEvent& PE)
{
    wxPaintDC dc(this);     // It is VERY important not to omit this, or otherwise everything goes havoc.

    if (!wxGetApp().IsActive()) return;

    // We're drawing to this view now.
    SetCurrent(*wxGetApp().GetParentFrame()->m_GLContext);    // This is the method from the wxGLCanvas for activating the given RC with this window.
    wxSize CanvasSize=GetClientSize();

    MatSys::Renderer->SetViewport(0, 0, CanvasSize.GetWidth(), CanvasSize.GetHeight());

    // Determine how much time has passed since the previous frame.
    unsigned long TimeNow=::wxGetLocalTimeMillis().GetLo();
    unsigned long TimeElapsed=(m_TimeLastFrame==0) ? 0 : TimeNow-m_TimeLastFrame;

    m_TimeLastFrame=TimeNow;

    // Clear the buffers.
    MatSys::Renderer->ClearColor(0, 0, 0, 0);
    MatSys::Renderer->BeginFrame(TimeNow/1000.0);

    MatSys::Renderer->SetCurrentRenderAction(MatSys::RendererI::AMBIENT);
    MatSys::Renderer->SetCurrentLightMap(wxGetApp().GetParentFrame()->m_WhiteTexture);    // Set a proper default lightmap.
    MatSys::Renderer->SetCurrentLightDirMap(NULL);      // The MatSys provides a default for LightDirMaps when NULL is set.

    // Setup the matrices.
    MatSys::Renderer->PushMatrix(MatSys::RendererI::PROJECTION    );
    MatSys::Renderer->PushMatrix(MatSys::RendererI::MODEL_TO_WORLD);
    MatSys::Renderer->PushMatrix(MatSys::RendererI::WORLD_TO_VIEW );

    const float zNear=0.0f;
    const float zFar =1.0f;
    MatSys::Renderer->SetMatrix(MatSys::RendererI::PROJECTION,     MatrixT::GetProjOrthoMatrix(0, cf::GuiSys::VIRTUAL_SCREEN_SIZE_X, cf::GuiSys::VIRTUAL_SCREEN_SIZE_Y, 0, zNear, zFar));
    MatSys::Renderer->SetMatrix(MatSys::RendererI::MODEL_TO_WORLD, MatrixT());
    MatSys::Renderer->SetMatrix(MatSys::RendererI::WORLD_TO_VIEW,  MatrixT());

    const float FrameTime = float(TimeElapsed) / 1000.0f;

    m_Parent->GetScriptState()->RunPendingCoroutines(FrameTime);
    m_Parent->GetGui()->DistributeClockTickEvents(FrameTime);

    m_Parent->GetGui()->Render();

    // Restore the previously active matrices.
    MatSys::Renderer->PopMatrix(MatSys::RendererI::PROJECTION    );
    MatSys::Renderer->PopMatrix(MatSys::RendererI::MODEL_TO_WORLD);
    MatSys::Renderer->PopMatrix(MatSys::RendererI::WORLD_TO_VIEW );

    MatSys::Renderer->EndFrame();
    SwapBuffers();
}


void PreviewCanvasT::OnMouseWheel(wxMouseEvent& ME)
{
    CaMouseEventT MouseEvent;
    MouseEvent.Type  =CaMouseEventT::CM_MOVE_Z;
    MouseEvent.Amount=ME.GetWheelDelta();

    m_Parent->GetGui()->ProcessDeviceEvent(MouseEvent);
}


void PreviewCanvasT::OnMouseMove(wxMouseEvent& ME)
{
    CaMouseEventT MouseEvent;
    MouseEvent.Type  =CaMouseEventT::CM_MOVE_X;
    MouseEvent.Amount=ME.GetX()-m_LastMousePos.x;

    m_Parent->GetGui()->ProcessDeviceEvent(MouseEvent);

    MouseEvent.Type  =CaMouseEventT::CM_MOVE_Y;
    MouseEvent.Amount=ME.GetY()-m_LastMousePos.y;

    m_Parent->GetGui()->ProcessDeviceEvent(MouseEvent);

    m_LastMousePos=ME.GetPosition();
}


void PreviewCanvasT::OnLMouseDown(wxMouseEvent& ME)
{
    CaMouseEventT MouseEvent;
    MouseEvent.Type  =CaMouseEventT::CM_BUTTON0;
    MouseEvent.Amount=1;

    m_Parent->GetGui()->ProcessDeviceEvent(MouseEvent);
}


void PreviewCanvasT::OnLMouseUp(wxMouseEvent& ME)
{
    CaMouseEventT MouseEvent;
    MouseEvent.Type=CaMouseEventT::CM_BUTTON0;
    MouseEvent.Amount=0;

    m_Parent->GetGui()->ProcessDeviceEvent(MouseEvent);
}


void PreviewCanvasT::OnRMouseDown(wxMouseEvent& ME)
{
    CaMouseEventT MouseEvent;
    MouseEvent.Type  =CaMouseEventT::CM_BUTTON1;
    MouseEvent.Amount=1;

    m_Parent->GetGui()->ProcessDeviceEvent(MouseEvent);
}


void PreviewCanvasT::OnRMouseUp(wxMouseEvent& ME)
{
    CaMouseEventT MouseEvent;
    MouseEvent.Type  =CaMouseEventT::CM_BUTTON1;
    MouseEvent.Amount=0;

    m_Parent->GetGui()->ProcessDeviceEvent(MouseEvent);
}


void PreviewCanvasT::OnMouseEnter(wxMouseEvent& ME)
{
    m_Parent->GetGui()->SetMousePos(ME.GetX(), ME.GetY());

    m_LastMousePos=ME.GetPosition();
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


void PreviewCanvasT::OnKeyDown(wxKeyEvent& KE)
{
    // Look for the pressed keys keycode in the table and translate it to a CaKeyCode if found.
    for (int KeyCodeNr=0; KeyCodes[KeyCodeNr].wxKC!=0; KeyCodeNr++)
    {
        if (KeyCodes[KeyCodeNr].wxKC==KE.GetKeyCode())
        {
            CaKeyboardEventT KeyboardEvent;
            KeyboardEvent.Type=CaKeyboardEventT::CKE_KEYDOWN;
            KeyboardEvent.Key =KeyCodes[KeyCodeNr].CaKC;

            if (m_Parent->GetGui()->ProcessDeviceEvent(KeyboardEvent)) return;
            break;  // Event not handled, call KE.Skip() below.
        }
    }

    KE.Skip();
}


void PreviewCanvasT::OnKeyUp(wxKeyEvent& KE)
{
    // Look for the released keys keycode in the table and translate it to a CaKeyCode if found.
    for (int KeyCodeNr=0; KeyCodes[KeyCodeNr].wxKC!=0; KeyCodeNr++)
    {
        if (KeyCodes[KeyCodeNr].wxKC==KE.GetKeyCode())
        {
            CaKeyboardEventT KeyboardEvent;
            KeyboardEvent.Type=CaKeyboardEventT::CKE_KEYUP;
            KeyboardEvent.Key =KeyCodes[KeyCodeNr].CaKC;

            if (m_Parent->GetGui()->ProcessDeviceEvent(KeyboardEvent)) return;
            break;  // Event not handled, call KE.Skip() below.
        }
    }

    KE.Skip();
}


void PreviewCanvasT::OnChar(wxKeyEvent& CE)
{
    CaKeyboardEventT KeyboardEvent;
    KeyboardEvent.Type=CaKeyboardEventT::CKE_CHAR;
    KeyboardEvent.Key =CE.GetKeyCode();

    m_Parent->GetGui()->ProcessDeviceEvent(KeyboardEvent);
}


BEGIN_EVENT_TABLE(LivePreviewT, wxDialog)
    EVT_CLOSE(LivePreviewT::OnClose)
END_EVENT_TABLE()


LivePreviewT::LivePreviewT(wxWindow* Parent, cf::UniScriptStateT* ScriptState, IntrusivePtrT<cf::GuiSys::GuiImplT> Gui, const wxString& ScriptFileName)
    : wxDialog(Parent, wxID_ANY, "GUI Live Preview: "+ScriptFileName),
      m_ScriptState(ScriptState),
      m_Gui(Gui),
      m_Canvas(NULL)
{
    wxASSERT(m_Gui!=NULL);

    // Create sizer and insert canvas.
    this->SetSizeHints(wxDefaultSize, wxDefaultSize);

    wxBoxSizer* Sizer;
    Sizer=new wxBoxSizer(wxVERTICAL);

    m_Canvas=new PreviewCanvasT(this);
    Sizer->Add(m_Canvas, 1, wxEXPAND, 0);

    this->SetSizer(Sizer);
    this->Layout();
    Sizer->Fit(this);
}


LivePreviewT::~LivePreviewT()
{
    m_Gui = NULL;

    delete m_ScriptState;
    m_ScriptState = NULL;
}


void LivePreviewT::OnClose(wxCloseEvent& CE)
{
    Destroy();
}
