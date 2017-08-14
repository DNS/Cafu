/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "ChildFrame.hpp"
#include "GuiDocument.hpp"
#include "RenderWindow.hpp"
#include "WindowHierarchy.hpp"
#include "WindowInspector.hpp"
#include "GuiInspector.hpp"
#include "LivePreview.hpp"

#include "Commands/AddComponent.hpp"
#include "Commands/Create.hpp"
#include "Commands/Delete.hpp"
#include "Commands/Select.hpp"
#include "Commands/ChangeWindowHierarchy.hpp"
#include "Commands/Paste.hpp"

#include "../GameConfig.hpp"
#include "../ParentFrame.hpp"
#include "../SetCompVar.hpp"

#include "GuiSys/AllComponents.hpp"
#include "GuiSys/CompBase.hpp"
#include "GuiSys/CompText.hpp"
#include "GuiSys/Window.hpp"
#include "GuiSys/GuiImpl.hpp"
#include "Math3D/Misc.hpp"

#include "wx/wx.h"
#include "wx/artprov.h"
#include "wx/aui/auibar.h"
#include "wx/confbase.h"
#include "wx/settings.h"
#include "wx/file.h"
#include "wx/filename.h"
#include "wx/numdlg.h"

#include <fstream>


namespace
{
    // Default perspective set by the first childframe instance and used to restore default settings later.
    wxString AUIDefaultPerspective;


    bool CompareTypeInfoNames(const cf::TypeSys::TypeInfoT* const& TI1, const cf::TypeSys::TypeInfoT* const& TI2)
    {
        return wxStricmp(TI1->ClassName, TI2->ClassName) < 0;
    }


    // This function has been duplicated into other modules, too... can we reconcile them?
    wxMenuItem* AppendMI(wxMenu& Menu, int MenuID, const wxString& Label, const wxArtID& ArtID, bool Active=true, const wxString& Help="")
    {
        wxMenuItem* MI = new wxMenuItem(&Menu, MenuID, Label, Help);

        // Under wxMSW (2.9.2), the bitmap must be set before the menu item is added to the menu.
        if (ArtID != "")
            MI->SetBitmap(wxArtProvider::GetBitmap(ArtID, wxART_MENU));

        // Under wxGTK (2.9.2), the menu item must be added to the menu before we can call Enable().
        Menu.Append(MI);

        MI->Enable(Active);

        return MI;
    }
}


/*static*/ void GuiEditor::ChildFrameT::BuildComponentsMenu(wxMenu* MenuParent, const cf::TypeSys::TypeInfoT* TypeParent)
{
    // For each child of TypeParent (but not TypeParent itself), add an item to MenuParent.
    ArrayT<const cf::TypeSys::TypeInfoT*> CompTIs;

    for (const cf::TypeSys::TypeInfoT* TI = TypeParent->Child; TI; TI = TI->Sibling)
    {
        // The topmost root and thus the ComponentBaseT class is never added.
        wxASSERT(TI->Base);

        // Skip fundamental component types (each window has one instance anyway).
        if (cf::GuiSys::IsFundamental(TI)) continue;

        // Skip the ComponentSelectionT class, that is specific to this GUI Editor application.
        if (TI == &ComponentSelectionT::TypeInfo) continue;

        CompTIs.PushBack(TI);
    }

    CompTIs.QuickSort(CompareTypeInfoNames);

    for (unsigned int TINr = 0; TINr < CompTIs.Size(); TINr++)
    {
        const cf::TypeSys::TypeInfoT* TI   = CompTIs[TINr];
        wxString                      Name = TI->ClassName;

        if (Name.StartsWith("Component") && Name.EndsWith("T"))
            Name = Name.SubString(9, Name.length() - 2);

        if (Name.StartsWith("GuiSys::Component") && Name.EndsWith("T"))
            Name = Name.SubString(17, Name.length() - 2);

        wxASSERT(ID_MENU_CREATE_COMPONENT_FIRST + TI->TypeNr <= ID_MENU_CREATE_COMPONENT_MAX);

        if (TI->Child)
        {
            wxMenu* SubMenu = new wxMenu;

            BuildComponentsMenu(SubMenu, TI);
            MenuParent->AppendSubMenu(SubMenu, "&" + Name);
        }
        else
        {
            MenuParent->Append(ID_MENU_CREATE_COMPONENT_FIRST + TI->TypeNr, "&" + Name, "Add component to the selected window");
        }
    }
}


BEGIN_EVENT_TABLE(GuiEditor::ChildFrameT, wxMDIChildFrame)
    EVT_MENU_RANGE     (ID_MENU_FILE_CLOSE,            ID_MENU_FILE_SAVEAS,           GuiEditor::ChildFrameT::OnMenuFile)
    EVT_UPDATE_UI_RANGE(ID_MENU_FILE_CLOSE,            ID_MENU_FILE_SAVEAS,           GuiEditor::ChildFrameT::OnMenuFileUpdate)
    EVT_MENU_RANGE     (wxID_UNDO,                     wxID_REDO,                     GuiEditor::ChildFrameT::OnMenuUndoRedo)
    EVT_UPDATE_UI_RANGE(wxID_UNDO,                     wxID_REDO,                     GuiEditor::ChildFrameT::OnUpdateEditUndoRedo)
    EVT_UPDATE_UI_RANGE(wxID_CUT,                      wxID_PASTE,                    GuiEditor::ChildFrameT::OnMenuEditUpdate)
    EVT_UPDATE_UI_RANGE(ID_MENU_EDIT_DELETE,           ID_MENU_EDIT_SET_GRID_SIZE,    GuiEditor::ChildFrameT::OnMenuEditUpdate)
    EVT_MENU           (wxID_CUT,                                                     GuiEditor::ChildFrameT::OnMenuEditCut)
    EVT_MENU           (wxID_COPY,                                                    GuiEditor::ChildFrameT::OnMenuEditCopy)
    EVT_MENU           (wxID_PASTE,                                                   GuiEditor::ChildFrameT::OnMenuEditPaste)
    EVT_MENU           (ID_MENU_EDIT_DELETE,                                          GuiEditor::ChildFrameT::OnMenuEditDelete)
    EVT_MENU_RANGE     (ID_MENU_EDIT_SNAP_TO_GRID,     ID_MENU_EDIT_SET_GRID_SIZE,    GuiEditor::ChildFrameT::OnMenuEditGrid)
    EVT_MENU_RANGE     (ID_MENU_CREATE_WINDOW,         ID_MENU_CREATE_COMPONENT_MAX,  GuiEditor::ChildFrameT::OnMenuCreate)
    EVT_MENU_RANGE     (ID_MENU_VIEW_WINDOW_HIERARCHY, ID_MENU_VIEW_SAVE_USER_LAYOUT, GuiEditor::ChildFrameT::OnMenuView)
    EVT_UPDATE_UI_RANGE(ID_MENU_VIEW_WINDOW_HIERARCHY, ID_MENU_VIEW_GUIINSPECTOR,     GuiEditor::ChildFrameT::OnMenuViewUpdate)
    EVT_CLOSE          (                                                              GuiEditor::ChildFrameT::OnClose)
    EVT_TOOL_RANGE     (ID_TOOLBAR_DOC_PREVIEW,        ID_TOOLBAR_ZOOM_100,           GuiEditor::ChildFrameT::OnToolbar)
END_EVENT_TABLE()


GuiEditor::ChildFrameT::ChildFrameT(ParentFrameT* Parent, const wxString& FileName, GuiDocumentT* GuiDocument)
    : wxMDIChildFrame(Parent, wxID_ANY, FileName, wxDefaultPosition, wxSize(800, 600), wxDEFAULT_FRAME_STYLE | wxMAXIMIZE),
      m_GuiDocument(GuiDocument),   // Must use a fixed size in place of wxDefaultSize, see <http://trac.wxwidgets.org/ticket/12490> for details.
      m_FileName(FileName),
      m_GameConfig(GuiDocument->GetGameConfig()),
      m_LastSavedAtCommandNr(0),
      m_SnapToGrid(true),
      m_GridSpacing(5),
      m_ToolManager(GuiDocument, this),
      m_Parent(Parent),
      m_RenderWindow(NULL),
      m_WindowHierarchy(NULL),
      m_WindowInspector(NULL),
      m_FileMenu(NULL),
      m_EditMenu(NULL),
      m_CreateMenu(NULL),
      m_ViewMenu(NULL)
{
    // Register us with the parents list of children.
    m_Parent->m_GuiChildFrames.PushBack(this);

    // Set up menu.
    wxMenuBar *item0 = new wxMenuBar;

    m_FileMenu=new wxMenu;

    wxMenu* NewMenu = new wxMenu;

    NewMenu->Append(ParentFrameT::ID_MENU_FILE_NEW_MAP,   wxT("New &Map\tCtrl+N"), wxT(""));
    NewMenu->Append(ParentFrameT::ID_MENU_FILE_NEW_MODEL, wxT("New M&odel\tCtrl+Shift+N"), wxT(""));
    NewMenu->Append(ParentFrameT::ID_MENU_FILE_NEW_GUI,   wxT("New &GUI\tCtrl+Alt+N"), wxT(""));
    NewMenu->Append(ParentFrameT::ID_MENU_FILE_NEW_FONT,  wxT("New &Font"), wxT(""));

    m_FileMenu->AppendSubMenu(NewMenu, wxT("&New"));

    m_FileMenu->Append(ParentFrameT::ID_MENU_FILE_OPEN, wxT("&Open...\tCtrl+O"), wxT("") );
    m_FileMenu->Append(ID_MENU_FILE_CLOSE, wxT("&Close\tCtrl+F4"), wxT("") );
    m_FileMenu->Append(ID_MENU_FILE_SAVE, wxT("&Save\tCtrl+S"), wxT("") );
    m_FileMenu->Append(ID_MENU_FILE_SAVEAS, wxT("Save &As..."), wxT("") );
    m_FileMenu->AppendSeparator();
    m_FileMenu->Append(ParentFrameT::ID_MENU_FILE_CONFIGURE, wxT("Conf&igure CaWE..."), wxT("") );
    m_FileMenu->Append(ParentFrameT::ID_MENU_FILE_EXIT, wxT("E&xit"), wxT("") );
    m_Parent->m_FileHistory.UseMenu(m_FileMenu);
    m_Parent->m_FileHistory.AddFilesToMenu(m_FileMenu);

    item0->Append( m_FileMenu, wxT("&File") );


    m_EditMenu=new wxMenu;
    m_EditMenu->Append(wxID_UNDO, "&Undo\tCtrl+Z", "");
    m_EditMenu->Append(wxID_REDO, "&Redo\tCtrl+Y", "");
    m_EditMenu->AppendSeparator();
    m_EditMenu->Append(wxID_CUT, "Cu&t\tCtrl+X", "");
    m_EditMenu->Append(wxID_COPY, "&Copy\tCtrl+C", "");
    m_EditMenu->Append(wxID_PASTE, "&Paste\tCtrl+V", "");
    m_EditMenu->Append(ID_MENU_EDIT_DELETE, "&Delete\tShift+Del", "");
    m_EditMenu->AppendSeparator();
    m_EditMenu->Append(ID_MENU_EDIT_SNAP_TO_GRID, wxString::Format("Snap to grid (%lu)\tCtrl+G", m_GridSpacing), "", wxITEM_CHECK);
    m_EditMenu->Append(ID_MENU_EDIT_SET_GRID_SIZE, "Set grid size\tCtrl+H", "");
    item0->Append(m_EditMenu, "&Edit");


    m_CreateMenu = new wxMenu;
    AppendMI(*m_CreateMenu, ID_MENU_CREATE_WINDOW, "Window", "window-new", true, "Create new window");
    m_CreateMenu->AppendSeparator();

    const ArrayT<const cf::TypeSys::TypeInfoT*>& CompRoots = cf::GuiSys::GetComponentTIM().GetTypeInfoRoots();

    for (unsigned long RootNr = 0; RootNr < CompRoots.Size(); RootNr++)
    {
        BuildComponentsMenu(m_CreateMenu, CompRoots[RootNr]);

        if (RootNr + 1 < CompRoots.Size())
            m_CreateMenu->AppendSeparator();
    }

    item0->Append(m_CreateMenu, "&Create");


    m_ViewMenu=new wxMenu;
    m_ViewMenu->Append(ID_MENU_VIEW_WINDOW_HIERARCHY, "Window Hierarchy", "Show/Hide the Window Hierarchy", wxITEM_CHECK);
    m_ViewMenu->Append(ID_MENU_VIEW_WINDOWINSPECTOR, "Window Inspector", "Show/Hide the Window Inspector", wxITEM_CHECK);
    m_ViewMenu->Append(ID_MENU_VIEW_GUIINSPECTOR, "GUI Inspector", "Show/Hide the GUI Inspector", wxITEM_CHECK);
    m_ViewMenu->AppendSeparator();
    m_ViewMenu->Append(ID_MENU_VIEW_RESTORE_DEFAULT_LAYOUT, "Restore default layout", "Restores the GUI editor default layout");
    m_ViewMenu->Append(ID_MENU_VIEW_RESTORE_USER_LAYOUT,    "Restore user layout", "Restores the GUI editor user defined layout");
    m_ViewMenu->Append(ID_MENU_VIEW_SAVE_USER_LAYOUT, "Save user layout", "Saves the current GUI editor layout as user defined layout");
    item0->Append(m_ViewMenu, "&View");


    wxMenu* HelpMenu = new wxMenu;
    HelpMenu->Append(ParentFrameT::ID_MENU_HELP_CONTENTS, wxT("&CaWE Help\tF1"), wxT("") );
    HelpMenu->AppendSeparator();
    HelpMenu->Append(ParentFrameT::ID_MENU_HELP_CAFU_WEBSITE, wxT("Cafu &Website"), wxT("") );
    HelpMenu->Append(ParentFrameT::ID_MENU_HELP_CAFU_FORUM, wxT("Cafu &Forum"), wxT("") );
    HelpMenu->AppendSeparator();
    HelpMenu->Append(ParentFrameT::ID_MENU_HELP_ABOUT, wxT("&About..."), wxT("") );
    item0->Append(HelpMenu, wxT("&Help") );

    SetMenuBar(item0);

    // Setup wxAUI.
    m_AUIManager.SetManagedWindow(this);

    // Create GUI views.
    m_RenderWindow   =new RenderWindowT   (this);
    m_WindowHierarchy=new WindowHierarchyT(this, wxSize(230, 500));
    m_WindowInspector=new WindowInspectorT(this, wxSize(230, 500));
    m_GuiInspector   =new GuiInspectorT   (this, wxSize(230, 150));

    m_AUIManager.AddPane(m_RenderWindow, wxAuiPaneInfo().
                         Name("RenderWindow").Caption("Render Window").
                         CenterPane());

    m_AUIManager.AddPane(m_WindowHierarchy, wxAuiPaneInfo().
                         Name("WindowHierarchy").Caption("Window Hierarchy").
                         Left().Position(0));

    m_AUIManager.AddPane(m_WindowInspector, wxAuiPaneInfo().
                         Name("WindowInspector").Caption("Window Inspector").
                         Right().Position(0));

    m_AUIManager.AddPane(m_GuiInspector, wxAuiPaneInfo().
                         Name("GuiInspector").Caption("GUI Inspector").
                         Left().Position(1));

    // Create AUI toolbars.
    wxAuiToolBar* ToolbarDocument=new wxAuiToolBar(this, wxID_ANY);
    ToolbarDocument->AddTool(ParentFrameT::ID_MENU_FILE_NEW_GUI, "New", wxArtProvider::GetBitmap(wxART_NEW, wxART_TOOLBAR), "Create a new file");
    ToolbarDocument->AddTool(ParentFrameT::ID_MENU_FILE_OPEN,    "Open", wxArtProvider::GetBitmap(wxART_FILE_OPEN, wxART_TOOLBAR), "Open an existing file");
    ToolbarDocument->AddTool(ID_MENU_FILE_SAVE,                  "Save", wxArtProvider::GetBitmap(wxART_FILE_SAVE, wxART_TOOLBAR), "Save the file");
    ToolbarDocument->AddTool(ID_MENU_FILE_SAVEAS,                "Save as", wxArtProvider::GetBitmap(wxART_FILE_SAVE_AS, wxART_TOOLBAR), "Save the file under a different name");
    ToolbarDocument->AddSeparator();
    ToolbarDocument->AddTool(wxID_UNDO,           "Undo", wxArtProvider::GetBitmap(wxART_UNDO, wxART_TOOLBAR), "Undo the last action");
    ToolbarDocument->AddTool(wxID_REDO,           "Redo", wxArtProvider::GetBitmap(wxART_REDO, wxART_TOOLBAR), "Redo the previously undone action");
    ToolbarDocument->AddSeparator();
    ToolbarDocument->AddTool(wxID_CUT,            "Cut", wxArtProvider::GetBitmap(wxART_CUT, wxART_TOOLBAR), "Cut");
    ToolbarDocument->AddTool(wxID_COPY,           "Copy", wxArtProvider::GetBitmap(wxART_COPY, wxART_TOOLBAR), "Copy");
    ToolbarDocument->AddTool(wxID_PASTE,          "Paste", wxArtProvider::GetBitmap(wxART_PASTE, wxART_TOOLBAR), "Paste");
    ToolbarDocument->AddTool(ID_MENU_EDIT_DELETE, "Delete", wxArtProvider::GetBitmap(wxART_DELETE, wxART_TOOLBAR), "Delete");
    ToolbarDocument->Realize();

    m_ToolbarTools=new wxAuiToolBar(this, wxID_ANY);
    m_ToolbarTools->AddTool(ID_TOOLBAR_TOOL_SELECTION, "Selection tool", wxArtProvider::GetBitmap("cursor_mouse", wxART_TOOLBAR, wxSize(16, 16) /*The only one that we have at this time; don't scale it.*/), "Selection tool", wxITEM_CHECK);
    m_ToolbarTools->ToggleTool(ID_TOOLBAR_TOOL_SELECTION, true); // Selection tool is active by default.
    m_ToolbarTools->AddTool(ID_TOOLBAR_TOOL_NEW_WINDOW, "Window Creation tool", wxArtProvider::GetBitmap("window-new", wxART_TOOLBAR), "Window creation tool (in this version, use the Create menu, or the context menu in the Window Hierarchy or main view in order to create new windows)", wxITEM_CHECK);
    m_ToolbarTools->EnableTool(ID_TOOLBAR_TOOL_NEW_WINDOW, false);
    m_ToolbarTools->Realize();

    wxAuiToolBar* ToolbarWindow=new wxAuiToolBar(this, wxID_ANY);
    ToolbarWindow->AddTool(ID_TOOLBAR_WINDOW_MOVE_UP,    "Move up",              wxArtProvider::GetBitmap("list-selection-up", wxART_TOOLBAR), "Move window up in the parent's list of children");
    ToolbarWindow->AddTool(ID_TOOLBAR_WINDOW_MOVE_DOWN,  "Move down",            wxArtProvider::GetBitmap("list-selection-down", wxART_TOOLBAR), "Move window down in the parent's list of children");
    ToolbarWindow->AddTool(ID_TOOLBAR_WINDOW_ROTATE_CCW, "Rotate anticlockwise", wxArtProvider::GetBitmap("transform-rotate-left", wxART_TOOLBAR), "Rotate anticlockwise");
    ToolbarWindow->AddTool(ID_TOOLBAR_WINDOW_ROTATE_CW,  "Rotate clockwise",     wxArtProvider::GetBitmap("transform-rotate-right", wxART_TOOLBAR), "Rotate clockwise");
    ToolbarWindow->Realize();

    wxAuiToolBar* ToolbarText=new wxAuiToolBar(this, wxID_ANY);
    ToolbarText->AddTool(ID_TOOLBAR_TEXT_ALIGN_LEFT,   "Align left",   wxArtProvider::GetBitmap("format-justify-left",   wxART_TOOLBAR), "Left align text");
    ToolbarText->AddTool(ID_TOOLBAR_TEXT_ALIGN_CENTER, "Align center", wxArtProvider::GetBitmap("format-justify-center", wxART_TOOLBAR), "Center text");
    ToolbarText->AddTool(ID_TOOLBAR_TEXT_ALIGN_RIGHT,  "Align right",  wxArtProvider::GetBitmap("format-justify-right",  wxART_TOOLBAR), "Right align text");
    ToolbarText->Realize();

    wxAuiToolBar* ToolbarZoom=new wxAuiToolBar(this, wxID_ANY);
    ToolbarZoom->AddTool(ID_TOOLBAR_ZOOM_IN, "Zoom in",        wxArtProvider::GetBitmap("zoom-in", wxART_TOOLBAR), "Zoom in");
    ToolbarZoom->AddTool(ID_TOOLBAR_ZOOM_OUT, "Zoom out",      wxArtProvider::GetBitmap("zoom-out", wxART_TOOLBAR), "Zoom out");
    ToolbarZoom->AddTool(ID_TOOLBAR_ZOOM_FIT, "Best fit",      wxArtProvider::GetBitmap("zoom-fit-best", wxART_TOOLBAR), "Best fit");
    ToolbarZoom->AddTool(ID_TOOLBAR_ZOOM_100, "Set zoom 100%", wxArtProvider::GetBitmap("zoom-original", wxART_TOOLBAR), "Set zoom to 100%");
    ToolbarZoom->AddSeparator();
    ToolbarZoom->AddTool(ID_TOOLBAR_DOC_PREVIEW, "Live preview", wxArtProvider::GetBitmap("x-office-presentation", wxART_TOOLBAR), "Live preview");
    ToolbarZoom->Realize();

    m_AUIManager.AddPane(ToolbarDocument, wxAuiPaneInfo().Name("ToolbarDocument").
                         Caption("Toolbar Document").ToolbarPane().Top().Row(0).Position(0).
                         LeftDockable(false).RightDockable(false));

    m_AUIManager.AddPane(m_ToolbarTools, wxAuiPaneInfo().Name("ToolbarTools").
                         Caption("Toolbar Tools").ToolbarPane().Top().Row(0).Position(1).
                         LeftDockable(false).RightDockable(false));

    m_AUIManager.AddPane(ToolbarWindow, wxAuiPaneInfo().Name("ToolbarWindow").
                         Caption("Toolbar Window").ToolbarPane().Top().Row(0).Position(2).
                         LeftDockable(false).RightDockable(false));

    m_AUIManager.AddPane(ToolbarText, wxAuiPaneInfo().Name("ToolbarText").
                         Caption("Toolbar Text").ToolbarPane().Top().Row(0).Position(3).
                         LeftDockable(false).RightDockable(false));

    m_AUIManager.AddPane(ToolbarZoom, wxAuiPaneInfo().Name("ToolbarZoom").
                         Caption("Toolbar Zoom").ToolbarPane().Top().Row(0).Position(4).
                         LeftDockable(false).RightDockable(false));

    // Set default perspective if not yet set.
    if (AUIDefaultPerspective.IsEmpty()) AUIDefaultPerspective=m_AUIManager.SavePerspective();

    // Load user perspective (calls m_AUIManager.Update() automatically).
    m_AUIManager.LoadPerspective(wxConfigBase::Get()->Read("GuiEditorUserLayout", m_AUIManager.SavePerspective()));

    // Register observers.
    m_GuiDocument->RegisterObserver(m_RenderWindow);
    m_GuiDocument->RegisterObserver(m_WindowHierarchy);
    m_GuiDocument->RegisterObserver(m_WindowInspector);
    m_GuiDocument->RegisterObserver(m_GuiInspector);

    if (!IsMaximized()) Maximize(true);     // Also have wxMAXIMIZE set as frame style.
    Show(true);

    // Initial update of the gui documents observers.
    m_RenderWindow->Refresh(false);
    m_WindowHierarchy->RefreshTree();
    m_WindowInspector->RefreshPropGrid();
    m_GuiInspector->RefreshPropGrid();
}


GuiEditor::ChildFrameT::~ChildFrameT()
{
    m_Parent->m_FileHistory.RemoveMenu(m_FileMenu);

    // Unregister us from the parents list of children.
    const int Index=m_Parent->m_GuiChildFrames.Find(this);
    m_Parent->m_GuiChildFrames.RemoveAt(Index);

    m_AUIManager.UnInit();

    delete m_GuiDocument;
}


float GuiEditor::ChildFrameT::SnapToGrid(float Value) const
{
    if (!m_SnapToGrid) return cf::math::round(Value);

    return cf::math::round(Value/m_GridSpacing)*m_GridSpacing;
}


Vector2fT GuiEditor::ChildFrameT::SnapToGrid(const Vector2fT& Position) const
{
    const float GridSpacing=m_SnapToGrid ? m_GridSpacing : 1.0f;
    Vector2fT   NewPosition;

    NewPosition.x=cf::math::round(Position.x/GridSpacing)*GridSpacing;
    NewPosition.y=cf::math::round(Position.y/GridSpacing)*GridSpacing;

    return NewPosition;
}


bool GuiEditor::ChildFrameT::SubmitCommand(CommandT* Command)
{
    if (m_History.SubmitCommand(Command))
    {
        if (Command->SuggestsSave()) SetTitle(m_FileName+"*");
        return true;
    }

    return false;
}


bool GuiEditor::ChildFrameT::Save(bool AskForFileName)
{
    wxString FileName=m_FileName;

    if (AskForFileName || FileName=="" || FileName=="New GUI" || !FileName.EndsWith("_init.cgui") ||
        !wxFileExists(FileName) || !wxFile::Access(FileName, wxFile::write))
    {
        static wxString  LastUsedDir=m_GameConfig->ModDir+"/GUIs";
        const wxFileName FN(m_FileName);

        wxFileDialog SaveFileDialog(NULL,                               // parent
                                    "Save Cafu GUI File",               // message
                                    (FN.IsOk() && wxDirExists(FN.GetPath())) ? FN.GetPath() : LastUsedDir, // default dir
                                    (FN.IsOk() && FN.GetExt() == "cgui") ? FN.GetFullName() : "", // default file
                                    "Cafu GUI Files (*.cgui)|*.cgui"    // wildcard
                                    "|All Files (*.*)|*.*",
                                    wxFD_SAVE | wxFD_OVERWRITE_PROMPT);

        if (SaveFileDialog.ShowModal()!=wxID_OK) return false;

        LastUsedDir=SaveFileDialog.GetDirectory();
        FileName=SaveFileDialog.GetPath();     // directory + filename
    }

    if (!FileName.EndsWith("_init.cgui"))
    {
        // Remove extension from filename.
        wxFileName Tmp=wxFileName(FileName);
                   Tmp.ClearExt();
        FileName=Tmp.GetFullPath();
        FileName=FileName+"_init.cgui";
    }

    // Backup the previous file before overwriting it.
    if (wxFileExists(FileName) && !wxCopyFile(FileName, FileName+"_bak"))
    {
        wxMessageBox(wxString("I was not able to backup file '")+FileName+"' to '"+FileName+"_bak"+"'.\n"
                     "Please make sure that there is enough disk space left and that the path still exists,\n"
                     "or use 'File -> Save As...' to save the current GUI elsewhere.", "File not saved!", wxOK | wxICON_ERROR);
        return false;
    }

    std::ofstream InitFile(FileName.fn_str());

    if (!InitFile.is_open())
    {
        wxMessageBox(wxString("CaWE was unable to open the file \"")+FileName+"\" for writing. Please verify that the file is writable and that the path exists.");
        return false;
    }

    // This sets the cursor to the busy cursor in its ctor, and back to the default cursor in the dtor.
    wxBusyCursor BusyCursor;

    if (!m_GuiDocument->SaveInit_cgui(InitFile)) return false;


    // Create _main.cgui file if not existent.
    wxString GuiMainScript;
    wxASSERT(FileName.EndsWith("_init.cgui"));
    FileName.EndsWith("_init.cgui", &GuiMainScript);
    GuiMainScript+="_main.cgui";

    if (!wxFileExists(GuiMainScript))
    {
        std::ofstream MainFile(GuiMainScript.fn_str());

        wxFileName RelFileName=FileName;
        RelFileName.MakeRelativeTo(".");    // Make it relative to the current working directory.

        if (MainFile.is_open())
        {
            MainFile << "local gui = ...\n";
            MainFile << "\n";
            MainFile << "assert(loadfile(\"" << RelFileName.GetFullPath(wxPATH_UNIX) << "\"))(gui)\n";
        }
        else wxMessageBox("Unable to create template file\n"+GuiMainScript);
    }

    // Mark the document as "not modified" only if the save was successful.
    m_LastSavedAtCommandNr=m_History.GetLastSaveSuggestedCommandID();
    m_FileName=FileName;
    SetTitle(m_FileName);

    m_Parent->m_FileHistory.AddFileToHistory(m_FileName);
    return true;
}


void GuiEditor::ChildFrameT::OnMenuFile(wxCommandEvent& CE)
{
    // The events for menu entries that are duplicated from the parents file menu are forwarded to the parent.
    // All other events are child frame specific, and handled here.
    switch (CE.GetId())
    {
        case ID_MENU_FILE_CLOSE:
        {
            // Close() generates a EVT_CLOSE event which is handled by our OnClose() handler.
            // See wx Window Deletion Overview for more details.
            Close();
            break;
        }

        case ID_MENU_FILE_SAVE:
        {
            Save();
            break;
        }

        case ID_MENU_FILE_SAVEAS:
        {
            Save(true);
            break;
        }
    }
}


void GuiEditor::ChildFrameT::OnMenuFileUpdate(wxUpdateUIEvent& UE)
{
    switch (UE.GetId())
    {
        case ID_MENU_FILE_SAVE:
            UE.Enable(m_History.GetLastSaveSuggestedCommandID()!=m_LastSavedAtCommandNr);
            break;
    }
}


void GuiEditor::ChildFrameT::OnMenuUndoRedo(wxCommandEvent& CE)
{
    // Step forward or backward in the command history.
    if (CE.GetId()==wxID_UNDO) m_History.Undo();
                          else m_History.Redo();

    SetTitle(m_FileName + (m_History.GetLastSaveSuggestedCommandID()==m_LastSavedAtCommandNr ? "" : "*"));
}


void GuiEditor::ChildFrameT::OnUpdateEditUndoRedo(wxUpdateUIEvent& UE)
{
    const CommandT* Cmd   =(UE.GetId()==wxID_UNDO) ? m_History.GetUndoCommand() : m_History.GetRedoCommand();
    wxString        Action=(UE.GetId()==wxID_UNDO) ? "Undo" : "Redo";
    wxString        Hotkey=(UE.GetId()==wxID_UNDO) ? "Ctrl+Z" : "Ctrl+Y";

    if (Cmd)
    {
        UE.SetText(Action+" "+Cmd->GetName()+"\t"+Hotkey);
        UE.Enable(true);
    }
    else
    {
        UE.SetText(wxString("Cannot ")+Action+"\t"+Hotkey);
        UE.Enable(false);
    }
}


void GuiEditor::ChildFrameT::OnMenuEditCut(wxCommandEvent& CE)
{
    OnMenuEditCopy(CE);
    OnMenuEditDelete(CE);
}


void GuiEditor::ChildFrameT::OnMenuEditCopy(wxCommandEvent& CE)
{
    m_Parent->m_GuiClipboard.Clear();

    const ArrayT< IntrusivePtrT<cf::GuiSys::WindowT> >& Selection=m_GuiDocument->GetSelection();

    for (unsigned long SelNr=0; SelNr<Selection.Size(); SelNr++)
        m_Parent->m_GuiClipboard.PushBack(new cf::GuiSys::WindowT(*Selection[SelNr], true));
}


void GuiEditor::ChildFrameT::OnMenuEditPaste(wxCommandEvent& CE)
{
    SubmitCommand(new CommandPasteT(m_GuiDocument, m_Parent->m_GuiClipboard,
        m_GuiDocument->GetSelection().Size()==1 ? m_GuiDocument->GetSelection()[0] : m_GuiDocument->GetRootWindow()));
}


void GuiEditor::ChildFrameT::OnMenuEditDelete(wxCommandEvent& CE)
{
    if (m_GuiDocument->GetSelection().Size()>0)
        SubmitCommand(new CommandDeleteT(m_GuiDocument, m_GuiDocument->GetSelection()));
}


void GuiEditor::ChildFrameT::OnMenuEditGrid(wxCommandEvent& CE)
{
    switch (CE.GetId())
    {
        case ID_MENU_EDIT_SNAP_TO_GRID:
            m_SnapToGrid=!m_SnapToGrid;
            break;

        case ID_MENU_EDIT_SET_GRID_SIZE:
            long NewGridSpacing=wxGetNumberFromUser("Enter a grid spacing between 1 and 100.", "", "Enter new grid spacing", m_GridSpacing, 1, 100);
            if (NewGridSpacing!=-1) m_GridSpacing=NewGridSpacing;
            break;
    }
}


void GuiEditor::ChildFrameT::OnMenuEditUpdate(wxUpdateUIEvent& UE)
{
    switch (UE.GetId())
    {
        case wxID_CUT:
        case wxID_COPY:
        case ID_MENU_EDIT_DELETE:
            UE.Enable(m_GuiDocument->GetSelection().Size()>0);
            break;

        case wxID_PASTE:
            UE.Enable(m_Parent->m_GuiClipboard.Size()>0);
            break;

        case ID_MENU_EDIT_SNAP_TO_GRID:
            UE.Check(m_SnapToGrid);
            UE.SetText(wxString::Format("Snap to grid (%lu)\tCtrl+G", m_GridSpacing));
            break;
    }
}


void GuiEditor::ChildFrameT::OnMenuCreate(wxCommandEvent& CE)
{
    if (CE.GetId() == ID_MENU_CREATE_WINDOW)
    {
        IntrusivePtrT<cf::GuiSys::WindowT> Parent = m_GuiDocument->GetSelection().Size() > 0 ? m_GuiDocument->GetSelection()[0] : m_GuiDocument->GetRootWindow();

        SubmitCommand(new CommandCreateT(m_GuiDocument, Parent));
        return;
    }

    const unsigned long           Nr = CE.GetId() - ID_MENU_CREATE_COMPONENT_FIRST;
    const cf::TypeSys::TypeInfoT* TI = cf::GuiSys::GetComponentTIM().FindTypeInfoByNr(Nr);

    if (!TI)
    {
        wxMessageBox("Could not find a TypeInfo for this type number.", "Add component");
        return;
    }

    if (m_GuiDocument->GetSelection().Size() != 1)
    {
        wxMessageBox("Please select exactly one window to add the component to.", "Add component");
        return;
    }

    IntrusivePtrT<cf::GuiSys::ComponentBaseT> Comp = static_cast<cf::GuiSys::ComponentBaseT*>(
        TI->CreateInstance(
            cf::TypeSys::CreateParamsT()));

    if (Comp.IsNull())
    {
        wxMessageBox("Could not instantiate the component.", "Add component");
        return;
    }

    SubmitCommand(new CommandAddComponentT(m_GuiDocument, m_GuiDocument->GetSelection()[0], Comp));
}


void GuiEditor::ChildFrameT::OnMenuView(wxCommandEvent& CE)
{
    switch (CE.GetId())
    {
        case ID_MENU_VIEW_WINDOW_HIERARCHY:
            m_AUIManager.GetPane(m_WindowHierarchy).Show(m_ViewMenu->IsChecked(ID_MENU_VIEW_WINDOW_HIERARCHY));
            m_AUIManager.Update();
            break;

        case ID_MENU_VIEW_WINDOWINSPECTOR:
            m_AUIManager.GetPane(m_WindowInspector).Show(m_ViewMenu->IsChecked(ID_MENU_VIEW_WINDOWINSPECTOR));
            m_AUIManager.Update();
            break;

        case ID_MENU_VIEW_GUIINSPECTOR:
            m_AUIManager.GetPane(m_GuiInspector).Show(m_ViewMenu->IsChecked(ID_MENU_VIEW_GUIINSPECTOR));
            m_AUIManager.Update();
            break;

        case ID_MENU_VIEW_RESTORE_DEFAULT_LAYOUT:
            m_AUIManager.LoadPerspective(AUIDefaultPerspective);
            break;

        case ID_MENU_VIEW_RESTORE_USER_LAYOUT:
            m_AUIManager.LoadPerspective(wxConfigBase::Get()->Read("GuiEditorUserLayout", m_AUIManager.SavePerspective()));
            break;

        case ID_MENU_VIEW_SAVE_USER_LAYOUT:
            wxConfigBase::Get()->Write("GuiEditorUserLayout", m_AUIManager.SavePerspective());
            break;

        default:
            break;
    }
}


void GuiEditor::ChildFrameT::OnMenuViewUpdate(wxUpdateUIEvent& UE)
{
    switch (UE.GetId())
    {
        case ID_MENU_VIEW_WINDOW_HIERARCHY:
            m_ViewMenu->Check(ID_MENU_VIEW_WINDOW_HIERARCHY, m_AUIManager.GetPane(m_WindowHierarchy).IsShown());
            break;

        case ID_MENU_VIEW_WINDOWINSPECTOR:
            m_ViewMenu->Check(ID_MENU_VIEW_WINDOWINSPECTOR, m_AUIManager.GetPane(m_WindowInspector).IsShown());
            break;

        case ID_MENU_VIEW_GUIINSPECTOR:
            m_ViewMenu->Check(ID_MENU_VIEW_GUIINSPECTOR, m_AUIManager.GetPane(m_GuiInspector).IsShown());
            break;

        default:
            break;
    }
}


void GuiEditor::ChildFrameT::OnClose(wxCloseEvent& CE)
{
    if (!CE.CanVeto())
    {
        Destroy();
        return;
    }

    if (m_LastSavedAtCommandNr==m_History.GetLastSaveSuggestedCommandID())
    {
        // Our document has not been modified since the last save - close this window.
        Destroy();
        return;
    }

    // This "Save Confirmation Alert" essentially follows the GNOME Human Interface Guidelines,
    // see http://developer.gnome.org/hig-book/ for details.
    wxMessageDialog Msg(NULL, "Save changes to GUI \"" + m_FileName + "\" before closing?", "CaWE GUI Editor", wxYES_NO | wxCANCEL);

    Msg.SetExtendedMessage("If you close without saving, your changes will be discarded.");
    Msg.SetYesNoLabels("Save", "Close without Saving");

    switch (Msg.ShowModal())
    {
        case wxID_YES:
            if (!Save())
            {
                // The document could not be saved - keep the window open.
                CE.Veto();
                return;
            }

            // The GUI was successfully saved - close the window.
            Destroy();
            return;

        case wxID_NO:
            Destroy();
            return;

        default:    // wxID_CANCEL
            CE.Veto();
            return;
    }
}


void GuiEditor::ChildFrameT::OnToolbar(wxCommandEvent& CE)
{
    switch (CE.GetId())
    {
        case ID_TOOLBAR_DOC_PREVIEW:
        {
            // Activating the live preview requires saving the document first, as this avoids
            // a lot of subtle problems with alternatively merging the main script and our "in memory" script.
            // For example, we make sure that we experience the exact same errors here than the engine would,
            // and we also get proper line numbers in Lua error messages!
            if (!Save()) break;

            wxString MainScriptFileName;
            m_FileName.EndsWith("_init.cgui", &MainScriptFileName);
            MainScriptFileName+="_main.cgui";

            try
            {
                cf::UniScriptStateT* ScriptState = new cf::UniScriptStateT();
                cf::GuiSys::GuiImplT::InitScriptState(*ScriptState);

                IntrusivePtrT<cf::GuiSys::GuiImplT> Gui = new cf::GuiSys::GuiImplT(*ScriptState, m_GameConfig->GetGuiResources());

                Gui->LoadScript(std::string(MainScriptFileName));

                // Make sure that the Gui is active for the live preview, so that clock tick events are properly propagated to all windows.
                Gui->Activate();

                LivePreviewT* Preview = new LivePreviewT(this, ScriptState, Gui, MainScriptFileName);
                Preview->Show();
            }
            catch (const cf::GuiSys::GuiImplT::InitErrorT& IE)
            {
                wxMessageBox(
                    "There was a problem initializing the live preview.\n\n"
                    "Loading script " + MainScriptFileName + " finished with an error:\n" + IE.what(),
                    "Live Preview", wxOK | wxICON_ERROR);
            }

            break;
        }

        case ID_TOOLBAR_TOOL_SELECTION:
        case ID_TOOLBAR_TOOL_NEW_WINDOW:
        {
            // Disable all tool buttons then reactivate button according to event ID and set tool.
            m_ToolbarTools->ToggleTool(ID_TOOLBAR_TOOL_SELECTION,  false);
            m_ToolbarTools->ToggleTool(ID_TOOLBAR_TOOL_NEW_WINDOW, false);

            m_ToolbarTools->ToggleTool(CE.GetId(), true);

            if (CE.GetId()==ID_TOOLBAR_TOOL_SELECTION)
                m_ToolManager.SetActiveTool(TOOL_SELECTION);
            else if (CE.GetId()==ID_TOOLBAR_TOOL_NEW_WINDOW)
                m_ToolManager.SetActiveTool(TOOL_NEW_WINDOW);

            break;
        }

        case ID_TOOLBAR_WINDOW_MOVE_UP:
        case ID_TOOLBAR_WINDOW_MOVE_DOWN:
        {
            const ArrayT< IntrusivePtrT<cf::GuiSys::WindowT> >& Sel=m_GuiDocument->GetSelection();

            if (Sel.Size()==0)
            {
                wxMessageBox("Please select a window first.");
                break;
            }

            if (Sel.Size()>1)
            {
                wxMessageBox("Sorry, only one window can be moved at a time.");
                break;
            }

            if (Sel[0]->GetParent().IsNull())
            {
                wxMessageBox("Sorry, the topmost (root) window cannot be moved.");
                break;
            }

            const int NewPos=Sel[0]->GetParent()->GetChildren().Find(Sel[0]) + (CE.GetId()==ID_TOOLBAR_WINDOW_MOVE_UP ? -1 : 1);

            if (NewPos<0)
            {
                wxMessageBox("This window is already the first child of its parent.\n\nUse cut and paste if you would like to assign the window to another parent.");
                break;
            }

            if (NewPos>=int(Sel[0]->GetParent()->GetChildren().Size()))
            {
                wxMessageBox("This window is already the last child of its parent.\n\nUse cut and paste if you would like to assign the window to another parent.");
                break;
            }

            SubmitCommand(new CommandChangeWindowHierarchyT(m_GuiDocument, Sel[0], Sel[0]->GetParent(), NewPos));
            break;
        }

        case ID_TOOLBAR_WINDOW_ROTATE_CW:
        case ID_TOOLBAR_WINDOW_ROTATE_CCW:
        {
            const ArrayT< IntrusivePtrT<cf::GuiSys::WindowT> >& Sel = m_GuiDocument->GetSelection();
            ArrayT<CommandT*> SubCommands;

            for (unsigned int SelNr = 0; SelNr < Sel.Size(); SelNr++)
            {
                cf::TypeSys::VarT<float>* Rotation = dynamic_cast<cf::TypeSys::VarT<float>*>(Sel[SelNr]->GetTransform()->GetMemberVars().Find("Rotation"));

                if (Rotation)
                {
                    const float NewRotAngle = Rotation->Get() + (CE.GetId() == ID_TOOLBAR_WINDOW_ROTATE_CW ? 15.0f : 345.0f);

                    SubCommands.PushBack(new CommandSetCompVarT<float>(m_GuiDocument->GetAdapter(), *Rotation, fmod(NewRotAngle, 360.0f)));
                }
            }

            if (SubCommands.Size() == 1)
            {
                SubmitCommand(SubCommands[0]);
            }
            else if (SubCommands.Size() > 1)
            {
                SubmitCommand(new CommandMacroT(SubCommands, "Rotate windows"));
            }

            break;
        }

        case ID_TOOLBAR_TEXT_ALIGN_LEFT:
        case ID_TOOLBAR_TEXT_ALIGN_CENTER:
        case ID_TOOLBAR_TEXT_ALIGN_RIGHT:
        {
            const ArrayT< IntrusivePtrT<cf::GuiSys::WindowT> >& Sel = m_GuiDocument->GetSelection();
            ArrayT<CommandT*> SubCommands;

            for (unsigned int SelNr = 0; SelNr < Sel.Size(); SelNr++)
            {
                IntrusivePtrT<cf::GuiSys::ComponentBaseT> CompText = Sel[SelNr]->GetComponent("Text");

                if (CompText != NULL)
                {
                    cf::GuiSys::ComponentTextT::VarTextAlignHorT* AlignHor = dynamic_cast<cf::GuiSys::ComponentTextT::VarTextAlignHorT*>(CompText->GetMemberVars().Find("horAlign"));

                    if (AlignHor)
                    {
                        const int HOR_ALIGN = (CE.GetId() == ID_TOOLBAR_TEXT_ALIGN_LEFT)   ? cf::GuiSys::ComponentTextT::VarTextAlignHorT::LEFT :
                                              (CE.GetId() == ID_TOOLBAR_TEXT_ALIGN_CENTER) ? cf::GuiSys::ComponentTextT::VarTextAlignHorT::CENTER :
                                                                                             cf::GuiSys::ComponentTextT::VarTextAlignHorT::RIGHT;

                        SubCommands.PushBack(new CommandSetCompVarT<int>(m_GuiDocument->GetAdapter(), *AlignHor, HOR_ALIGN));
                    }
                }
            }

            if (SubCommands.Size() == 1)
            {
                SubmitCommand(SubCommands[0]);
            }
            else if (SubCommands.Size() > 1)
            {
                SubmitCommand(new CommandMacroT(SubCommands, "Set hor. Align"));
            }

            break;
        }

        case ID_TOOLBAR_ZOOM_IN:
            m_RenderWindow->ZoomIn();
            break;

        case ID_TOOLBAR_ZOOM_OUT:
            m_RenderWindow->ZoomOut();
            break;

        case ID_TOOLBAR_ZOOM_FIT:
            m_RenderWindow->ZoomFit();
            break;

        case ID_TOOLBAR_ZOOM_100:
            m_RenderWindow->ZoomSet(1.0f);
            break;
    }
}
