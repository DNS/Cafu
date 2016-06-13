/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "ChildFrame.hpp"
#include "AnimInspector.hpp"
#include "ChannelInspector.hpp"
#include "ElementsList.hpp"
#include "GuiFixtureInspector.hpp"
#include "JointInspector.hpp"
#include "JointsHierarchy.hpp"
#include "MeshInspector.hpp"
#include "ModelDocument.hpp"
#include "SceneView3D.hpp"
#include "ScenePropGrid.hpp"
#include "SubmodelsList.hpp"
#include "TransformDialog.hpp"
#include "Commands/Add.hpp"
#include "Commands/Delete.hpp"

#include "../GameConfig.hpp"
#include "../ParentFrame.hpp"

#include "MaterialSystem/Material.hpp"
#include "Models/Loader_md5.hpp"
#include "Models/Model_cmdl.hpp"

#include "wx/wx.h"
#include "wx/artprov.h"
#include "wx/aui/auibar.h"
#include "wx/confbase.h"
#include "wx/dir.h"

#include <fstream>


namespace ModelEditor
{
    // Default perspective set by the first childframe instance and used to restore default settings later.
    static wxString AUIDefaultPerspective;
}


BEGIN_EVENT_TABLE(ModelEditor::ChildFrameT, wxMDIChildFrame)
    EVT_MENU_RANGE     (ID_MENU_FILE_CLOSE,                    ID_MENU_FILE_SAVEAS,                   ModelEditor::ChildFrameT::OnMenuFile)
    EVT_UPDATE_UI_RANGE(ID_MENU_FILE_CLOSE,                    ID_MENU_FILE_SAVEAS,                   ModelEditor::ChildFrameT::OnMenuFileUpdate)
    EVT_MENU_RANGE     (wxID_UNDO,                             wxID_REDO,                             ModelEditor::ChildFrameT::OnMenuUndoRedo)
    EVT_UPDATE_UI_RANGE(wxID_UNDO,                             wxID_REDO,                             ModelEditor::ChildFrameT::OnMenuUndoRedoUpdate)
    EVT_MENU_RANGE     (wxID_CUT,                              wxID_DELETE,                           ModelEditor::ChildFrameT::OnMenuEdit)
    EVT_UPDATE_UI_RANGE(wxID_CUT,                              wxID_DELETE,                           ModelEditor::ChildFrameT::OnMenuEditUpdate)
    EVT_MENU_RANGE     (ID_MENU_VIEW_AUIPANE_JOINTS_HIERARCHY, ID_MENU_VIEW_LOAD_DEFAULT_PERSPECTIVE, ModelEditor::ChildFrameT::OnMenuView)
    EVT_UPDATE_UI_RANGE(ID_MENU_VIEW_AUIPANE_JOINTS_HIERARCHY, ID_MENU_VIEW_LOAD_DEFAULT_PERSPECTIVE, ModelEditor::ChildFrameT::OnMenuViewUpdate)
    EVT_MENU_RANGE     (ID_MENU_MODEL_ANIM_SKIP_BACKWARD,      ID_MENU_MODEL_UNLOAD_SUBMODELS,        ModelEditor::ChildFrameT::OnMenuModel)
    EVT_UPDATE_UI_RANGE(ID_MENU_MODEL_ANIM_SKIP_BACKWARD,      ID_MENU_MODEL_UNLOAD_SUBMODELS,        ModelEditor::ChildFrameT::OnMenuModelUpdate)
    EVT_CLOSE          (ModelEditor::ChildFrameT::OnClose)
END_EVENT_TABLE()


ModelEditor::ChildFrameT::ChildFrameT(ParentFrameT* Parent, const wxString& FileName, ModelDocumentT* ModelDoc)
    : wxMDIChildFrame(Parent, wxID_ANY, FileName, wxDefaultPosition, wxSize(800, 600), wxDEFAULT_FRAME_STYLE | wxMAXIMIZE),
      m_FileName(FileName),     // Must use a fixed size in place of wxDefaultSize, see <http://trac.wxwidgets.org/ticket/12490> for details.
      m_ModelDoc(ModelDoc),
      m_History(),
      m_LastSavedAtCommandNr(0),
      m_LastUsedType(JOINT),
      m_Parent(Parent),
      m_FileMenu(NULL),
      m_EditMenu(NULL),
      m_SceneView3D(NULL),
      m_JointsHierarchy(NULL),
      m_JointInspector(NULL),
      m_MeshesList(NULL),
      m_MeshInspector(NULL),
      m_SkinsList(NULL),
      m_SkinInspector(NULL),
      m_GuiFixturesList(NULL),
      m_GuiFixtureInspector(NULL),
      m_AnimsList(NULL),
      m_AnimInspector(NULL),
      m_ChannelsList(NULL),
      m_ChannelInspector(NULL),
      m_ScenePropGrid(NULL),
      m_SubmodelsPanel(NULL),
      m_DlodModelsList(NULL),
      m_TransformDialog(NULL)
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
    m_EditMenu->Append(wxID_DELETE, "&Delete\tShift+Del", "");
    item0->Append(m_EditMenu, "&Edit");

    wxMenu* ViewMenu=new wxMenu;
    ViewMenu->AppendCheckItem(ID_MENU_VIEW_AUIPANE_JOINTS_HIERARCHY,     "Joints Hierarchy",      "Show or hide the joints hierarchy (the skeleton)");
    ViewMenu->AppendCheckItem(ID_MENU_VIEW_AUIPANE_JOINT_INSPECTOR,      "Joint Inspector",       "Show or hide the joint inspector");
    ViewMenu->AppendCheckItem(ID_MENU_VIEW_AUIPANE_MESHES_LIST,          "Meshes List",           "Show or hide the meshes list");
    ViewMenu->AppendCheckItem(ID_MENU_VIEW_AUIPANE_MESH_INSPECTOR,       "Mesh Inspector",        "Show or hide the mesh inspector");
    ViewMenu->AppendCheckItem(ID_MENU_VIEW_AUIPANE_SKINS_LIST,           "Skins List",            "Show or hide the skins list");
    ViewMenu->AppendCheckItem(ID_MENU_VIEW_AUIPANE_SKIN_INSPECTOR,       "Skin Inspector",        "Show or hide the skin inspector");
    ViewMenu->AppendCheckItem(ID_MENU_VIEW_AUIPANE_GUIFIXTURES_LIST,     "GUI Fixtures List",     "Show or hide the GUI fixtures list");
    ViewMenu->AppendCheckItem(ID_MENU_VIEW_AUIPANE_GUIFIXTURE_INSPECTOR, "GUI Fixture Inspector", "Show or hide the GUI fixture inspector");
    ViewMenu->AppendCheckItem(ID_MENU_VIEW_AUIPANE_ANIMS_LIST,           "Animations List",       "Show or hide the animations list");
    ViewMenu->AppendCheckItem(ID_MENU_VIEW_AUIPANE_ANIM_INSPECTOR,       "Animation Inspector",   "Show or hide the animation inspector");
    ViewMenu->AppendCheckItem(ID_MENU_VIEW_AUIPANE_CHANNELS_LIST,        "Channels List",         "Show or hide the animation channels list");
    ViewMenu->AppendCheckItem(ID_MENU_VIEW_AUIPANE_CHANNEL_INSPECTOR,    "Channel Inspector",     "Show or hide the animation channel inspector");
    ViewMenu->AppendSeparator();
    ViewMenu->AppendCheckItem(ID_MENU_VIEW_AUIPANE_SCENE_SETUP,          "Scene Setup",           "Show or hide the scene setup inspector");
    ViewMenu->AppendCheckItem(ID_MENU_VIEW_AUIPANE_SUBMODELS_LIST,       "Submodels List",        "Show or hide the submodels list");
    ViewMenu->AppendCheckItem(ID_MENU_VIEW_AUIPANE_DLOD_MODELS_LIST,     "LoD Models",            "Show or hide the list of models for reduced levels of detail");
    ViewMenu->AppendCheckItem(ID_MENU_VIEW_AUIPANE_TRANSFORM_DIALOG,     "Transform Dialog",      "Show or hide the model transform dialog");
    ViewMenu->AppendSeparator();
    ViewMenu->Append(ID_MENU_VIEW_LOAD_USER_PERSPECTIVE, "&Load user window layout", "Loads the user defined window layout");
    ViewMenu->Append(ID_MENU_VIEW_SAVE_USER_PERSPECTIVE, "&Save user window layout", "Saves the current window layout");
    ViewMenu->Append(ID_MENU_VIEW_LOAD_DEFAULT_PERSPECTIVE, "Load &default window layout", "Restores the default window layout");
    item0->Append(ViewMenu, "&View");

    wxMenu* ModelMenu=new wxMenu;
    ModelMenu->AppendRadioItem(ID_MENU_MODEL_ANIM_PLAY,  "&Play anim"/*, "Loads the user defined window layout"*/);
    ModelMenu->AppendRadioItem(ID_MENU_MODEL_ANIM_PAUSE, "P&ause anim"/*, "Loads the user defined window layout"*/);
    ModelMenu->AppendSeparator();
    ModelMenu->Append(ID_MENU_MODEL_TRANSFORM, "&Transform...\tCtrl+T", "Transform the model");
    ModelMenu->Append(ID_MENU_MODEL_SKIN_ADD, "Add skin", "Adds a new skin to the model");
    ModelMenu->Append(ID_MENU_MODEL_GUIFIXTURE_ADD, "Add GUI fixture", "Adds a new GUI fixture to the model");
    ModelMenu->Append(ID_MENU_MODEL_ANIM_IMPORT, "Import animation sequences...", "Imports additional animation sequences into the model");
    ModelMenu->Append(ID_MENU_MODEL_CHANNEL_ADD, "Add channel", "Adds a new animation channel to the model");
    ModelMenu->Append(-1, "Run benchmark", "Move the camera along a predefined path and determine the time taken")->Enable(false);
    ModelMenu->AppendSeparator();
    ModelMenu->Append(ID_MENU_MODEL_LOAD_SUBMODEL, "&Load submodel...", "Loads a submodel (such as a weapon) to show with the main model");
    ModelMenu->Append(ID_MENU_MODEL_UNLOAD_SUBMODELS, "&Unload submodels", "Unloads all currently loaded submodels");
    item0->Append(ModelMenu, "&Model");

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

    m_JointsHierarchy=new JointsHierarchyT(this, wxSize(230, 400));
    m_AUIManager.AddPane(m_JointsHierarchy, wxAuiPaneInfo().
                         Name("JointsHierarchy").Caption("Skeleton / Joints Hierarchy").
                         Left().Position(1));

    m_JointInspector=new JointInspectorT(this, wxSize(360, 200));
    m_AUIManager.AddPane(m_JointInspector, wxAuiPaneInfo().
                         Name("JointInspector").Caption("Joint Inspector").
                         Float().Hide());

    m_MeshesList=new ElementsPanelT(this, wxSize(230, 400), MESH);
    m_AUIManager.AddPane(m_MeshesList, wxAuiPaneInfo().
                         Name("MeshesList").Caption("Meshes").
                         Left().Position(3));

    m_MeshInspector=new MeshInspectorT(this, wxSize(480, 180));
    m_AUIManager.AddPane(m_MeshInspector, wxAuiPaneInfo().
                         Name("MeshInspector").Caption("Mesh Inspector").
                         Float().Hide());

    m_SkinsList=new ElementsPanelT(this, wxSize(230, 150), SKIN);
    m_AUIManager.AddPane(m_SkinsList, wxAuiPaneInfo().
                         Name("SkinsList").Caption("Skins").
                         Left().Position(5));

    m_SkinInspector=new wxStaticText(this, wxID_ANY, "\nSkins are used to assign alternative materials to the meshes in the model.\n\nTo use a skin, select it in the Skins list, then use the Mesh Inspector to assign a material to the mesh in the selected skin.", wxDefaultPosition, wxSize(260, 180));
    m_AUIManager.AddPane(m_SkinInspector, wxAuiPaneInfo().
                         Name("SkinInspector").Caption("Skin Inspector").
                         Float().Hide());

    m_GuiFixturesList=new ElementsPanelT(this, wxSize(230, 150), GFIX);
    m_AUIManager.AddPane(m_GuiFixturesList, wxAuiPaneInfo().
                         Name("GuiFixturesList").Caption("Gui Fixtures").
                         Left().Position(7));

    m_GuiFixtureInspector=new GuiFixInspectorT(this, wxSize(260, 320));
    m_AUIManager.AddPane(m_GuiFixtureInspector, wxAuiPaneInfo().
                         Name("GuiFixtureInspector").Caption("GUI Fixture Inspector").
                         Float().Hide());

    m_AnimsList=new ElementsPanelT(this, wxSize(230, 400), ANIM);
    m_AUIManager.AddPane(m_AnimsList, wxAuiPaneInfo().
                         Name("AnimsList").Caption("Animations").
                         Right().Position(0));

    m_AnimInspector=new AnimInspectorT(this, wxSize(240, 160));
    m_AUIManager.AddPane(m_AnimInspector, wxAuiPaneInfo().
                         Name("AnimInspector").Caption("Animation Inspector").
                         Float().Hide());

    m_ChannelsList=new ElementsPanelT(this, wxSize(230, 150), CHAN);
    m_AUIManager.AddPane(m_ChannelsList, wxAuiPaneInfo().
                         Name("ChannelsList").Caption("Channels").
                         Right().Position(2));

    m_ChannelInspector=new ChannelInspectorT(this, wxSize(260, 320));
    m_AUIManager.AddPane(m_ChannelInspector, wxAuiPaneInfo().
                         Name("ChannelInspector").Caption("Channel Inspector").
                         Float().Hide());

    m_ScenePropGrid=new ScenePropGridT(this, wxSize(230, 500));
    m_AUIManager.AddPane(m_ScenePropGrid, wxAuiPaneInfo().
                         Name("ScenePropGrid").Caption("Scene Setup").
                         Right().Position(4));

    m_SubmodelsPanel=new SubmodelsPanelT(this, wxSize(230, 150));
    m_AUIManager.AddPane(m_SubmodelsPanel, wxAuiPaneInfo().
                         Name("SubmodelsPanel").Caption("Submodels").
                         Right().Position(5));

    m_DlodModelsList=new wxStaticText(this, wxID_ANY, GetLodModelsString(), wxDefaultPosition, wxSize(260, 180), wxST_ELLIPSIZE_START);
    m_AUIManager.AddPane(m_DlodModelsList, wxAuiPaneInfo().
                         Name("DlodModelsList").Caption("Level-of-Detail Models").
                         Right().Position(6));

    m_TransformDialog=new TransformDialogT(this, wxSize(248, 240));
    m_AUIManager.AddPane(m_TransformDialog, wxAuiPaneInfo().
                         Name("TransformDialog").Caption("Model Transform").
                         Float().Hide());

    // Create AUI toolbars.
    wxAuiToolBar* ToolbarDocument=new wxAuiToolBar(this, wxID_ANY);
    ToolbarDocument->AddTool(ParentFrameT::ID_MENU_FILE_NEW_MODEL, "New", wxArtProvider::GetBitmap(wxART_NEW, wxART_TOOLBAR), "Create a new file");
    ToolbarDocument->AddTool(ParentFrameT::ID_MENU_FILE_OPEN,      "Open", wxArtProvider::GetBitmap(wxART_FILE_OPEN, wxART_TOOLBAR), "Open an existing file");
    ToolbarDocument->AddTool(ID_MENU_FILE_SAVE,                    "Save", wxArtProvider::GetBitmap(wxART_FILE_SAVE, wxART_TOOLBAR), "Save the file");
    ToolbarDocument->AddTool(ID_MENU_FILE_SAVEAS,                  "Save as", wxArtProvider::GetBitmap(wxART_FILE_SAVE_AS, wxART_TOOLBAR), "Save the file under a different name");
    ToolbarDocument->AddSeparator();
    ToolbarDocument->AddTool(wxID_UNDO,   "Undo", wxArtProvider::GetBitmap(wxART_UNDO, wxART_TOOLBAR), "Undo the last action");
    ToolbarDocument->AddTool(wxID_REDO,   "Redo", wxArtProvider::GetBitmap(wxART_REDO, wxART_TOOLBAR), "Redo the previously undone action");
    ToolbarDocument->AddSeparator();
    ToolbarDocument->AddTool(wxID_CUT,    "Cut", wxArtProvider::GetBitmap(wxART_CUT, wxART_TOOLBAR), "Cut");
    ToolbarDocument->AddTool(wxID_COPY,   "Copy", wxArtProvider::GetBitmap(wxART_COPY, wxART_TOOLBAR), "Copy");
    ToolbarDocument->AddTool(wxID_PASTE,  "Paste", wxArtProvider::GetBitmap(wxART_PASTE, wxART_TOOLBAR), "Paste");
    ToolbarDocument->AddTool(wxID_DELETE, "Delete", wxArtProvider::GetBitmap(wxART_DELETE, wxART_TOOLBAR), "Delete");
    ToolbarDocument->Realize();

    m_AUIManager.AddPane(ToolbarDocument, wxAuiPaneInfo().Name("ToolbarDocument").
                         Caption("Toolbar Document").ToolbarPane().Top().Row(0).Position(0).
                         LeftDockable(false).RightDockable(false));

    wxAuiToolBar* AnimToolbar=new wxAuiToolBar(this, wxID_ANY);
    AnimToolbar->AddTool(ID_MENU_MODEL_ANIM_SKIP_BACKWARD, "Skip backward", wxArtProvider::GetBitmap("media-skip-backward", wxART_TOOLBAR), "Select previous animation sequence");
    AnimToolbar->AddTool(ID_MENU_MODEL_ANIM_PLAY,          "Play Anim",     wxArtProvider::GetBitmap("media-playback-start", wxART_TOOLBAR), "Playback the animation sequence", wxITEM_RADIO);
    AnimToolbar->AddTool(ID_MENU_MODEL_ANIM_PAUSE,         "Pause Anim",    wxArtProvider::GetBitmap("media-playback-pause", wxART_TOOLBAR), "Pause/stop the animation sequence", wxITEM_RADIO);
    AnimToolbar->AddTool(ID_MENU_MODEL_ANIM_SKIP_FORWARD,  "Skip forward",  wxArtProvider::GetBitmap("media-skip-forward", wxART_TOOLBAR), "Select next animation sequence");
    AnimToolbar->AddSeparator();
    AnimToolbar->AddTool(ID_MENU_MODEL_TRANSFORM,      "Transform",       wxArtProvider::GetBitmap("transform-rotate-right", wxART_TOOLBAR), "Transform model");
 // AnimToolbar->AddTool(ID_MENU_MODEL_SKIN_ADD,       "Add skin",        wxArtProvider::GetBitmap("window-new", wxART_TOOLBAR), "Add skin");
    AnimToolbar->AddTool(ID_MENU_MODEL_GUIFIXTURE_ADD, "Add GUI fixture", wxArtProvider::GetBitmap("window-new", wxART_TOOLBAR), "Add GUI fixture");
 // AnimToolbar->AddTool(ID_MENU_MODEL_ANIM_IMPORT,    "Import anim",     wxArtProvider::GetBitmap("window-new", wxART_TOOLBAR), "Import anim");
 // AnimToolbar->AddTool(ID_MENU_MODEL_CHANNEL_ADD,    "Add channel",     wxArtProvider::GetBitmap("window-new", wxART_TOOLBAR), "Add channel");
    AnimToolbar->Realize();

    m_AUIManager.AddPane(AnimToolbar, wxAuiPaneInfo().Name("AnimToolbar").
                         Caption("Anim Toolbar").ToolbarPane().Top().Row(0).Position(1).
                         LeftDockable(false).RightDockable(false));


    // Save the AUI default perspective if not yet set.
    if (AUIDefaultPerspective.IsEmpty()) AUIDefaultPerspective=m_AUIManager.SavePerspective();

    // Load user perspective (calls m_AUIManager.Update() automatically).
    m_AUIManager.LoadPerspective(wxConfigBase::Get()->Read("ModelEditor/AUI_UserLayout", m_AUIManager.SavePerspective()));

    if (!IsMaximized()) Maximize(true);     // Also have wxMAXIMIZE set as frame style.
    Show(true);
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


bool ModelEditor::ChildFrameT::SubmitNewSkin()
{
    CafuModelT::SkinT Skin;

    Skin.Name="New Skin";
    while (Skin.Materials.Size()       < m_ModelDoc->GetModel()->GetMeshes().Size()) Skin.Materials.PushBack(NULL);
    while (Skin.RenderMaterials.Size() < m_ModelDoc->GetModel()->GetMeshes().Size()) Skin.RenderMaterials.PushBack(NULL);

    return SubmitCommand(new CommandAddT(m_ModelDoc, Skin));
}


bool ModelEditor::ChildFrameT::SubmitNewGuiFixture()
{
    ArrayT<CafuModelT::GuiFixtureT> GuiFixtures;

    GuiFixtures.PushBackEmpty();
    GuiFixtures[0].Name="New GUI Fixture";

    return SubmitCommand(new CommandAddT(m_ModelDoc, GuiFixtures));
}


bool ModelEditor::ChildFrameT::SubmitImportAnims()
{
    wxFileDialog FileDialog(this,                           // The window parent.
                            "Import animation sequences",   // Message.
                            "",                             // The default directory.
                            "*.md5anim",                    // The default file name.
                            "All files (*.*)|*.*"           // The wildcard.
                            "|md5anim files|*.md5anim",
                            wxFD_OPEN | wxFD_FILE_MUST_EXIST | wxFD_MULTIPLE);

    if (FileDialog.ShowModal()!=wxID_OK)
        return false;

    wxArrayString     Paths;
    ArrayT<CommandT*> SubCommands;
    wxString          Errors;

    FileDialog.GetPaths(Paths);

    for (size_t PathNr=0; PathNr<Paths.GetCount(); PathNr++)
    {
        try
        {
            ImporterMd5AnimT          Importer(Paths[PathNr].ToStdString());
            ArrayT<CafuModelT::AnimT> Anims=Importer.Import(m_ModelDoc->GetModel()->GetJoints(), m_ModelDoc->GetModel()->GetMeshes());

            SubCommands.PushBack(new CommandAddT(m_ModelDoc, Anims));
        }
        catch (const ModelLoaderT::LoadErrorT& LE)
        {
            Errors += "\n" + Paths[PathNr] + "\n" + LE.what() + "\n";
        }
    }

    if (Errors!="")
    {
        wxMessageBox(Errors, "Couldn't import animation sequences");
    }

    if (SubCommands.Size()==1)
    {
        return SubmitCommand(SubCommands[0]);
    }
    else if (SubCommands.Size()>1)
    {
        return SubmitCommand(new CommandMacroT(SubCommands, "Import anim sequences"));
    }

    return false;
}


bool ModelEditor::ChildFrameT::SubmitNewChannel()
{
    CafuModelT::ChannelT Channel;

    Channel.Name="New Channel";
    return SubmitCommand(new CommandAddT(m_ModelDoc, Channel));
}


void ModelEditor::ChildFrameT::SaveMaterials(const wxString& OldBaseName, const wxString& BaseName)
{
    const wxString EditorCmatName=BaseName+"_editor.cmat";
    const wxString MainCmatName  =BaseName+".cmat";
    const wxString OldCmatName   =OldBaseName+".cmat";


    // Save the editor materials in a separate file.
    std::ofstream EditorCmatFile(EditorCmatName.fn_str());

    if (EditorCmatFile.is_open())
    {
        const std::map<std::string, MaterialT*>& Materials=m_ModelDoc->GetModel()->GetMaterialManager().GetAllMaterials();
        bool IsFirst=true;

        for (std::map<std::string, MaterialT*>::const_iterator It=Materials.begin(); It!=Materials.end(); It++)
        {
            const MaterialT* Mat=It->second;

            if (Mat->meta_EditorSave)
            {
                if (!IsFirst)
                    EditorCmatFile << "\n\n";

                Mat->Save(EditorCmatFile);
                IsFirst=false;
            }
        }
    }
    else wxMessageBox("Unable to create editor cmat file\n"+EditorCmatName);


    // If we happen to have a main cmat file in the old location (but not yet in the new),
    // carry the file from the old location into the new.
    if (wxFileExists(OldCmatName) && !wxFileExists(MainCmatName))
    {
        wxCopyFile(OldCmatName, MainCmatName, false /*overwrite?*/);
    }


    // Insert the dofile() include-statement into the main .cmat file.
    // The code works whether the file already exists or not.
    bool NeedsPatch=true;

    try
    {
        TextParserT TextParser(MainCmatName.c_str(), "({[]}),");

        while (!TextParser.IsAtEOF())
        {
            const std::string Token=TextParser.GetNextToken();

            if (Token=="dofile")
            {
                TextParser.AssertAndSkipToken("(");
                if (TextParser.GetNextToken().find("_editor.cmat")!=std::string::npos)
                {
                    NeedsPatch=false;
                    break;
                }
                TextParser.AssertAndSkipToken(")");
            }
        }
    }
    catch (const TextParserT::ParseError&)
    {
        NeedsPatch=false;
    }

    if (NeedsPatch)
    {
        std::ofstream MainCmatFile(MainCmatName.fn_str(), std::ios_base::out | std::ios_base::app);

        if (MainCmatFile.is_open())
        {
            wxFileName RelName=EditorCmatName;
            RelName.MakeRelativeTo(wxFileName(MainCmatName).GetPath());     // Make it relative to the main .cmat file.

            MainCmatFile << "\n";
            MainCmatFile << "dofile(\"" << RelName.GetFullPath(wxPATH_UNIX) << "\")\n";     // Put no semicolon at the end of the dofile() statement: cmat scripts are not (yet) in the Lua programming language.
        }
        else wxMessageBox("Unable to insert dofile() statement into main cmat file\n"+MainCmatName);
    }
}


bool ModelEditor::ChildFrameT::Save(bool AskForFileName)
{
    wxString FileName=m_FileName;

    if (AskForFileName || FileName=="" || FileName=="New Model" || !FileName.EndsWith(".cmdl") ||
        !wxFileExists(FileName) || !wxFile::Access(FileName, wxFile::write))
    {
        static wxString  LastUsedDir=m_ModelDoc->GetGameConfig()->ModDir+"/Models";
        const wxFileName FN(m_FileName);

        wxFileDialog SaveFileDialog(this,                               // parent
                                    "Save Cafu Model File",             // message
                                    (FN.IsOk() && wxDirExists(FN.GetPath())) ? FN.GetPath() : LastUsedDir, // default dir
                                    FN.IsOk() ? FN.GetName() : wxString("NewModel"),                       // default file
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

    // Save the model file.
    {
        wxBusyCursor BusyCursor;    // This sets the cursor to the busy cursor in its ctor, and back to the default cursor in the dtor.

        m_ModelDoc->GetModel()->Save(ModelFile);
    }


    // Save the materials that are not hand-crafted.
    {
        wxBusyCursor BusyCursor;    // This sets the cursor to the busy cursor in its ctor, and back to the default cursor in the dtor.
        wxString BaseName;

        wxASSERT(FileName.EndsWith(".cmdl"));
        FileName.EndsWith(".cmdl", &BaseName);

        wxFileName OldBase(m_FileName);
        OldBase.ClearExt();

        SaveMaterials(OldBase.GetFullPath(), BaseName);
    }


    // Attempt to save (copy) the related bitmaps as well, if
    //   - we have the old/previous file (acting as the "source"),
    //   - the new file is in a directory that is *different* from the old file.
    wxFileName OldFileName(m_FileName); OldFileName.Normalize();
    wxFileName NewFileName(FileName  ); NewFileName.Normalize();

    if (OldFileName.FileExists() && wxFileName::DirName(OldFileName.GetPath())!=wxFileName::DirName(NewFileName.GetPath()))
    {
        wxArrayString AllFiles;

        wxDir::GetAllFiles(OldFileName.GetPath(), &AllFiles, "", wxDIR_FILES | wxDIR_DIRS /*but not wxDIR_HIDDEN*/);

        // Remove all files that are no texture images from the list.
        for (size_t FileNr=0; FileNr<AllFiles.GetCount(); FileNr++)
        {
            const wxString& fn=AllFiles[FileNr];

            if (fn.EndsWith(".bmp" )) continue;
            if (fn.EndsWith(".png" )) continue;
            if (fn.EndsWith(".jpg" )) continue;
            if (fn.EndsWith(".jpeg")) continue;
            if (fn.EndsWith(".tga" )) continue;

            AllFiles.RemoveAt(FileNr);
            FileNr--;
        }

        // Make all file names relative (to OldFileName.GetPath()).
        for (size_t FileNr=0; FileNr<AllFiles.GetCount(); FileNr++)
        {
            wxFileName tmp(AllFiles[FileNr]);
            tmp.MakeRelativeTo(OldFileName.GetPath());

            AllFiles[FileNr]=tmp.GetFullPath();
        }

        // Try to guess which files the user might want to copy.
        wxArrayInt SelIndices;

        for (size_t FileNr=0; FileNr<AllFiles.GetCount(); FileNr++)
        {
            if (AllFiles[FileNr].Lower().Find(OldFileName.GetName().Lower())!=wxNOT_FOUND)
                SelIndices.Add(FileNr);
        }

        if (AllFiles.GetCount()>0)
        {
            const int Result=wxGetSelectedChoices(SelIndices,
                "The model file was successfully saved into the new location.\n"
                "Please select the related texture images, if any, that you would\n"
                "like to copy into the new location along with the model file.",
                "Copy related texture images into new location?", AllFiles, this);

            for (int i=0; i<Result; i++)
            {
                const wxFileName from(OldFileName.GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR) + AllFiles[SelIndices[i]]);
                const wxFileName to  (NewFileName.GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR) + AllFiles[SelIndices[i]]);

                if (!wxFileName::Mkdir(to.GetPath(), wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL))
                {
                    wxMessageBox("Could not create directory " + to.GetPath());
                    continue;
                }

                if (!wxCopyFile(from.GetFullPath(), to.GetFullPath()))
                {
                    wxMessageBox("Could not copy file\n" + from.GetFullPath() + " to\n" + to.GetFullPath());
                    continue;
                }
            }
        }
    }


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
    else if (List==m_SkinsList)       Insp=m_SkinInspector;
    else if (List==m_AnimsList)       Insp=m_AnimInspector;
    else if (List==m_ChannelsList)    Insp=m_ChannelInspector;
    else if (List==m_GuiFixturesList) Insp=m_GuiFixtureInspector;

    if (Insp==NULL) return;

    wxAuiPaneInfo& PaneInfo=m_AUIManager.GetPane(Insp);

    if (!PaneInfo.IsOk()) return;

    PaneInfo.Show(DoShow);

    if (DoShow && PaneInfo.IsFloating() && PaneInfo.floating_pos==wxDefaultPosition)
        PaneInfo.FloatingPosition(ClientToScreen(wxPoint(20, 20)));

    if (DoShow && PaneInfo.IsFloating() && PaneInfo.frame)
        PaneInfo.frame->Raise();

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


void ModelEditor::ChildFrameT::OnMenuFileUpdate(wxUpdateUIEvent& UE)
{
    switch (UE.GetId())
    {
        case ID_MENU_FILE_SAVE:
            UE.Enable(m_History.GetLastSaveSuggestedCommandID()!=m_LastSavedAtCommandNr);
            break;
    }
}


void ModelEditor::ChildFrameT::OnMenuUndoRedo(wxCommandEvent& CE)
{
    // Step forward or backward in the command history.
    if (CE.GetId()==wxID_UNDO) m_History.Undo();
                          else m_History.Redo();

    SetTitle(m_FileName + (m_History.GetLastSaveSuggestedCommandID()==m_LastSavedAtCommandNr ? "" : "*"));
}


void ModelEditor::ChildFrameT::OnMenuUndoRedoUpdate(wxUpdateUIEvent& UE)
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


void ModelEditor::ChildFrameT::OnMenuEdit(wxCommandEvent& CE)
{
    //if (CE.GetId()==wxID_CUT || CE.GetId()==wxID_COPY)
    //    ;

    if (CE.GetId()==wxID_CUT || CE.GetId()==wxID_DELETE)
    {
        CommandDeleteT* DelCmd=new CommandDeleteT(m_ModelDoc, m_LastUsedType, m_ModelDoc->GetSelection(m_LastUsedType));
        bool            Result=DelCmd->Do();

        if (DelCmd->GetMessage()!="") wxMessageBox(DelCmd->GetMessage(), "Delete");
        if (Result) SubmitCommand(DelCmd); else delete DelCmd;
    }
}


void ModelEditor::ChildFrameT::OnMenuEditUpdate(wxUpdateUIEvent& UE)
{
    const bool HaveSel=m_ModelDoc->GetSelection(m_LastUsedType).Size()>0;

    switch (UE.GetId())
    {
        case wxID_CUT:    UE.Enable(false); break;
        case wxID_COPY:   UE.Enable(false); break;
        case wxID_PASTE:  UE.Enable(false); break;
        case wxID_DELETE: UE.Enable(HaveSel && m_LastUsedType!=JOINT); break;
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


wxString ModelEditor::ChildFrameT::GetLodModelsString() const
{
    wxString s;

    for (const CafuModelT* Model=m_ModelDoc->GetModel(); Model; Model=Model->GetDlodModel())
    {
        wxFileName fn(Model->GetFileName());

        fn.MakeRelativeTo(m_ModelDoc->GetGameConfig()->ModDir);
        s+=fn.GetFullPath(wxPATH_UNIX);

        if (Model->GetDlodModel())
            s+=wxString::Format(" (%.2f)\n", Model->GetDlodDist());
        else
            s+=" (inf)";
    }

    return s;
}


void ModelEditor::ChildFrameT::OnMenuView(wxCommandEvent& CE)
{
    switch (CE.GetId())
    {
        case ID_MENU_VIEW_AUIPANE_JOINTS_HIERARCHY:     PaneToggleShow(m_AUIManager.GetPane(m_JointsHierarchy    )); break;
        case ID_MENU_VIEW_AUIPANE_JOINT_INSPECTOR:      PaneToggleShow(m_AUIManager.GetPane(m_JointInspector     )); break;
        case ID_MENU_VIEW_AUIPANE_MESHES_LIST:          PaneToggleShow(m_AUIManager.GetPane(m_MeshesList         )); break;
        case ID_MENU_VIEW_AUIPANE_MESH_INSPECTOR:       PaneToggleShow(m_AUIManager.GetPane(m_MeshInspector      )); break;
        case ID_MENU_VIEW_AUIPANE_ANIMS_LIST:           PaneToggleShow(m_AUIManager.GetPane(m_AnimsList          )); break;
        case ID_MENU_VIEW_AUIPANE_ANIM_INSPECTOR:       PaneToggleShow(m_AUIManager.GetPane(m_AnimInspector      )); break;
        case ID_MENU_VIEW_AUIPANE_CHANNELS_LIST:        PaneToggleShow(m_AUIManager.GetPane(m_ChannelsList       )); break;
        case ID_MENU_VIEW_AUIPANE_CHANNEL_INSPECTOR:    PaneToggleShow(m_AUIManager.GetPane(m_ChannelInspector   )); break;
        case ID_MENU_VIEW_AUIPANE_SKINS_LIST:           PaneToggleShow(m_AUIManager.GetPane(m_SkinsList          )); break;
        case ID_MENU_VIEW_AUIPANE_SKIN_INSPECTOR:       PaneToggleShow(m_AUIManager.GetPane(m_SkinInspector      )); break;
        case ID_MENU_VIEW_AUIPANE_GUIFIXTURES_LIST:     PaneToggleShow(m_AUIManager.GetPane(m_GuiFixturesList    )); break;
        case ID_MENU_VIEW_AUIPANE_GUIFIXTURE_INSPECTOR: PaneToggleShow(m_AUIManager.GetPane(m_GuiFixtureInspector)); break;
        case ID_MENU_VIEW_AUIPANE_SCENE_SETUP:          PaneToggleShow(m_AUIManager.GetPane(m_ScenePropGrid      )); break;
        case ID_MENU_VIEW_AUIPANE_SUBMODELS_LIST:       PaneToggleShow(m_AUIManager.GetPane(m_SubmodelsPanel     )); break;
        case ID_MENU_VIEW_AUIPANE_DLOD_MODELS_LIST:     PaneToggleShow(m_AUIManager.GetPane(m_DlodModelsList     )); break;
        case ID_MENU_VIEW_AUIPANE_TRANSFORM_DIALOG:     PaneToggleShow(m_AUIManager.GetPane(m_TransformDialog    )); break;

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
        case ID_MENU_VIEW_AUIPANE_JOINTS_HIERARCHY:     UE.Check(m_AUIManager.GetPane(m_JointsHierarchy    ).IsShown()); break;
        case ID_MENU_VIEW_AUIPANE_JOINT_INSPECTOR:      UE.Check(m_AUIManager.GetPane(m_JointInspector     ).IsShown()); break;
        case ID_MENU_VIEW_AUIPANE_MESHES_LIST:          UE.Check(m_AUIManager.GetPane(m_MeshesList         ).IsShown()); break;
        case ID_MENU_VIEW_AUIPANE_MESH_INSPECTOR:       UE.Check(m_AUIManager.GetPane(m_MeshInspector      ).IsShown()); break;
        case ID_MENU_VIEW_AUIPANE_ANIMS_LIST:           UE.Check(m_AUIManager.GetPane(m_AnimsList          ).IsShown()); break;
        case ID_MENU_VIEW_AUIPANE_ANIM_INSPECTOR:       UE.Check(m_AUIManager.GetPane(m_AnimInspector      ).IsShown()); break;
        case ID_MENU_VIEW_AUIPANE_CHANNELS_LIST:        UE.Check(m_AUIManager.GetPane(m_ChannelsList       ).IsShown()); break;
        case ID_MENU_VIEW_AUIPANE_CHANNEL_INSPECTOR:    UE.Check(m_AUIManager.GetPane(m_ChannelInspector   ).IsShown()); break;
        case ID_MENU_VIEW_AUIPANE_SKINS_LIST:           UE.Check(m_AUIManager.GetPane(m_SkinsList          ).IsShown()); break;
        case ID_MENU_VIEW_AUIPANE_SKIN_INSPECTOR:       UE.Check(m_AUIManager.GetPane(m_SkinInspector      ).IsShown()); break;
        case ID_MENU_VIEW_AUIPANE_GUIFIXTURES_LIST:     UE.Check(m_AUIManager.GetPane(m_GuiFixturesList    ).IsShown()); break;
        case ID_MENU_VIEW_AUIPANE_GUIFIXTURE_INSPECTOR: UE.Check(m_AUIManager.GetPane(m_GuiFixtureInspector).IsShown()); break;
        case ID_MENU_VIEW_AUIPANE_SCENE_SETUP:          UE.Check(m_AUIManager.GetPane(m_ScenePropGrid      ).IsShown()); break;
        case ID_MENU_VIEW_AUIPANE_SUBMODELS_LIST:       UE.Check(m_AUIManager.GetPane(m_SubmodelsPanel     ).IsShown()); break;
        case ID_MENU_VIEW_AUIPANE_DLOD_MODELS_LIST:     UE.Check(m_AUIManager.GetPane(m_DlodModelsList     ).IsShown()); break;
        case ID_MENU_VIEW_AUIPANE_TRANSFORM_DIALOG:     UE.Check(m_AUIManager.GetPane(m_TransformDialog    ).IsShown()); break;
    }
}


void ModelEditor::ChildFrameT::OnMenuModel(wxCommandEvent& CE)
{
    switch (CE.GetId())
    {
        case ID_MENU_MODEL_ANIM_SKIP_FORWARD:
        {
            const ArrayT<unsigned int> OldSel=m_ModelDoc->GetSelection(ANIM);
            m_ModelDoc->SetSelection(ANIM, m_ModelDoc->GetSelection_NextAnimSequ());
            m_ModelDoc->UpdateAllObservers_SelectionChanged(ANIM, OldSel, m_ModelDoc->GetSelection(ANIM));

            // This is not needed: setting a new ANIM selection also sets the frame number to 0.
            // m_ModelDoc->GetAnimState().LastStdAE->SetFrameNr(0.0f);
            // m_ModelDoc->UpdateAllObservers_AnimStateChanged();
            break;
        }

        case ID_MENU_MODEL_ANIM_SKIP_BACKWARD:
        {
            const ArrayT<unsigned int> OldSel=m_ModelDoc->GetSelection(ANIM);
            m_ModelDoc->SetSelection(ANIM, m_ModelDoc->GetSelection_PrevAnimSequ());
            m_ModelDoc->UpdateAllObservers_SelectionChanged(ANIM, OldSel, m_ModelDoc->GetSelection(ANIM));

            // This is not needed: setting a new ANIM selection also sets the frame number to 0.
            // m_ModelDoc->GetAnimState().LastStdAE->SetFrameNr(0.0f);
            // m_ModelDoc->UpdateAllObservers_AnimStateChanged();
            break;
        }

        case ID_MENU_MODEL_ANIM_PLAY:
        {
            m_ModelDoc->SetAnimSpeed(1.0f);
            m_ModelDoc->UpdateAllObservers_AnimStateChanged();
            break;
        }

        case ID_MENU_MODEL_ANIM_PAUSE:
        {
            m_ModelDoc->SetAnimSpeed(0.0f);
            m_ModelDoc->UpdateAllObservers_AnimStateChanged();
            break;
        }

        case ID_MENU_MODEL_TRANSFORM:
        {
            if (!m_AUIManager.GetPane(m_TransformDialog).IsShown())
                PaneToggleShow(m_AUIManager.GetPane(m_TransformDialog));
            break;
        }

        case ID_MENU_MODEL_SKIN_ADD:
        {
            SubmitNewSkin();
            break;
        }

        case ID_MENU_MODEL_GUIFIXTURE_ADD:
        {
            SubmitNewGuiFixture();
            break;
        }

        case ID_MENU_MODEL_ANIM_IMPORT:
        {
            SubmitImportAnims();
            break;
        }

        case ID_MENU_MODEL_CHANNEL_ADD:
        {
            SubmitNewChannel();
            break;
        }

        case ID_MENU_MODEL_LOAD_SUBMODEL:
        {
            wxFileDialog FileDialog(this, "Load submodel", "", "", "Model files (*.*)|*.*", wxFD_OPEN | wxFD_FILE_MUST_EXIST);

            if (FileDialog.ShowModal()==wxID_OK)
            {
                m_ModelDoc->LoadSubmodel(FileDialog.GetPath());
                m_ModelDoc->UpdateAllObservers_SubmodelsChanged();
            }
            break;
        }

        case ID_MENU_MODEL_UNLOAD_SUBMODELS:
        {
            while (m_ModelDoc->GetSubmodels().Size()>0)
                m_ModelDoc->UnloadSubmodel(0);

            m_ModelDoc->UpdateAllObservers_SubmodelsChanged();
            break;
        }
    }
}


void ModelEditor::ChildFrameT::OnMenuModelUpdate(wxUpdateUIEvent& UE)
{
    // Alternatively, ChildFrameT should derive from ObserverT and implement its Notify_AnimStateChanged() method.
    switch (UE.GetId())
    {
        case ID_MENU_MODEL_ANIM_PLAY:        UE.Check(m_ModelDoc->GetAnimState().Speed!=0.0f); break;
        case ID_MENU_MODEL_ANIM_PAUSE:       UE.Check(m_ModelDoc->GetAnimState().Speed==0.0f); break;
        case ID_MENU_MODEL_UNLOAD_SUBMODELS: UE.Enable(m_ModelDoc->GetSubmodels().Size()>0); break;
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

    // This "Save Confirmation Alert" essentially follows the GNOME Human Interface Guidelines,
    // see http://developer.gnome.org/hig-book/ for details.
    wxMessageDialog Msg(NULL, "Save changes to model \"" + m_FileName + "\" before closing?", "CaWE Model Editor", wxYES_NO | wxCANCEL);

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

            // The model was successfully saved - close the window.
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
