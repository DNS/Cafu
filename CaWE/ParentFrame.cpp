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

#include "ChildFrame.hpp"
#include "GameConfig.hpp"
#include "ParentFrame.hpp"
#include "MapDocument.hpp"
#include "Options.hpp"
#include "DialogOptions.hpp"

#include "GuiEditor/ChildFrame.hpp"
#include "GuiEditor/GuiDocument.hpp"
#include "FontWizard/FontWizard.hpp"
#include "ModelEditor/ChildFrame.hpp"
#include "ModelEditor/Document.hpp"

#include "ConsoleCommands/Console.hpp"
#include "FileSys/FileManImpl.hpp"
#include "GuiSys/GuiImpl.hpp"   // Needed to catch InitErrorT if GUI document creation fails.
#include "GuiSys/GuiManImpl.hpp"
#include "MaterialSystem/MapComposition.hpp"
#include "MaterialSystem/Renderer.hpp"
#include "MaterialSystem/TextureMap.hpp"
#include "TextParser/TextParser.hpp"
#include "PlatformAux.hpp"

#include "wx/wx.h"
#include "wx/aboutdlg.h"
#include "wx/confbase.h"
#include "wx/dir.h"
#include "wx/filename.h"
#include "wx/glcanvas.h"
#include "wx/progdlg.h"
#include "wx/stdpaths.h"
#include "wx/utils.h"

#include <fstream>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <direct.h>
#elif __linux__
#include <dirent.h>
#include <dlfcn.h>
#define __stdcall
#define GetProcAddress dlsym
#define FreeLibrary dlclose
#endif


BEGIN_EVENT_TABLE(ParentFrameT, wxMDIParentFrame)
#ifdef __WXGTK__
    EVT_SIZE(ParentFrameT::OnSize)
#endif
    EVT_SHOW(ParentFrameT::OnShow)
    EVT_CLOSE(ParentFrameT::OnClose)
    EVT_MENU_RANGE(ID_MENU_FILE_NEW_MAP,  ID_MENU_FILE_EXIT,  ParentFrameT::OnMenuFile)
    EVT_MENU_RANGE(wxID_FILE1,            wxID_FILE9,         ParentFrameT::OnMenuFile)
    EVT_MENU_RANGE(ID_MENU_HELP_CONTENTS, ID_MENU_HELP_ABOUT, ParentFrameT::OnMenuHelp)
END_EVENT_TABLE()


int ParentFrameT::OpenGLAttributeList[]=
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


ParentFrameT::ParentFrameT()
#ifdef DEBUG
      // We need the wxMAXIMIZE window style here, or else the window is not properly maximized (looks like a bug?).
    : wxMDIParentFrame(NULL /*parent*/, -1 /*id*/, wxString("Cafu World Editor ") + "[DEBUG] - " + __DATE__, wxDefaultPosition, wxDefaultSize, wxDEFAULT_FRAME_STYLE | wxMAXIMIZE),
#else
      // We need the wxMAXIMIZE window style here, or else the window is not properly maximized (looks like a bug?).
    : wxMDIParentFrame(NULL /*parent*/, -1 /*id*/, wxString("Cafu World Editor - ") + __DATE__, wxDefaultPosition, wxDefaultSize, wxDEFAULT_FRAME_STYLE | wxMAXIMIZE),
#endif
      m_GLCanvas(NULL),
      m_GLContext(NULL),
      m_RendererDLL(NULL)
{
    wxMenuBar *item0 = new wxMenuBar;

    wxMenu* item1 = new wxMenu;

    wxMenu* NewMenu = new wxMenu;

    NewMenu->Append(ID_MENU_FILE_NEW_MAP,   wxT("New &Map\tCtrl+N"), wxT(""));
    NewMenu->Append(ID_MENU_FILE_NEW_MODEL, wxT("New M&odel\tCtrl+Shift+N"), wxT(""));
    NewMenu->Append(ID_MENU_FILE_NEW_GUI,   wxT("New &GUI\tCtrl+Alt+N"), wxT(""));
    NewMenu->Append(ID_MENU_FILE_NEW_FONT,  wxT("New &Font"), wxT(""));

    item1->AppendSubMenu(NewMenu, wxT("&New"));
    item1->Append(ID_MENU_FILE_OPEN, wxT("&Open...\tCtrl+O"), wxT(""));

    item1->AppendSeparator();
    item1->Append(ID_MENU_FILE_CONFIGURE, wxT("&Configure CaWE..."), wxT("") );
    item1->Append(ID_MENU_FILE_EXIT, wxT("E&xit"), wxT("") );
    m_FileHistory.Load(*wxConfigBase::Get());
    m_FileHistory.UseMenu(item1);
    m_FileHistory.AddFilesToMenu(item1);
    item0->Append(item1, wxT("&File") );

    wxMenu* item2 = new wxMenu;
    item2->Append(ID_MENU_HELP_CONTENTS, wxT("&CaWE Help\tF1"), wxT("") );
    item2->AppendSeparator();
    item2->Append(ID_MENU_HELP_CAFU_WEBSITE, wxT("Cafu &Website"), wxT("") );
    item2->Append(ID_MENU_HELP_CAFU_FORUM, wxT("Cafu &Forum"), wxT("") );
    item2->AppendSeparator();
    if (wxConfigBase::Get()->Read("General/Activate Hidden", 0L)==0x1978)
    {
        // "Secret, hidden" CaWE functions...
        item2->Append(ID_MENU_HELP_D3_MTR_CONVERTER, wxT("Doom3 materials converter"), wxT("") );
        item2->AppendSeparator();
    }
    item2->Append(ID_MENU_HELP_ABOUT, wxT("&About..."), wxT("") );
    item0->Append(item2, wxT("&Help") );

    SetMenuBar(item0);      // According to the documentation of wxFrame::SetMenuBar(), this generates a call to OnSize()...


#ifdef __WXMSW__
#if 0
    wxIconBundle Icons;

    Icons.AddIcon(wxIcon("aaaa", wxBITMAP_TYPE_ICO_RESOURCE, 48, 48));
    Icons.AddIcon(wxIcon("aaaa", wxBITMAP_TYPE_ICO_RESOURCE, 32, 32));
    Icons.AddIcon(wxIcon("aaaa", wxBITMAP_TYPE_ICO_RESOURCE, 16, 16));

    SetIcons(Icons);
#else
    SetIcon(wxIcon("aaaa", wxBITMAP_TYPE_ICO_RESOURCE));
#endif
#endif


    // Create the parent GL canvas and a GL context.
    m_GLCanvas =new wxGLCanvas(this, -1, OpenGLAttributeList, wxPoint(600, 5), wxSize(10, 10), 0, "ParentGLCanvas");
    m_GLContext=new wxGLContext(m_GLCanvas);

    Maximize();     // Under wxGTK, the wxMAXIMIZE frame style does not seem to suffice...
    Show();         // Without this, the parent frame is not shown...

#ifdef __WXMSW__
    // ARGH! See my message "wx 2.9.0, Showing parent frame during app init" to wx-users
    // at http://article.gmane.org/gmane.comp.lib.wxwindows.general/68490 for details.
    wxShowEvent SE(0, true);
    OnShow(SE);
#endif
}


ParentFrameT::~ParentFrameT()
{
    m_FileHistory.Save(*wxConfigBase::Get());

    // Release the GuiManager (BEFORE the renderer).
    if (cf::GuiSys::GuiMan!=NULL)
    {
        delete cf::GuiSys::GuiMan;
        cf::GuiSys::GuiMan=NULL;
    }

    // Release the Cafu Material System.
    MatSys::TextureMapManager=NULL;

    if (MatSys::Renderer!=NULL)
    {
        MatSys::Renderer->Release();
        MatSys::Renderer=NULL;
    }

    if (m_RendererDLL!=NULL)
    {
        FreeLibrary(m_RendererDLL);
        m_RendererDLL=NULL;
    }
}


ChildFrameT* ParentFrameT::GetActiveMapChildFrame() const
{
    wxMDIChildFrame* ActiveChildFrame=GetActiveChild();
    if (ActiveChildFrame==NULL) return NULL;

    ChildFrameT* ActiveChildFrameCast=dynamic_cast<ChildFrameT*>(ActiveChildFrame);

    return ActiveChildFrameCast;
}


MapDocumentT* ParentFrameT::GetActiveMapDoc() const
{
    ChildFrameT* ACF=GetActiveMapChildFrame();

    if (ACF==NULL) return NULL;

    return ACF->GetDoc();
}


/**********************/
/*** Event Handlers ***/
/**********************/

#ifdef __WXGTK__
// This is the default behaviour under WXMSW, but apparently not under WXGTK...
void ParentFrameT::OnSize(wxSizeEvent& SE)
{
    if (GetClientWindow())
    {
        GetClientWindow()->SetSize(GetClientSize());
    }
}
#endif


void ParentFrameT::OnShow(wxShowEvent& SE)
{
    if (SE.IsShown() && m_RendererDLL==NULL)
    {
        // Initialize the Material System.
        // This code is in this place due to a few peculiarities of OpenGL under GTK that do not exist under MSW:
        //   - First, an OpenGL context can only be made current with a canvas that is shown on the screen.
        //   - Second, calling Show() in the ctor above doesn't show the frame immediately - that requires
        //     getting back to the main event loop first.
        // Consequently, the first and best opportunity for initializing the MatSys is here.
        wxASSERT(m_GLCanvas->IsShownOnScreen());

        // If this was in the ctor, it would trigger an assertion in debug build and yield an invalid (unusable)
        // OpenGL context in release builds (the GL code in the MatSys::Renderer->IsSupported() methods would fail).
        m_GLCanvas->SetCurrent(*m_GLContext);

        // Prepare the name strings.
        #ifdef SCONS_BUILD_DIR
            #define QUOTE(str) QUOTE_HELPER(str)
            #define QUOTE_HELPER(str) #str

            #ifdef _WIN32
            const wxString MatSysRendererDLLName=wxString("Libs/")+QUOTE(SCONS_BUILD_DIR)+"/MaterialSystem/RendererOpenGL12.dll";
            #else
            const wxString MatSysRendererDLLName=wxString("Libs/")+QUOTE(SCONS_BUILD_DIR)+"/MaterialSystem/libRendererOpenGL12.so";
            #endif

            #undef QUOTE
            #undef QUOTE_HELPER
        #else
            const wxString MatSysRendererDLLName=wxString("Renderers/RendererOpenGL12")+PlatformAux::GetEnvFileSuffix().c_str()+".dll";
        #endif


        // Load the DLL.
        #ifdef _WIN32
            m_RendererDLL=LoadLibrary(L"./RendererOpenGL12.dll");

            if (m_RendererDLL==NULL)
                m_RendererDLL=LoadLibrary(MatSysRendererDLLName);
        #else
            // Note that RTLD_GLOBAL must *not* be passed-in here, or else we get in trouble with subsequently loaded libraries.
            // (E.g. dlsym(RendererDLL, "GetRenderer") return identical results for different RendererDLLs.)
            // Please refer to the man page of dlopen for more details.
            m_RendererDLL=dlopen("./libRendererOpenGL12.so", RTLD_NOW);
            if (!m_RendererDLL) m_RendererDLL=dlopen(MatSysRendererDLLName.c_str(), RTLD_NOW);

            if (!m_RendererDLL) printf("%s\n", dlerror());
        #endif

        if (m_RendererDLL==NULL) { wxMessageBox("FAILED - could not load the library at "+MatSysRendererDLLName, "ERROR"); Destroy(); return; }


        // Get the renderer.
        typedef MatSys::RendererI* (__stdcall *GetRendererT)(cf::ConsoleI* Console_, cf::FileSys::FileManI* FileMan_);

        #ifdef _WIN32
            GetRendererT GetRenderer=(GetRendererT)GetProcAddress(m_RendererDLL, "_GetRenderer@8");
        #else
            GetRendererT GetRenderer=(GetRendererT)GetProcAddress(m_RendererDLL, "GetRenderer");
        #endif

        if (!GetRenderer) { wxMessageBox("FAILED - could not get the address of the GetRenderer() function.", "ERROR"); Destroy(); return; }

        // When we get here, the console and the file man must already have been initialized.
        wxASSERT(Console!=NULL);
        wxASSERT(cf::FileSys::FileMan!=NULL);
        MatSys::Renderer=GetRenderer(Console, cf::FileSys::FileMan);

        if (MatSys::Renderer==NULL) { wxMessageBox("FAILED - could not get the renderer.", "ERROR"); Destroy(); return; }


        // Check if we already have OpenGL errors here.
        // Shouldn't be the case though, because any errors here must have been caused by the ParentFrames wxCanvas ctor.
        GLenum Error=glGetError();
        if (Error!=GL_NO_ERROR) wxMessageBox(wxString::Format("glGetError() reported error %i!", Error));

        if (!MatSys::Renderer->IsSupported())
        {
            wxMessageBox("Renderer "+MatSysRendererDLLName+" says that it's not supported.\n\n"
                         "This may be caused by your desktop being set to 16 BPP (or less).\n"
                         "Please set your desktop bit-depth to 24 or 32 BPP (True Color), and try again.");
            Destroy();
            return;
        }

        MatSys::Renderer->Initialize();


        // Get the texture manager.
        typedef MatSys::TextureMapManagerI* (__stdcall *GetTMMT)();

        #ifdef _WIN32
            GetTMMT GetTMM=(GetTMMT)GetProcAddress(m_RendererDLL, "_GetTextureMapManager@0");
        #else
            GetTMMT GetTMM=(GetTMMT)GetProcAddress(m_RendererDLL, "GetTextureMapManager");
        #endif

        if (!GetTMM) { wxMessageBox("FAILED - could not get the address of the GetTextureMapManager() function.", "ERROR"); Destroy(); return; }
        MatSys::TextureMapManager=GetTMM();
        if (MatSys::TextureMapManager==NULL) { wxMessageBox("No TextureMapManager obtained.", "ERROR"); Destroy(); return; }

        // Create a very simple lightmap for the materials that need one, and register it with the renderer.
        char Data[]={ 255, 255, 255, 255, 255, 255, 0, 0,
                      255, 255, 255, 255, 255, 255, 0, 0 };

        MatSys::Renderer->SetCurrentLightMap(MatSys::TextureMapManager->GetTextureMap2D(Data, 2, 2, 3, true, MapCompositionT(MapCompositionT::Linear, MapCompositionT::Linear)));
        MatSys::Renderer->SetCurrentLightDirMap(NULL);      // The MatSys provides a default for LightDirMaps when NULL is set.


        // Initialize the GUI managager.
        // This has to be done after all materials are loaded (AppCaWE::OnInit()) and after the MatSys::Renderer has been initialized,
        // so that the GuiManager finds its default material and can register it for rendering.
        cf::GuiSys::GuiMan=new cf::GuiSys::GuiManImplT();
    }
}


void ParentFrameT::OnClose(wxCloseEvent& CE)
{
    if (!CE.CanVeto())
    {
        Destroy();
        return;
    }


    // Have to work with a copy of the list, because child frames remove themselves from the original list
    // when they are successfully closed during the following process.
    ArrayT<ChildFrameT*> ChildFrames=m_ChildFrames;

    for (unsigned long ChildNr=0; ChildNr<ChildFrames.Size(); ChildNr++)
        if (!ChildFrames[ChildNr]->Close())
        {
            // If the child was not closed, e.g. because the user vetoed against it,
            // veto in turn against the close of the parent frame and stop trying to close the remaining children.
            CE.Veto();
            return;
        }


    // See comment above.
    ArrayT<ModelEditor::ChildFrameT*> MdlChildFrames=m_MdlChildFrames;

    for (unsigned long ChildNr=0; ChildNr<MdlChildFrames.Size(); ChildNr++)
        if (!MdlChildFrames[ChildNr]->Close())
        {
            // If the child was not closed, e.g. because the user vetoed against it,
            // veto in turn against the close of the parent frame and stop trying to close the remaining children.
            CE.Veto();
            return;
        }


    // See comment above.
    ArrayT<GuiEditor::ChildFrameT*> GuiChildFrames=m_GuiChildFrames;

    for (unsigned long ChildNr=0; ChildNr<GuiChildFrames.Size(); ChildNr++)
        if (!GuiChildFrames[ChildNr]->Close())
        {
            // If the child was not closed, e.g. because the user vetoed against it,
            // veto in turn against the close of the parent frame and stop trying to close the remaining children.
            CE.Veto();
            return;
        }


    // All child frames were successfully closed.
    // It's safe now to also close this parent window, which will gracefully exit the application.
    Destroy();
}


GameConfigT* ParentFrameT::AskUserForGameConfig(const wxFileName& DocumentPath)
{
    if (Options.GameConfigs.Size()==0)
    {
        wxMessageBox("I scanned the Games subdirectory for game configurations,\n"
                     "but didn't find any.\n"
                     "You need to have at least one game configuration inside your\n"
                     "Games directory for the editor to work.\n"
                     "Please re-install Cafu or contact the Cafu forums for help.", "No game configurations found", wxICON_ERROR);

        return NULL;
    }

    // If there is only exactly one game configuration, take it.
    if (Options.GameConfigs.Size()==1) return Options.GameConfigs[0];

    // If there is more than one game configuration, try to choose one using the pathname of the map.
    unsigned int i=1;

    // Find subdirectory from which we can extrapolate the configuration name.
    for (i=1; i<DocumentPath.GetDirCount(); i++)
    {
        if (DocumentPath.GetExt()=="cmap" && DocumentPath.GetDirs()[i]=="Maps") break;
        if (DocumentPath.GetExt()=="cgui" && DocumentPath.GetDirs()[i]=="GUIs") break;
    }

    // Now we got the index from the first subdirectory of our document (one directory before it lies our game directory and configuration name).
    if (i<DocumentPath.GetDirCount())
    {
        // Compare the game directory name with all available configurations and return if a match is found.
        for (unsigned long CfgNr=0; CfgNr<Options.GameConfigs.Size(); CfgNr++)
        {
            if (DocumentPath.GetDirs()[i-1]==Options.GameConfigs[CfgNr]->Name) return Options.GameConfigs[CfgNr];
        }
    }

    // If there is more than one game configuration, prompt the user to select one.
    ArrayT<wxString> Choices;

    for (unsigned long CfgNr=0; CfgNr<Options.GameConfigs.Size(); CfgNr++)
        Choices.PushBack(wxString(Options.GameConfigs[CfgNr]->Name));

    const int SelectionNr=wxGetSingleChoiceIndex("Available configurations:", wxString("Please select a game for ")+DocumentPath.GetFullName(), Choices.Size(), &Choices[0]);

    if (SelectionNr==-1) return NULL;
    return Options.GameConfigs[SelectionNr];
}


void ParentFrameT::OnMenuFile(wxCommandEvent& CE)
{
    wxASSERT(m_RendererDLL!=NULL && MatSys::Renderer!=NULL);

    switch (CE.GetId())
    {
        case ID_MENU_FILE_NEW_MAP:
        {
            GameConfigT* GameConfig=AskUserForGameConfig(wxFileName("the new map"));
            if (GameConfig==NULL) break;

            // Create the new, empty document.
            MapDocumentT* NewDocument=new MapDocumentT(GameConfig);

            // Create the child frame and give the NewDocument to it (the child frame becomes the owner of the NewDocument).
            new ChildFrameT(this, "New Document", NewDocument);
            break;
        }

        case ID_MENU_FILE_NEW_MODEL:
        {
            GameConfigT* GameConfig=AskUserForGameConfig(wxFileName("the new model"));
            if (GameConfig==NULL) break;

            // try
            // {
                new ModelEditor::ChildFrameT(this, "New Model", new ModelEditor::ModelDocumentT(GameConfig));
            // }
            // catch (const ...& /*E*/)
            // {
            //     wxMessageBox(wxString("An error occured during model creation, no model has been created!", "Couldn't create model"));
            // }
            break;
        }

        case ID_MENU_FILE_NEW_GUI:
        {
            GameConfigT* GameConfig=AskUserForGameConfig(wxFileName("the new gui"));
            if (GameConfig==NULL) break;

            try
            {
                new GuiEditor::ChildFrameT(this, "New GUI", new GuiEditor::GuiDocumentT(GameConfig));
            }
            catch (const cf::GuiSys::GuiImplT::InitErrorT& /*E*/)
            {
                wxMessageBox(wxString("An error occured during GUI creation, no GUI has been created!", "Couldn't create GUI"));
            }
            break;
        }

        case ID_MENU_FILE_NEW_FONT:
        {
            FontWizardT* FontWizard=new FontWizardT(this);

            FontWizard->Run();
            FontWizard->Destroy();
            break;
        }

        case ID_MENU_FILE_OPEN:
        {
            // Ask user for file name.
            wxFileDialog FileDialog(this,                                               // The window parent.
                                    "Choose a map, model or gui to open or import.",    // Message.
                                    "",                                                 // The default directory.
                                    "",                                                 // The default file name.
                                    "All Cafu files (*.cmap, *.cmdl, *.cgui)|*.cmap;*.cmdl;*.cgui"  // The wildcard.
                                    "|Cafu map file (*.cmap)|*.cmap"
                                    "|Cafu model file (*.cmdl)|*.cmdl"
                                    "|Cafu GUI file (*.cgui)|*.cgui"
                                    "|Import any model file (*.*)|*.*"
                                    "|Import Doom3 map file (*.map)|*.map"
                                    "|Import Hammer 3.x (HL1) map file (*.map)|*.map"
                                    "|Import Hammer 4.0 (HL2) map file (*.vmf)|*.vmf",
                                    wxFD_OPEN | wxFD_FILE_MUST_EXIST);

            if (FileDialog.ShowModal()!=wxID_OK) break;

            GameConfigT* GameConfig=AskUserForGameConfig(wxFileName(FileDialog.GetPath()));

            if (GameConfig==NULL) break;

            try
            {
                if (FileDialog.GetPath().EndsWith(".cmap"))
                {
                    wxProgressDialog ProgressDialog("Loading Cafu Map File", "Almost done...", 100, this, wxPD_AUTO_HIDE | wxPD_APP_MODAL | wxPD_SMOOTH);

                    new ChildFrameT(this, FileDialog.GetPath(), new MapDocumentT(GameConfig, &ProgressDialog, FileDialog.GetPath()));
                    m_FileHistory.AddFileToHistory(FileDialog.GetPath());   // Native cmap files are added to the MRU list.
                }
                else if (FileDialog.GetPath().EndsWith(".cmdl"))
                {
                    new ModelEditor::ChildFrameT(this, FileDialog.GetPath(), new ModelEditor::ModelDocumentT(GameConfig, FileDialog.GetPath()));
                    m_FileHistory.AddFileToHistory(FileDialog.GetPath());   // Native cmdl files are added to the MRU list.
                }
                else if (FileDialog.GetPath().EndsWith(".cgui"))
                {
                    if (FileDialog.GetPath().EndsWith("_init.cgui"))
                    {
                        new GuiEditor::ChildFrameT(this, FileDialog.GetPath(), new GuiEditor::GuiDocumentT(GameConfig, FileDialog.GetPath()));
                        m_FileHistory.AddFileToHistory(FileDialog.GetPath());   // GUI files are added to the MRU list.
                    }
                    else wxMessageBox("In order to load this GUI, please open the related file whose name ends with _init.cgui\n"
                                      "(instead of "+FileDialog.GetPath()+").\n\n"
                                      "CaWE always deals with the *_init.cgui files, everything else is for your customizations\n"
                                      "(hand-written script code). This way the files never overwrite each other.", "*_init.gui file expected");
                }
                else
                {
                    switch (FileDialog.GetFilterIndex())
                    {
                        case 4:
                            new ModelEditor::ChildFrameT(this, FileDialog.GetPath(), new ModelEditor::ModelDocumentT(GameConfig, FileDialog.GetPath()));
                            break;

                        case 5:
                        {
                            wxProgressDialog ProgressDialog("Importing Doom3 map file", "Almost done...", 100, this, wxPD_AUTO_HIDE | wxPD_APP_MODAL | wxPD_SMOOTH);

                            new ChildFrameT(this, FileDialog.GetPath(), MapDocumentT::ImportDoom3Map(GameConfig, &ProgressDialog, FileDialog.GetPath()));
                            break;
                        }

                        case 6:
                        {
                            wxProgressDialog ProgressDialog("Importing HL1 map file", "Almost done...", 100, this, wxPD_AUTO_HIDE | wxPD_APP_MODAL | wxPD_SMOOTH);

                            new ChildFrameT(this, FileDialog.GetPath(), MapDocumentT::ImportHalfLife1Map(GameConfig, &ProgressDialog, FileDialog.GetPath()));
                            break;
                        }

                        case 7:
                        {
                            wxProgressDialog ProgressDialog("Importing HL2 vmf file", "Almost done...", 100, this, wxPD_AUTO_HIDE | wxPD_APP_MODAL | wxPD_SMOOTH);

                            new ChildFrameT(this, FileDialog.GetPath(), MapDocumentT::ImportHalfLife2Vmf(GameConfig, &ProgressDialog, FileDialog.GetPath()));
                            break;
                        }
                    }
                }
            }
            catch (const MapDocumentT::LoadErrorT& /*E*/)
            {
                wxMessageBox("The document could not be loaded or imported!");
            }
            catch (const cf::GuiSys::GuiImplT::InitErrorT& /*E*/)
            {
                wxMessageBox(wxString("The GUI script \"")+FileDialog.GetPath()+"\" could not be loaded!", "Couldn't load GUI script");
            }
            break;
        }

        case ID_MENU_FILE_CONFIGURE:
        {
            OptionsDialogT OptionsDialog(this);

            if (OptionsDialog.ShowModal()==wxID_OK) Options.Write();
            break;
        }

        case ID_MENU_FILE_EXIT:
        {
            // Close() generates a EVT_CLOSE event which is handled by our OnClose() handler.
            // See wx Window Deletion Overview for more details.
            Close();
            break;
        }

        default:
        {
            if (CE.GetId()>=wxID_FILE1 && CE.GetId()<=wxID_FILE9)
            {
                // Handle the file history (MRU list).
                wxString     FileName  =m_FileHistory.GetHistoryFile(CE.GetId()-wxID_FILE1);
                GameConfigT* GameConfig=AskUserForGameConfig(wxFileName(FileName));

                if (GameConfig==NULL) break;

                try
                {
                    // Handle GUI files here.
                    if (FileName.EndsWith(".cmdl"))
                    {
                        new ModelEditor::ChildFrameT(this, FileName, new ModelEditor::ModelDocumentT(GameConfig, FileName));
                    }
                    else if (FileName.EndsWith(".cgui"))
                    {
                        if (FileName.EndsWith("_init.cgui")) new GuiEditor::ChildFrameT(this, FileName, new GuiEditor::GuiDocumentT(GameConfig, FileName));
                                                        else wxMessageBox("Only GUI initialization scripts (_init.cgui extension) can be opened by CaWE", "Error", wxICON_ERROR);
                    }
                    else
                    {
                        wxProgressDialog ProgressDialog("Loading Cafu Map File", "Almost done...", 100, this, wxPD_AUTO_HIDE | wxPD_APP_MODAL | wxPD_SMOOTH);

                        new ChildFrameT(this, FileName, new MapDocumentT(GameConfig, &ProgressDialog, FileName));
                    }

                    m_FileHistory.AddFileToHistory(FileName);   // All map, model and GUI files are added to the MRU list.
                }
                catch (const MapDocumentT::LoadErrorT& /*E*/)
                {
                    wxMessageBox(wxString("The map file \"")+FileName+"\" could not be loaded!", "Couldn't load the map");
                }
                catch (const cf::GuiSys::GuiImplT::InitErrorT& /*E*/)
                {
                    wxMessageBox(wxString("The GUI script \"")+FileName+"\" could not be loaded!", "Couldn't load GUI script");
                }
            }

            break;
        }
    }
}


/// Skips a block in TP, e.g. a { ... } or ( ... ) block. The block can contain nested blocks.
/// It can (and must) be stated if the caller has already read the opening token.
static void SkipBlock(TextParserT& TP, const std::string& OpeningToken, const std::string& ClosingToken, bool CallerAlreadyReadOpeningToken)
{
    if (!CallerAlreadyReadOpeningToken) TP.GetNextToken();

    unsigned long NestedLevel=1;

    while (true)
    {
        const std::string Token=TP.GetNextToken();

        if (Token==OpeningToken)
        {
            NestedLevel++;
        }
        else if (Token==ClosingToken)
        {
            NestedLevel--;
            if (NestedLevel==0) break;
        }
    }
}


/// Replaces the "textures/" prefix of Doom3 texture image paths with "texturesD3/",
/// and "models/" with "modelsD3/". For all other texture names, simply "D3/" is prepended.
static wxString ReplaceTexturePrefix(const wxString& D3TexPath)
{
    if (D3TexPath.StartsWith("textures/") || D3TexPath.StartsWith("textures\\"))
    {
        wxString FixedString=D3TexPath;
        FixedString.insert(8, "D3");
        return FixedString;
    }
    else if (D3TexPath.StartsWith("models/") || D3TexPath.StartsWith("models\\"))
    {
        wxString FixedString=D3TexPath;
        FixedString.insert(6, "D3");
        return FixedString;
    }
    else
    {
        // There are countless other prefixes, and a few apparently built-in "textures":
        // _scratch _fog _cubicLight _spotlight _pointLight1,2,3 _black _quadratic _default _flat
        return wxString("D3/")+D3TexPath;
    }
}


static wxString ParseD3MapComposition(TextParserT& TP)
{
    std::string Token=TP.GetNextToken();

    if (Token=="addnormals")
    {
        TP.AssertAndSkipToken("(");
        wxString NMap1=ParseD3MapComposition(TP);
        TP.AssertAndSkipToken(",");
        wxString NMap2=ParseD3MapComposition(TP);
        TP.AssertAndSkipToken(")");

        return wxString("combineNMs(")+NMap1+", "+NMap2+")";
    }
    else if (Token=="heightmap")
    {
        TP.AssertAndSkipToken("(");
        wxString HeightMap=ParseD3MapComposition(TP);
        TP.AssertAndSkipToken(",");
        wxString Scale=TP.GetNextToken().c_str();
        TP.AssertAndSkipToken(")");

        return wxString("hm2nm(")+HeightMap+", "+Scale+")";
    }
    else if (Token=="makealpha")
    {
        TP.AssertAndSkipToken("(");
        wxString Map=ParseD3MapComposition(TP);
        TP.AssertAndSkipToken(")");

        return wxString("blue2alpha(")+Map+")";
    }
    else if (Token=="makeintensity")
    {
        TP.AssertAndSkipToken("(");
        wxString Map=ParseD3MapComposition(TP);
        TP.AssertAndSkipToken(")");

        // Don't know exactly what makeintensity means (for a normal texture).
        return Map;
    }
    else
    {
        // It's a texture name.
        wxString TexName=ReplaceTexturePrefix(Token.c_str());

        if (TexName.Right(4)!=".tga") TexName+=".tga";

        return TexName;
    }
}


static wxString ParseD3RenderStage(TextParserT& TP, char& MapID)
{
    MapID=0;
    wxString TexName="";

    while (true)
    {
        std::string Token=TP.GetNextToken();

        if (Token=="}")
        {
            return TexName;
        }
        else if (Token=="{")
        {
            SkipBlock(TP, "{", "}", true);
        }
        else if (Token=="if")
        {
            if (TP.PeekNextToken()=="(") SkipBlock(TP, "(", ")", false);
                                    else TP.GetNextToken();
        }
        else if (Token=="blend")
        {
            wxString MapToken=wxString(TP.GetNextToken().c_str()).Lower();

                 if (MapToken=="diffusemap" ) MapID='d';
            else if (MapToken=="bumpmap"    ) MapID='b';
            else if (MapToken=="specularmap") MapID='s';
            else                              MapID='c';    // Custom blend mode.
        }
        else if (Token=="map")
        {
            TexName=ParseD3MapComposition(TP);
            if (MapID==0) MapID='c';
        }
    }
}


void ParentFrameT::OnMenuHelp(wxCommandEvent& CE)
{
    wxASSERT(m_RendererDLL!=NULL && MatSys::Renderer!=NULL);

    switch (CE.GetId())
    {
        case ID_MENU_HELP_CONTENTS:
            if (!wxLaunchDefaultBrowser("http://www.cafu.de/wiki"))
                wxMessageBox("Sorry, I could not open www.cafu.de/wiki in your default browser automatically.");
            break;

        case ID_MENU_HELP_CAFU_WEBSITE:
            if (!wxLaunchDefaultBrowser("http://www.cafu.de"))
                wxMessageBox("Sorry, I could not open www.cafu.de in your default browser automatically.");
            break;

        case ID_MENU_HELP_CAFU_FORUM:
            if (!wxLaunchDefaultBrowser("http://www.cafu.de/forum"))
                wxMessageBox("Sorry, I could not open www.cafu.de/forum in your default browser automatically.");
            break;

        case ID_MENU_HELP_D3_MTR_CONVERTER:
        {
            /// This method converts Doom3 *.mtr materials to Cafu *.cmat materials.
            /// The source and destination folders can be chosen.
            /// The file names are kept, just the extention is replaced (e.g. "senetemp.mtr" in the source folder becomes "senetemp.cmat" in the dest folder).
            /// For a Doom3 *.mtr material name of the form "textures/xy/z", the Cafu *.cmat material name "D3conv/textures/xy/z" is generated,
            /// i.e. "D3conv/" is prepended.
            /// For a Doom3 texture name of the form "textures/xy/z.tga", the texture name "texturesD3/xy/z.tga" is written into the Cafu *.cmat file,
            /// i.e. "D3" is inserted.
            wxString SourceMtrDirName=wxDirSelector("Please choose the folder with the Doom3 *.mtr source material scripts."); if (SourceMtrDirName=="") break;
            wxString DestCmatDirName =wxDirSelector("Please choose the folder for the Cafu *.cmat destination material scripts."); if (DestCmatDirName=="") break;

            // This sets the cursor to the busy cursor in its ctor, and back to the default cursor in the dtor.
            wxBusyCursor BusyCursor;

            wxDir SourceMtrDir(SourceMtrDirName);
            if (!SourceMtrDir.IsOpened()) break;

            unsigned long CountMaterialFiles            =0;
            unsigned long CountMaterialDefs             =0;
            unsigned long CountMatsWithNoDiffuseMap     =0;
            unsigned long CountMatsThatStartWithTextures=0;

            wxString MtrName;
            for (bool more=SourceMtrDir.GetFirst(&MtrName, "*.mtr", wxDIR_FILES); more; more=SourceMtrDir.GetNext(&MtrName))
            {
                wxString MtrPathName =SourceMtrDirName+"/"+MtrName;
                wxString CMatName    =MtrName; CMatName.replace(MtrName.length()-3, 3, "cmat");
                wxString CMatPathName=DestCmatDirName+"/"+CMatName;

                TextParserT TP(MtrPathName.c_str(), "({,})");
                std::ofstream CMatFile(CMatPathName.fn_str());
                CMatFile << "// Cafu material definition file.\n";
                CMatFile << "// Created by the CaWE built-in D3 mtr converter.\n\n";
                CountMaterialFiles++;

                try
                {
                    while (!TP.IsAtEOF())
                    {
                        if (TP.PeekNextToken()=="table")
                        {
                            // Overread the table.
                            TP.AssertAndSkipToken("table");
                            TP.GetNextToken();  // Table name.
                            SkipBlock(TP, "{", "}", false);
                        }
                        else if (TP.PeekNextToken()=="/*"  ) SkipBlock(TP, "/*",     "*/", false);
                        else if (TP.PeekNextToken()=="/**" ) SkipBlock(TP, "/**",   "**/", false);
                        else if (TP.PeekNextToken()=="/***") SkipBlock(TP, "/***", "***/", false);
                        else if (TP.PeekNextToken()=="skin")
                        {
                            TP.GetNextToken();
                            TP.GetNextToken();
                            SkipBlock(TP, "{", "}", false);
                        }
                        else if (TP.PeekNextToken()=="particle")
                        {
                            TP.GetNextToken();
                            TP.GetNextToken();
                            SkipBlock(TP, "{", "}", false);
                        }
                        else
                        {
                            // It's a material definition.
                            CountMaterialDefs++;
                            wxString MaterialName=TP.GetNextToken().c_str();
                            if (MaterialName=="material") MaterialName=TP.GetNextToken().c_str();
                            if (MaterialName.StartsWith("textures")) CountMatsThatStartWithTextures++;

                            // It seems that some material names in D3 end with ".tga", which they shouldn't.
                            if (MaterialName.Right(4)==".tga") MaterialName.erase(MaterialName.length()-4);

                            TP.AssertAndSkipToken("{");

                            CMatFile << "D3conv/" << MaterialName.c_str() << "\n";
                            CMatFile << "{\n";

                            wxString diffuseMap    ="";
                            wxString normalMap     ="";
                            wxString specularMap   ="";
                            wxString customBlendMap="";
                            wxString editorImage   ="";

                            while (true)
                            {
                                std::string Token=TP.GetNextToken();

                                if (Token=="}")
                                {
                                    break;
                                }
                                else if (Token=="/*" ) SkipBlock(TP, "/*",   "*/", true);
                                else if (Token=="/**") SkipBlock(TP, "/**", "**/", true);
                                else if (Token=="{")
                                {
                                    // See if this is a stage / renderpass definition of a map that we have not (yet) found by the simple keywords.
                                    char     MapID =0;
                                    wxString Result=ParseD3RenderStage(TP, MapID);

                                    if (Result!="" && MapID!=0)
                                    {
                                        switch (MapID)
                                        {
                                            // The "if (...=="")" is there for two purposes:
                                            // a) the simple keywords should have priority over the stage definitions, and
                                            // b) if there are multiple stage definitions for the same map, always stay with the first.
                                            case 'd': if (diffuseMap    =="") diffuseMap    =Result; break;
                                            case 'b': if (normalMap     =="") normalMap     =Result; break;
                                            case 's': if (specularMap   =="") specularMap   =Result; break;
                                            case 'c': if (customBlendMap=="") customBlendMap=Result; break;
                                            default: break;
                                        }
                                    }
                                }
                                else if (wxString(Token.c_str()).Lower()=="diffusemap"     ) diffuseMap =ParseD3MapComposition(TP);
                                else if (wxString(Token.c_str()).Lower()=="bumpmap"        ) normalMap  =ParseD3MapComposition(TP);
                                else if (wxString(Token.c_str()).Lower()=="specularmap"    ) specularMap=ParseD3MapComposition(TP);
                                else if (wxString(Token.c_str()).Lower()=="qer_editorimage") editorImage=ParseD3MapComposition(TP);
                                else
                                {
                                    // Ignore everything else.
                                }
                            }

                            if (diffuseMap=="") diffuseMap=editorImage;
                            if (diffuseMap=="") diffuseMap=customBlendMap;


                            if (diffuseMap=="")
                            {
                                // wxMessageBox(wxString("No diffusemap found for mtr material\n")+MaterialName+"\nin file\n"+MtrPathName);
                                CountMatsWithNoDiffuseMap++;
                            }

                            if (diffuseMap !="") CMatFile << "    diffusemap  " << diffuseMap .c_str() << "\n";
                            if (diffuseMap !="") CMatFile << "    lightmap    " << "$lightmap"         << "\n";
                            if (normalMap  !="") CMatFile << "    normalmap   " << normalMap  .c_str() << "\n";
                            if (specularMap!="") CMatFile << "    specularmap " << specularMap.c_str() << "\n";

                            CMatFile << "}\n\n";
                        }
                    }
                }
                catch (const TextParserT::ParseError&)
                {
                    wxMessageBox(wxString("There was an error parsing the file\n")+MtrPathName+wxString::Format("\nnear byte %lu.", TP.GetReadPosByte()));
                }
            }

            wxMessageBox(wxString("The material conversion is complete.\n")+
                         wxString::Format("Converted %lu mtr files to cmat files which contained a total of %lu materials.\n", CountMaterialFiles, CountMaterialDefs)+
                         wxString::Format("For %lu materials no diffusemap was found (e.g. video materials etc.).\n", CountMatsWithNoDiffuseMap)+
                         wxString::Format("The names of %lu materials start with \"textures/\".\n\n", CountMatsThatStartWithTextures)+
                         "ALL materials with a diffusemap also got a \"lightmap $lightmap\" line\n"+
                         "(they are intended to be *temporarily* used in *maps*, after all).\n\n"+
                         "Note that you must restart CaWE in order to use the new materials.");
            break;
        }

        case ID_MENU_HELP_ABOUT:
        {
            wxString TextTop=
                "CaWE is copyright (c) 2002-2010 Carsten Fuchs Software.\n"
                "\n"
                "Please report all encountered bugs.\n";

            wxString TextBottom=
                "Build date: "+wxString(__DATE__)+wxString::Format(". CaWE is installed since %lu days.", Options.DaysSinceInstalled)
                +wxString("\n\nYour CaWE configuration files are located at:\n")+wxStandardPaths::Get().GetUserDataDir();

            wxDialog  AboutDialog(this, -1, "CaWE, the Cafu World Editor - "+wxString(__DATE__), wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER);
            wxWindow* parent=&AboutDialog;

            // ### BEGIN of code generated by and copied from wxDesigner.
            wxBoxSizer *item0 = new wxBoxSizer( wxHORIZONTAL );

            wxStaticBitmap *item1 = new wxStaticBitmap( parent, -1, GetIcon() /*Icon of parent frame.*/, wxDefaultPosition, wxDefaultSize );
            item0->Add( item1, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5 );

            wxBoxSizer *item2 = new wxBoxSizer( wxVERTICAL );

            wxStaticText *item3 = new wxStaticText( parent, -1, TextTop, wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE );
            item2->Add( item3, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 5 );

            wxStaticBox *item5 = new wxStaticBox( parent, -1, wxT("External Libraries and Artwork") );
            wxStaticBoxSizer *item4 = new wxStaticBoxSizer( item5, wxVERTICAL );

            class AutoUrlTextCtrlT : public wxTextCtrl
            {
                public:

                AutoUrlTextCtrlT(wxWindow* Parent, wxWindowID ID, const wxString& Value="", const wxPoint& Pos=wxDefaultPosition, const wxSize& Size=wxDefaultSize, long Style=0)
                    : wxTextCtrl(Parent, ID, Value, Pos, Size, Style)
                {
                    Connect(wxEVT_COMMAND_TEXT_URL, wxTextUrlEventHandler(AutoUrlTextCtrlT::OnAboutUrl));
                }

                /// Event handler for "text URL" events.
                void OnAboutUrl(wxTextUrlEvent& TUE)
                {
                    const wxMouseEvent& ME=TUE.GetMouseEvent();

                    // Filter out mouse moves, too many of them.
                    if (ME.Moving()) return;
                    if (!ME.LeftDown()) return;

                    const int UrlStart=TUE.GetURLStart();
                    const int UrlEnd  =TUE.GetURLEnd();

                    wxLaunchDefaultBrowser(GetValue().Mid(UrlStart, UrlEnd-UrlStart));
                }
            };

            wxTextCtrl *item6 = new AutoUrlTextCtrlT(parent, -1, wxT(""), wxDefaultPosition, wxSize(400, 200),
                wxTE_MULTILINE | wxTE_READONLY | wxTE_RICH2 | wxTE_AUTO_URL | wxBORDER_SUNKEN);

            item6->LoadFile("CaWE/res/AboutExtLibs.txt");           // This is much simpler and more flexible than any string constant.
         // item6->SetInsertionPoint(item6->GetLastPosition());     // Scrolls to the bottom of the text.

            item4->Add( item6, 1, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 5 );

            item2->Add( item4, 1, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 5 );

            wxStaticText *item7 = new wxStaticText( parent, -1, TextBottom, wxDefaultPosition, wxDefaultSize, 0 );
            item2->Add( item7, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 5 );

            wxButton *item8 = new wxButton( parent, wxID_OK, wxT("OK"), wxDefaultPosition, wxDefaultSize, 0 );
            item8->SetDefault();
            item2->Add( item8, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL|wxALL, 5 );

            item0->Add( item2, 1, wxGROW|wxALIGN_CENTER_HORIZONTAL, 5 );

            parent->SetSizer( item0 );
            item0->SetSizeHints( parent );
            // ### END of code generated by and copied from wxDesigner.

            AboutDialog.CenterOnParent();
            AboutDialog.ShowModal();
            break;
        }
    }
}