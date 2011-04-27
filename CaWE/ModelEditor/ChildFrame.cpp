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

#include "ChildFrame.hpp"
#include "AnimInspector.hpp"
#include "ElementsList.hpp"
#include "GlobalsInspector.hpp"
#include "JointInspector.hpp"
#include "JointsHierarchy.hpp"
#include "MeshInspector.hpp"
#include "ModelDocument.hpp"
#include "SceneView3D.hpp"
#include "ScenePropGrid.hpp"

#include "../GameConfig.hpp"
#include "../ParentFrame.hpp"

#include "Models/Model_cmdl.hpp"

#include "wx/wx.h"
#include "wx/confbase.h"

#include <fstream>


namespace ModelEditor
{
    // Default perspective set by the first childframe instance and used to restore default settings later.
    static wxString AUIDefaultPerspective;
}


BEGIN_EVENT_TABLE(ModelEditor::ChildFrameT, wxMDIChildFrame)
    EVT_MENU_RANGE     (ID_MENU_FILE_CLOSE,                     ID_MENU_FILE_SAVEAS,                   ModelEditor::ChildFrameT::OnMenuFile)
    EVT_MENU_RANGE     (wxID_UNDO,                              wxID_REDO,                             ModelEditor::ChildFrameT::OnMenuUndoRedo)
    EVT_UPDATE_UI_RANGE(wxID_UNDO,                              wxID_REDO,                             ModelEditor::ChildFrameT::OnUpdateEditUndoRedo)
    EVT_MENU_RANGE     (ID_MENU_VIEW_AUIPANE_GLOBALS_INSPECTOR, ID_MENU_VIEW_LOAD_DEFAULT_PERSPECTIVE, ModelEditor::ChildFrameT::OnMenuView)
    EVT_UPDATE_UI_RANGE(ID_MENU_VIEW_AUIPANE_GLOBALS_INSPECTOR, ID_MENU_VIEW_LOAD_DEFAULT_PERSPECTIVE, ModelEditor::ChildFrameT::OnMenuViewUpdate)
    EVT_CLOSE          (ModelEditor::ChildFrameT::OnClose)
END_EVENT_TABLE()


ModelEditor::ChildFrameT::ChildFrameT(ParentFrameT* Parent, const wxString& FileName, ModelDocumentT* ModelDoc)
    : wxMDIChildFrame(Parent, wxID_ANY, FileName, wxDefaultPosition, wxSize(800, 600), wxDEFAULT_FRAME_STYLE | wxMAXIMIZE),
      m_FileName(FileName),     // Must use a fixed size in place of wxDefaultSize, see <http://trac.wxwidgets.org/ticket/12490> for details.
      m_ModelDoc(ModelDoc),
      m_History(),
      m_LastSavedAtCommandNr(0),
      m_Parent(Parent),
      m_SceneView3D(NULL),
      m_GlobalsInspector(NULL),
      m_JointsHierarchy(NULL),
      m_JointInspector(NULL),
      m_MeshesList(NULL),
      m_MeshInspector(NULL),
      m_AnimsList(NULL),
      m_AnimInspector(NULL),
      m_ScenePropGrid(NULL),
      m_FileMenu(NULL),
      m_EditMenu(NULL)
{
    // Register us with the parents list of children.
    m_Parent->m_MdlChildFrames.PushBack(this);

    // Set up menu.
    wxMenuBar *item0 = new wxMenuBar;


    wxMenu* NewMenu = new wxMenu;
    NewMenu->Append(ParentFrameT::ID_MENU_FILE_NEW_MAP,   wxT("New &Map\tCtrl+N"), wxT(""));
    NewMenu->Append(ParentFrameT::ID_MENU_FILE_NEW_MODEL, wxT("New M&odel\tCtrl+Shift+N"), wxT(""));
    NewMenu->Append(ParentFrameT::ID_MENU_FILE_NEW_GUI,   wxT("New &GUI\tCtrl+Alt+N"), wxT(""));
    NewMenu->Append(ParentFrameT::ID_MENU_FILE_NEW_FONT,  wxT("New &Font"), wxT(""));

    m_FileMenu=new wxMenu;
    m_FileMenu->AppendSubMenu(NewMenu, wxT("&New"));

    m_FileMenu->Append(ParentFrameT::ID_MENU_FILE_OPEN, wxT("&Open...\tCtrl+O"), wxT("") );
    m_FileMenu->Append(ID_MENU_FILE_CLOSE, wxT("&Close\tCtrl+F4"), wxT("") );
    m_FileMenu->Append(ID_MENU_FILE_SAVE, wxT("&Save\tCtrl+S"), wxT("") );
    m_FileMenu->Append(ID_MENU_FILE_SAVEAS, wxT("Save &As..."), wxT("") );
    m_FileMenu->AppendSeparator();
    m_FileMenu->Append(ParentFrameT::ID_MENU_FILE_CONFIGURE, wxT("&Configure CaWE..."), wxT("") );
    m_FileMenu->Append(ParentFrameT::ID_MENU_FILE_EXIT, wxT("E&xit"), wxT("") );
    m_Parent->m_FileHistory.UseMenu(m_FileMenu);
    m_Parent->m_FileHistory.AddFilesToMenu(m_FileMenu);
    item0->Append( m_FileMenu, wxT("&File") );

    m_EditMenu=new wxMenu;
    m_EditMenu->Append(wxID_UNDO, "&Undo\tCtrl+Z", "");
    m_EditMenu->Append(wxID_REDO, "&Redo\tCtrl+Y", "");
    item0->Append(m_EditMenu, "&Edit");

    wxMenu* ViewMenu=new wxMenu;
    ViewMenu->AppendCheckItem(ID_MENU_VIEW_AUIPANE_GLOBALS_INSPECTOR, "Globals Inspector",   "Show or hide the global model properties");
    ViewMenu->AppendCheckItem(ID_MENU_VIEW_AUIPANE_JOINTS_HIERARCHY,  "Joints Hierarchy",    "Show or hide the joints hierarchy (the skeleton)");
    ViewMenu->AppendCheckItem(ID_MENU_VIEW_AUIPANE_JOINT_INSPECTOR,   "Joint Inspector",     "Show or hide the joint inspector");
    ViewMenu->AppendCheckItem(ID_MENU_VIEW_AUIPANE_MESHES_LIST,       "Meshes List",         "Show or hide the meshes list");
    ViewMenu->AppendCheckItem(ID_MENU_VIEW_AUIPANE_MESH_INSPECTOR,    "Mesh Inspector",      "Show or hide the mesh inspector");
    ViewMenu->AppendCheckItem(ID_MENU_VIEW_AUIPANE_ANIMS_LIST,        "Animations List",     "Show or hide the animations list");
    ViewMenu->AppendCheckItem(ID_MENU_VIEW_AUIPANE_ANIM_INSPECTOR,    "Animation Inspector", "Show or hide the animation inspector");
    ViewMenu->AppendCheckItem(ID_MENU_VIEW_AUIPANE_SCENE_SETUP,       "Scene Setup",         "Show or hide the scene setup inspector");
    ViewMenu->AppendSeparator();
    ViewMenu->Append(ID_MENU_VIEW_LOAD_USER_PERSPECTIVE, "&Load user window layout", "Loads the user defined window layout");
    ViewMenu->Append(ID_MENU_VIEW_SAVE_USER_PERSPECTIVE, "&Save user window layout", "Saves the current window layout");
    ViewMenu->Append(ID_MENU_VIEW_LOAD_DEFAULT_PERSPECTIVE, "Load &default window layout", "Restores the default window layout");
    item0->Append(ViewMenu, "&View");

    wxMenu* HelpMenu=new wxMenu;
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

    // Create model editor panes.
    m_SceneView3D=new SceneView3DT(this);
    m_AUIManager.AddPane(m_SceneView3D, wxAuiPaneInfo().
                         Name("SceneView").Caption("Scene View").
                         CenterPane());

    m_GlobalsInspector=new GlobalsInspectorT(this, wxSize(230, 180));
    m_AUIManager.AddPane(m_GlobalsInspector, wxAuiPaneInfo().
                         Name("GlobalsInspector").Caption("Global Model Properties").
                         Left().Position(0));

    m_JointsHierarchy=new JointsHierarchyT(this, wxSize(230, 400));
    m_AUIManager.AddPane(m_JointsHierarchy, wxAuiPaneInfo().
                         Name("JointsHierarchy").Caption("Skeleton / Joints Hierarchy").
                         Left().Position(1));

    m_JointInspector=new JointInspectorT(this, wxSize(360, 200));
    m_AUIManager.AddPane(m_JointInspector, wxAuiPaneInfo().
                         Name("JointInspector").Caption("Joint Inspector").
                         Float().Hide());

    m_MeshesList=new ElementsListT(this, wxSize(230, 400), MESH);
    m_AUIManager.AddPane(m_MeshesList, wxAuiPaneInfo().
                         Name("MeshesList").Caption("Meshes List").
                         Left().Position(3));

    m_MeshInspector=new MeshInspectorT(this, wxSize(480, 180));
    m_AUIManager.AddPane(m_MeshInspector, wxAuiPaneInfo().
                         Name("MeshInspector").Caption("Mesh Inspector").
                         Float().Hide());

    m_AnimsList=new ElementsListT(this, wxSize(230, 400), ANIM);
    m_AUIManager.AddPane(m_AnimsList, wxAuiPaneInfo().
                         Name("AnimsList").Caption("Animations List").
                         Left().Position(5));

    m_AnimInspector=new AnimInspectorT(this, wxSize(240, 160));
    m_AUIManager.AddPane(m_AnimInspector, wxAuiPaneInfo().
                         Name("AnimInspector").Caption("Animation Inspector").
                         Float().Hide());

    m_ScenePropGrid=new ScenePropGridT(this, wxSize(230, 500));
    m_AUIManager.AddPane(m_ScenePropGrid, wxAuiPaneInfo().
                         Name("ScenePropGrid").Caption("Scene Setup").
                         Right());

    // Create AUI toolbars.
    // Note: Right now those toolbars don't look to well under Windows Vista because of the new windows toolbar style that is used
    // to render the toolbars but not the wxAUI handles. Insert this code to see the problem.
    wxToolBar* ToolbarDocument=new wxToolBar(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTB_FLAT | wxTB_NODIVIDER);
    ToolbarDocument->SetToolBitmapSize(wxSize(16,16));
    ToolbarDocument->AddTool(ParentFrameT::ID_MENU_FILE_NEW_MODEL, "New model", wxBitmap("CaWE/res/GuiEditor/page_white.png", wxBITMAP_TYPE_PNG), "New model");
    ToolbarDocument->AddTool(ID_MENU_FILE_SAVE,                    "Save", wxBitmap("CaWE/res/GuiEditor/disk.png", wxBITMAP_TYPE_PNG), "Save model");
    ToolbarDocument->Realize();

    m_AUIManager.AddPane(ToolbarDocument, wxAuiPaneInfo().Name("ToolbarDocument").
                         Caption("Toolbar Document").ToolbarPane().Top().Row(1).
                         LeftDockable(false).RightDockable(false));


    // Save the AUI default perspective if not yet set.
    if (AUIDefaultPerspective.IsEmpty()) AUIDefaultPerspective=m_AUIManager.SavePerspective();

    // Load user perspective (calls m_AUIManager.Update() automatically).
    m_AUIManager.LoadPerspective(wxConfigBase::Get()->Read("ModelEditor/AUI_UserLayout", m_AUIManager.SavePerspective()));

    if (!IsMaximized()) Maximize(true);     // Also have wxMAXIMIZE set as frame style.
    Show(true);

    // Initial update of the model documents observers.
    m_SceneView3D->Refresh(false);
    m_ScenePropGrid->RefreshPropGrid();
}


ModelEditor::ChildFrameT::~ChildFrameT()
{
    m_Parent->m_FileHistory.RemoveMenu(m_FileMenu);

    // Unregister us from the parents list of children.
    const int Index=m_Parent->m_MdlChildFrames.Find(this);
    m_Parent->m_MdlChildFrames.RemoveAt(Index);

    m_AUIManager.UnInit();

    delete m_ModelDoc;
    m_ModelDoc=NULL;
}


bool ModelEditor::ChildFrameT::SubmitCommand(CommandT* Command)
{
    if (m_History.SubmitCommand(Command))
    {
        if (Command->SuggestsSave()) SetTitle(m_FileName+"*");
        return true;
    }

    return false;
}


bool ModelEditor::ChildFrameT::Save(bool AskForFileName)
{
    wxString FileName=m_FileName;

    if (AskForFileName || FileName=="" || FileName=="New Model" || !FileName.EndsWith(".cmdl") ||
        !wxFileExists(FileName) || !wxFile::Access(FileName, wxFile::write))
    {
        static wxString  LastUsedDir=m_ModelDoc->GetGameConfig()->ModDir+"/Models";
        const wxFileName FN(m_FileName);

        wxFileDialog SaveFileDialog(NULL,                               // parent
                                    "Save Cafu Model File",             // message
                                    (FN.IsOk() && wxDirExists(FN.GetPath())) ? FN.GetPath() : LastUsedDir, // default dir
                                    "",                                 // default file
                                    "Cafu Model Files (*.cmdl)|*.cmdl"  // wildcard
                                    "|All Files (*.*)|*.*",
                                    wxFD_SAVE | wxFD_OVERWRITE_PROMPT);

        if (SaveFileDialog.ShowModal()!=wxID_OK) return false;

        LastUsedDir=SaveFileDialog.GetDirectory();
        FileName=SaveFileDialog.GetPath();     // directory + filename
    }

    if (!FileName.EndsWith(".cmdl"))
    {
        // Remove extension from filename.
        wxFileName Tmp=wxFileName(FileName);

        Tmp.ClearExt();
        FileName=Tmp.GetFullPath();
        FileName=FileName+".cmdl";
    }

    // Backup the previous file before overwriting it.
    if (wxFileExists(FileName) && !wxCopyFile(FileName, FileName+"_bak"))
    {
        wxMessageBox(wxString("Sorry, creating a backup of file '")+FileName+"' at '"+FileName+"_bak"+"' did not work out.\n"
                     "Please make sure that there is enough disk space left and that the path still exists,\n"
                     "or use 'File -> Save As...' to save the current model elsewhere.", "File not saved!", wxOK | wxICON_ERROR);
        return false;
    }

    std::ofstream ModelFile(FileName.fn_str());

    if (!ModelFile.is_open())
    {
        wxMessageBox(wxString("CaWE was unable to open the file \"")+FileName+"\" for writing. Please verify that the file is writable and that the path exists.");
        return false;
    }

    // This sets the cursor to the busy cursor in its ctor, and back to the default cursor in the dtor.
    wxBusyCursor BusyCursor;

    m_ModelDoc->GetModel()->Save(ModelFile);

    // if (Materials have been edited (or are "new"))
    // {
    //     SaveRelated_cmat_File();
    //     possibly with related bitmaps? (those with relative paths)
    // }

    // Mark the document as "not modified" only if the save was successful.
    m_LastSavedAtCommandNr=m_History.GetLastSaveSuggestedCommandID();
    m_FileName=FileName;
    SetTitle(m_FileName);

    m_Parent->m_FileHistory.AddFileToHistory(m_FileName);
    return true;
}


void ModelEditor::ChildFrameT::ShowRelatedInspector(wxWindow* List, bool DoShow)
{
    wxWindow* Insp=NULL;

         if (List==m_JointsHierarchy) Insp=m_JointInspector;
    else if (List==m_MeshesList)      Insp=m_MeshInspector;
    else if (List==m_AnimsList)       Insp=m_AnimInspector;

    if (Insp==NULL) return;

    wxAuiPaneInfo& PaneInfo=m_AUIManager.GetPane(Insp);

    if (!PaneInfo.IsOk()) return;

    if (PaneInfo.IsFloating() && DoShow)
    {
        // Hide first, then show again: this brings the pane into the foreground (regarding z order).
        PaneInfo.Show(false);
        m_AUIManager.Update();  // Required for this to be effective.
    }

    PaneInfo.Show(DoShow);

    if (DoShow && PaneInfo.IsFloating() && PaneInfo.floating_pos==wxDefaultPosition)
        PaneInfo.FloatingPosition(ClientToScreen(wxPoint(20, 20)));

    m_AUIManager.Update();
}


void ModelEditor::ChildFrameT::OnMenuFile(wxCommandEvent& CE)
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


void ModelEditor::ChildFrameT::OnMenuUndoRedo(wxCommandEvent& CE)
{
    // Step forward or backward in the command history.
    if (CE.GetId()==wxID_UNDO) m_History.Undo();
                          else m_History.Redo();

    SetTitle(m_FileName + (m_History.GetLastSaveSuggestedCommandID()==m_LastSavedAtCommandNr ? "" : "*"));
}


void ModelEditor::ChildFrameT::OnUpdateEditUndoRedo(wxUpdateUIEvent& UE)
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


void ModelEditor::ChildFrameT::PaneToggleShow(wxAuiPaneInfo& PaneInfo)
{
    if (!PaneInfo.IsOk()) return;

    const bool DoShow=!PaneInfo.IsShown();
    PaneInfo.Show(DoShow);

    if (DoShow && PaneInfo.IsFloating() && PaneInfo.floating_pos==wxDefaultPosition)
        PaneInfo.FloatingPosition(ClientToScreen(wxPoint(20, 20)));

    m_AUIManager.Update();
}


void ModelEditor::ChildFrameT::OnMenuView(wxCommandEvent& CE)
{
    switch (CE.GetId())
    {
        case ID_MENU_VIEW_AUIPANE_GLOBALS_INSPECTOR: PaneToggleShow(m_AUIManager.GetPane(m_GlobalsInspector)); break;
        case ID_MENU_VIEW_AUIPANE_JOINTS_HIERARCHY:  PaneToggleShow(m_AUIManager.GetPane(m_JointsHierarchy )); break;
        case ID_MENU_VIEW_AUIPANE_JOINT_INSPECTOR:   PaneToggleShow(m_AUIManager.GetPane(m_JointInspector  )); break;
        case ID_MENU_VIEW_AUIPANE_MESHES_LIST:       PaneToggleShow(m_AUIManager.GetPane(m_MeshesList      )); break;
        case ID_MENU_VIEW_AUIPANE_MESH_INSPECTOR:    PaneToggleShow(m_AUIManager.GetPane(m_MeshInspector   )); break;
        case ID_MENU_VIEW_AUIPANE_ANIMS_LIST:        PaneToggleShow(m_AUIManager.GetPane(m_AnimsList       )); break;
        case ID_MENU_VIEW_AUIPANE_ANIM_INSPECTOR:    PaneToggleShow(m_AUIManager.GetPane(m_AnimInspector   )); break;
        case ID_MENU_VIEW_AUIPANE_SCENE_SETUP:       PaneToggleShow(m_AUIManager.GetPane(m_ScenePropGrid   )); break;

        case ID_MENU_VIEW_LOAD_DEFAULT_PERSPECTIVE:
            m_AUIManager.LoadPerspective(AUIDefaultPerspective);
            break;

        case ID_MENU_VIEW_LOAD_USER_PERSPECTIVE:
            m_AUIManager.LoadPerspective(wxConfigBase::Get()->Read("ModelEditor/AUI_UserLayout", m_AUIManager.SavePerspective()));
            break;

        case ID_MENU_VIEW_SAVE_USER_PERSPECTIVE:
            wxConfigBase::Get()->Write("ModelEditor/AUI_UserLayout", m_AUIManager.SavePerspective());
            break;
    }
}


void ModelEditor::ChildFrameT::OnMenuViewUpdate(wxUpdateUIEvent& UE)
{
    switch (UE.GetId())
    {
        case ID_MENU_VIEW_AUIPANE_GLOBALS_INSPECTOR: UE.Check(m_AUIManager.GetPane(m_GlobalsInspector).IsShown()); break;
        case ID_MENU_VIEW_AUIPANE_JOINTS_HIERARCHY:  UE.Check(m_AUIManager.GetPane(m_JointsHierarchy ).IsShown()); break;
        case ID_MENU_VIEW_AUIPANE_JOINT_INSPECTOR:   UE.Check(m_AUIManager.GetPane(m_JointInspector  ).IsShown()); break;
        case ID_MENU_VIEW_AUIPANE_MESHES_LIST:       UE.Check(m_AUIManager.GetPane(m_MeshesList      ).IsShown()); break;
        case ID_MENU_VIEW_AUIPANE_MESH_INSPECTOR:    UE.Check(m_AUIManager.GetPane(m_MeshInspector   ).IsShown()); break;
        case ID_MENU_VIEW_AUIPANE_ANIMS_LIST:        UE.Check(m_AUIManager.GetPane(m_AnimsList       ).IsShown()); break;
        case ID_MENU_VIEW_AUIPANE_ANIM_INSPECTOR:    UE.Check(m_AUIManager.GetPane(m_AnimInspector   ).IsShown()); break;
        case ID_MENU_VIEW_AUIPANE_SCENE_SETUP:       UE.Check(m_AUIManager.GetPane(m_ScenePropGrid   ).IsShown()); break;
    }
}


void ModelEditor::ChildFrameT::OnClose(wxCloseEvent& CE)
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

    const int Answer=wxMessageBox("The model has been modified since it was last saved.\n"
        "Do you want to save it before closing?\n"
        "Note that when you select 'No', all changes since the last save will be LOST.",
        "Save GUI before closing?", wxYES_NO | wxCANCEL | wxICON_EXCLAMATION);

    switch (Answer)
    {
        case wxYES:
            if (!Save())
            {
                // The document could not be saved - keep the window open.
                CE.Veto();
                return;
            }

            // The GUI was successfully saved - close the window.
            Destroy();
            return;

        case wxNO:
            Destroy();
            return;

        default:    // Answer==wxCANCEL
            CE.Veto();
            return;
    }
}
