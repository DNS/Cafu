/*
=================================================================================
This file is part of Cafu, the open-source game engine and graphics engine
for multiplayer, cross-platform, real-time 3D action.
Copyright (C) 2002-2011 Carsten Fuchs Software.

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

#define DLL_EXPORT_HEADER
#include "OpenGLWindow.hpp"

#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
    #include "DirectX/DirectInput.hpp"
#else
    #include <GL/glx.h>
    #include <X11/Xlib.h>
#endif

#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <GL/gl.h>

#include "Templates/Array.hpp"


static const int CKE_QUEUE_MAX=256;     ///< Max. size of keyboard event queue. MUST be a power of 2!
static const int CME_QUEUE_MAX=256;     ///< Max. size of mouse event queue. MUST be a power of 2!


// This class implements the SingleOpenGLWindowT interface.
class SingleOpenGLWindowImplT : public SingleOpenGLWindowT
{
    public:

    SingleOpenGLWindowImplT();

    // Methods of the SingleOpenGLWindowT interface.
    const char*        Open(const std::string& Title_, unsigned int Width_, unsigned int Height_, char BPP_, bool FullScreen_);
    bool               HandleWindowMessages();
    void               PostQuitMessage();
    unsigned int       GetSystemFontDisplayList(int FontHeight, const char* FontName);
    void               SwapBuffers();
    uint32_t*          GetFrameBuffer(unsigned int& Width_, unsigned int& Height_);
    void               Close();
    int                GetNextKeyboardEvent(CaKeyboardEventT& KeyboardEvent);
    int                GetNextMouseEvent(CaMouseEventT& MouseEvent);

    const std::string& GetTitle() { return Title; }
    unsigned int       GetWidth() { return Width; }
    unsigned int       GetHeight() { return Height; }
    char               GetBPP() { return BPP; }
    bool               GetFullScreen() { return FullScreen; }
    bool               GetIsMinimized() { return IsMinimized; }
 // bool               GetWindowIsOpen();
 // unsigned int       GetRenderingContextCounter();
    bool*              GetKeyboardState() { return KeyboardState; }
    char               GetMouseButtonState() { return MouseButtonState; }


    // Other methods.
    void SetIsMinimized(bool IsMin_) { IsMinimized=IsMin_; }    ///< Called from the windows main proc.
    void Resize(unsigned int Width_, unsigned int Height_);


    private:

    std::string         Title;
    unsigned int        Width;
    unsigned int        Height;
    char                BPP;
    bool                FullScreen;
    bool                IsMinimized;
    bool                WindowIsOpen;
    unsigned int        RenderingContextCounter;
    int                 CKE_Queue_Start;            ///< First valid entry in queue.
    int                 CKE_Queue_Size;             ///< Number of entries in queue.
    CaKeyboardEventT    CKE_Queue[CKE_QUEUE_MAX];   ///< Keyboard event queue.
    bool                KeyboardState[256];
    char                MouseButtonState;

    // OS specific private data.
#ifdef _WIN32
    friend LRESULT CALLBACK WinProc(HWND hWnd, UINT MessageID, WPARAM wParam, LPARAM lParam);

    HINSTANCE           hInstance;                  ///< Unsere Programminstanz.
    HWND                hWindow;                    ///< Handle dieses Windows!
    HDC                 hDC;                        ///< Device Context.
    HGLRC               hRC;                        ///< OpenGL Rendering Context.
    DirectInputT        DirectInput;                ///< DirectInput.
    const bool          IsMouseSwapped;             ///< Are the meanings of the left and right mouse buttons swapped on this system?
#else
    Display*            DisplayPtr;                 ///< Connection to the X server.
    Window              Win;                        ///< The window.
    GLXContext          RC;                         ///< OpenGL Rendering Context.
    const unsigned long EventMask;                  ///< Inited in the ctor.
    bool                PostQuitMsgFlag;            ///< Post quit message next time?
    int                 CME_Queue_Start;            ///< First valid entry in queue.
    int                 CME_Queue_Size;             ///< Number of entries in queue.
    CaMouseEventT       CME_Queue[CME_QUEUE_MAX];   ///< Mouse event queue.
#endif
};


static SingleOpenGLWindowImplT SingleWin;
SingleOpenGLWindowT*           SingleOpenGLWindow=&SingleWin;



/// Passt den OpenGL-ViewPort an eine neue Fensterhöhe/-breite an.
void SingleOpenGLWindowImplT::Resize(unsigned int Width_, unsigned int Height_)
{
    Width =Width_;
    Height=Height_;

    if (Height==0) Height=1;          // Division durch 0 vermeiden.

    glViewport(0, 0, Width, Height);  // ViewPort neu setzen.


    glMatrixMode(GL_PROJECTION);                                // Projektionsmatrix zurücksetzen.

    const double FieldOfView=67.5;      // Field of view angle, in y-direction.
    const double AspectRatio=double(Width)/double(Height);
    const double Near       =100.0;
 // const double Far        =100000.0;
    const double cotanFOV   =1.0/tan(FieldOfView/2.0/180.0*3.14159265359);

    // This is the OpenGL projection matrix with the "far" clip plane moved to infinity,
    // according to the NVidia paper about robust stencil buffered shadows.
    // Note that this matrix is actually transposed, as this is what 'glLoadMatrix()' expects.
    const double ProjMatInf[4][4]={ { cotanFOV/AspectRatio,      0.0,       0.0,  0.0 },
                                    {                  0.0, cotanFOV,       0.0,  0.0 },
                                    {                  0.0,      0.0,      -1.0, -1.0 },
                                    {                  0.0,      0.0, -2.0*Near,  0.0 } };

    glLoadMatrixd(&ProjMatInf[0][0]);

    // Aspekt-Ratio neu setzen, 67.5 Grad entspricht 90 Grad der DOS Version (90 Grad in X-Richtung).
    // glLoadIdentity();
    // gluPerspective(FieldOfView, AspectRatio, Near, Far);


    glMatrixMode(GL_MODELVIEW);                                 // Modelview-Matrix zurücksetzen.
    glLoadIdentity();
}


// CallBack-Funktion für die Window-Messages.
#ifdef _WIN32
LRESULT CALLBACK WinProc(HWND hWnd, UINT MessageID, WPARAM wParam, LPARAM lParam)
{
    bool IsKeyDown=false;

    switch (MessageID)
    {
        case WM_ACTIVATE:
            SingleWin.SetIsMinimized(HIWORD(wParam)!=0);
            return 0;

        case WM_SYSCOMMAND:
            if (wParam==SC_SCREENSAVE || wParam==SC_MONITORPOWER) return 0;
            break;

        case WM_KEYDOWN:
        case WM_SYSKEYDOWN: IsKeyDown=true;     // Intentional fall-though here.
        case WM_KEYUP:
        case WM_SYSKEYUP:
        {
            // Für lParam vgl. Petzold S. 227 und 215.
            CaKeyboardEventT CKE;

            CKE.Type=IsKeyDown ? CaKeyboardEventT::CKE_KEYDOWN : CaKeyboardEventT::CKE_KEYUP;
            CKE.Key =(lParam >> 16) & 0xFF;     // Convert the OEM scancode into a CK_* key type.

            // I've made some experiments, i.e. printing out the value of CKE.Key for all sorts of keys.
            // It turns out that the upmost (8th) bit of the OEM scancode is apparently never used,
            // but instead the flag for extended keys is set for some keys.
            // However, if I set the 8th bit whenever the extended flag is set, it turns out that the result exactly
            // matches the CaKeyboardEventT::KeyT enumaration, which in turn matches the DirectInput DIK_ constants list!
            // Also see Petzolds description about the "lParam" value for WM_*KEY* messages at pages 215-217, especially the
            // sections about "OEM-Scancode" and "Flag für erweiterte Tasten" - it's very good!
            if ((lParam & (1 << 24))!=0) CKE.Key|=0x80;

            // Store the event in the event queue.
            if (SingleWin.CKE_Queue_Size<CKE_QUEUE_MAX)
            {
                SingleWin.CKE_Queue[(SingleWin.CKE_Queue_Start+SingleWin.CKE_Queue_Size) & (CKE_QUEUE_MAX-1)]=CKE;
                SingleWin.CKE_Queue_Size++;
            }
            break;      // Intentionally break, not return 0, so that the DefWindowProc() is called.
        }

        case WM_CHAR:
        {
            CaKeyboardEventT CKE;

            CKE.Type=CaKeyboardEventT::CKE_CHAR;
            CKE.Key =int(wParam);

            // Store the event in the event queue.
            if (SingleWin.CKE_Queue_Size<CKE_QUEUE_MAX)
            {
                SingleWin.CKE_Queue[(SingleWin.CKE_Queue_Start+SingleWin.CKE_Queue_Size) & (CKE_QUEUE_MAX-1)]=CKE;
                SingleWin.CKE_Queue_Size++;
            }
            return 0;
        }

        case WM_SIZE:
            SingleWin.Resize(LOWORD(lParam), HIWORD(lParam));
            return 0;

        case WM_CLOSE:
            PostQuitMessage(0);
            return 0;
    }

    return DefWindowProc(hWnd, MessageID, wParam, lParam);
}
#endif


SingleOpenGLWindowImplT::SingleOpenGLWindowImplT()
    : Title(""),
      Width(0),
      Height(0),
      BPP(0),
      FullScreen(false),
      IsMinimized(false),
      WindowIsOpen(false),
      RenderingContextCounter(0),
      CKE_Queue_Start(0),
      CKE_Queue_Size(0),
   // CKE_Queue(),
   // KeyboardState(),
      MouseButtonState(false)
#ifdef _WIN32
    , IsMouseSwapped(GetSystemMetrics(SM_SWAPBUTTON)!=0)
#else
    , EventMask(EnterWindowMask | PointerMotionMask | ButtonPressMask | ButtonReleaseMask | StructureNotifyMask | KeyPressMask | KeyReleaseMask),
      PostQuitMsgFlag(false),
      CME_Queue_Start(0),
      CME_Queue_Size(0)
   // CME_Queue()
#endif
{
}


const char* SingleOpenGLWindowImplT::Open(const std::string& Title_, unsigned int Width_, unsigned int Height_, char BPP_, bool FullScreen_)
{
#ifdef _WIN32
    if (WindowIsOpen) return "OpenGLWindow is already open.";

    hInstance=GetModuleHandle(NULL);   // 'GetModuleHandle(NULL)' entspricht 'hInstance' von 'WinMain()'.
    hWindow  =NULL;
    hDC      =NULL;
    hRC      =NULL;

    Title      =Title_;
    Width      =Width_;
    Height     =Height_;
    BPP        =BPP_;
    FullScreen =FullScreen_;
    IsMinimized=false;

    for (unsigned int c=0; c<256; c++) KeyboardState[c]=false;


    // 1. Window-Klasse ausfüllen und registrieren
    // *******************************************

    WNDCLASSEX MainWindowClass;

    MainWindowClass.cbSize       =sizeof(WNDCLASSEX);                       // Größe dieser Struktur
    MainWindowClass.style        =CS_VREDRAW | CS_HREDRAW | CS_OWNDC;       // Fensterklassenstil
    MainWindowClass.lpfnWndProc  =WinProc;                                  // Zeiger auf Handler-Funktion
    MainWindowClass.cbClsExtra   =0;                                        // Zusätzlicher Platz für KlassenInfos
    MainWindowClass.cbWndExtra   =0;                                        // Zusätzlicher Platz für FensterInfos
    MainWindowClass.hInstance    =hInstance;                                // Unsere Programm-ID
    MainWindowClass.hIcon        =LoadIcon(NULL, IDI_APPLICATION);          // Icon
    MainWindowClass.hIconSm      =LoadIcon(NULL, IDI_APPLICATION);          // Kleines Icon für Task-Bar
    MainWindowClass.hCursor      =LoadCursor(NULL, IDC_ARROW);              // Cursor
    MainWindowClass.hbrBackground=NULL;                                     // Hintergrund-Brush
    MainWindowClass.lpszMenuName =NULL;                                     // Menü
    MainWindowClass.lpszClassName="CafuMain";                               // Name dieser Fensterklasse

    if (!RegisterClassEx(&MainWindowClass)) return "Unable to register the window class.";


    // 2. Display Settings ggf. anpassen
    // *********************************

    if (FullScreen)
    {
        DEVMODE dmScreenSettings;

        memset(&dmScreenSettings, 0, sizeof(dmScreenSettings));
        dmScreenSettings.dmSize      =sizeof(dmScreenSettings);
        dmScreenSettings.dmPelsWidth =Width;
        dmScreenSettings.dmPelsHeight=Height;
        dmScreenSettings.dmBitsPerPel=BPP;
        dmScreenSettings.dmFields    =DM_PELSWIDTH | DM_PELSHEIGHT | DM_BITSPERPEL;

        // CDS_FULLSCREEN gets rid of start bar.
        if (ChangeDisplaySettings(&dmScreenSettings, CDS_FULLSCREEN)!=DISP_CHANGE_SUCCESSFUL)
        {
            UnregisterClass("CafuMain", hInstance);
            return "Unable to change display settings.";
        }

        ShowCursor(false);
    }


    // 3. Window-Rectangle anpassen
    // ****************************

    unsigned int Style=FullScreen ? WS_POPUP : WS_OVERLAPPEDWINDOW;
    unsigned int ExStyle=WS_EX_APPWINDOW;

    RECT GLWindowRect;

    GLWindowRect.top   =0;
    GLWindowRect.left  =0;
    GLWindowRect.right =Width;
    GLWindowRect.bottom=Height;

    if (!AdjustWindowRectEx(&GLWindowRect, Style, false, ExStyle))
    {
        Close();
        return "Unable to adjust window rectangle.";
    }


    // 4. Window erzeugen
    // ******************

    hWindow=CreateWindowEx(ExStyle,                                    // Fensterstil (erweitert)
                           "CafuMain",                                 // Name der Fensterklasse
                           Title.c_str(),                              // Fenstertitel
                           Style | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,  // Fensterstil
                           0, // FullScreen ? 0 : CW_USEDEFAULT,       // X-Position
                           0, // FullScreen ? 0 : CW_USEDEFAULT,       // Y-Position
                           GLWindowRect.right-GLWindowRect.left,       // Breite
                           GLWindowRect.bottom-GLWindowRect.top,       // Höhe
                           NULL,                                       // Übergeordnetes Fenster
                           NULL,                                       // Menü
                           hInstance,                                  // Unsere Programm-ID
                           NULL);                                      // Zusätzliche Parameter

    if (!hWindow)
    {
        Close();
        return "Unable to create window.";
    }


    // 5. Device Context erfragen
    // **************************

    hDC=GetDC(hWindow);

    if (!hDC)
    {
        Close();
        return "Unable to obtain a GL device context.";
    }


    // 6. PixelFormat abfragen und setzen
    // **********************************

    PIXELFORMATDESCRIPTOR PFD;

    memset(&PFD, 0, sizeof(PFD));
    PFD.nSize       =sizeof(PFD);
    PFD.nVersion    =1;
    PFD.dwFlags     =PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    PFD.iPixelType  =PFD_TYPE_RGBA;
    PFD.cColorBits  =BPP;
    PFD.cDepthBits  =32;
    PFD.cStencilBits=8;
    PFD.iLayerType=PFD_MAIN_PLANE;

    int PixelFormat=ChoosePixelFormat(hDC, &PFD);

    if (!PixelFormat)
    {
        Close();
        return "Unable to choose a pixel format.";
    }

    if (!DescribePixelFormat(hDC, PixelFormat, sizeof(PFD), &PFD))
    {
        Close();
        return "Unable to verify pixel format.";
    }

    static char ErrorMsg[1024];
    const char* s1="Selected pixel format mismatches:\n";
    const char* s2="\n\nThis is probably a problem with your video card (or its driver).\n"
                   "Please make sure you have the latest drivers installed.\n"
                   "If it still doesn't work, please let me know.\n"
                   "(Email to CarstenFuchs@T-Online.de, and please include the above message!)";

    if ((PFD.dwFlags & PFD_DRAW_TO_WINDOW)!=PFD_DRAW_TO_WINDOW) { Close(); sprintf(ErrorMsg, "%sPFD_DRAW_TO_WINDOW is not supported.                 %s", s1,                   s2); return ErrorMsg; }
    if ((PFD.dwFlags & PFD_SUPPORT_OPENGL)!=PFD_SUPPORT_OPENGL) { Close(); sprintf(ErrorMsg, "%sOpenGL is not supported.                             %s", s1,                   s2); return ErrorMsg; }
    if ((PFD.dwFlags & PFD_DOUBLEBUFFER  )!=PFD_DOUBLEBUFFER  ) { Close(); sprintf(ErrorMsg, "%sDouble-buffering is not supported.                   %s", s1,                   s2); return ErrorMsg; }
    if (PFD.iPixelType!=PFD_TYPE_RGBA                         ) { Close(); sprintf(ErrorMsg, "%sPixel type RGBA is not supported.                    %s", s1,                   s2); return ErrorMsg; }
 // if (PFD.cColorBits<BPP                                    ) { Close(); sprintf(ErrorMsg, "%sOnly %u color bits found (at least 15 are required). %s", s1, PFD.cColorBits  , s2); return ErrorMsg; }
    if (PFD.cDepthBits<16                                     ) { Close(); sprintf(ErrorMsg, "%sOnly %u depth bits found (at least 16 are required). %s", s1, PFD.cDepthBits  , s2); return ErrorMsg; }
    if (PFD.cStencilBits<8                                    ) { Close(); sprintf(ErrorMsg, "%sOnly %u stencil bits found (at least 8 are required).%s", s1, PFD.cStencilBits, s2); return ErrorMsg; }
    if (PFD.iLayerType!=PFD_MAIN_PLANE                        ) { Close(); sprintf(ErrorMsg, "%sLayer type PFD_MAIN_PLANE is not supported.          %s", s1,                   s2); return ErrorMsg; }

    if(!SetPixelFormat(hDC, PixelFormat, &PFD))
    {
        Close();
        return "Unable to set the pixel format.";
    }


    // 7. Rendering Context erzeugen und aktivieren
    // ********************************************

    hRC=wglCreateContext(hDC);

    if (!hRC)
    {
        Close();
        return "Unable to create a GL rendering context.";
    }

    if(!wglMakeCurrent(hDC, hRC))
    {
        Close();
        return "Unable to activate the GL rendering context.";
    }


    // 8. Fenster anzeigen und einrichten
    // **********************************

    ShowWindow(hWindow, SW_SHOW);                      // Show the window.
    SetForegroundWindow(hWindow);                      // Slightly higher priority.
    SetFocus(hWindow);                                 // Sets keyboard focus to the window.


    // 8.1. DirectInput initialisieren
    // *******************************

    HRESULT hResult=DirectInput.Initialize(hInstance, hWindow);

    if (hResult!=DI_OK)
    {
        static char ErrorMsg[80];

        Close();
        sprintf(ErrorMsg, "Unable to initialize DirectInput!\nhResult=%u", hResult);
        return ErrorMsg;
    }
#else
    PostQuitMsgFlag=false;
    DisplayPtr     =XOpenDisplay(NULL);

    if (!DisplayPtr)                                return "Cannot open display.";
    if (!glXQueryExtension(DisplayPtr, NULL, NULL)) return "No GLX extension.";

#ifdef DEBUG
    int ExtVersionMajor=0;
    int ExtVersionMinor=0;
    if (glXQueryVersion(DisplayPtr, &ExtVersionMajor, &ExtVersionMinor)) printf("GLX version: %i.%i\n", ExtVersionMajor, ExtVersionMinor);
                                                                    else printf("GLX version could not be determined.\n");
#endif

    Title      =Title_;
    Width      =FullScreen_ ? DisplayWidth (DisplayPtr, DefaultScreen(DisplayPtr)) : Width_;
    Height     =FullScreen_ ? DisplayHeight(DisplayPtr, DefaultScreen(DisplayPtr)) : Height_;
    BPP        =BPP_;
    FullScreen =FullScreen_;
    IsMinimized=false;

    for (unsigned int c=0; c<256; c++) KeyboardState[c]=false;


    int VisualInfo_Features[] =
    {
        GLX_RGBA,
        GLX_RED_SIZE, 8,
        GLX_GREEN_SIZE, 8,
        GLX_BLUE_SIZE, 8,
        GLX_ALPHA_SIZE, 8,
        GLX_DEPTH_SIZE, 16,
        GLX_STENCIL_SIZE, 8,
        GLX_DOUBLEBUFFER,
        None
    };


    XVisualInfo* VisualInfo=glXChooseVisual(DisplayPtr, DefaultScreen(DisplayPtr), VisualInfo_Features);
    if (!VisualInfo) return "Cannot find visual. (You can possibly fix this by setting your desktop bit depth to 32 BPP. Just a guess.)";

    RC=glXCreateContext(DisplayPtr, VisualInfo, None, True);
    if (!RC) return "Cannot create GLX context.";


    XSetWindowAttributes WinAttribs;

    WinAttribs.colormap         =XCreateColormap(DisplayPtr, RootWindow(DisplayPtr, VisualInfo->screen), VisualInfo->visual, AllocNone);
    WinAttribs.border_pixel     =0;
    WinAttribs.override_redirect=True;
    WinAttribs.event_mask       =EventMask;

    Win=XCreateWindow(DisplayPtr,
                      RootWindow(DisplayPtr, VisualInfo->screen),
                      0, 0, Width, Height,
                      FullScreen ? 0 : 4,
                      VisualInfo->depth,
                      InputOutput,
                      VisualInfo->visual,
                      FullScreen ? CWBorderPixel | CWColormap | CWEventMask | CWOverrideRedirect : CWColormap | CWEventMask,
                      &WinAttribs);

    if (!Win) return "Cannot open window.";


    XSizeHints SizeHints;

    SizeHints.flags =PSize | PMaxSize | PMinSize;
    SizeHints.width =SizeHints.max_width =SizeHints.min_width =Width;
    SizeHints.height=SizeHints.max_height=SizeHints.min_height=Height;

    XSetStandardProperties(DisplayPtr, Win, Title.c_str(), NULL, None, NULL, 0, &SizeHints);


    glXMakeCurrent(DisplayPtr, Win, RC);
    XMapRaised(DisplayPtr, Win);

    XEvent Event;

    do
    {
        XMaskEvent(DisplayPtr, StructureNotifyMask, &Event);
    } while ((Event.type!=MapNotify) || (Event.xmap.event!=Win));


    if (FullScreen) XSetInputFocus(DisplayPtr, Win, RevertToPointerRoot, CurrentTime);

    // Turn off auto-repeat.
    // XAutoRepeatOff(DisplayPtr);

    // Make the mouse cursor invisible.
    // This is necessary in order to have it not flickering it in mid-screen all the time.
    Pixmap CursorMask=XCreatePixmap(DisplayPtr, Win, 1, 1, 1);

    XGCValues xgc;
    xgc.function=GXclear;

    GC gc=XCreateGC(DisplayPtr, CursorMask, GCFunction, &xgc);

    XFillRectangle(DisplayPtr, CursorMask, gc, 0, 0, 1, 1);

    XColor DummyColor;
    DummyColor.pixel=0;
    DummyColor.red  =0;
    DummyColor.flags=4;

    Cursor cursor=XCreatePixmapCursor(DisplayPtr, CursorMask, CursorMask, &DummyColor, &DummyColor, 0, 0);

    XFreePixmap(DisplayPtr, CursorMask);
    XFreeGC(DisplayPtr, gc);
 // XDefineCursor(DisplayPtr, Win, XCreateFontCursor(DisplayPtr, XC_tcross));
    XDefineCursor(DisplayPtr, Win, cursor);
#endif

    Resize(Width, Height);                // Perspektivischen OpenGL Screen einrichten.


    // 9. OpenGL initialisieren
    // ************************

    /* if (atof((char const*)glGetString(GL_VERSION))<1.2)
    {
        Close();
        return "Need at least OpenGL version 1.2 to run. Please update your video board drivers.";
    } */

    // glDepthFunc(GL_LEQUAL);
    // glPolygonOffset(1.0, 1.0);

    // glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    // glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

    glShadeModel(GL_SMOOTH);                                        // Enables smooth shading.
    glClearColor(0.0, 0.0, 0.0, 0.0);                               // Black background.

    glClearDepth(1.0);                                              // Depth Buffer setup.
    glEnable(GL_DEPTH_TEST);                                        // Enables depth testing.

    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);              // Really nice perspective calculations.

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); SwapBuffers();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); SwapBuffers();


    // 10. Geschafft: Keine Initialisierungsfehler
    // *******************************************

    WindowIsOpen=true;
    RenderingContextCounter++;
    return NULL;
}


bool SingleOpenGLWindowImplT::HandleWindowMessages()
{
#ifdef _WIN32
    MSG Message;

    // Note that we must process all the messages of the Windows message queue until the queue is empty.
    // If we only processed a single message per frame, controlling the player via the keyboard gets *really* laggy,
    // because even a single keypress causes *many* messages to be added to the queue *per frame*.
    while (PeekMessage(&Message, NULL, 0, 0, PM_REMOVE))
    {
        if (Message.message==WM_QUIT) return true;  // Programm-Ende?

        TranslateMessage(&Message);                 // Tastencodes übersetzen.
        DispatchMessage (&Message);                 // WindowProzedur aufrufen.
    }
#else
    if (PostQuitMsgFlag)
    {
        PostQuitMsgFlag=false;
        return true;
    }

    static bool IgnoreFirst  =true;
    static int  LastMousePosX=0;
    static int  LastMousePosY=0;

    bool RecenterMouse=false;

    // Bearbeite alle X-Events dieses Fensters.
    while (XPending(DisplayPtr))
    {
        XEvent Event;

        // Hole das Event ab.
        XNextEvent(DisplayPtr, &Event);

        switch(Event.type)
        {
         // case ConfigureNotify:
         //     Resize(Event.xconfigure.width, event.xconfigure.height);
         //     break;

            case EnterNotify:
            {
                // Move the mouse into the center of the window,
                // and ignore all motion events until the "re-center" event got through.
                RecenterMouse=true;
                IgnoreFirst  =true;
                break;
            }

            case KeyPress:
            case KeyRelease:
            {
                // unsigned long ks=XKeycodeToKeysym(DisplayPtr, Event.xkey.keycode, 0);
                // printf("keycode %u, keysym %lu  '%c'\n", Event.xkey.keycode, ks, char(ks));

                // Convert 'Event.xkey' into the 'CaKeys' space,
                // and create a 'CaKeyboardEventT' from the X event.
                CaKeyboardEventT CKE;

                switch (Event.xkey.keycode)
                {
                    case  97: CKE.Key=CaKeyboardEventT::CK_HOME;        break;
                    case  98: CKE.Key=CaKeyboardEventT::CK_UP;          break;
                    case  99: CKE.Key=CaKeyboardEventT::CK_PGUP;        break;
                    case 100: CKE.Key=CaKeyboardEventT::CK_LEFT;        break;
                    case 102: CKE.Key=CaKeyboardEventT::CK_RIGHT;       break;
                    case 103: CKE.Key=CaKeyboardEventT::CK_END;         break;
                    case 104: CKE.Key=CaKeyboardEventT::CK_DOWN;        break;
                    case 105: CKE.Key=CaKeyboardEventT::CK_PGDN;        break;
                    case 106: CKE.Key=CaKeyboardEventT::CK_INSERT;      break;
                    case 107: CKE.Key=CaKeyboardEventT::CK_DELETE;      break;
                    case 108: CKE.Key=CaKeyboardEventT::CK_NUMPADENTER; break;
                    case 110: CKE.Key=CaKeyboardEventT::CK_PAUSE;       break;
                //  case 111: CKE.Key=CaKeyboardEventT::CK_?????;       break;    // "Drucken/S-Abf" Taste
                    case 112: CKE.Key=CaKeyboardEventT::CK_DIVIDE;      break;
                    default : CKE.Key=CaKeyboardEventT::KeyT((Event.xkey.keycode-8) & 0xFF); break;
                }

                CKE.Type=(Event.type==KeyPress) ? CaKeyboardEventT::CKE_KEYDOWN : CaKeyboardEventT::CKE_KEYUP;

                // Store the event in the event queue.
                if (CKE_Queue_Size<CKE_QUEUE_MAX)
                {
                    CKE_Queue[(CKE_Queue_Start+CKE_Queue_Size) & (CKE_QUEUE_MAX-1)]=CKE;
                    CKE_Queue_Size++;
                }

                if (CKE.Type==CaKeyboardEventT::CKE_KEYDOWN)
                {
                    char Buffer[64];

                    // Also generate a character event, if possible.
                    const int NumChars=XLookupString(&Event.xkey, Buffer, sizeof(Buffer), NULL /*Don't want a KeySym.*/, NULL);

                    CKE.Type=CaKeyboardEventT::CKE_CHAR;

                    for (int CharNr=0; CharNr<NumChars; CharNr++)
                    {
                        CKE.Key=Buffer[CharNr];

                        // Store the event in the event queue.
                        if (CKE_Queue_Size<CKE_QUEUE_MAX)
                        {
                            CKE_Queue[(CKE_Queue_Start+CKE_Queue_Size) & (CKE_QUEUE_MAX-1)]=CKE;
                            CKE_Queue_Size++;
                        }
                    }
                }
                break;
            }

            case ButtonPress:
            case ButtonRelease:
            {
                // Create a 'CaMouseEventT' from the X event.
                CaMouseEventT CME;

                CME.Type  =CaMouseEventT::TypeT(Event.xbutton.button-Button1);
                CME.Amount=(Event.type==ButtonPress) ? 1 : 0;

                // Store the event in the event queue.
                if (CME_Queue_Size<CME_QUEUE_MAX)
                {
                    CME_Queue[(CME_Queue_Start+CME_Queue_Size) & (CME_QUEUE_MAX-1)]=CME;
                    CME_Queue_Size++;
                }

                break;
            }

            case MotionNotify:
            {
                int RelMotionX=Event.xmotion.x-LastMousePosX; LastMousePosX=Event.xmotion.x;
                int RelMotionY=Event.xmotion.y-LastMousePosY; LastMousePosY=Event.xmotion.y;

                if (Event.xmotion.x==int(Width/2) && Event.xmotion.y==int(Height/2))
                {
                    // This was probably an 'XWarpPointer()' event.
                    // We'll ignore it, and hope that we are not mistaken.
                    // In the very rare case that we actually *are* mistaken,
                    // a small (but hardly ever noticeable) pointer glitch will occur.
                    // We also stop ignoring regular motion events.
                    // Regular motion events are ignored from receiving an 'EnterNotify' event
                    // and re-centering the pointer until we herewith receive the re-center "acknowledgement".
                    IgnoreFirst=false;
                    break;
                }

                if (IgnoreFirst)
                {
                    RecenterMouse=true;
                    break;
                }

                if (RelMotionX!=0)
                {
                    CaMouseEventT CME;

                    CME.Type  =CaMouseEventT::CM_MOVE_X;
                    CME.Amount=RelMotionX;

                    // Store the event in the event queue.
                    if (CME_Queue_Size<CME_QUEUE_MAX)
                    {
                        CME_Queue[(CME_Queue_Start+CME_Queue_Size) & (CME_QUEUE_MAX-1)]=CME;
                        CME_Queue_Size++;
                    }

                    RecenterMouse=true;
                }

                if (RelMotionY!=0)
                {
                    CaMouseEventT CME;

                    CME.Type  =CaMouseEventT::CM_MOVE_Y;
                    CME.Amount=RelMotionY;

                    // Store the event in the event queue.
                    if (CME_Queue_Size<CME_QUEUE_MAX)
                    {
                        CME_Queue[(CME_Queue_Start+CME_Queue_Size) & (CME_QUEUE_MAX-1)]=CME;
                        CME_Queue_Size++;
                    }

                    RecenterMouse=true;
                }

                break;
            }
        }
    }

    // Move the mouse pointer back to the window center.
    if (RecenterMouse)
    {
        // 'XWarpPointer()' does not only warp the pointer, but it also generates an appropriate 'MotionNotify' event.
        // However, it is unforeseeable when and where such events are placed into the event queue.
        // That means that when the above while loop is next entered, there is no guarantee that the here
        // generated event is the first that will be dequeued!
        XWarpPointer(DisplayPtr, None, Win, 0, 0, 0, 0, Width/2, Height/2);
    }
#endif

    return false;
}


void SingleOpenGLWindowImplT::PostQuitMessage()
{
#ifdef _WIN32
    ::PostQuitMessage(0);
#else
    PostQuitMsgFlag=true;
#endif
}


unsigned int SingleOpenGLWindowImplT::GetSystemFontDisplayList(int Height, const char* Name)
{
    GLuint FontDisplayList=glGenLists(256);

#ifdef _WIN32
    HFONT Font=CreateFont(-Height,                          // Height of font (character height)
                          0,                                // Width of font
                          0,                                // Angle of escapement
                          0,                                // Orientation angle
                          FW_BOLD,                          // Font weight
                          false, false, false,              // Italic, Underline, Strikeout
                          ANSI_CHARSET,                     // Character set identifier
                          OUT_TT_PRECIS,                    // Output Precision
                          CLIP_DEFAULT_PRECIS,              // Clipping Precision
                          ANTIALIASED_QUALITY,              // Output Quality
                          FF_DONTCARE | DEFAULT_PITCH,      // Family and Pitch
                          Name);                            // Font name

    if (!FontDisplayList) return 0;
    if (!Font) return 0;

    SelectObject(hDC, Font);
    wglUseFontBitmaps(hDC, 0, 256, FontDisplayList);
#else
    // See xfontsel for more information about X Logical Font Descriptions.
    static char XLFD[256];

    // "-adobe-times-medium-r-normal--17-120-100-100-p-88-iso8859-1"
    sprintf(XLFD, "-*-%s-*-r-*-*-%i-*-*-*-*-*-*-*", Name, Height);
    XFontStruct* FontInfo=XLoadQueryFont(DisplayPtr, XLFD);

    if (!FontDisplayList) return 0;
    if (!FontInfo) return 0;

    glXUseXFont(FontInfo->fid, 0, 256, FontDisplayList);
    XFreeFont(DisplayPtr, FontInfo);
#endif

    return FontDisplayList;
}


void SingleOpenGLWindowImplT::SwapBuffers()
{
#ifdef _WIN32
    ::SwapBuffers(hDC);
#else
    glXSwapBuffers(DisplayPtr, Win);
#endif
}


uint32_t* SingleOpenGLWindowImplT::GetFrameBuffer(unsigned int& Width_, unsigned int& Height_)
{
    static ArrayT<uint32_t> FrameBuffer;

    FrameBuffer.Overwrite();
    FrameBuffer.PushBackEmpty(Width*Height);

    // Pixel vom BackBuffer in den FrameBuffer lesen.
    // Beachte: Die ersten beiden Parameter (0, 0) spezifizieren die linke UNTERE Ecke des gewünschten Bereichs!
    glReadPixels(0, 0, Width, Height, GL_RGBA, GL_UNSIGNED_BYTE, &FrameBuffer[0]);

    Width_ =Width;
    Height_=Height;

    // Wie oben schon erwähnt, steht der 'FrameBuffer' leider auf dem Kopf.
    // Vertausche daher alle Zeilen (vertikale Spiegelung).
    for (unsigned int y=0; y<Height_/2; y++)
    {
        uint32_t* UpperRow=&FrameBuffer[0]+         y   *Width_;
        uint32_t* LowerRow=&FrameBuffer[0]+(Height_-y-1)*Width_;

        for (unsigned int x=0; x<Width_; x++)
        {
            const uint32_t Swap=*UpperRow;

            *UpperRow=*LowerRow;
            *LowerRow=Swap;

            UpperRow++;
            LowerRow++;
        }
    }

    return &FrameBuffer[0];
}


void SingleOpenGLWindowImplT::Close()
{
#ifdef _WIN32
    // Release DirectInput.
    DirectInput.Release();

    // Rendering-Context zurückgeben.
    if (hRC)
    {
        // Vorher noch prüfen, ob es OpenGL-Fehler gab.
        while (true)
        {
            GLenum Error=glGetError();

            switch (Error)
            {
                case GL_NO_ERROR         : break;
                case GL_INVALID_ENUM     : MessageBox(NULL, "GL_INVALID_ENUM     ", NULL, MB_OK | MB_ICONERROR | MB_TOPMOST); break;
                case GL_INVALID_VALUE    : MessageBox(NULL, "GL_INVALID_VALUE    ", NULL, MB_OK | MB_ICONERROR | MB_TOPMOST); break;
                case GL_INVALID_OPERATION: MessageBox(NULL, "GL_INVALID_OPERATION", NULL, MB_OK | MB_ICONERROR | MB_TOPMOST); break;
                case GL_STACK_OVERFLOW   : MessageBox(NULL, "GL_STACK_OVERFLOW   ", NULL, MB_OK | MB_ICONERROR | MB_TOPMOST); break;
                case GL_STACK_UNDERFLOW  : MessageBox(NULL, "GL_STACK_UNDERFLOW  ", NULL, MB_OK | MB_ICONERROR | MB_TOPMOST); break;
                case GL_OUT_OF_MEMORY    : MessageBox(NULL, "GL_OUT_OF_MEMORY    ", NULL, MB_OK | MB_ICONERROR | MB_TOPMOST); break;
                default                  : MessageBox(NULL, "GL_UNKNOWN!!!       ", NULL, MB_OK | MB_ICONERROR | MB_TOPMOST); break;
            }

            if (Error==GL_NO_ERROR) break;
        }

        if (!wglMakeCurrent  (NULL, NULL)) MessageBox(NULL, "Unable to deactivate DC and RC.",      NULL, MB_OK | MB_ICONERROR);
        if (!wglDeleteContext(hRC)       ) MessageBox(NULL, "Unable to release rendering context.", NULL, MB_OK | MB_ICONERROR);

        hRC=NULL;
    }

    // Device-Context zurückgeben.
    if (hDC)
    {
        if (!ReleaseDC(hWindow, hDC)) MessageBox(NULL, "Unable to release device context.", NULL, MB_OK | MB_ICONERROR);
        hDC=NULL;
    }

    // Fenster schließen.
    if (hWindow)
    {
        if (!DestroyWindow(hWindow)) MessageBox(NULL, "Unable to destroy window.", NULL, MB_OK | MB_ICONERROR);
        hWindow=NULL;
    }

    // Fensterklasse abmelden. Dabei zurückgegebene Fehlercodes ignorieren.
    UnregisterClass("CafuMain", hInstance);

    // Standard-Display wiederherstellen.
    if (FullScreen)
    {
        ChangeDisplaySettings(NULL, 0);
        ShowCursor(true);
    }
#else
    // De-activate the invisible mouse cursor again (restore the previous visible cursor).
    XUndefineCursor(DisplayPtr, Win);

    // Turn on auto-repeat.
    // XAutoRepeatOn(DisplayPtr);

    // WARNING: Omitting the following line worked when this library was a static library.
    // When it became a shared library, the next call to glXMakeCurrent() caused a segmentation fault without this line.
    // That's strange. Probably it's just a driver bug on my machine, but I thought I'd better mention it...
    glXDestroyContext(DisplayPtr, RC);

    XCloseDisplay(DisplayPtr);
#endif

    // OpenGLWindow geschlossen.
    WindowIsOpen=false;
}


int SingleOpenGLWindowImplT::GetNextKeyboardEvent(CaKeyboardEventT& KeyboardEvent)
{
    if (!CKE_Queue_Size) return 0;

    KeyboardEvent=CKE_Queue[CKE_Queue_Start];

    CKE_Queue_Size--;
    CKE_Queue_Start=(CKE_Queue_Start+1) & (CKE_QUEUE_MAX-1);

         if (KeyboardEvent.Type==CaKeyboardEventT::CKE_KEYDOWN) KeyboardState[KeyboardEvent.Key]=true;
    else if (KeyboardEvent.Type==CaKeyboardEventT::CKE_KEYUP  ) KeyboardState[KeyboardEvent.Key]=false;

    return 1;
}


int SingleOpenGLWindowImplT::GetNextMouseEvent(CaMouseEventT& MouseEvent)
{
#ifdef _WIN32
    DIDEVICEOBJECTDATA MEvent;
    unsigned long      ReadNrOfEvents;
    HRESULT            hResult=DirectInput.GetNextMouseEvent(&MEvent, &ReadNrOfEvents);

    if (FAILED(hResult))
    {
        if (hResult!=DIERR_OTHERAPPHASPRIO)
        {
            // GameConsole     .Print("DI.GetNextMouseEvent() failed! hResult==0x%X", hResult);
            // SystemScrollInfo.Print("DI.GetNextMouseEvent() failed! hResult==0x%X", hResult);
        }

        return -1;
    }

    if (!ReadNrOfEvents) return 0;  // Der Buffer ist leer.

    // Geht leider nicht mit einem switch-case Ausdruck ("expression (DIMOFS_X/Y) must be constant").
    #if defined (__WATCOMC__)
        #pragma warning 7 9;
    #endif

         if (MEvent.dwOfs==DIMOFS_BUTTON0) { MouseEvent.Type=IsMouseSwapped ? CaMouseEventT::CM_BUTTON1 : CaMouseEventT::CM_BUTTON0; MouseEvent.Amount=((MEvent.dwData & 0x80)!=0) ? 1 : 0; }
    else if (MEvent.dwOfs==DIMOFS_BUTTON1) { MouseEvent.Type=IsMouseSwapped ? CaMouseEventT::CM_BUTTON0 : CaMouseEventT::CM_BUTTON1; MouseEvent.Amount=((MEvent.dwData & 0x80)!=0) ? 1 : 0; }
    else if (MEvent.dwOfs==DIMOFS_BUTTON2) { MouseEvent.Type=CaMouseEventT::CM_BUTTON2; MouseEvent.Amount=((MEvent.dwData & 0x80)!=0) ? 1 : 0; }
    else if (MEvent.dwOfs==DIMOFS_BUTTON3) { MouseEvent.Type=CaMouseEventT::CM_BUTTON3; MouseEvent.Amount=((MEvent.dwData & 0x80)!=0) ? 1 : 0; }
    else if (MEvent.dwOfs==DIMOFS_X      ) { MouseEvent.Type=CaMouseEventT::CM_MOVE_X;  MouseEvent.Amount=MEvent.dwData; }
    else if (MEvent.dwOfs==DIMOFS_Y      ) { MouseEvent.Type=CaMouseEventT::CM_MOVE_Y;  MouseEvent.Amount=MEvent.dwData; }
    else if (MEvent.dwOfs==DIMOFS_Z      ) { MouseEvent.Type=CaMouseEventT::CM_MOVE_Z;  MouseEvent.Amount=MEvent.dwData; }

    #if defined (__WATCOMC__)
        #pragma warning 7 4;
    #endif

    if (MouseEvent.Type<8)
    {
        char BitMask=1 << MouseEvent.Type;

        if (MouseEvent.Amount) MouseButtonState|= BitMask;
                          else MouseButtonState&=~BitMask;
    }

    if (hResult==DI_BUFFEROVERFLOW)
    {
        // GameConsole     .Print("WARNING: DirectInput Mouse Buffer Overflow!");
        // SystemScrollInfo.Print("WARNING: DirectInput Mouse Buffer Overflow!");

        return 2;
    }

    return 1;
#else
    if (!CME_Queue_Size) return 0;

    MouseEvent=CME_Queue[CME_Queue_Start];

    CME_Queue_Size--;
    CME_Queue_Start=(CME_Queue_Start+1) & (CME_QUEUE_MAX-1);

    if (MouseEvent.Type<8)
    {
        char BitMask=1 << MouseEvent.Type;

        if (MouseEvent.Amount) MouseButtonState|= BitMask;
                          else MouseButtonState&=~BitMask;
    }

    return 1;
#endif
}
