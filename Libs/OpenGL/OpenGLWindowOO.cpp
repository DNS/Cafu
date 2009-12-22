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

/********************************/
/*** OpenGL Window (DLL Code) ***/
/********************************/

#include "OpenGLWindow.hpp"

#include <string.h>
#include <stdlib.h>
#include <GL/gl.h>      // Header File für OpenGL
#include <GL/glu.h>     // Header File für OpenGL Utils

#ifdef _WIN32
#include "DirectX/DirectInput.hpp"
#else
#include <X11/Xlib.h>
#include <GL/glx.h>
struct DirectInputT { int Dummy; };
#endif


struct PrivateVariablesT
{
#ifdef _WIN32
    HINSTANCE hInstance;    // Unsere Programminstanz.
    HWND      hWindow;      // Handle dieses Windows!
    HDC       hDC;          // Device Context.
    HGLRC     hRC;          // OpenGL Rendering Context.
#else
    Display*  DisplayPtr;   // Connection to the X server.
    Window    Win;          // The Window.
#endif
};


void OpenGLWindowT::Close()
{
#ifdef _WIN32
    // Rendering-Context zurückgeben.
    if (pv->hRC)
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
        if (!wglDeleteContext(pv->hRC)   ) MessageBox(NULL, "Unable to release rendering context.", NULL, MB_OK | MB_ICONERROR);

        pv->hRC=NULL;
    }

    // Device-Context zurückgeben
    if (pv->hDC)
    {
        if (!ReleaseDC(pv->hWindow, pv->hDC)) MessageBox(NULL, "Unable to release device context.", NULL, MB_OK | MB_ICONERROR);
        pv->hDC=NULL;
    }

    // Fenster schließen
    if (pv->hWindow)
    {
        if (!DestroyWindow(pv->hWindow)) MessageBox(NULL, "Unable to destroy window.", NULL, MB_OK | MB_ICONERROR);
        pv->hWindow=NULL;
    }

    // Fensterklasse abmelden. Dabei zurückgegebene Fehlercodes ignorieren.
    UnregisterClass("Ca3DMain", pv->hInstance);

    // Standard-Display wiederherstellen
    if (FullScreen)
    {
        ChangeDisplaySettings(NULL, 0);
        ShowCursor(true);
    }
#else
    XCloseDisplay(pv->DisplayPtr);
#endif
}


// Paßt den OpenGL-ViewPort an eine neue Fensterhöhe/-breite an.
void OpenGLWindowT::Resize(unsigned long Width_, unsigned long Height_)
{
    Width =Width_;
    Height=Height_;

    if (Height==0) Height=1;                                    // Division durch 0 vermeiden

    glViewport(0, 0, Width, Height);                            // ViewPort neu setzen

    glMatrixMode(GL_PROJECTION);                                // Projektionsmatrix zurücksetzen
    glLoadIdentity();

    // Aspekt Ratio neu setzen, entspricht 90 Grad der DOS Version
    gluPerspective(67.5, (float)Width/(float)Height, 100.0, 100000.0);

    glMatrixMode(GL_MODELVIEW);                                 // Modelview-Matrix zurücksetzen
    glLoadIdentity();
}


LRESULT OpenGLWindowT::WinProc(HWND hWnd, UINT MessageID, WPARAM wParam, LPARAM lParam)
{
#ifdef _WIN32
    switch (MessageID)
    {
        case WM_ACTIVATE:
            IsMinimized=HIWORD(wParam)!=0;
            return 0;

        case WM_SYSCOMMAND:
            if (wParam==SC_SCREENSAVE || wParam==SC_MONITORPOWER) return 0;
            break;

     // case WM_KEYDOWN:
     //     Keys[char(wParam)]=true;
     //     return 0;

     // case WM_CHAR:
     //     LastChar     =char(wParam);
     //     CharAvailable=true;
     //     return 0;

     // case WM_KEYUP:
     //     Keys[char(wParam)]=false;
     //     return 0;

        case WM_SIZE:
            Resize(LOWORD(lParam), HIWORD(lParam));
            return 0;

        case WM_CLOSE:
            PostQuitMessage(0);
            return 0;
    }

    return DefWindowProc(hWnd, MessageID, wParam, lParam);
#endif
}


OpenGLWindowT* OpenGLWindowT::List_OpenGLWindow[16];
char           OpenGLWindowT::List_Size=0;
unsigned long  OpenGLWindowT::RenderingContextCounter=0;


// CallBack-Funktion für die Window-Messages.
LRESULT CALLBACK OpenGLWindowT::GlobalWinProc(HWND hWnd, UINT MessageID, WPARAM wParam, LPARAM lParam)
{
#ifdef _WIN32
    for (unsigned long Nr=0; Nr<List_Size; Nr++)
        if (List_OpenGLWindow[Nr]->pv->hWindow==hWnd)
            return List_OpenGLWindow[Nr]->WinProc(hWnd, MessageID, wParam, lParam);

    return DefWindowProc(hWnd, MessageID, wParam, lParam);
#endif
}


OpenGLWindowT::OpenGLWindowT(const char* Title_, unsigned long Width_, unsigned long Height_, char BPP_, bool FullScreen_) : pv(new PrivateVariablesT)
{
#ifdef _WIN32
    pv->hInstance=GetModuleHandle(NULL);    // 'GetModuleHandle(NULL)' entspricht 'hInstance' von 'WinMain()'.
    pv->hWindow  =NULL;
    pv->hDC      =NULL;
    pv->hRC      =NULL;

    strncpy(Title, Title_, sizeof(Title));
    Title[sizeof(Title)-1]=0;

    Width      =Width_;
    Height     =Height_;
    BPP        =BPP_;
    FullScreen =FullScreen_;
    IsMinimized=false;


    // 1. Window-Klasse ausfüllen und registrieren
    // *******************************************

    WNDCLASSEX MainWindowClass;

    MainWindowClass.cbSize       =sizeof(WNDCLASSEX);                       // Größe dieser Struktur.
    MainWindowClass.style        =CS_VREDRAW | CS_HREDRAW | CS_OWNDC;       // Fensterklassenstil.
    MainWindowClass.lpfnWndProc  =GlobalWinProc;                            // Zeiger auf Handler-Funktion.
    MainWindowClass.cbClsExtra   =0;                                        // Zusätzlicher Platz für KlassenInfos.
    MainWindowClass.cbWndExtra   =0;                                        // Zusätzlicher Platz für FensterInfos.
    MainWindowClass.hInstance    =pv->hInstance;                            // Unsere Programm-ID.
    MainWindowClass.hIcon        =LoadIcon(NULL, IDI_APPLICATION);          // Icon.
    MainWindowClass.hIconSm      =LoadIcon(NULL, IDI_APPLICATION);          // Kleines Icon für Task-Bar.
    MainWindowClass.hCursor      =LoadCursor(NULL, IDC_ARROW);              // Cursor.
    MainWindowClass.hbrBackground=NULL;                                     // Hintergrund-Brush.
    MainWindowClass.lpszMenuName =NULL;                                     // Menü.
    MainWindowClass.lpszClassName="Ca3DMain";                               // Name dieser Fensterklasse.

    if (!RegisterClassEx(&MainWindowClass)) throw Error("Unable to register the window class.");


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
            UnregisterClass("Ca3DMain", pv->hInstance);
            throw Error("Unable to change display settings.");
        }

        ShowCursor(false);
    }


    // 3. Window-Rectangle anpassen
    // ****************************

    unsigned long   Style=FullScreen ? WS_POPUP : WS_OVERLAPPEDWINDOW;
    unsigned long ExStyle=WS_EX_APPWINDOW;

    RECT GLWindowRect;

    GLWindowRect.top   =0;
    GLWindowRect.left  =0;
    GLWindowRect.right =Width;
    GLWindowRect.bottom=Height;

    if (!AdjustWindowRectEx(&GLWindowRect, Style, false, ExStyle))
    {
        Close();
        throw Error("Unable to adjust window rectangle.");
    }


    // 4. Window erzeugen
    // ******************

    pv->hWindow=CreateWindowEx(ExStyle,                                     // Fensterstil (erweitert)
                               "Ca3DMain",                                  // Name der Fensterklasse
                               Title,                                       // Fenstertitel
                               Style | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,   // Fensterstil
                               0, // FullScreen ? 0 : CW_USEDEFAULT,        // X-Position
                               0, // FullScreen ? 0 : CW_USEDEFAULT,        // Y-Position
                               GLWindowRect.right-GLWindowRect.left,        // Breite
                               GLWindowRect.bottom-GLWindowRect.top,        // Höhe
                               NULL,                                        // Übergeordnetes Fenster
                               NULL,                                        // Menü
                               pv->hInstance,                               // Unsere Programm-ID
                               NULL);                                       // Zusätzliche Parameter

    if (!pv->hWindow)
    {
        Close();
        throw Error("Unable to create window.");
    }


    // 5. Device Context erfragen
    // **************************

    pv->hDC=GetDC(pv->hWindow);

    if (!pv->hDC)
    {
        Close();
        throw Error("Unable to obtain a GL device context.");
    }


    // 6. PixelFormat abfragen und setzen
    // **********************************

    PIXELFORMATDESCRIPTOR PFD;

    memset(&PFD, 0, sizeof(PFD));
    PFD.nSize     =sizeof(PFD);
    PFD.nVersion  =1;
    PFD.dwFlags   =PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    PFD.iPixelType=PFD_TYPE_RGBA;
    PFD.cColorBits=BPP;
    PFD.cDepthBits=32;
    PFD.iLayerType=PFD_MAIN_PLANE;

    int PixelFormat=ChoosePixelFormat(pv->hDC, &PFD);

    if (!PixelFormat)
    {
        Close();
        throw Error("Unable to choose a pixel format.");
    }

    if (!DescribePixelFormat(pv->hDC, PixelFormat, sizeof(PFD), &PFD))
    {
        Close();
        throw Error("Unable to verify pixel format.");
    }

    if ((PFD.dwFlags & PFD_DRAW_TO_WINDOW)!=PFD_DRAW_TO_WINDOW ||
        (PFD.dwFlags & PFD_SUPPORT_OPENGL)!=PFD_SUPPORT_OPENGL ||
        (PFD.dwFlags & PFD_DOUBLEBUFFER  )!=PFD_DOUBLEBUFFER   ||
        PFD.iPixelType!=PFD_TYPE_RGBA /*|| PFD.cColorBits<BPP*/ || PFD.cDepthBits<16 || PFD.iLayerType!=PFD_MAIN_PLANE)
    {
        Close();
        throw Error("Selected pixel format mismatches.");
    }

    if(!SetPixelFormat(pv->hDC, PixelFormat, &PFD))
    {
        Close();
        throw Error("Unable to set the pixel format.");
    }


    // 7. Rendering Context erzeugen und aktivieren
    // ********************************************

    pv->hRC=wglCreateContext(pv->hDC);

    if (!pv->hRC)
    {
        Close();
        throw Error("Unable to create a GL rendering context.");
    }

    if(!wglMakeCurrent(pv->hDC, pv->hRC))
    {
        Close();
        throw Error("Unable to activate the GL rendering context.");
    }


    // 8. Fenster anzeigen und einrichten
    // **********************************

    ShowWindow(pv->hWindow, SW_SHOW);                               // Show the window.
    SetForegroundWindow(pv->hWindow);                               // Slightly higher priority.
    SetFocus(pv->hWindow);                                          // Sets keyboard focus to the window.
#else
    pv->DisplayPtr=XOpenDisplay(NULL);

    if (!pv->DisplayPtr)                                throw Error("Cannot open display.");
    if (!glXQueryExtension(pv->DisplayPtr, NULL, NULL)) throw Error("No GLX extension.");

    strncpy(Title, Title_, sizeof(Title));
    Title[sizeof(Title)-1]=0;

    Width      =FullScreen_ ? DisplayWidth (pv->DisplayPtr, DefaultScreen(pv->DisplayPtr)) : Width_;
    Height     =FullScreen_ ? DisplayHeight(pv->DisplayPtr, DefaultScreen(pv->DisplayPtr)) : Height_;
    BPP        =BPP_;
    FullScreen =FullScreen_;
    IsMinimized=false;


    int VisualInfo_Features[] =
    {
        GLX_RGBA,
        GLX_RED_SIZE, 8,
        GLX_GREEN_SIZE, 8,
        GLX_BLUE_SIZE, 8,
        GLX_DEPTH_SIZE, 16,
        GLX_DOUBLEBUFFER,
        None
    };


    XVisualInfo* VisualInfo=glXChooseVisual(pv->DisplayPtr, DefaultScreen(pv->DisplayPtr), VisualInfo_Features);
    if (!VisualInfo) throw Error("Cannot find visual.");

    GLXContext Context=glXCreateContext(pv->DisplayPtr, VisualInfo, None, True);
    if (!Context) throw Error("Cannot create GLX context.");


    XSetWindowAttributes WinAttribs;

    WinAttribs.colormap         =XCreateColormap(pv->DisplayPtr, RootWindow(pv->DisplayPtr, VisualInfo->screen), VisualInfo->visual, AllocNone);
    WinAttribs.border_pixel     =0;
    WinAttribs.override_redirect=True;
    WinAttribs.event_mask       =ExposureMask | ButtonPressMask | StructureNotifyMask | KeyPressMask | KeyReleaseMask;

    pv->Win=XCreateWindow(pv->DisplayPtr,
                          RootWindow(pv->DisplayPtr, VisualInfo->screen),
                          0, 0, Width, Height,
                          FullScreen ? 0 : 4,
                          VisualInfo->depth,
                          InputOutput,
                          VisualInfo->visual,
                          FullScreen ? CWBorderPixel | CWColormap | CWEventMask | CWOverrideRedirect : CWColormap | CWEventMask,
                          &WinAttribs);

    if (!pv->Win) throw Error("Cannot open window.");


    XSizeHints SizeHints;

    SizeHints.flags =PSize | PMaxSize | PMinSize;
    SizeHints.width =SizeHints.max_width =SizeHints.min_width =Width;
    SizeHints.height=SizeHints.max_height=SizeHints.min_height=Height;

    XSetStandardProperties(pv->DisplayPtr, pv->Win, Title, NULL, None, NULL, 0, &SizeHints);


    glXMakeCurrent(pv->DisplayPtr, pv->Win, Context);
    XMapRaised(pv->DisplayPtr, pv->Win);

    XEvent Event;

    do
    {
        XMaskEvent(pv->DisplayPtr, StructureNotifyMask, &Event);
    } while ((Event.type!=MapNotify) || (Event.xmap.event!=pv->Win));


    if (FullScreen) XSetInputFocus(pv->DisplayPtr, pv->Win, RevertToPointerRoot, CurrentTime);

    // XDefineCursor(pv->DisplayPtr, Win, XCreateFontCursor(pv->DisplayPtr, XC_tcross));
#endif

    Resize(Width, Height);                                          // Perspektivischen OpenGL Screen einrichten.


    // 9. OpenGL initialisieren
    // ************************

    /* if (atof((char const*)glGetString(GL_VERSION))<1.2)
    {
        Close();
        throw Error("Need at least OpenGL version 1.2 to run. Please update your video board drivers.");
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


    // 10. Dieses Fenster in die Liste eintragen
    // *****************************************

    if (List_Size>=16)
    {
        Close();
        throw Error("Sorry - too many open windows (more than 16).");
    }

    List_OpenGLWindow[List_Size]=this;
    List_Size++;


    // 11. Geschafft: Keine Initialisierungsfehler
    // *******************************************

    RenderingContextCounter++;
}


bool OpenGLWindowT::HandleWindowMessages() const
{
#ifdef _WIN32
    MSG Message;

    if (PeekMessage(&Message, NULL, 0, 0, PM_REMOVE))
    {
        if (Message.message==WM_QUIT) return true;  // Programm-Ende?

        TranslateMessage(&Message);                 // Tastencodes übersetzen.
        DispatchMessage (&Message);                 // WindowProzedur aufrufen.
    }
#else
    // Bearbeite alle X-Events dieses Fensters.
    while (XPending(pv->DisplayPtr))
    {
        XEvent Event;

        // Hole das Event ab.
        XNextEvent(pv->DisplayPtr, &Event);

        switch(Event.type)
        {
         // case ConfigureNotify:
         //     Resize(Event.xconfigure.width, event.xconfigure.height);
         //     break;

            case KeyPress:    // Es ist ein Tastendruck!
            {
                char   Buffer[32];
                KeySym keysym;

                XLookupString(&Event.xkey, Buffer, sizeof(Buffer), &keysym, NULL);

                switch (keysym)
                {
                    case 0x1b: // XK_Escape
                 // case XK_q:
                        // Quit program
                        return true;
                }
                break;
            }

            case KeyRelease:
                // WICHTIG: Zusammen mit XAutoRepeatOff() (und XAutoRepeatOn()) kann man
                // mit den KeyPress- und KeyRelease-Events die fuer "Spiele" notwendige
                // Funktionalitaet erreichen ("Ist gerade die Taste __ gedrueckt?").
                // Siehe aber auch XQueryKeymap()!
                break;
        }
    }
#endif

    return false;
}


void OpenGLWindowT::SwapBuffers() const
{
#ifdef _WIN32
    ::SwapBuffers(pv->hDC);
#else
    glXSwapBuffers(pv->DisplayPtr, pv->Win);
#endif
}


unsigned long* OpenGLWindowT::GetFrameBuffer(unsigned long& Width_, unsigned long& Height_) const
{
    unsigned long* FrameBuffer=(unsigned long*)calloc(Width*Height, sizeof(unsigned long));

    if (FrameBuffer==NULL) return NULL;

    // Pixel vom BackBuffer in den FrameBuffer lesen.
    // Beachte: Die ersten beiden Parameter (0, 0) spezifizieren die linke UNTERE Ecke des gewünschten Bereichs!
    glReadPixels(0, 0, Width, Height, GL_RGBA, GL_UNSIGNED_BYTE, FrameBuffer);

    Width_ =Width;
    Height_=Height;

    // Wie oben schon erwähnt, steht der 'FrameBuffer' leider auf dem Kopf.
    // Vertausche daher alle Zeilen (vertikale Spiegelung).
    for (unsigned long y=0; y<Height/2; y++)
    {
        unsigned long* UpperRow=FrameBuffer+        y   *Width;
        unsigned long* LowerRow=FrameBuffer+(Height-y-1)*Width;

        for (unsigned long x=0; x<Width; x++)
        {
            unsigned long Swap=*UpperRow;

            *UpperRow=*LowerRow;
            *LowerRow=Swap;

            UpperRow++;
            LowerRow++;
        }
    }

    return FrameBuffer;
}


unsigned long OpenGLWindowT::GetSystemFontDisplayList(int Height, const char* Name) const
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

    SelectObject(pv->hDC, Font);
    wglUseFontBitmaps(pv->hDC, 0, 256, FontDisplayList);
#else
    // TODO!
#endif

    return FontDisplayList;
}


OpenGLWindowT::~OpenGLWindowT()
{
    // Remove 'this' OpenGLWindowT from the global list.
    for (unsigned long Nr=0; Nr<List_Size; Nr++)
        if (List_OpenGLWindow[Nr]==this)
        {
            List_OpenGLWindow[Nr]=List_OpenGLWindow[List_Size-1];
            List_Size--;
            break;
        }

    // Let others know that the RC changed (i.e., is not present any more).
    RenderingContextCounter++;

    Close();
    delete pv;
}


OpenGLWindowInputT::OpenGLWindowInputT(const char* Title_, unsigned long Width_, unsigned long Height_, char BPP_, bool FullScreen_)
    : OpenGLWindowT(Title_, Width_, Height_, BPP_, FullScreen_),
      DirectInput(new DirectInputT())
{
#ifdef _WIN32
    HRESULT hResult=DirectInput->Initialize(pv->hInstance, pv->hWindow);

    if (hResult!=DI_OK)
    {
        Close();
        throw Error("Unable to initialize DirectInput!");

        // Close();
        // static char ErrorMsg[80];
        // sprintf(ErrorMsg, "Unable to initialize DirectInput!\nhResult=%u", hResult);
        // throw Error(ErrorMsg);
    }
#endif

    for (unsigned long c=0; c<256; c++) KeyboardState[c]=false;
}


int OpenGLWindowInputT::GetNextKeyboardEvent(KeyboardEventT& KeyboardEvent)
{
#ifdef _WIN32
    DIDEVICEOBJECTDATA KEvent;
    unsigned long      ReadNrOfEvents;
    HRESULT            hResult=DirectInput->GetNextKeyEvent(&KEvent, &ReadNrOfEvents);

    if (FAILED(hResult))
    {
        if (hResult!=DIERR_OTHERAPPHASPRIO)
        {
            // GameConsole     .Print("DI.GetNextKeyEvent() failed! hResult==0x%X", hResult);
            // SystemScrollInfo.Print("DI.GetNextKeyEvent() failed! hResult==0x%X", hResult);
        }

        return -1;
    }

    if (!ReadNrOfEvents) return 0;  // Der Buffer ist leer.

    KeyboardEvent.Key    =CaKeys(KEvent.dwOfs);
    KeyboardEvent.Pressed=(KEvent.dwData & 0x80)!=0;

    KeyboardState[KeyboardEvent.Key]=KeyboardEvent.Pressed;

    if (hResult==DI_BUFFEROVERFLOW)
    {
        // GameConsole     .Print("WARNING: DirectInput Keyboard Buffer Overflow!");
        // SystemScrollInfo.Print("WARNING: DirectInput Keyboard Buffer Overflow!");

        return 2;
    }

    return 1;
#else
    return 0;
#endif
}


int OpenGLWindowInputT::GetNextMouseEvent(MouseEventT& MouseEvent)
{
#ifdef _WIN32
    DIDEVICEOBJECTDATA MEvent;
    unsigned long      ReadNrOfEvents;
    HRESULT            hResult=DirectInput->GetNextMouseEvent(&MEvent, &ReadNrOfEvents);

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
    #pragma warning 7 9;

         if (MEvent.dwOfs==DIMOFS_BUTTON0) { MouseEvent.Type=  0; MouseEvent.Amount=((MEvent.dwData & 0x80)!=0) ? 1 : 0; }
    else if (MEvent.dwOfs==DIMOFS_BUTTON1) { MouseEvent.Type=  1; MouseEvent.Amount=((MEvent.dwData & 0x80)!=0) ? 1 : 0; }
    else if (MEvent.dwOfs==DIMOFS_BUTTON2) { MouseEvent.Type=  2; MouseEvent.Amount=((MEvent.dwData & 0x80)!=0) ? 1 : 0; }
    else if (MEvent.dwOfs==DIMOFS_BUTTON3) { MouseEvent.Type=  3; MouseEvent.Amount=((MEvent.dwData & 0x80)!=0) ? 1 : 0; }
    else if (MEvent.dwOfs==DIMOFS_X      ) { MouseEvent.Type=253; MouseEvent.Amount=MEvent.dwData; }
    else if (MEvent.dwOfs==DIMOFS_Y      ) { MouseEvent.Type=254; MouseEvent.Amount=MEvent.dwData; }
    else if (MEvent.dwOfs==DIMOFS_Z      ) { MouseEvent.Type=255; MouseEvent.Amount=MEvent.dwData; }

    #pragma warning 7 4;


    if (hResult==DI_BUFFEROVERFLOW)
    {
        // GameConsole     .Print("WARNING: DirectInput Mouse Buffer Overflow!");
        // SystemScrollInfo.Print("WARNING: DirectInput Mouse Buffer Overflow!");

        return 2;
    }

    return 1;
#else
    return 0;
#endif
}


OpenGLWindowInputT::~OpenGLWindowInputT()
{
    // Release DirectInput.
    delete DirectInput;
}
