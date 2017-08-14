/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "ChildFrame.hpp"
#include "ChildFrameViewWin2D.hpp"
#include "ChildFrameViewWin3D.hpp"
#include "CompMapEntity.hpp"
#include "DialogConsole.hpp"
#include "DialogCustomCompile.hpp"
#include "DialogEditSurfaceProps.hpp"
#include "DialogEntityHierarchy.hpp"
#include "DialogInspector.hpp"
#include "DialogPasteSpecial.hpp"
#include "DialogTerrainEdit.hpp"
#include "Group.hpp"
#include "MapDocument.hpp"
#include "MapEntRepres.hpp"
#include "Tool.hpp"
#include "ToolbarMaterials.hpp"
#include "ToolbarGroups.hpp"
#include "ToolCamera.hpp"
#include "ToolEditSurface.hpp"
#include "ToolTerrainEdit.hpp"
#include "ToolManager.hpp"

#include "../Camera.hpp"
#include "../GameConfig.hpp"
#include "../Options.hpp"
#include "../ParentFrame.hpp"

#include "Commands/AddComponent.hpp"
#include "Commands/AddPrim.hpp"
#include "Commands/Delete.hpp"
#include "Commands/Group_Assign.hpp"
#include "Commands/Group_Delete.hpp"
#include "Commands/Group_New.hpp"
#include "Commands/NewEntity.hpp"
#include "Commands/Select.hpp"
#include "Commands/Transform.hpp"

#include "GameSys/AllComponents.hpp"
#include "GameSys/Entity.hpp"
#include "GameSys/EntityCreateParams.hpp"
#include "GameSys/World.hpp"

#include "TextParser/TextParser.hpp"

#include "wx/wx.h"
#include "wx/artprov.h"
#include "wx/aui/auibar.h"
#include "wx/clipbrd.h"
#include "wx/confbase.h"
#include "wx/dir.h"
#include "wx/filename.h"
#include "wx/process.h"
#include "wx/stdpaths.h"


using namespace MapEditor;


// TODO:
// 1. Rename: Some toolbars are panels now, some dialogs are panels now...
// 2. Menu icons...
// 3. Update the pane captions properly, e.g. "2D view, top (XY), zoom: ...".
// 4. properly load/save the perspectives, i.e. with the list of open 2D and 3D views.
// 5. Toolbar toggles for: Texture locking und Options.general.NewUVsFaceAligned.


AutoSaveTimerT::AutoSaveTimerT(MapDocumentT* Doc, unsigned long ChildFrameNr)
    : m_Doc(Doc),
      m_AutoSaveName(wxStandardPaths::Get().GetUserDataDir()+"/"+wxString::Format("autosave%lu.cmap", ChildFrameNr))
{
}


void AutoSaveTimerT::Notify()
{
    // Auto-save the document under name m_AutoSaveName, overwriting any existing file.
    if (wxFileExists(m_AutoSaveName)) wxRemoveFile(m_AutoSaveName);
    m_Doc->OnSaveDocument(m_AutoSaveName, true);
}


AutoSaveTimerT::~AutoSaveTimerT()
{
    // Note that the document is already deleted when we get here!

    // On closing the child frame, delete the corresponding auto-saved file,
    // because the user is assumed to save it himself, as is usual when closing CaWE.
    // In fact, we only get here after a successful save or if the user explicitly denied the saving a modified file.
    if (wxFileExists(m_AutoSaveName)) wxRemoveFile(m_AutoSaveName);
}


/// A class that acts as an observer of the map document and the tools for the child frame.
/// It updates the associated child frames status bar, tool options bar, etc.
/// Note that the child frame could also derive from the observer classes and thus be the observer itself,
/// but having the functionality encapsulated in a separate class like this increases clarity.
class ChildFrameT::UpdaterT : public ObserverT, public ToolsObserverT
{
    public:

    UpdaterT(ChildFrameT* ChildFrame);
    ~UpdaterT();

    // Methods inherited from ObserverT.
    void NotifySubjectChanged(SubjectT* Subject, MapDocOtherDetailT OtherDetail);
    void NotifySubjectChanged_Selection(SubjectT* Subject, const ArrayT<MapElementT*>& OldSelection, const ArrayT<MapElementT*>& NewSelection);
 // void NotifySubjectChanged_Created(SubjectT* Subject, const ArrayT< IntrusivePtrT<cf::GameSys::EntityT> >& Entities);
 // void NotifySubjectChanged_Created(SubjectT* Subject, const ArrayT<MapPrimitiveT*>& Primitives);
 // void NotifySubjectChanged_Deleted(SubjectT* Subject, const ArrayT< IntrusivePtrT<cf::GameSys::EntityT> >& Entities);
 // void NotifySubjectChanged_Deleted(SubjectT* Subject, const ArrayT<MapPrimitiveT*>& Primitives);
 // void NotifySubjectChanged_Modified(SubjectT* Subject, const ArrayT<MapElementT*>& MapElements, MapElemModDetailE Detail);
 // void NotifySubjectChanged_Modified(SubjectT* Subject, const ArrayT<MapElementT*>& MapElements, MapElemModDetailE Detail, const ArrayT<BoundingBox3fT>& OldBounds);
 // void Notify_EntChanged(SubjectT* Subject, const ArrayT< IntrusivePtrT<CompMapEntityT> >& Entities, EntityModDetailE Detail);
    void NotifySubjectDies(SubjectT* dyingSubject) { }  // The subjects always live longer than this updater (in contrast to the view windows etc.).

    // Methods inherited from ToolsObserverT.
    void NotifySubjectChanged(ToolsSubjectT* Subject, ToolT* Tool, ToolsUpdatePriorityT Priority);


    private:

    ChildFrameT* m_ChildFrame;
    ToolT*       m_PrevTool;
};


ChildFrameT::UpdaterT::UpdaterT(ChildFrameT* ChildFrame)
    : m_ChildFrame(ChildFrame),
      m_PrevTool(NULL)
{
    m_ChildFrame->m_Doc->RegisterObserver(this);
    m_ChildFrame->m_ToolManager->RegisterObserver(this);

    // Issue the initial notifications.
    NotifySubjectChanged(m_ChildFrame->m_Doc, UPDATE_GRID);
    NotifySubjectChanged_Selection(m_ChildFrame->m_Doc, m_ChildFrame->m_Doc->GetSelection(), m_ChildFrame->m_Doc->GetSelection());
    NotifySubjectChanged(m_ChildFrame->m_ToolManager, m_ChildFrame->m_ToolManager->GetActiveTool(), UPDATE_NOW);
}


ChildFrameT::UpdaterT::~UpdaterT()
{
    m_ChildFrame->m_Doc->UnregisterObserver(this);
    m_ChildFrame->m_ToolManager->UnregisterObserver(this);
}


void ChildFrameT::UpdaterT::NotifySubjectChanged(SubjectT* Subject, MapDocOtherDetailT OtherDetail)
{
    switch (OtherDetail)
    {
        case UPDATE_GRID:
            m_ChildFrame->SetStatusText(wxString::Format(" Snap: %s  Grid: %d ",
                m_ChildFrame->m_Doc->IsSnapEnabled() ? "On" : "Off", m_ChildFrame->m_Doc->GetGridSpacing()), SBP_GRID_SNAP);
            break;

        case UPDATE_POINTFILE:
            break;

        case UPDATE_GLOBALOPTIONS:
            break;
    }
}


void ChildFrameT::UpdaterT::NotifySubjectChanged_Selection(SubjectT* Subject, const ArrayT<MapElementT*>& OldSelection, const ArrayT<MapElementT*>& NewSelection)
{
    switch (NewSelection.Size())
    {
        case 0:
            m_ChildFrame->SetStatusText(" Nothing is selected.", SBP_SELECTION);
            break;

        case 1:
            m_ChildFrame->SetStatusText(wxString(" ")+NewSelection[0]->GetDescription(), SBP_SELECTION);
            break;

        default:
            m_ChildFrame->SetStatusText(wxString::Format(" %lu items are selected.", NewSelection.Size()), SBP_SELECTION);
            break;
    }
}


void ChildFrameT::UpdaterT::NotifySubjectChanged(ToolsSubjectT* Subject, ToolT* Tool, ToolsUpdatePriorityT Priority)
{
    if (Tool->IsActiveTool())
    {
        if (Tool!=m_PrevTool)
        {
            const ArrayT<ToolT*>& Tools  =m_ChildFrame->m_ToolManager->GetTools();
            wxWindow*             Toolbar=NULL;

            // Show the options bar of the active tool, hide everything else.
            for (unsigned long ToolNr=0; ToolNr<Tools.Size(); ToolNr++)
            {
                wxWindow* OptionsBar=Tools[ToolNr]->GetOptionsBar();

                if (!OptionsBar) continue;

                OptionsBar->Show(Tools[ToolNr]->IsActiveTool());

                if (!Toolbar) Toolbar=OptionsBar->GetParent();
            }

            if (Toolbar) Toolbar->Layout();
            m_PrevTool=Tool;
        }

        // Update the status bar.
        Tool->UpdateStatusBar(m_ChildFrame);
    }
}


BEGIN_EVENT_TABLE(ChildFrameT, wxMDIChildFrame)
    EVT_CLOSE(ChildFrameT::OnClose)
    EVT_IDLE(ChildFrameT::OnIdle)
    EVT_END_PROCESS(wxID_ANY, ChildFrameT::OnProcessEnd)

    EVT_MENU_RANGE     (ID_MENU_FILE_CLOSE,            ID_MENU_FILE_SAVEAS,             ChildFrameT::OnMenuFile)
    EVT_UPDATE_UI_RANGE(ID_MENU_FILE_CLOSE,            ID_MENU_FILE_SAVEAS,             ChildFrameT::OnMenuFileUpdate)

    EVT_MENU  (wxID_UNDO,                          ChildFrameT::OnMenuEditUndoRedo)
    EVT_MENU  (wxID_REDO,                          ChildFrameT::OnMenuEditUndoRedo)
    EVT_MENU  (wxID_CUT,                           ChildFrameT::OnMenuEditCut)
    EVT_MENU  (wxID_COPY,                          ChildFrameT::OnMenuEditCopy)
    EVT_MENU  (wxID_PASTE,                         ChildFrameT::OnMenuEditPaste)
    EVT_MENU  (ID_MENU_EDIT_PASTE_SPECIAL,         ChildFrameT::OnMenuEditPaste)
    EVT_MENU  (ID_MENU_EDIT_DELETE,                ChildFrameT::OnMenuEditDelete)
    EVT_BUTTON(ID_MENU_EDIT_DELETE,                ChildFrameT::OnMenuEditDelete)       // This ID is re-used for a button in the "Entity Hierarchy" dialog.
    EVT_MENU  (ID_MENU_EDIT_SELECT_NONE,           ChildFrameT::OnMenuEditSelectNone)
    EVT_MENU  (wxID_SELECTALL,                     ChildFrameT::OnMenuEditSelectAll)

    EVT_UPDATE_UI_RANGE(wxID_UNDO,                  wxID_REDO,                ChildFrameT::OnMenuEditUpdate)
    EVT_UPDATE_UI_RANGE(wxID_CUT,                   wxID_PASTE,               ChildFrameT::OnMenuEditUpdate)
    EVT_UPDATE_UI_RANGE(ID_MENU_EDIT_PASTE_SPECIAL, ID_MENU_EDIT_SELECT_NONE, ChildFrameT::OnMenuEditUpdate)
    EVT_UPDATE_UI      (wxID_SELECTALL,                                       ChildFrameT::OnMenuEditUpdate)

    EVT_MENU_RANGE     (ID_MENU_VIEW_TOOLBARS,         ID_MENU_VIEW_CENTER_3D_VIEWS,    ChildFrameT::OnMenuView)
    EVT_UPDATE_UI_RANGE(ID_MENU_VIEW_TOOLBARS,         ID_MENU_VIEW_CENTER_3D_VIEWS,    ChildFrameT::OnMenuViewUpdate)
    EVT_BUTTON         (ID_MENU_VIEW_CENTER_2D_VIEWS,                                   ChildFrameT::OnMenuView)
    EVT_MENU_RANGE     (ID_MENU_TOOLS_TOOL_SELECTION,  ID_MENU_TOOLS_TOOL_EDITVERTICES, ChildFrameT::OnMenuTools)
    EVT_UPDATE_UI_RANGE(ID_MENU_TOOLS_TOOL_SELECTION,  ID_MENU_TOOLS_TOOL_EDITVERTICES, ChildFrameT::OnMenuToolsUpdate)
    EVT_MENU_RANGE     (ID_MENU_COMPONENTS_FIRST,      ID_MENU_COMPONENTS_MAX,          ChildFrameT::OnMenuComponents)
    EVT_MENU_RANGE     (ID_MENU_PREFABS_LOAD,          ID_MENU_PREFABS_PATH_LAST,       ChildFrameT::OnMenuPrefabs)
    EVT_MENU_RANGE     (ID_MENU_COMPILE_FLAG_SAVE_MAP, ID_MENU_COMPILE_ABORT,           ChildFrameT::OnMenuCompile)

    EVT_ACTIVATE(ChildFrameT::OnWindowActivate)
    EVT_AUI_PANE_CLOSE(ChildFrameT::OnAuiPaneClose)
    // EVT_SIZE(ChildFrameT::OnSize)
END_EVENT_TABLE()


namespace
{
    bool CompareTypeInfoNames(const cf::TypeSys::TypeInfoT* const& TI1, const cf::TypeSys::TypeInfoT* const& TI2)
    {
        return wxStricmp(TI1->ClassName, TI2->ClassName) < 0;
    }
}


/*static*/ void ChildFrameT::BuildComponentsMenu(wxMenu* MenuParent, const cf::TypeSys::TypeInfoT* TypeParent)
{
    // For each child of TypeParent (but not TypeParent itself), add an item to MenuParent.
    ArrayT<const cf::TypeSys::TypeInfoT*> CompTIs;

    for (const cf::TypeSys::TypeInfoT* TI = TypeParent->Child; TI; TI = TI->Sibling)
    {
        // The topmost root and thus the ComponentBaseT class is never added.
        wxASSERT(TI->Base);

        // Skip fundamental component types (each entity has one instance anyway).
        if (cf::GameSys::IsFundamental(TI)) continue;

        // // Skip the ComponentSelectionT class, that is specific to this GUI Editor application.
        // if (TI == &ComponentSelectionT::TypeInfo) continue;

        CompTIs.PushBack(TI);
    }

    CompTIs.QuickSort(CompareTypeInfoNames);

    for (unsigned int TINr = 0; TINr < CompTIs.Size(); TINr++)
    {
        const cf::TypeSys::TypeInfoT* TI   = CompTIs[TINr];
        wxString                      Name = TI->ClassName;

        if (Name.StartsWith("Component") && Name.EndsWith("T"))
            Name = Name.SubString(9, Name.length() - 2);

        if (Name.StartsWith("GameSys::Component") && Name.EndsWith("T"))
            Name = Name.SubString(18, Name.length() - 2);

        wxASSERT(ID_MENU_COMPONENTS_FIRST + TI->TypeNr <= ID_MENU_COMPONENTS_MAX);

        if (TI->Child)
        {
            wxMenu* SubMenu = new wxMenu;

            BuildComponentsMenu(SubMenu, TI);
            MenuParent->AppendSubMenu(SubMenu, "&" + Name);
        }
        else
        {
            MenuParent->Append(ID_MENU_COMPONENTS_FIRST + TI->TypeNr, "&" + Name, "Add component to the selected entity");
        }
    }
}


// About the wxMDIChildFrame constructor:
// Although the parent should actually be the parent frames MDI *client* (sub-)window, according to wx documentation
// it is fine to pass the MDI parent window itself (rather than its client window) here. See wxMDIChildFrame documentation.
// Contrary to wx docs, the wxDocMDIChildFrame even *requires* a pointer to wxMDIParentFrame here (not just to a wxFrame, as the docs say)!
ChildFrameT::ChildFrameT(ParentFrameT* Parent, MapDocumentT* MapDoc)
    : wxMDIChildFrame(Parent, wxID_ANY, MapDoc->GetFileName(), wxDefaultPosition, wxSize(800, 600), wxDEFAULT_FRAME_STYLE | wxMAXIMIZE),
      m_Parent(Parent),      // Must use a fixed size in place of wxDefaultSize, see <http://trac.wxwidgets.org/ticket/12490> for details.
      m_AUIManager(this),
      m_AUIDefaultPerspective(""),
      m_Doc(MapDoc),
      m_History(new CommandHistoryT()),
      m_LastSavedAtCommandNr(0),
      m_AutoSaveTimer(m_Doc, Parent->m_ChildFrames.Size()),
      m_ToolManager(NULL),
      m_EntityHierarchyDialog(NULL),
      m_MaterialsToolbar(NULL),
      m_GroupsToolbar(NULL),
      m_ConsoleDialog(NULL),
      m_SurfacePropsDialog(NULL),
      m_TerrainEditorDialog(NULL),
      m_Updater(NULL),
      FileMenu(NULL),
      m_ComponentsMenu(NULL),
      m_PrefabsMenu(NULL),
      m_PrefabsMenuPaths(),
      CompileMenu(NULL),
      CurrentProcess(NULL),
      CurrentProcessID(0),
      PendingCompileCommands(),
      m_ViewWindows()
{
    // Register us with the parents list of children.
    m_Parent->m_ChildFrames.PushBack(this);

    // Should really pass this in the MapDocumentT ctor (the mapdoc should be a member of us).
    // Correction: NO!  While the doc should be a member of us, it might be *created* before us so that its ctor may throw exceptions,
    // and then be given to us (we become the owner). In this case, we have to set us as the child frame here - no chance to do it
    // in the docs ctor. However, I wonder why the doc should know us in the first place...?
    m_Doc->SetChildFrame(this);

    // Register the document as our first event handler.
    PushEventHandler(m_Doc);


    // Create the menu.
    wxMenuBar *item0 = new wxMenuBar;

    FileMenu = new wxMenu;

    wxMenu* NewMenu = new wxMenu;

    NewMenu->Append(ParentFrameT::ID_MENU_FILE_NEW_MAP,   wxT("New &Map\tCtrl+N"), wxT(""));
    NewMenu->Append(ParentFrameT::ID_MENU_FILE_NEW_MODEL, wxT("New M&odel\tCtrl+Shift+N"), wxT(""));
    NewMenu->Append(ParentFrameT::ID_MENU_FILE_NEW_GUI,   wxT("New &GUI\tCtrl+Alt+N"), wxT(""));
    NewMenu->Append(ParentFrameT::ID_MENU_FILE_NEW_FONT,  wxT("New &Font"), wxT(""));

    FileMenu->AppendSubMenu(NewMenu, wxT("&New"));

    FileMenu->Append(ParentFrameT::ID_MENU_FILE_OPEN, wxT("&Open...\tCtrl+O"), wxT("") );
    FileMenu->Append(ID_MENU_FILE_CLOSE, wxT("&Close\tCtrl+F4"), wxT("") );
    FileMenu->Append(ID_MENU_FILE_SAVE, wxT("&Save\tCtrl+S"), wxT("") );
    FileMenu->Append(ID_MENU_FILE_SAVEAS, wxT("Save &As..."), wxT("") );
    FileMenu->AppendSeparator();
    FileMenu->Append(ParentFrameT::ID_MENU_FILE_CONFIGURE, wxT("Conf&igure CaWE..."), wxT("") );
    FileMenu->Append(ParentFrameT::ID_MENU_FILE_EXIT, wxT("E&xit"), wxT("") );
    m_Parent->m_FileHistory.UseMenu(FileMenu);
    m_Parent->m_FileHistory.AddFilesToMenu(FileMenu);
    item0->Append( FileMenu, wxT("&File") );

    wxMenu* item2 = new wxMenu;
    item2->Append( wxID_UNDO, wxT("&Undo\tCtrl+Z"), wxT("") );
    item2->Append( wxID_REDO, wxT("&Redo\tCtrl+Y"), wxT("") );
    item2->AppendSeparator();
    item2->Append( wxID_CUT, wxT("Cu&t\tCtrl+X"), wxT("") );
    item2->Append( wxID_COPY, wxT("&Copy\tCtrl+C"), wxT("") );
    item2->Append( wxID_PASTE, wxT("&Paste\tCtrl+V"), wxT("") );
    item2->Append(ID_MENU_EDIT_PASTE_SPECIAL, wxT("Paste &Special...\tShift+Ctrl+V"), wxT("") );
    item2->Append(ID_MENU_EDIT_DELETE, wxT("&Delete\tCtrl+Del"), wxT("") );
    item2->AppendSeparator();
    item2->Append(ID_MENU_EDIT_SELECT_NONE, wxT("Select &None\tCtrl+Q"), wxT("") );
    item2->Append( wxID_SELECTALL, wxT("Select All"), wxT("") );
    item2->AppendSeparator();
    item2->Append(ID_MENU_SELECTION_ASSIGN_TO_ENTITY, wxT("&Assign to Entity\tCtrl+A"), wxT("") );
    item2->AppendSeparator();
    item2->Append(ID_MENU_SELECTION_GROUP, wxT("&Group\tCtrl+G"), "Group the selected items.");
    item2->Append(ID_MENU_SELECTION_HIDE, wxT("H&ide"), "Hide the selected items in a new group.");
    item2->Append(ID_MENU_SELECTION_HIDE_OTHER, wxT("Hide &Other"), "Hide all unselected items in a new group (only those that are not in a group already).");
    item0->Append( item2, wxT("&Edit") );

    wxMenu* item3 = new wxMenu;
    item3->AppendCheckItem(ID_MENU_MAP_SNAP_TO_GRID, wxT("&Snap to grid\tShift+W"), wxT("") ); item3->Check(ID_MENU_MAP_SNAP_TO_GRID, m_Doc->IsSnapEnabled());
    item3->AppendCheckItem(ID_MENU_MAP_SHOW_GRID_2D, wxT("Sho&w grid\tShift+R"), wxT("") );    item3->Check(ID_MENU_MAP_SHOW_GRID_2D, m_Doc->Is2DGridEnabled());
 // item3->AppendCheckItem(ID_MENU_MAP_SHOW_GRID_3D, wxT("Show 3D grid\tShift+R"), wxT("") );  item3->Check(ID_MENU_MAP_SHOW_GRID_3D, m_Doc->Is3DGridEnabled());  // Maybe later.

    wxMenu* item4 = new wxMenu;
    item4->Append(ID_MENU_MAP_FINER_GRID, wxT("&Finer Grid"), wxT("") );
    item4->Append(ID_MENU_MAP_COARSER_GRID, wxT("&Coarser Grid"), wxT("") );
    item3->Append(ID_MENU_MAP_GRID_SETTINGS, wxT("&Grid settings"), item4 );

    item3->AppendSeparator();
    item3->AppendCheckItem(ID_MENU_MAP_AUTO_GROUP_ENTITIES, wxT("Auto-Group Entities"), wxT("") );
    item3->AppendSeparator();
    item3->Append(ID_MENU_MAP_GOTO_PRIMITIVE, wxT("G&o to Primitive...\tShift+Ctrl+G"), wxT("") );
    item3->Append(ID_MENU_MAP_SHOW_INFO, wxT("Show &Information..."), wxT("") );
    item3->Append(ID_MENU_MAP_CHECK_FOR_PROBLEMS, wxT("&Check for Problems..."), wxT("") );
    item3->AppendSeparator();
    item3->Append(ID_MENU_MAP_LOAD_POINTFILE, wxT("&Load Pointfile..."), wxT("") );
    item3->Append(ID_MENU_MAP_UNLOAD_POINTFILE, wxT("&Unload Pointfile"), wxT("") );
    item0->Append( item3, wxT("&Map") );

    wxMenu* item6 = new wxMenu;

    wxMenu* item7 = new wxMenu;
    item7->AppendCheckItem(ID_MENU_VIEW_TOOLBARS_FILE, wxT("&General"), wxT("") );
    item7->AppendCheckItem(ID_MENU_VIEW_TOOLBARS_TOOLS, wxT("&Tools"), wxT("") );
    item7->AppendCheckItem(ID_MENU_VIEW_TOOLBARS_TOOLOPTIONS, "&Tool Options", "");
    item6->Append(ID_MENU_VIEW_TOOLBARS, wxT("&Toolbars"), item7 );
    item6->AppendSeparator();

    item6->AppendCheckItem(ID_MENU_VIEW_PANELS_ENTITY_HIERARCHY, "Entity &Hierarchy", "Show/Hide the Entity Hierarchy");
    item6->AppendCheckItem(ID_MENU_VIEW_PANELS_ENTITY_INSPECTOR, "Entity &Inspector\tAlt+Enter", "Show/Hide the Entity Inspector");
    item6->AppendCheckItem(ID_MENU_VIEW_PANELS_MATERIALS,        "&Materials", "");
    item6->AppendCheckItem(ID_MENU_VIEW_PANELS_GROUPS,           "&Groups", "");
    item6->AppendCheckItem(ID_MENU_VIEW_PANELS_CONSOLE,          "&Console", "");
    item6->AppendSeparator();

    item6->Append(ID_MENU_VIEW_NEW_2D_VIEW, "New &2D view", "Opens a new 2D view on the map");
    item6->Append(ID_MENU_VIEW_NEW_3D_VIEW, "New &3D view", "Opens a new 3D view on the map");
    item6->AppendSeparator();

    item6->Append(ID_MENU_VIEW_LOAD_USER_PERSPECTIVE, "&Load user window layout", "Loads the user defined window layout");
    item6->Append(ID_MENU_VIEW_SAVE_USER_PERSPECTIVE, "&Save user window layout", "Saves the current window layout");
    item6->Append(ID_MENU_VIEW_LOAD_DEFAULT_PERSPECTIVE, "Load &default window layout", "Restores the default window layout");
    item6->AppendSeparator();
    item6->Append(ID_MENU_VIEW_CENTER_2D_VIEWS, wxT("Center &2D Views on Sel.\tCtrl+E"), wxT("") );
    item6->Append(ID_MENU_VIEW_CENTER_3D_VIEWS, wxT("Center &3D Views on Sel.\tCtrl+Shift+E"), wxT("") );
    item6->AppendSeparator();
    item6->Append(ID_MENU_VIEW_SHOW_ENTITY_INFO, wxT("Show Entity &Info"), wxT(""), wxITEM_CHECK);
    item6->Append(ID_MENU_VIEW_SHOW_ENTITY_TARGETS, wxT("Show Entity &Targets"), wxT(""), wxITEM_CHECK );
    item0->Append( item6, wxT("&View") );

    wxMenu* item8 = new wxMenu;
    item8->AppendRadioItem(ID_MENU_TOOLS_TOOL_SELECTION,             wxT("&Selection\tCtrl+1"), wxT(""));
    item8->AppendRadioItem(ID_MENU_TOOLS_TOOL_CAMERA,                wxT("&Camera\tCtrl+2"), wxT(""));
    item8->AppendRadioItem(ID_MENU_TOOLS_TOOL_NEWBRUSH,              wxT("New &Brush\tCtrl+3"), wxT(""));
    item8->AppendRadioItem(ID_MENU_TOOLS_TOOL_NEWENTITY,             wxT("New &Entity\tCtrl+4"), wxT(""));
    item8->AppendRadioItem(ID_MENU_TOOLS_TOOL_NEWBEZIERPATCH,        wxT("New Bezier &Patch\tCtrl+5"), wxT(""));
    item8->AppendRadioItem(ID_MENU_TOOLS_TOOL_NEWTERRAIN,            wxT("New &Terrain\tCtrl+6"), wxT(""));
    item8->AppendRadioItem(ID_MENU_TOOLS_TOOL_NEWDECAL,              wxT("New &Decal\tCtrl+7"), wxT(""));
    item8->AppendRadioItem(ID_MENU_TOOLS_TOOL_EDITSURFACEPROPERTIES, wxT("Edit Surf&ace Properties\tCtrl+Shift+A"), wxT(""));
    item8->AppendRadioItem(ID_MENU_TOOLS_TOOL_TERRAINEDITOR,         wxT("Edit Te&rrain\tCtrl+Shift+T"), wxT(""));
    item8->AppendRadioItem(ID_MENU_TOOLS_TOOL_CLIP,                  wxT("Clip Br&ushes\tCtrl+Shift+X"), wxT(""));
    item8->AppendRadioItem(ID_MENU_TOOLS_TOOL_EDITVERTICES,          wxT("Edit &Vertices\tCtrl+Shift+V"), wxT(""));
    item8->AppendSeparator();
    item8->Append(ID_MENU_TOOLS_CARVE, wxT("&Carve\tCtrl+Shift+C"), wxT("") );
    item8->Append(ID_MENU_TOOLS_MAKE_HOLLOW, wxT("Make Hollow\tCtrl+H"), wxT("") );
    item8->AppendSeparator();
    item8->Append(ID_MENU_TOOLS_APPLY_MATERIAL, wxT("Apply &Material"), "Apply the current material to the selected elements.");
    item8->Append(ID_MENU_TOOLS_REPLACE_MATERIALS, wxT("R&eplace Materials..."), wxT("") );
    item8->AppendCheckItem(ID_MENU_TOOLS_MATERIAL_LOCK, wxT("Material &Lock\tCtrl+L"), wxT("") );
    item8->AppendSeparator();
    item8->Append(ID_MENU_TOOLS_TRANSFORM, wxT("Transform\tCtrl+T"), wxT("") );

    wxMenu* item9 = new wxMenu;
    item9->Append(ID_MENU_TOOLS_ALIGN_LEFT, wxT("to &Left"), wxT("") );
    item9->Append(ID_MENU_TOOLS_ALIGN_RIGHT, wxT("to &Right"), wxT("") );
    item9->Append(ID_MENU_TOOLS_ALIGN_HOR_CENTER, wxT("to &hor. Center"), wxT("") );
    item9->Append(ID_MENU_TOOLS_ALIGN_TOP, wxT("to &Top"), wxT("") );
    item9->Append(ID_MENU_TOOLS_ALIGN_BOTTOM, wxT("to &Bottom"), wxT("") );
    item9->Append(ID_MENU_TOOLS_ALIGN_VERT_CENTER, wxT("to &vert. Center"), wxT("") );
    item8->Append(ID_MENU_TOOLS_ALIGN, wxT("&Align"), item9 );

    wxMenu* item10 = new wxMenu;
    item10->Append(ID_MENU_TOOLS_MIRROR_HOR, wxT("&Horizontally"), wxT("") );
    item10->Append(ID_MENU_TOOLS_MIRROR_VERT, wxT("&Vertically"), wxT("") );
    item8->Append(ID_MENU_TOOLS_MIRROR, wxT("M&irror"), item10 );

    item0->Append( item8, wxT("&Tools") );


    m_ComponentsMenu = new wxMenu;
    const ArrayT<const cf::TypeSys::TypeInfoT*>& CompRoots = cf::GameSys::GetComponentTIM().GetTypeInfoRoots();

    for (unsigned long RootNr = 0; RootNr < CompRoots.Size(); RootNr++)
    {
        BuildComponentsMenu(m_ComponentsMenu, CompRoots[RootNr]);

        if (RootNr + 1 < CompRoots.Size())
            m_ComponentsMenu->AppendSeparator();
    }

    item0->Append(m_ComponentsMenu, "&Components");


    m_PrefabsMenu = new wxMenu;
    m_PrefabsMenu->Append(ID_MENU_PREFABS_LOAD, "&Load Prefab...", "");
    m_PrefabsMenu->Append(ID_MENU_PREFABS_SAVE, "&Save Prefab...", "");
    m_PrefabsMenu->AppendSeparator();
    UpdatePrefabsMenu();
    item0->Append(m_PrefabsMenu, "&Prefabs");

    CompileMenu=new wxMenu;
    CompileMenu->AppendCheckItem(ID_MENU_COMPILE_FLAG_SAVE_MAP,   "1. &Save Map",     ""); CompileMenu->Check(ID_MENU_COMPILE_FLAG_SAVE_MAP,   true);
    CompileMenu->AppendCheckItem(ID_MENU_COMPILE_FLAG_RUN_BSP,    "2. Run Ca&BSP",    ""); CompileMenu->Check(ID_MENU_COMPILE_FLAG_RUN_BSP,    true); CompileMenu->Enable(ID_MENU_COMPILE_FLAG_RUN_BSP, false);
    CompileMenu->AppendCheckItem(ID_MENU_COMPILE_FLAG_RUN_PVS,    "3. Run Ca&PVS",    ""); CompileMenu->Check(ID_MENU_COMPILE_FLAG_RUN_PVS,    true);
    CompileMenu->AppendCheckItem(ID_MENU_COMPILE_FLAG_RUN_LIGHT,  "4. Run Ca&Light",  ""); CompileMenu->Check(ID_MENU_COMPILE_FLAG_RUN_LIGHT,  false);  // Intentionally off per default, in order to make sure that first-time users are not confronted with a multi-hour CaLight session.
    CompileMenu->AppendCheckItem(ID_MENU_COMPILE_FLAG_RUN_ENGINE, "5. Start &Engine", ""); CompileMenu->Check(ID_MENU_COMPILE_FLAG_RUN_ENGINE, true);
    CompileMenu->AppendSeparator();
    CompileMenu->Append(ID_MENU_COMPILE_QUICK,   "Run &Quick...",        "");
    CompileMenu->Append(ID_MENU_COMPILE_NORMAL,  "Run &Normal...",       "");
    CompileMenu->Append(ID_MENU_COMPILE_QUALITY, "Run &High Quality...", "");
    CompileMenu->Append(ID_MENU_COMPILE_CUSTOM,  "Run Custom...",        "");
    CompileMenu->AppendSeparator();
    CompileMenu->Append(ID_MENU_COMPILE_ABORT, "Abort", ""); CompileMenu->Enable(ID_MENU_COMPILE_ABORT, false);
    item0->Append(CompileMenu, wxT("C&ompile") );

    wxMenu* item12 = new wxMenu;
    item12->Append(ParentFrameT::ID_MENU_HELP_CONTENTS, wxT("&CaWE Help\tF1"), wxT("") );
    item12->AppendSeparator();
    item12->Append(ParentFrameT::ID_MENU_HELP_CAFU_WEBSITE, wxT("Cafu &Website"), wxT("") );
    item12->Append(ParentFrameT::ID_MENU_HELP_CAFU_FORUM, wxT("Cafu &Forum"), wxT("") );
    item12->AppendSeparator();
    item12->Append(ParentFrameT::ID_MENU_HELP_ABOUT, wxT("&About..."), wxT("") );
    item0->Append(item12, wxT("&Help") );

    SetMenuBar(item0);
    SetMinSize(wxSize(400,300));


    // Create the status bar at the bottom of the frame.
    int StatusBarWidths[6]={ -1, 200, 130, 130, 100, 120 };

    CreateStatusBar(6, wxST_SIZEGRIP);                  // Create the status bar with 6 panes.
    SetStatusWidths(6, StatusBarWidths);                // Set the widths of the individual panes.
    SetStatusBarPane(SBP_MENU_HELP);                    // Use the first pane (number 0) for displaying menu and toolbar help.
    SetStatusText(" Welcome to CaWE!", SBP_MENU_HELP);  // Set an initial text in the first pane.


    // Create the toolbars.
    wxAuiToolBar* ToolbarFile=new wxAuiToolBar(this, wxID_ANY);
    ToolbarFile->AddTool(ParentFrameT::ID_MENU_FILE_NEW_GUI, "New", wxArtProvider::GetBitmap(wxART_NEW, wxART_TOOLBAR), "Create a new file");
    ToolbarFile->AddTool(ParentFrameT::ID_MENU_FILE_OPEN,    "Open", wxArtProvider::GetBitmap(wxART_FILE_OPEN, wxART_TOOLBAR), "Open an existing file");
    ToolbarFile->AddTool(ID_MENU_FILE_SAVE,                  "Save", wxArtProvider::GetBitmap(wxART_FILE_SAVE, wxART_TOOLBAR), "Save the file");
    ToolbarFile->AddTool(ID_MENU_FILE_SAVEAS,                "Save as", wxArtProvider::GetBitmap(wxART_FILE_SAVE_AS, wxART_TOOLBAR), "Save the file under a different name");
    ToolbarFile->AddSeparator();
    ToolbarFile->AddTool(wxID_UNDO,           "Undo", wxArtProvider::GetBitmap(wxART_UNDO, wxART_TOOLBAR), "Undo the last action");
    ToolbarFile->AddTool(wxID_REDO,           "Redo", wxArtProvider::GetBitmap(wxART_REDO, wxART_TOOLBAR), "Redo the previously undone action");
    ToolbarFile->AddSeparator();
    ToolbarFile->AddTool(wxID_CUT,            "Cut", wxArtProvider::GetBitmap(wxART_CUT, wxART_TOOLBAR), "Cut");
    ToolbarFile->AddTool(wxID_COPY,           "Copy", wxArtProvider::GetBitmap(wxART_COPY, wxART_TOOLBAR), "Copy");
    ToolbarFile->AddTool(wxID_PASTE,          "Paste", wxArtProvider::GetBitmap(wxART_PASTE, wxART_TOOLBAR), "Paste");
    ToolbarFile->AddTool(ID_MENU_EDIT_DELETE, "Delete", wxArtProvider::GetBitmap(wxART_DELETE, wxART_TOOLBAR), "Delete");
    ToolbarFile->Realize();

    // Note that we cannot have separators between these tools, because they must all be in the same radio group.
    wxAuiToolBar* ToolbarTools=new wxAuiToolBar(this, wxID_ANY);
    ToolbarTools->AddTool(ID_MENU_TOOLS_TOOL_SELECTION,             "Selection",               wxArtProvider::GetBitmap("MapEditor/tool-selection",          wxART_TOOLBAR, wxSize(21, 18)), "Selection", wxITEM_RADIO);
    ToolbarTools->AddTool(ID_MENU_TOOLS_TOOL_CAMERA,                "Camera",                  wxArtProvider::GetBitmap("MapEditor/tool-camera",             wxART_TOOLBAR, wxSize(21, 18)), "Camera", wxITEM_RADIO);
    ToolbarTools->AddTool(ID_MENU_TOOLS_TOOL_NEWBRUSH,              "New Brush",               wxArtProvider::GetBitmap("MapEditor/tool-new-brush",          wxART_TOOLBAR, wxSize(21, 18)), "New Brush", wxITEM_RADIO);
    ToolbarTools->AddTool(ID_MENU_TOOLS_TOOL_NEWENTITY,             "New Entity",              wxArtProvider::GetBitmap("MapEditor/tool-new-entity",         wxART_TOOLBAR, wxSize(21, 18)), "New Entity", wxITEM_RADIO);
    ToolbarTools->AddTool(ID_MENU_TOOLS_TOOL_NEWBEZIERPATCH,        "New Bezier Patch",        wxArtProvider::GetBitmap("MapEditor/tool-new-bezierpatch",    wxART_TOOLBAR, wxSize(21, 18)), "New Bezier Patch", wxITEM_RADIO);
    ToolbarTools->AddTool(ID_MENU_TOOLS_TOOL_NEWTERRAIN,            "New Terrain",             wxArtProvider::GetBitmap("MapEditor/tool-new-terrain",        wxART_TOOLBAR, wxSize(21, 18)), "New Terrain", wxITEM_RADIO);
    ToolbarTools->AddTool(ID_MENU_TOOLS_TOOL_NEWDECAL,              "New Decal",               wxArtProvider::GetBitmap("MapEditor/tool-new-decal",          wxART_TOOLBAR, wxSize(21, 18)), "New Decal", wxITEM_RADIO);
    ToolbarTools->AddTool(ID_MENU_TOOLS_TOOL_EDITSURFACEPROPERTIES, "Edit Surface Properties", wxArtProvider::GetBitmap("MapEditor/tool-edit-surface-props", wxART_TOOLBAR, wxSize(21, 18)), "Edit Surface Properties", wxITEM_RADIO);
    ToolbarTools->AddTool(ID_MENU_TOOLS_TOOL_TERRAINEDITOR,         "Edit Terrain",            wxArtProvider::GetBitmap("MapEditor/tool-edit-terrain",       wxART_TOOLBAR, wxSize(21, 18)), "Edit Terrain", wxITEM_RADIO);
    ToolbarTools->AddTool(ID_MENU_TOOLS_TOOL_CLIP,                  "Clip Brushes",            wxArtProvider::GetBitmap("MapEditor/tool-clip",               wxART_TOOLBAR, wxSize(21, 18)), "Clip Brushes", wxITEM_RADIO);
    ToolbarTools->AddTool(ID_MENU_TOOLS_TOOL_EDITVERTICES,          "Edit Brush Vertices",     wxArtProvider::GetBitmap("MapEditor/tool-edit-vertices",      wxART_TOOLBAR, wxSize(21, 18)), "Edit Brush Vertices", wxITEM_RADIO);
    ToolbarTools->Realize();

    m_AUIManager.AddPane(ToolbarFile, wxAuiPaneInfo().ToolbarPane().
                         Name("File Toolbar").Caption("File Toolbar").
                         Top().Row(0).Position(0).LeftDockable(false).RightDockable(false));

    m_AUIManager.AddPane(ToolbarTools, wxAuiPaneInfo().ToolbarPane().
                         Name("Tools Toolbar").Caption("Tools Toolbar").
                         Top().Row(0).Position(1).LeftDockable(false).RightDockable(false));


    wxPanel* ToolbarToolOptions=new wxPanel(this, -1, wxDefaultPosition, wxSize(852, 30));
    m_ToolManager=new ToolManagerT(*m_Doc, ToolbarToolOptions);     // Allocate our tool manager.
    // This can NOT be called here - it will attempt to update the status bar and the tool options bars,
    // which do not yet exist at this point. Instead, the call is made below, near the end of this constructor.
    // m_ToolManager->SetActiveTool(GetToolTIM().FindTypeInfoByName("ToolSelectionT"));

    wxSizer* TTOSizer=new wxBoxSizer(wxVERTICAL);   // The sizer makes sure that the individual panels each extend to the full width and height of the toolbar.

    // Add all tool options bars to the sizer (initially hidden).
    for (unsigned long ToolNr=0; ToolNr<m_ToolManager->GetTools().Size(); ToolNr++)
    {
        ToolT*    Tool      =m_ToolManager->GetTools()[ToolNr];
        wxWindow* OptionsBar=Tool->GetOptionsBar();

        if (!OptionsBar) continue;

        OptionsBar->Hide();
        TTOSizer->Add(OptionsBar, 1, wxGROW);
    }

    ToolbarToolOptions->SetSizer(TTOSizer);
    // TTOSizer->SetSizeHints(ToolbarToolOptions);      // When this line is activated ("commented in"), the size of the pane gets wrong.

    m_AUIManager.AddPane(ToolbarToolOptions, wxAuiPaneInfo().
                         Name("Tool Options Toolbar").Caption("Tool Options Toolbar").
                         Top().Row(1));


    // Create the toolbars and non-modal dialogs.
    // Note that most if not all of these wxAUI panes have extra style wxWS_EX_BLOCK_EVENTS set,
    // so that they do not propagate their events to us, but behave as if they were derived from wxDialog.
    m_EntityHierarchyDialog = new EntityHierarchyPanelT(this, wxSize(230, 500));
    m_AUIManager.AddPane(m_EntityHierarchyDialog, wxAuiPaneInfo().
                         Name("EntityHierarchy").Caption("Entity Hierarchy").
                         Left().Position(0));

    m_MaterialsToolbar = new MaterialsToolbarT(this, m_Doc);
    m_AUIManager.AddPane(m_MaterialsToolbar, wxAuiPaneInfo().
                         Name("Materials").Caption("Materials").
                         Left().Position(1));

    m_GroupsToolbar = new GroupsToolbarT(this, m_History);
    m_AUIManager.AddPane(m_GroupsToolbar, wxAuiPaneInfo().
                         Name("Groups").Caption("Groups").
                         Left().Position(2));

    m_ConsoleDialog = new ConsoleDialogT(this);
    m_AUIManager.AddPane(m_ConsoleDialog, wxAuiPaneInfo().
                         Name("Console").Caption("Console").
                         BestSize(400, 300).
                         Bottom().Hide());

    m_SurfacePropsDialog = new EditSurfacePropsDialogT(this, m_Doc);
    m_AUIManager.AddPane(m_SurfacePropsDialog, wxAuiPaneInfo().
                         Name("Surface Properties").Caption("Surface Properties").
                         Float().Hide());

    m_TerrainEditorDialog = new TerrainEditorDialogT(this, *m_Doc->GetGameConfig(), static_cast<ToolTerrainEditorT*>(m_ToolManager->GetTool(ToolTerrainEditorT::TypeInfo)));
    m_AUIManager.AddPane(m_TerrainEditorDialog, wxAuiPaneInfo().
                         Name("Terrain Editor").Caption("Terrain Editor").
                         Float().Hide());


    ViewWindow3DT* CenterPaneView = new ViewWindow3DT(this, this, NULL, (ViewWindowT::ViewTypeT)wxConfigBase::Get()->Read("Splitter/ViewType00", ViewWindowT::VT_3D_FULL_MATS));
    m_AUIManager.AddPane(CenterPaneView, wxAuiPaneInfo().
                         Name("Main View").Caption(CenterPaneView->GetCaption()).
                         CenterPane().CaptionVisible().MaximizeButton().MinimizeButton());

    ViewWindow2DT* ViewTopRight = new ViewWindow2DT(this, this, (ViewWindowT::ViewTypeT)wxConfigBase::Get()->Read("Splitter/ViewType01", ViewWindowT::VT_2D_XY));
    m_AUIManager.AddPane(ViewTopRight, wxAuiPaneInfo().
                         // Name("xy").
                         Caption(ViewTopRight->GetCaption()).
                         DestroyOnClose().Right().Row(1).Position(0).MaximizeButton().MinimizeButton());

    ViewWindow2DT* ViewBottomRight = new ViewWindow2DT(this, this, (ViewWindowT::ViewTypeT)wxConfigBase::Get()->Read("Splitter/ViewType11", ViewWindowT::VT_2D_XZ));
    m_AUIManager.AddPane(ViewBottomRight, wxAuiPaneInfo().
                         // Name("xy").
                         Caption(ViewBottomRight->GetCaption()).
                         DestroyOnClose().Right().Row(1).Position(1).MaximizeButton().MinimizeButton());

    m_InspectorDialog = new InspectorDialogT(this, m_Doc);
    m_AUIManager.AddPane(m_InspectorDialog, wxAuiPaneInfo().
                         Name("EntityInspector").Caption("Entity Inspector").
                         Right().Row(0).Position(0));


    // Save the AUI perspective that we set up in this ctor code as the "default perspective".
    m_AUIDefaultPerspective = m_AUIManager.SavePerspective();

    // Load the AUI user perspective (implies a call to m_AUIManager.Update()).
    m_AUIManager.LoadPerspective(wxConfigBase::Get()->Read("MapEditor_AUIUserPerspective", m_AUIDefaultPerspective));


    // Activate the Selection tool as the default tool (having initially no tool active at all can be quite confusing for users).
    // This cannot be done near the creation of the tool manager above, because then the status bar and tool options bars do not yet exist.
    m_ToolManager->SetActiveTool(GetToolTIM().FindTypeInfoByName("ToolSelectionT"));

#ifndef __WXGTK__
    Centre(wxBOTH);
#endif
    if (!IsMaximized()) Maximize(true);     // Also have wxMAXIMIZE set as frame style.
    Show(true);

    m_Updater=new UpdaterT(this);

    // Start the autosave timer.
    int Minutes=0;
    wxConfigBase::Get()->Read("General/AutoSave Interval", &Minutes, 10);
    if (Minutes>0) m_AutoSaveTimer.Start(Minutes*60*1000);
              else m_AutoSaveTimer.Stop();
}


ChildFrameT::~ChildFrameT()
{
    m_AutoSaveTimer.Stop();

    if (CurrentProcess != NULL)
    {
        CurrentProcess->Detach();
        CurrentProcess = NULL;
        CurrentProcessID = 0;
    }

    m_Parent->m_FileHistory.RemoveMenu(FileMenu);

    delete m_Updater;
    m_Updater = NULL;

    delete m_ToolManager;
    m_ToolManager = NULL;

    // Unregister us from the parents list of children.
    const int Index = m_Parent->m_ChildFrames.Find(this);
    m_Parent->m_ChildFrames.RemoveAt(Index);

    // Remove the document as our first event handler.
    PopEventHandler();


    // Uninit the AUI manager.
    m_AUIManager.UnInit();

    // Delete the history.
    // This must be done *before* the MapDocumentT is deleted, because the history may contain commands that contain
    // entities that contain components. The destructors of these components need access to the (still existing)
    // world in order to release any shared resources (see ComponentCollisionModelT::CleanUp() for an example).
    delete m_History;
    m_History = NULL;

    // Delete the document.
    delete m_Doc;
    m_Doc = NULL;
}


bool ChildFrameT::SubmitCommand(CommandT* Command)
{
    if (m_History->SubmitCommand(Command))
    {
        if (Command->SuggestsSave()) SetTitle(m_Doc->GetFileName() + "*");
        return true;
    }

    return false;
}


Vector3fT ChildFrameT::GuessUserVisiblePoint() const
{
    Vector3fT     Point(m_Doc->GetMostRecentSelBB().GetCenter());
    unsigned long HaveAxes = 0;     // Bit i is set if we have a coordinate for the i-th axis.

    for (unsigned long ViewWinNr=0; ViewWinNr<m_ViewWindows.Size() && HaveAxes<7; ViewWinNr++)
    {
        const ViewWindow2DT* ViewWin2D =dynamic_cast<ViewWindow2DT*>(m_ViewWindows[ViewWinNr]); if (!ViewWin2D) continue;
        const int            HorzAxis  =ViewWin2D->GetAxesInfo().HorzAxis;
        const int            VertAxis  =ViewWin2D->GetAxesInfo().VertAxis;
        const wxRect         ClientRect=wxRect(wxPoint(0, 0), ViewWin2D->GetClientSize());
        const Vector3fT      Center    =ViewWin2D->WindowToWorld(wxPoint(ClientRect.x+ClientRect.width/2, ClientRect.y+ClientRect.height/2), 0.0f);

        if ((HaveAxes & (1ul << HorzAxis))==0)
        {
            Point[HorzAxis]=Center[HorzAxis];
            HaveAxes|=(1ul << HorzAxis);
        }

        if ((HaveAxes & (1ul << VertAxis))==0)
        {
            Point[VertAxis]=Center[VertAxis];
            HaveAxes|=(1ul << VertAxis);
        }
    }

    return Point;
}


void ChildFrameT::All2DViews_Zoom(float ZoomFactor)
{
    for (unsigned long ViewWinNr=0; ViewWinNr<m_ViewWindows.Size(); ViewWinNr++)
    {
        ViewWindowT*   ViewWin  =m_ViewWindows[ViewWinNr];
        ViewWindow2DT* ViewWin2D=dynamic_cast<ViewWindow2DT*>(ViewWin);

        if (ViewWin2D)
        {
            ViewWin2D->SetZoom(ZoomFactor);
            ViewWin2D->Refresh();
        }
    }
}


void ChildFrameT::All2DViews_Center(const Vector3fT& CenterPoint)
{
    for (unsigned long ViewWinNr=0; ViewWinNr<m_ViewWindows.Size(); ViewWinNr++)
    {
        ViewWindowT*   ViewWin  =m_ViewWindows[ViewWinNr];
        ViewWindow2DT* ViewWin2D=dynamic_cast<ViewWindow2DT*>(ViewWin);

        if (ViewWin2D)
        {
            ViewWin2D->CenterView(CenterPoint);
         // ViewWin2D->Refresh();   // Already in CenterView().
        }
    }
}


bool ChildFrameT::IsPaneShown(wxWindow* Pane)
{
    wxAuiPaneInfo& PaneInfo=m_AUIManager.GetPane(Pane);

    if (!PaneInfo.IsOk()) return false;

    return PaneInfo.IsShown();
}


void ChildFrameT::ShowPane(wxWindow* Pane, bool DoShow)
{
    wxAuiPaneInfo& PaneInfo=m_AUIManager.GetPane(Pane);

    if (!PaneInfo.IsOk()) return;

    PaneInfo.Show(DoShow);

    if (DoShow && PaneInfo.IsFloating() && PaneInfo.floating_pos==wxDefaultPosition)
        PaneInfo.FloatingPosition(ClientToScreen(wxPoint(20, 20)));

    m_AUIManager.Update();
}


void ChildFrameT::SetCaption(wxWindow* Pane, const wxString& Caption)
{
    wxAuiPaneInfo& PaneInfo=m_AUIManager.GetPane(Pane);

    if (!PaneInfo.IsOk()) return;

    PaneInfo.Caption(Caption);
    m_AUIManager.Update();
}


/**********************/
/*** Event Handlers ***/
/**********************/

void ChildFrameT::OnClose(wxCloseEvent& CE)
{
    if (!CE.CanVeto())
    {
        Destroy();
        return;
    }

    if (m_LastSavedAtCommandNr == m_History->GetLastSaveSuggestedCommandID())
    {
        // Our document has not been modified since the last save - close this window.
        Destroy();
        return;
    }

    // This "Save Confirmation Alert" essentially follows the GNOME Human Interface Guidelines,
    // see http://developer.gnome.org/hig-book/ for details.
    wxMessageDialog Msg(NULL, "Save changes to map \"" + m_Doc->GetFileName() + "\" before closing?", "CaWE Map Editor", wxYES_NO | wxCANCEL);

    Msg.SetExtendedMessage("If you close without saving, your changes will be discarded.");
    Msg.SetYesNoLabels("Save", "Close without Saving");

    switch (Msg.ShowModal())
    {
        case wxID_YES:
            if (!m_Doc->Save())
            {
                // The document could not be saved - keep the window open.
                CE.Veto();
                return;
            }

            if (m_LastSavedAtCommandNr != m_History->GetLastSaveSuggestedCommandID())
            {
                // The save was successful, but maybe it was a map export rather than a native cmap save.
                // In this case, also keep the window open.
                wxMessageBox("The map was exported, not saved. Please save the map before leaving.", "Map not closed.", wxOK | wxICON_INFORMATION);
                CE.Veto();
                return;
            }

            // The map was successfully saved - close the window.
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


void ChildFrameT::OnIdle(wxIdleEvent& IE)
{
    // We have a compile process running - forward its output to the console.
    // This code is duplicated in ChildFrameT::OnProcessEnd(), to make sure we don't miss anything.
    if (CurrentProcess && CurrentProcess->IsInputAvailable())
    {
        wxInputStream* InStream=CurrentProcess->GetInputStream();
        char           Buffer[256];

        while (InStream->CanRead())
        {
            InStream->Read(Buffer, 255);

            size_t count=InStream->LastRead();

#ifdef __WXGTK__
            if (count==0) break;   // This apparently can happen under wxGTK; I don't know why.
#endif
            wxASSERT(count>0 && count<256);
            Buffer[count]=0;

            m_ConsoleDialog->Print(Buffer, wxLIGHT_GREY);
        }
    }

    // The following things are only performed if the child frame is the active childframe.
    ChildFrameT* ActiveChild=dynamic_cast<ChildFrameT*>(m_Parent->GetActiveChild());

    if (this==ActiveChild)
    {
        // Cache materials for the childframes game configuration (one material per idle event).
        m_Doc->GetGameConfig()->GetMatMan().LazilyUpdateProxies();

        // Update the map documents BSP tree (regarding map elements that have moved or otherwise changed their bounding-box).
        const unsigned long ElemsUpdated=m_Doc->GetBspTree()->Update();

        if (ElemsUpdated>0)
        {
            wxLogDebug("OrthoBspTreeT::Update() updated %lu map elements.", ElemsUpdated);
        }

        // Update all 3D views.
        for (unsigned long ViewWinNr=0; ViewWinNr<m_ViewWindows.Size(); ViewWinNr++)
        {
            ViewWindowT*   ViewWin  =m_ViewWindows[ViewWinNr];
            ViewWindow3DT* ViewWin3D=dynamic_cast<ViewWindow3DT*>(ViewWin);

            if (ViewWin3D)
            {
                ViewWin3D->Refresh(false);
                ViewWin3D->Update();
            }
        }
    }

    IE.RequestMore();
}


void ChildFrameT::OnProcessEnd(wxProcessEvent& PE)
{
    // Code duplicated from ChildFrameT::OnIdle(), to make sure we don't miss anything.
    if (CurrentProcess && CurrentProcess->IsInputAvailable())
    {
        wxInputStream* InStream=CurrentProcess->GetInputStream();
        char           Buffer[256];

        while (InStream->CanRead())
        {
            InStream->Read(Buffer, 255);

            size_t count=InStream->LastRead();

#ifdef __WXGTK__
            if (count==0) break;   // This apparently can happen under wxGTK; I don't know why.
#endif
            wxASSERT(count>0 && count<256);
            Buffer[count]=0;

            m_ConsoleDialog->Print(Buffer, wxLIGHT_GREY);
        }
    }

    if (PE.GetExitCode()!=0)
    {
        EndCompiling(wxString::Format("\nError: Map compilation process %i exited with code %i.\n", PE.GetPid(), PE.GetExitCode()), wxRED);
        return;
    }

    if (PendingCompileCommands.Size()==0)
    {
        EndCompiling("\nMap compilation ended successfully.");
        return;
    }

    m_ConsoleDialog->Print("\nMap compilation: Running command '"+PendingCompileCommands[0]+"'\n", wxBLUE);

    // Delete previous compile process.
    delete CurrentProcess;
    CurrentProcess=NULL;
    CurrentProcessID=0;

    // Start new compile process.
    CurrentProcess=new wxProcess(this);
    CurrentProcess->Redirect();

    if (PendingCompileCommands[0].StartsWith(Options.general.EngineExe))
        CurrentProcessID=wxExecute(PendingCompileCommands[0], wxEXEC_ASYNC | wxEXEC_NOHIDE, CurrentProcess);
    else
        CurrentProcessID=wxExecute(PendingCompileCommands[0], wxEXEC_ASYNC, CurrentProcess);

    PendingCompileCommands.RemoveAtAndKeepOrder(0);

    if (CurrentProcessID<=0)
        EndCompiling("\nCould not execute the map compilation process!\n", wxRED);
}


void ChildFrameT::OnMenuFile(wxCommandEvent& CE)
{
    bool SaveOK = false;

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
            SaveOK = m_Doc->Save();
            break;
        }

        case ID_MENU_FILE_SAVEAS:
        {
            SaveOK = m_Doc->SaveAs();
            break;
        }
    }

    if (SaveOK && m_Doc->GetFileName().Right(5).MakeLower() == ".cmap")
    {
        // Mark the document as "not modified" only if the save was successful AND in cmap file format.
        // All other file formats are exported formats, and may not be able to store all information that is stored in a cmap file.
        // Thus data loss may occur if the document is only saved in an exported, but not in cmap file format.
        // Considering file *exports* not as file *saves* (that mark the document as "not modified" when successful)
        // makes sure that on application exit, the user is warned about possible data loss in such cases.
        m_LastSavedAtCommandNr = m_History->GetLastSaveSuggestedCommandID();
        // m_FileName = FileName;   // A member of the document, updated in its Save() and SaveAs() methods.

        SetTitle(m_Doc->GetFileName());
        m_Parent->m_FileHistory.AddFileToHistory(m_Doc->GetFileName());
    }
}


void ChildFrameT::OnMenuFileUpdate(wxUpdateUIEvent& UE)
{
    switch (UE.GetId())
    {
        case ID_MENU_FILE_SAVE:
            UE.Enable(m_History->GetLastSaveSuggestedCommandID() != m_LastSavedAtCommandNr);
            break;
    }
}


void ChildFrameT::OnMenuEditUndoRedo(wxCommandEvent& CE)
{
    // The undo system doesn't keep track of selected faces, so clear the face selection just to be safe.
    if (GetToolManager().GetActiveToolType() == &ToolEditSurfaceT::TypeInfo)
        GetSurfacePropsDialog()->ClearSelection();

    // Step forward or backward in the command history.
    if (CE.GetId() == wxID_UNDO) m_History->Undo();
                            else m_History->Redo();

    SetTitle(m_Doc->GetFileName() + (m_History->GetLastSaveSuggestedCommandID() == m_LastSavedAtCommandNr ? "" : "*"));
}


void ChildFrameT::OnMenuEditCut(wxCommandEvent& CE)
{
    OnMenuEditCopy(CE);
    OnMenuEditDelete(CE);
}


namespace
{
    IntrusivePtrT<cf::GameSys::EntityT> FindCommonAnchestor(ArrayT<MapElementT*>& Elems)
    {
        IntrusivePtrT<cf::GameSys::EntityT> Anchestor = NULL;

        for (unsigned int ElemNr = 0; ElemNr < Elems.Size(); ElemNr++)
        {
            IntrusivePtrT<cf::GameSys::EntityT> Ent = Elems[ElemNr]->GetParent()->GetEntity();

            if (Elems[ElemNr]->GetType() == &MapEntRepresT::TypeInfo)
            {
                // Double-check that this is really a MapEntRepresT.
                wxASSERT(Elems[ElemNr]->GetParent()->GetRepres() == Elems[ElemNr]);

                // An entity can never be its own anchestor (see OnMenuEditCopy() for details).
                Ent = Ent->GetParent();
            }

            if (Ent == NULL)
            {
                return NULL;
            }

            if (ElemNr == 0)
            {
                Anchestor = Ent;
                continue;
            }

            while (Ent != NULL && Ent != Anchestor)
            {
                Ent = Ent->GetParent();
            }

            if (Ent == NULL)    // Ent is not in the tree of Anchestor.
            {
                Anchestor = Anchestor->GetParent();

                // All elements in Elems *must* be part of a single common tree!
                wxASSERT(Anchestor != NULL);
                if (Anchestor == NULL) return NULL;

                ElemNr--;
            }
        }

        return Anchestor;
    }
}


/**
This method copies all selected map elements into the clipboard.
Thoughts about the proper copying of map elements:

  - The main difficulty is that the selection (the source set of elements) can be a completely arbitrary subset
    of the world's entity hierarchy, and we still must get all hierarchical relationships and all transforms right.
    As explained in the comment at ToolSelectionT::CloneDrag(), we choose to recursively copy whole subtrees, as that
    approach is the easiest to implement and is in fact required when the "Copy" operation is immediately followed by
    a "Delete", as e.g. in a "Cut" command: With "Cut", we must copy exactly what is deleted, so that in fact we have
    to deal with whole subtrees. See the comment at ToolSelectionT::CloneDrag() for details.

  - Cross-document support with proper resource handling: Contrary to the Selection tool's clone-drag, we want to be
    able to "Paste" the objects into different parent entities, a different map document with possibly a different
    GameConfig, or even an entirely different Map Editor instance, which raises the question how we can deal with the
    resources (e.g. texture images) that are associated to the copied objects. The best solution seems to properly
    *serialize* the objects when they are copied, and to *unserialize* them when they are pasted. This:

      - properly deals with each object's resources, re-associating them in the context of the target map,
      - using the system clipboard, it allows us to inspect or even modify the clipboard contents in a text editor,
      - and we can re-use much of the (de-)serialization code that is already used for prefabs and normal map files.

  - Note that in some special cases, the behavior of this "Copy" operation is subtly different from the Selection
    tool's clone-drag: If there are multiple entities (e.g. lifts) and each entity has child entities (e.g. doors),
    and selected are not the lifts, but only the doors, then clone-dragging this selection will attach each cloned
    door to its proper parent lift, whereas "Copy" and "Paste" will attach all copied doors to a single target entity.
    Also, as we will see below, while the transforms of copied elements are left alone whenever possible, they *can*
    be modified in order to compensate for "missing intermediary parents".

  - It seems wise to keep all copied elements by attaching them to a common parent entity, the "ClipboardRoot":

      - a single "atomic handle" / "natural grouping" for them all,
      - "reflects" the target entity that the elements will be pasted into,
      - a natural place for keeping a common transform, required when primitives are present (see below),
      - already the right and required form to serialize and deserialize them like prefabs and maps.

  - If only a single entity is selected, copying that single entity would be enough:
    As the entity's (parent-space) transform is stored in the entity itself, at Paste time the mere act of attaching
    the copy to the target entity is enough to correctly position the copy relative to the target entity.
    If the copy is kept as a child of a ClipboardRoot entity, the ClipboardRoot's transform does not matter.
    As a favorable side effect, this meets the user's expectations that the transform of the copy is unchanged,
    that is, identical to that of the source entity.

  - If siblings (multiple entities with the same parent) are selected, the situation is analogous:
    As each entity's (parent-space) transform is stored in the entity itself, at Paste time the mere act of attaching
    the copies to the target entity is enough to correctly position the copies relative to the target entity *and*
    relative to each other.
    If the copies are kept as children of a ClipboardRoot entity, the ClipboardRoot's transform does not matter.

  - If an entity and the entity's nephew are selected (but not the nephew's parent), it is necessary to adjust the
    nephew's transform in order to compensate for its missing parent (as if the nephew was, at the same world-space
    position, a direct child of its grandparent) so that the copies can be correctly positioned both relative to the
    target entity and to each other. Similar is true if cousins are involved.
    If the copies are kept as children of a ClipboardRoot entity, the ClipboardRoot's transform does still not matter.

  - In all cases, observe how the "nearest" common anchestor of the selected entities (this can never be one of the
    selected entities itself, but is built from the *parents* of the selected entities!) corresponds to the future
    target entity at Paste time and is used as the reference entity to compensate transforms for missing intermediary
    parents as in the nephews and cousins example above. Therefore, it is this entity whose world-space transform the
    ClipboardRoot should reflect. Although still not a technical neccessity (it will become one in the next considered
    point), this is already well justified for the above reasons.

  - If a single primitive is selected, things look a bit differently: Primitives are defined in world-space.
    As such, if only the primitive was copied (and kept as a child of the ClipboardRoot), this would be enough to
    recreate the primitive in world-space, but not to position it correctly relative to the target entity (and/or
    relative to any entities that may be contained in the selection was well).
    Therefore, we have to keep the (world-space) transform of the primitive's related ("parent") entity, storing it in
    the ClipboardRoot. With that, the primitive can (at Paste time) be transformed into the space of its entity, and
    from there into the space of the target entity, so that eventually the primitive is correctly positioned relative
    to the target entity.

  - If multiple primitives are selected, their "nearest" common anchestor must be used. Note that for primitives, the
    finding of this anchestor starts at the entity that is directly related to (contains) the primitive, *not* at the
    parent of a primitive's related entity (as it would with entities).
*/
void ChildFrameT::OnMenuEditCopy(wxCommandEvent& CE)
{
    ArrayT<MapElementT*> SourceElems = m_Doc->GetSelection();

    MapDocumentT::Reduce(SourceElems);

    if (SourceElems.Size() == 0) return;

    IntrusivePtrT<cf::GameSys::EntityT> ClipboardRoot   = new cf::GameSys::EntityT(cf::GameSys::EntityCreateParamsT(m_Doc->GetScriptWorld()));
    IntrusivePtrT<CompMapEntityT>       ClipboardMapEnt = new CompMapEntityT(*m_Doc);
    IntrusivePtrT<cf::GameSys::EntityT> CommonAnchestor = FindCommonAnchestor(SourceElems);

    ClipboardRoot->SetApp(ClipboardMapEnt);
    ClipboardRoot->GetBasics()->SetEntityName("Clipboard");

    if (CommonAnchestor != NULL)
    {
        // Make sure that the common sequence "Ctrl+C, Ctrl+V" creates siblings, not descendants.
        m_Doc->SetPasteParent(CommonAnchestor->GetID());

        ClipboardRoot->GetTransform()->SetOriginWS(CommonAnchestor->GetTransform()->GetOriginWS());
        ClipboardRoot->GetTransform()->SetQuatWS(CommonAnchestor->GetTransform()->GetQuatWS());
    }

    for (unsigned int ElemNr = 0; ElemNr < SourceElems.Size(); ElemNr++)
    {
        MapElementT* Elem = SourceElems[ElemNr];

        if (Elem->GetType() == &MapEntRepresT::TypeInfo)
        {
            // Double-check that this is really a MapEntRepresT.
            wxASSERT(Elem->GetParent()->GetRepres() == Elem);

            IntrusivePtrT<cf::GameSys::EntityT> OldEnt = Elem->GetParent()->GetEntity();
            IntrusivePtrT<cf::GameSys::EntityT> NewEnt = new cf::GameSys::EntityT(*OldEnt, true /*Recursive*/);

            GetMapEnt(NewEnt)->CopyPrimitives(*GetMapEnt(OldEnt), true /*Recursive*/);

            ClipboardRoot->AddChild(NewEnt);

            // As only the transforms of grandchildren of the common anchestor must be compensated,
            // this leaves the transforms unmodified whenever possible.
            if (OldEnt->GetParent() != CommonAnchestor)
            {
                NewEnt->GetTransform()->SetOriginWS(OldEnt->GetTransform()->GetOriginWS());
                NewEnt->GetTransform()->SetQuatWS(OldEnt->GetTransform()->GetQuatWS());
            }
        }
        else
        {
            // Double-check that this is really *not* a MapEntRepresT.
            wxASSERT(Elem->GetParent()->GetRepres() != Elem);

            MapPrimitiveT* OldPrim = dynamic_cast<MapPrimitiveT*>(Elem);
            MapPrimitiveT* NewPrim = OldPrim->Clone();

            ClipboardMapEnt->AddPrim(NewPrim);
        }
    }


    std::ostringstream out;

    MapDocumentT::SaveEntities(out, ClipboardRoot);

    out << "\n";
    out << "-- -- -- End of `cent` file, begin of `cmap` file. -- -- --\n";
    out << "\n";

    out << "// Cafu Map File\n"
        << "// Written by CaWE, the Cafu World Editor.\n"
        << "Version " << MapDocumentT::CMAP_FILE_VERSION << "\n"
        << "\n";

    // Save entities (in depth-first order, as in the .cent file).
    ArrayT< IntrusivePtrT<cf::GameSys::EntityT> > AllScriptEnts;
    ClipboardRoot->GetAll(AllScriptEnts);

    for (unsigned long EntNr = 0; EntNr < AllScriptEnts.Size(); EntNr++)
        GetMapEnt(AllScriptEnts[EntNr])->Save_cmap(*m_Doc, out, EntNr, NULL);

    if (wxTheClipboard->Open())
    {
        // The wxTextDataObject instance is owned (and deleted) by the clipboard.
        wxTheClipboard->SetData(new wxTextDataObject(out.str()));
        wxTheClipboard->Close();
    }
}


namespace
{
    IntrusivePtrT<cf::GameSys::EntityT> GetClipboardEntity(MapDocumentT& MapDoc)
    {
        try
        {
            if (!wxTheClipboard->Open())
                throw cf::GameSys::WorldT::InitErrorT("Could not open the clipboard.");

            if (!wxTheClipboard->IsSupported(wxDF_TEXT))
                throw cf::GameSys::WorldT::InitErrorT("The clipboard is empty.");

            wxTextDataObject TextData;

            wxTheClipboard->GetData(TextData);
            wxTheClipboard->Close();

            const std::string Delimiter = "-- -- -- End of `cent` file, begin of `cmap` file. -- -- --\n";
            const std::string ClipboardString = TextData.GetText().ToStdString();   // g++ complains without the `.ToStdString()`.
            const std::size_t SplitPos = ClipboardString.find(Delimiter);

            if (SplitPos == std::string::npos)
                throw cf::GameSys::WorldT::InitErrorT("Portions delimiter not found.");

            const std::string Clipboard_cent = ClipboardString.substr(0, SplitPos);
            const std::string Clipboard_cmap = ClipboardString.substr(SplitPos + Delimiter.length());

            TextParserT TP(Clipboard_cmap.c_str(), "({})", false /*not a filename, but direct input*/);

            if (TP.IsAtEOF())
                throw cf::GameSys::WorldT::InitErrorT("The cmap portion is empty.");

            unsigned int cmapFileVersion = 0;
            ArrayT< IntrusivePtrT<CompMapEntityT> > AllMapEnts;

            try
            {
                while (!TP.IsAtEOF())
                {
                    IntrusivePtrT<CompMapEntityT> Entity = new CompMapEntityT(MapDoc);

                    Entity->Load_cmap(TP, MapDoc, NULL, AllMapEnts.Size(), cmapFileVersion, true /*IgnoreGroups?*/);
                    AllMapEnts.PushBack(Entity);
                }
            }
            catch (const TextParserT::ParseError&)
            {
                throw cf::GameSys::WorldT::InitErrorT("Parse error in the cmap portion.");
            }

            if (cmapFileVersion < 14)
                throw cf::GameSys::WorldT::InitErrorT("Bad version in the cmap portion.");

            // Load the related `cent` file.
            IntrusivePtrT<cf::GameSys::EntityT> ClipboardRoot = MapDoc.GetScriptWorld().LoadScript(
                Clipboard_cent,   // Note the important "AsPrefab" flag in the line below!
                cf::GameSys::WorldT::InitFlag_InlineCode | cf::GameSys::WorldT::InitFlag_OnlyStatic | cf::GameSys::WorldT::InitFlag_AsPrefab);

            // Assign the "AllMapEnts" to the "AllScriptEnts".
            {
                ArrayT< IntrusivePtrT<cf::GameSys::EntityT> > AllScriptEnts;
                ClipboardRoot->GetAll(AllScriptEnts);

                if (AllScriptEnts.Size() != AllMapEnts.Size())
                    throw cf::GameSys::WorldT::InitErrorT("The entity definitions in the `cent` and `cmap` portions don't match.");

                for (unsigned int EntNr = 0; EntNr < AllScriptEnts.Size(); EntNr++)
                {
                    wxASSERT(AllScriptEnts[EntNr]->GetApp().IsNull());
                    AllScriptEnts[EntNr]->SetApp(AllMapEnts[EntNr]);
                }
            }

            return ClipboardRoot;
        }
        catch (const cf::GameSys::WorldT::InitErrorT& IE)
        {
            wxMessageBox(wxString("The clipboard contents could not be pasted:\n") + IE.what(), "Could not paste the clipboard contents");
        }

        return NULL;
    }


    BoundingBox3fT GetClipboardBB(IntrusivePtrT<cf::GameSys::EntityT> Clipboard, bool InvResult, const MatrixT& WorldToWorld)
    {
        BoundingBox3fT BB;

        const ArrayT<MapElementT*> Elems = GetMapEnt(Clipboard)->GetAllMapElements();

        for (unsigned int ElemNr = 0; ElemNr < Elems.Size(); ElemNr++)
        {
            const MapElementT* Elem = Elems[ElemNr];

            if (ElemNr == 0)
            {
                wxASSERT(Elem == GetMapEnt(Clipboard)->GetRepres());
                continue;
            }

            wxASSERT(Elem != GetMapEnt(Clipboard)->GetRepres());

            BB.InsertValid(Elem->GetBB());
        }

        if (!BB.IsInited()) return BB;
        if (!InvResult) return BB;

        Vector3fT Corners[8];
        BB.GetCornerVertices(Corners);

        BB = BoundingBox3fT(WorldToWorld.Mul1(Corners[0]));

        for (unsigned int i = 1; i < 8; i++)
            BB += WorldToWorld.Mul1(Corners[i]);

        return BB;
    }
}


void ChildFrameT::OnMenuEditPaste(wxCommandEvent& CE)
{
    bool MakeWellVisible = true;

    if (CE.GetId() == ID_MENU_EDIT_PASTE_SPECIAL)
    {
        PasteSpecialDialogT PSD;

        if (PSD.ShowModal() == wxID_CANCEL) return;

        MakeWellVisible = PSD.MakePastedElementsWellVisible();
    }

    IntrusivePtrT<cf::GameSys::EntityT> PasteParent = m_Doc->GetPasteParent();
    IntrusivePtrT<cf::GameSys::EntityT> Clipboard   = GetClipboardEntity(*m_Doc);

    if (Clipboard == NULL) return;

    // Primitives must be transformed from world-space (in which they are defined) into the
    // local space of the clipboard entity. As the clipboard entity's local space is then
    // mentally aligned to that of the PasteParent (the clipboard entity is oriented such that
    // its axes are congruent with that of the PasteParent), the primitives must from there be
    // transformed back into world-space.
    // Note that we right-multiply, that is, PP2W * W2Cb * v, thus the matrix order.
    bool          InvResult    = true;
    const MatrixT WorldToWorld = PasteParent->GetTransform()->GetEntityToWorld() * Clipboard->GetTransform()->GetEntityToWorld().GetInverse(&InvResult);

    if (WorldToWorld.IsEqual(MatrixT(), 0.001f))
    {
        // If WorldToWorld is (close to) the identity anyway, skip the Prim->Transform() calls below,
        // both as a performance optimization but mostly in order to not negatively impact numerical precision.
        InvResult = false;
    }

    // Determine the bounding-box of the clipboard's contents, transformed to (target) world-space.
    // Note that bounding-boxes can only be determined while the entities are attached to a proper parent.
    // Once they have been detached from the Clipboard entity below, a bounding-box can only be determined
    // again after the commands that attach them to the PasteParent have run.
    const BoundingBox3fT ClipboardBB = GetClipboardBB(Clipboard, InvResult, WorldToWorld);
    const unsigned long  NumToplevel = Clipboard->GetChildren().Size() + GetMapEnt(Clipboard)->GetPrimitives().Size();

    ArrayT<CommandT*>    SubCommands;
    ArrayT<MapElementT*> SelElems;

    while (Clipboard->GetChildren().Size() > 0)
    {
        IntrusivePtrT<cf::GameSys::EntityT> Ent = Clipboard->GetChildren()[0];

        Clipboard->RemoveChild(Ent);

        const ArrayT<MapElementT*> Elems = GetMapEnt(Ent)->GetAllMapElements();

        if (InvResult)
        {
            for (unsigned int ElemNr = 0; ElemNr < Elems.Size(); ElemNr++)
            {
                MapPrimitiveT* Prim = dynamic_cast<MapPrimitiveT*>(Elems[ElemNr]);

                if (Prim) Prim->Transform(WorldToWorld, Options.general.LockingTextures);
            }
        }

        SelElems.PushBack(Elems);
        SubCommands.PushBack(new CommandNewEntityT(*m_Doc, Ent, PasteParent, false /*don't select*/));
    }

    while (GetMapEnt(Clipboard)->GetPrimitives().Size() > 0)
    {
        MapPrimitiveT* Prim = GetMapEnt(Clipboard)->GetPrimitives()[0];

        GetMapEnt(Clipboard)->RemovePrim(Prim);

        if (InvResult)
        {
            Prim->Transform(WorldToWorld, Options.general.LockingTextures);
        }

        SelElems.PushBack(Prim);
        SubCommands.PushBack(new CommandAddPrimT(*m_Doc, Prim, GetMapEnt(PasteParent), "insert prims", false /*don't select*/));
    }

    if (SelElems.Size() > 0)
    {
        if (MakeWellVisible && ClipboardBB.IsInited())  // Translate to where the user can well see the pasted objects.
        {
            const Vector3fT GoodPastePos = GuessUserVisiblePoint();

            static Vector3fT    LastPastePoint(0, 0, 0);
            static unsigned int LastPasteCount = 0;

            if (GoodPastePos != LastPastePoint)
            {
                LastPastePoint = GoodPastePos;
                LastPasteCount = 0;
            }

            int PasteOffset = std::max(m_Doc->GetGridSpacing(), 1);

            while (PasteOffset < 8)
                PasteOffset *= 2;   // Make PasteOffset some multiple of the grid spacing larger than 8.0.

            const Vector3fT TotalTranslation = m_Doc->SnapToGrid(
                LastPastePoint
                + Vector3fT(((LastPasteCount % 8)+(LastPasteCount/8)) * PasteOffset, (LastPasteCount % 8) * PasteOffset, 0.0f),
                false, -1 /*Snap all axes.*/) - ClipboardBB.GetCenter();

            SubCommands.PushBack(new CommandTransformT(*m_Doc, SelElems, CommandTransformT::MODE_TRANSLATE, Vector3fT(), TotalTranslation, Options.general.LockingTextures));
            LastPasteCount++;
        }
    }

    if (NumToplevel > 1 /*&& Options.general.AutoGroupNewElements*/)
    {
        // TODO: We probably should have an Options.general.AutoGroupNewElements option that is accounted for
        // wherever new map elements are possibly auto-grouped: in the Carve and MakeHollow commands, the
        // "New Brush" and "New Bezier Patch" tools, and for Pasting objects from the clipboard.
        CommandNewGroupT* CmdNewGroup = new CommandNewGroupT(*m_Doc, "pasted elements");

        CmdNewGroup->GetGroup()->SelectAsGroup = true;
        SubCommands.PushBack(CmdNewGroup);

        CommandAssignGroupT* CmdAssignGroup = new CommandAssignGroupT(*m_Doc, SelElems, CmdNewGroup->GetGroup());

        SubCommands.PushBack(CmdAssignGroup);
    }

    if (SelElems.Size() > 0)
    {
        SubCommands.PushBack(CommandSelectT::Set(m_Doc, SelElems));
    }

    if (SubCommands.Size() > 0)
    {
        // Submit the composite macro command.
        SubmitCommand(new CommandMacroT(SubCommands, "Paste"));
    }

    GetToolManager().SetActiveTool(GetToolTIM().FindTypeInfoByName("ToolSelectionT"));
}


void ChildFrameT::OnMenuEditDelete(wxCommandEvent& CE)
{
    // If the camera tool is the currently active tool, delete its active camera.
    ToolCameraT* CameraTool = dynamic_cast<ToolCameraT*>(GetToolManager().GetActiveTool());

    if (CameraTool)
    {
        CameraTool->DeleteActiveCamera();
        return;
    }

    if (m_Doc->GetSelection().Size() > 0)
    {
        // Do the actual deletion of the selected elements.
        SubmitCommand(new CommandDeleteT(*m_Doc, m_Doc->GetSelection()));

        // If there are any empty groups (usually as a result from the deletion), purge them now.
        // We use an explicit command for deleting the groups (instead of putting everything into a macro command)
        // so that the user has the option to undo the purge (separately from the deletion) if he wishes.
        const ArrayT<GroupT*> EmptyGroups = m_Doc->GetAbandonedGroups();

        if (EmptyGroups.Size() > 0)
            SubmitCommand(new CommandDeleteGroupT(*m_Doc, EmptyGroups));
    }
}


void ChildFrameT::OnMenuEditSelectNone(wxCommandEvent& CE)
{
    if (GetToolManager().GetActiveToolType() != &ToolEditSurfaceT::TypeInfo)
    {
        SubmitCommand(CommandSelectT::Clear(m_Doc));
    }
    else
    {
        GetSurfacePropsDialog()->ClearSelection();
    }
}


void ChildFrameT::OnMenuEditSelectAll(wxCommandEvent& CE)
{
    ArrayT<MapElementT*> NewSelection;
    m_Doc->GetAllElems(NewSelection);

    // This used to remove the world entity (representation) at index 0,
    // but we don't want to make this exception any longer.
    // if (NewSelection.Size() > 0)
    // {
    //     wxASSERT(NewSelection[0]->GetType() == &MapEntRepresT::TypeInfo);
    //     NewSelection.RemoveAt(0);
    // }

    // Remove all invisible elements.
    for (unsigned long ElemNr = 0; ElemNr < NewSelection.Size(); ElemNr++)
        if (!NewSelection[ElemNr]->IsVisible())
        {
            NewSelection.RemoveAt(ElemNr);
            ElemNr--;
        }

    m_History->SubmitCommand(CommandSelectT::Add(m_Doc, NewSelection));
}


void ChildFrameT::OnMenuEditUpdate(wxUpdateUIEvent& UE)
{
    switch (UE.GetId())
    {
        case wxID_UNDO:
        {
            const CommandT* Cmd = m_History->GetUndoCommand();

            UE.SetText(Cmd != NULL ? "Undo " + Cmd->GetName() + "\tCtrl+Z" : "Cannot Undo\tCtrl+Z");
            UE.Enable(Cmd != NULL);
            break;
        }

        case wxID_REDO:
        {
            const CommandT* Cmd = m_History->GetRedoCommand();

            UE.SetText(Cmd != NULL ? "Redo " + Cmd->GetName() + "\tCtrl+Y" : "Cannot Redo\tCtrl+Y");
            UE.Enable(Cmd != NULL);
            break;
        }

        case wxID_CUT:
        case wxID_COPY:
        case ID_MENU_EDIT_DELETE:
        {
            UE.Enable(m_Doc->GetSelection().Size() > 0 &&
                      GetToolManager().GetActiveToolType() != &ToolEditSurfaceT::TypeInfo);
            break;
        }

        case wxID_PASTE:
        case ID_MENU_EDIT_PASTE_SPECIAL:
        {
            if (!wxTheClipboard->Open())
            {
                UE.Enable(false);
                break;
            }

            UE.Enable(wxTheClipboard->IsSupported(wxDF_TEXT));
            wxTheClipboard->Close();
            break;
        }
    }
}


void ChildFrameT::PaneToggleShow(wxAuiPaneInfo& PaneInfo)
{
    if (!PaneInfo.IsOk()) return;

    const bool DoShow=!PaneInfo.IsShown();
    PaneInfo.Show(DoShow);

    if (DoShow && PaneInfo.IsFloating() && PaneInfo.floating_pos==wxDefaultPosition)
        PaneInfo.FloatingPosition(ClientToScreen(wxPoint(20, 20)));

    m_AUIManager.Update();
}


void ChildFrameT::OnMenuView(wxCommandEvent& CE)
{
    switch (CE.GetId())
    {
        case ID_MENU_VIEW_TOOLBARS_FILE:
            PaneToggleShow(m_AUIManager.GetPane("File Toolbar"));
            break;

        case ID_MENU_VIEW_TOOLBARS_TOOLS:
            PaneToggleShow(m_AUIManager.GetPane("Tools Toolbar"));
            break;

        case ID_MENU_VIEW_TOOLBARS_TOOLOPTIONS:
            PaneToggleShow(m_AUIManager.GetPane("Tool Options Toolbar"));
            break;

        case ID_MENU_VIEW_PANELS_ENTITY_HIERARCHY:
            PaneToggleShow(m_AUIManager.GetPane(m_EntityHierarchyDialog));
            break;

        case ID_MENU_VIEW_PANELS_ENTITY_INSPECTOR:
            PaneToggleShow(m_AUIManager.GetPane(m_InspectorDialog));
            break;

        case ID_MENU_VIEW_PANELS_MATERIALS:
            PaneToggleShow(m_AUIManager.GetPane(m_MaterialsToolbar));
            break;

        case ID_MENU_VIEW_PANELS_GROUPS:
            PaneToggleShow(m_AUIManager.GetPane(m_GroupsToolbar));
            break;

        case ID_MENU_VIEW_PANELS_CONSOLE:
            PaneToggleShow(m_AUIManager.GetPane(m_ConsoleDialog));
            break;

        case ID_MENU_VIEW_NEW_2D_VIEW:
        {
            ViewWindow2DT* NewView=new ViewWindow2DT(this, this, ViewWindowT::VT_2D_XY);
            m_AUIManager.AddPane(NewView, wxAuiPaneInfo().
                                 // Name("xy").
                                 Caption(NewView->GetCaption()).
                                 DestroyOnClose().Float().MaximizeButton().MinimizeButton());

            m_AUIManager.Update();
            break;
        }

        case ID_MENU_VIEW_NEW_3D_VIEW:
        {
            ViewWindow3DT* NewView=new ViewWindow3DT(this, this, new CameraT(), (ViewWindowT::ViewTypeT)wxConfigBase::Get()->Read("Splitter/ViewType00", ViewWindowT::VT_3D_FULL_MATS));
            m_AUIManager.AddPane(NewView, wxAuiPaneInfo().
                                 // Name("xy").
                                 Caption(NewView->GetCaption()).
                                 DestroyOnClose().Float().MaximizeButton().MinimizeButton());

            m_AUIManager.Update();
            break;
        }

        case ID_MENU_VIEW_LOAD_DEFAULT_PERSPECTIVE:
            m_AUIManager.LoadPerspective(m_AUIDefaultPerspective);
            break;

        case ID_MENU_VIEW_LOAD_USER_PERSPECTIVE:
            m_AUIManager.LoadPerspective(wxConfigBase::Get()->Read("MapEditor_AUIUserPerspective", m_AUIManager.SavePerspective()));
            break;

        case ID_MENU_VIEW_SAVE_USER_PERSPECTIVE:
            wxConfigBase::Get()->Write("MapEditor_AUIUserPerspective", m_AUIManager.SavePerspective());
            break;

        case ID_MENU_VIEW_CENTER_2D_VIEWS:
            All2DViews_Center(m_Doc->GetMostRecentSelBB().GetCenter());
            break;

        case ID_MENU_VIEW_CENTER_3D_VIEWS:
        {
            const BoundingBox3fT& SelBB=m_Doc->GetMostRecentSelBB();
            const Vector3fT       Pos  =SelBB.GetCenter();

            for (unsigned long ViewWinNr=0; ViewWinNr<m_ViewWindows.Size(); ViewWinNr++)
            {
                ViewWindowT*   ViewWin  =m_ViewWindows[ViewWinNr];
                ViewWindow3DT* ViewWin3D=dynamic_cast<ViewWindow3DT*>(ViewWin);

                if (ViewWin3D)
                {
                    const CameraT& Camera=ViewWin3D->GetCamera();

                    ViewWin3D->MoveCamera(Pos-Camera.GetYAxis()*(std::max(Camera.ViewDirLength, length(SelBB.Max-SelBB.Min)/* /2.0f */)));
                    break;  // Only do this for the MRU 3D view - it really doesn't make sense to do it for all.
                }
            }

            break;
        }
    }
}


void ChildFrameT::OnMenuViewUpdate(wxUpdateUIEvent& UE)
{
    switch (UE.GetId())
    {
        case ID_MENU_VIEW_TOOLBARS_FILE:
            UE.Check(m_AUIManager.GetPane("File Toolbar").IsShown());
            break;

        case ID_MENU_VIEW_TOOLBARS_TOOLS:
            UE.Check(m_AUIManager.GetPane("Tools Toolbar").IsShown());
            break;

        case ID_MENU_VIEW_TOOLBARS_TOOLOPTIONS:
            UE.Check(m_AUIManager.GetPane("Tool Options Toolbar").IsShown());
            break;

        case ID_MENU_VIEW_PANELS_ENTITY_HIERARCHY:
            UE.Check(m_AUIManager.GetPane(m_EntityHierarchyDialog).IsShown());
            break;

        case ID_MENU_VIEW_PANELS_ENTITY_INSPECTOR:
            UE.Check(m_AUIManager.GetPane(m_InspectorDialog).IsShown());
            break;

        case ID_MENU_VIEW_PANELS_MATERIALS:
            UE.Check(m_AUIManager.GetPane(m_MaterialsToolbar).IsShown());
            break;

        case ID_MENU_VIEW_PANELS_GROUPS:
            UE.Check(m_AUIManager.GetPane(m_GroupsToolbar).IsShown());
            break;

        case ID_MENU_VIEW_PANELS_CONSOLE:
            UE.Check(m_AUIManager.GetPane(m_ConsoleDialog).IsShown());
            break;
    }
}


void ChildFrameT::OnMenuTools(wxCommandEvent& CE)
{
    // Find the tool whose wxWidgets event ID matches CE.GetId().
    const ArrayT<ToolT*>& Tools=m_ToolManager->GetTools();

    for (unsigned long ToolNr=0; ToolNr<Tools.Size(); ToolNr++)
    {
        ToolT* Tool=Tools[ToolNr];

        if (Tool->GetWxEventID()==CE.GetId())
        {
            m_ToolManager->SetActiveTool(Tool->GetType());
            break;
        }
    }
}


void ChildFrameT::OnMenuToolsUpdate(wxUpdateUIEvent& UE)
{
    const ToolT* ActiveTool=m_ToolManager->GetActiveTool();

    UE.Check(ActiveTool && ActiveTool->GetWxEventID()==UE.GetId());
}


void ChildFrameT::OnMenuComponents(wxCommandEvent& CE)
{
    const unsigned long           Nr = CE.GetId() - ID_MENU_COMPONENTS_FIRST;
    const cf::TypeSys::TypeInfoT* TI = cf::GameSys::GetComponentTIM().FindTypeInfoByNr(Nr);

    if (!TI)
    {
        wxMessageBox("Could not find a TypeInfo for this type number.", "Add component");
        return;
    }

    ArrayT<MapElementT*> Selection = m_Doc->GetSelection();

    // Remove all entities (and primitives) whose parents are in the selection as well.
    MapDocumentT::Reduce(Selection);

    // Remove all remaining primitives from the Selection, keep only the entities.
    for (unsigned int SelNr = 0; SelNr < Selection.Size(); SelNr++)
    {
        if (Selection[SelNr]->GetType() != &MapEntRepresT::TypeInfo)
        {
            Selection.RemoveAt(SelNr);
            SelNr--;
        }
    }

    if (Selection.Size() != 1)
    {
        wxMessageBox("Please select exactly one entity to add the component to.", "Add component");
        return;
    }

    IntrusivePtrT<cf::GameSys::ComponentBaseT> Comp = static_cast<cf::GameSys::ComponentBaseT*>(
        TI->CreateInstance(
            cf::TypeSys::CreateParamsT()));

    if (Comp.IsNull())
    {
        wxMessageBox("Could not instantiate the component.", "Add component");
        return;
    }

    SubmitCommand(new MapEditor::CommandAddComponentT(m_Doc, Selection[0]->GetParent()->GetEntity(), Comp));
}


void ChildFrameT::UpdatePrefabsMenu()
{
    m_PrefabsMenuPaths.Overwrite();

    for (int i = ID_MENU_PREFABS_PATH_FIRST; i <= ID_MENU_PREFABS_PATH_LAST; i++)
    {
        wxMenuItem* Item = m_PrefabsMenu->FindChildItem(i);

        if (!Item) break;
        m_PrefabsMenu->Destroy(Item);
    }

    const wxString BaseDir = m_Doc->GetGameConfig()->ModDir + "/Prefabs";
    wxArrayString  AllPrefabs;

    wxDir::GetAllFiles(BaseDir, &AllPrefabs, "*.cmap", wxDIR_FILES | wxDIR_DIRS /*but not wxDIR_HIDDEN*/);

    for (size_t i = 0; i < AllPrefabs.GetCount(); i++)
    {
        const int MenuID = ID_MENU_PREFABS_PATH_FIRST + i;

        // Limit the number of shown prefabs by the number of available menu IDs.
        if (MenuID > ID_MENU_PREFABS_PATH_LAST) break;

        // The returned AllPrefabs[i] are relative to the current working directory,
        // e.g. "Games/DeathMatch/Prefabs/PlayerStart.cmap".
        wxFileName Filename(AllPrefabs[i]);

        Filename.MakeRelativeTo(BaseDir);
        Filename.ClearExt();

        m_PrefabsMenu->Append(MenuID, Filename.GetFullPath(wxPATH_UNIX), "Load and insert this prefab");
        m_PrefabsMenuPaths.PushBack(AllPrefabs[i]);
    }
}


void ChildFrameT::LoadPrefab(const wxString& FileName)
{
    try
    {
        TextParserT TP(FileName.c_str(), "({})");

        if (TP.IsAtEOF())
            throw cf::GameSys::WorldT::InitErrorT("The file could not be opened.");

        unsigned int cmapFileVersion = 0;
        ArrayT< IntrusivePtrT<CompMapEntityT> > AllMapEnts;

        try
        {
            while (!TP.IsAtEOF())
            {
                IntrusivePtrT<CompMapEntityT> Entity = new CompMapEntityT(*m_Doc);

                Entity->Load_cmap(TP, *m_Doc, NULL, AllMapEnts.Size(), cmapFileVersion, true /*IgnoreGroups?*/);
                AllMapEnts.PushBack(Entity);
            }
        }
        catch (const TextParserT::ParseError&)
        {
            wxMessageBox(wxString::Format(
                "I'm sorry, but I was not able to load the map, due to a file error.\n"
                "Worse, I cannot even say where the error occured, except near byte %lu (%.3f%%) of the file.\n"
                "Later versions of CaWE will provide more detailed information.\n"
                "Please use a text editor to make sure that the file you tried to open is a proper cmap file,\n"
                "and/or post at the Cafu support forums.", TP.GetReadPosByte(), TP.GetReadPosPercent()*100.0),
                wxString("Could not load ")+FileName, wxOK | wxICON_EXCLAMATION);

            throw cf::GameSys::WorldT::InitErrorT("The file could not be parsed.");
        }

        if (cmapFileVersion < 14)
            throw cf::GameSys::WorldT::InitErrorT("Bad prefab (map) file version.");

        // Load the related `cent` file.
        wxString centFileName = FileName;

        if (centFileName.Replace(".cmap", ".cent") == 0)
            centFileName += ".cent";

        IntrusivePtrT<cf::GameSys::EntityT> PrefabRoot = m_Doc->GetScriptWorld().LoadScript(
            centFileName.ToStdString(),     // Note the important "AsPrefab" flag in the line below!
            cf::GameSys::WorldT::InitFlag_OnlyStatic | cf::GameSys::WorldT::InitFlag_AsPrefab);

        // Assign the "AllMapEnts" to the "AllScriptEnts".
        {
            ArrayT< IntrusivePtrT<cf::GameSys::EntityT> > AllScriptEnts;
            PrefabRoot->GetAll(AllScriptEnts);

            if (AllScriptEnts.Size() != AllMapEnts.Size())
                throw cf::GameSys::WorldT::InitErrorT("The entity definitions in the prefab's `cent` and `cmap` files don't match.");

            for (unsigned int EntNr = 0; EntNr < AllScriptEnts.Size(); EntNr++)
            {
                wxASSERT(AllScriptEnts[EntNr]->GetApp().IsNull());
                AllScriptEnts[EntNr]->SetApp(AllMapEnts[EntNr]);
            }
        }

        // A prefab, as any other entity, can have primitives and child entities.
        // If it is loaded into a map, it is attached to some entity (the PrefabParent, see below) as a child itself.
        //
        // An important consideration is that the PrefabParent may have any origin or orientation.
        // For the prefab itself, this is not a problem, because its own transform is by definition given in
        // parent-space, so that the mere act of attaching it to the PrefabParent is enough to cover all transform
        // issues.
        //
        // The prefab's primitives, however, are not as easily dealt with:
        // If a prefab is loaded *as a map*, we see that the map's world-space corresponds to the prefab's
        // parent-space, that is, the local space of PrefabParent!
        // This is also the space in which the prefab's primitives are given.
        //
        // Therefore, in order to attach the prefab to the PrefabParent, we must transform its primitives from the
        // PrefabParent's local space to world space. [A]
        //
        // As another detail, we may wish to apply a translation offset to the newly inserted entity, in order to avoid
        // that its origin perfectly aligns with the origin of the parent, or if the parent is the target map's root,
        // to not instantiate it at the world origin, but at a reasonable (user visible) location.
        // As can easily be seen from the above considerations, it is barely possible to apply such an offset in
        // world-space. Instead, the offset must be applied in the space of the prefab's parent, and therefore,
        // we must apply it *before* the transforms of [A] are actually done.
        IntrusivePtrT<cf::GameSys::EntityT>           PrefabParent = m_Doc->GetScriptWorld().GetRootEntity();
        Vector3fT                                     Offset       = GuessUserVisiblePoint();
        ArrayT< IntrusivePtrT<cf::GameSys::EntityT> > SelEnts      = m_Doc->GetSelectedEntities();

        if (SelEnts.Size() > 0)
        {
            const float gs = m_Doc->GetGridSpacing() * 2.0f;

            PrefabParent = SelEnts[0];
            Offset       = Vector3fT(gs, gs, 0.0f);
        }
        else
        {
            // PrefabParent is m_Doc->GetScriptWorld().GetRootEntity(), whose Transform component should always
            // represent the identity transform. As Offset must be specified in the space of the prefab's parent
            // entity, but GuessUserVisiblePoint() returns a point in world-space, we must apply the proper transform
            // if IsIdentity() is false.
            if (!PrefabParent->GetTransform()->IsIdentity())
                Offset = PrefabParent->GetTransform()->GetEntityToWorld().InvXForm(Offset);
        }

        ArrayT<MapElementT*> PrefabElems = GetMapEnt(PrefabRoot)->GetAllMapElements();

        // Move the PrefabRoot by Offset (all child entities are implicitly moved as well).
        PrefabRoot->GetTransform()->SetOriginPS(PrefabRoot->GetTransform()->GetOriginPS() + Offset);

        // Move all primitives by Offset.
        for (unsigned int ElemNr = 0; ElemNr < PrefabElems.Size(); ElemNr++)
        {
            MapPrimitiveT* Prim = dynamic_cast<MapPrimitiveT*>(PrefabElems[ElemNr]);

            if (Prim) Prim->TrafoMove(Offset, true /*LockTexCoords*/);
        }

        // Transform all primitives from parent-space to world-space.
        // (The PrefabRoot and its children implicitly undergo the same transformation
        //  by being attached to the PrefabParent below.)
        bool IsIdent = true;

        for (IntrusivePtrT<cf::GameSys::EntityT> Ent = PrefabParent; Ent != NULL; Ent = Ent->GetParent())
            IsIdent = IsIdent && Ent->GetTransform()->IsIdentity();

        if (!IsIdent)
        {
            const MatrixT PrefabToWorld = PrefabParent->GetTransform()->GetEntityToWorld();

            for (unsigned int ElemNr = 0; ElemNr < PrefabElems.Size(); ElemNr++)
            {
                MapPrimitiveT* Prim = dynamic_cast<MapPrimitiveT*>(PrefabElems[ElemNr]);

                if (Prim) Prim->Transform(PrefabToWorld, true /*LockTexCoords*/);
            }
        }

        // Insert the newly loaded prefab into the map.
        SubmitCommand(new CommandNewEntityT(*m_Doc, PrefabRoot, PrefabParent, true /*SetSel?*/));
    }
    catch (const cf::GameSys::WorldT::InitErrorT& IE)
    {
        wxMessageBox(wxString("The prefab file \"") + FileName + "\" could not be loaded:\n" + IE.what(), "Couldn't load prefab file");
    }
}


void ChildFrameT::OnMenuPrefabs(wxCommandEvent& CE)
{
    if (ID_MENU_PREFABS_PATH_FIRST <= CE.GetId() && CE.GetId() <= ID_MENU_PREFABS_PATH_LAST)
    {
        const unsigned int i = CE.GetId() - ID_MENU_PREFABS_PATH_FIRST;

        if (i < m_PrefabsMenuPaths.Size())
            LoadPrefab(m_PrefabsMenuPaths[i]);

        return;
    }

    switch (CE.GetId())
    {
        case ID_MENU_PREFABS_LOAD:
        {
            const wxString FileName = wxFileSelector(
                "Open Prefab",                      // Message.
                m_Doc->GetGameConfig()->ModDir + "/Prefabs",  // The default directory.
                "",                                 // The default file name.
                "",                                 // The default extension.
                "Cafu Prefabs (*.cmap)|*.cmap"      // The wildcard.
                "|All files (*.*)|*.*",
                wxFD_OPEN | wxFD_FILE_MUST_EXIST,
                this);

            if (FileName != "")
                LoadPrefab(FileName);

            break;
        }

        case ID_MENU_PREFABS_SAVE:
        {
            ArrayT<MapElementT*> Selection = m_Doc->GetSelection();

            MapDocumentT::Reduce(Selection);

            if (Selection.Size() != 1 || Selection[0]->GetType() != &MapEntRepresT::TypeInfo)
            {
                wxMessageBox(
                    "Select exactly one entity.\n\n"
                    "A prefab corresponds to exactly one entity (including all its components, primitives and child entities). "
                    "Make sure that exactly one entity is selected (the one that is to be saved as a prefab), then try again.",
                    "Save Prefab",
                    wxOK | wxICON_INFORMATION);
                break;
            }

            IntrusivePtrT<cf::GameSys::EntityT> SelEnt = Selection[0]->GetParent()->GetEntity();

            wxString FileName = wxFileSelector(
                "Save Prefab",                      // Message.
                m_Doc->GetGameConfig()->ModDir + "/Prefabs",    // The default directory.
                SelEnt->GetBasics()->GetEntityName(),           // The default file name.
                "",                                 // The default extension.
                "Cafu Prefabs (*.cmap)|*.cmap"      // The wildcard.
                "|All files (*.*)|*.*",
                wxFD_SAVE | wxFD_OVERWRITE_PROMPT,
                this);

            if (FileName == "")
                break;

            if (!wxFileName(FileName).HasExt())
                FileName += ".cmap";

            // It is not technically necessary, but reasonable and useful to save the prefab so that its local
            // space in the "source" map file is mapped to world-space in the prefab's newly saved map file.
            // For the prefab entity itself, which is saved as the root entity of its corresponding map file,
            // it is enough to set its transform to the identity. This implicitly covers all child entities of
            // the prefab as well.
            // However, as all primitives in the prefab's hierarchy are defined in world-space, we must transform
            // them explicitly into the new space.
            // In order to achieve all this and to not accidentally modify something in the original source map,
            // we clone the source prefab and make all adjustments and processing with the temporary clone.
            IntrusivePtrT<cf::GameSys::EntityT> Prefab = new cf::GameSys::EntityT(*SelEnt, true /*Recursive*/);

            GetMapEnt(Prefab)->CopyPrimitives(*GetMapEnt(SelEnt), true /*Recursive*/);

            Prefab->GetTransform()->SetOriginPS(Vector3fT());
            Prefab->GetTransform()->SetQuatPS(cf::math::QuaternionfT());

            bool          InvResult     = true;
            const MatrixT WorldToEntity = SelEnt->GetTransform()->GetEntityToWorld().GetInverse(&InvResult);

            if (InvResult)
            {
                ArrayT<MapElementT*> PrefabElems = GetMapEnt(Prefab)->GetAllMapElements();

                for (unsigned int ElemNr = 0; ElemNr < PrefabElems.Size(); ElemNr++)
                {
                    MapPrimitiveT* Prim = dynamic_cast<MapPrimitiveT*>(PrefabElems[ElemNr]);

                    if (Prim) Prim->Transform(WorldToEntity, true /*LockTexCoords*/);
                }
            }

            if (m_Doc->OnSaveDocument(FileName, false, Prefab))
                UpdatePrefabsMenu();

            break;
        }
    }
}


void ChildFrameT::OnMenuCompile(wxCommandEvent& CE)
{
    switch (CE.GetId())
    {
        case ID_MENU_COMPILE_QUICK:
        case ID_MENU_COMPILE_NORMAL:
        case ID_MENU_COMPILE_QUALITY:
        case ID_MENU_COMPILE_CUSTOM:
        {
            if (CurrentProcess!=NULL)
            {
                wxMessageBox("Another process is already running.\nWait until it is finished, then try again.");
                return;
            }

            wxASSERT(PendingCompileCommands.Size()==0 && CurrentProcessID==0);

            if (CompileMenu->IsChecked(ID_MENU_COMPILE_FLAG_SAVE_MAP))
            {
                // Do a regular map save.
                m_Doc->Save();
            }

            CustomCompileDialogT CustomCompileDialog(this);

            // If compile mode is CUSTOM, show Custom Compile Options dialog.
            if (CE.GetId()==ID_MENU_COMPILE_CUSTOM)
            {
                if (CustomCompileDialog.ShowModal()!=wxID_OK) return;
            }

            // Console Fenster anzeigen.
            m_AUIManager.GetPane(m_ConsoleDialog).Show();
            m_AUIManager.Update();

            if (CE.GetId()==ID_MENU_COMPILE_QUICK  ) m_ConsoleDialog->Print(wxString("\n> compile (quick) "  )+m_Doc->GetFileName()+"\n");
            if (CE.GetId()==ID_MENU_COMPILE_NORMAL ) m_ConsoleDialog->Print(wxString("\n> compile (normal) " )+m_Doc->GetFileName()+"\n");
            if (CE.GetId()==ID_MENU_COMPILE_QUALITY) m_ConsoleDialog->Print(wxString("\n> compile (quality) ")+m_Doc->GetFileName()+"\n");
            if (CE.GetId()==ID_MENU_COMPILE_CUSTOM ) m_ConsoleDialog->Print(wxString("\n> compile (custom) " )+m_Doc->GetFileName()+"\n");


            // Test the document file name for validity, and construct the target .cw file name from it.
            wxFileName MapName(m_Doc->GetFileName());

            if (!MapName.IsOk())
            {
                m_ConsoleDialog->Print("Map file name \""+MapName.GetFullPath()+"\" is malformed.\n", wxRED);
                return;
            }

         // MapName.Normalize();
            MapName.MakeRelativeTo(".");   // Make it relative to the current working directory.

            if (MapName.GetExt()!="cmap")
            {
                m_ConsoleDialog->Print("Map file name \""+MapName.GetFullPath()+"\" doesn't end with \".cmap\".\n", wxRED);
                return;
            }

            wxFileName WorldName(m_Doc->GetGameConfig()->ModDir+"/Worlds/"+MapName.GetName()+".cw");
            WorldName.MakeRelativeTo("."); // Make it relative to the current working directory.


            // Create the compile commands.
            {
                if (CompileMenu->IsChecked(ID_MENU_COMPILE_FLAG_RUN_BSP))
                {
                    if (!wxFileExists(Options.general.BSPExe))
                    {
                        EndCompiling("\nMap compilation error: '"+Options.general.BSPExe+"' File not found.\n", wxRED);
                        return;
                    }

                    wxString Params="";

                    if (CE.GetId()==ID_MENU_COMPILE_CUSTOM) Params=" "+CustomCompileDialog.GetCaBSPOptions();

                    PendingCompileCommands.PushBack(Options.general.BSPExe+" \""+MapName.GetFullPath()+"\" \""+WorldName.GetFullPath()+"\""+Params);
                }

                if (CompileMenu->IsChecked(ID_MENU_COMPILE_FLAG_RUN_PVS))
                {
                    if (!wxFileExists(Options.general.PVSExe))
                    {
                        EndCompiling("\nMap compilation error: '"+Options.general.PVSExe+"' File not found.\n", wxRED);
                        return;
                    }

                    wxString Params="";

                    if (CE.GetId()==ID_MENU_COMPILE_QUICK)  Params=" -maxRecDepthSL 15";
                    if (CE.GetId()==ID_MENU_COMPILE_CUSTOM) Params=" "+CustomCompileDialog.GetCaPVSOptions();

                    PendingCompileCommands.PushBack(Options.general.PVSExe+" \""+WorldName.GetFullPath()+"\""+Params);
                }

                if (CompileMenu->IsChecked(ID_MENU_COMPILE_FLAG_RUN_LIGHT))
                {
                    if (!wxFileExists(Options.general.LightExe))
                    {
                        EndCompiling("\nMap compilation error: '"+Options.general.LightExe+"' File not found.\n", wxRED);
                        return;
                    }

                    wxFileName ModDirName=wxFileName::DirName(m_Doc->GetGameConfig()->ModDir);
                    wxString   Params    ="";

                    ModDirName.MakeRelativeTo(".");

                    if (CE.GetId()==ID_MENU_COMPILE_QUICK  ) Params=" -fast";
                    if (CE.GetId()==ID_MENU_COMPILE_QUALITY) Params=" -StopUE 0.1";
                    if (CE.GetId()==ID_MENU_COMPILE_CUSTOM ) Params=" "+CustomCompileDialog.GetCaLightOptions();

                    PendingCompileCommands.PushBack(Options.general.LightExe+" \""+WorldName.GetFullPath()+"\" -gd="+ModDirName.GetFullPath()+Params);
                }

                if (CompileMenu->IsChecked(ID_MENU_COMPILE_FLAG_RUN_ENGINE))
                {
                    if (!wxFileExists(Options.general.EngineExe))
                    {
                        EndCompiling("\nMap compilation error: '"+Options.general.EngineExe+"' File not found.\n", wxRED);
                        return;
                    }

                    wxString Params="";

                    if (CE.GetId()==ID_MENU_COMPILE_CUSTOM) Params=" "+CustomCompileDialog.GetEngineOptions();

                    PendingCompileCommands.PushBack(Options.general.EngineExe +
                        " --sv-game \"" + m_Doc->GetGameConfig()->Name + "\"" +
                        " --sv-world \"" + WorldName.GetName() + "\"" + Params);
                }
            }

            // Disable the compile buttons. These are re-enabled in ChildFrameT::EndCompiling().
            CompileMenu->Enable(ID_MENU_COMPILE_QUICK,   false);
            CompileMenu->Enable(ID_MENU_COMPILE_NORMAL,  false);
            CompileMenu->Enable(ID_MENU_COMPILE_QUALITY, false);
            CompileMenu->Enable(ID_MENU_COMPILE_CUSTOM,  false);
            CompileMenu->Enable(ID_MENU_COMPILE_ABORT,   true);

            // Create dummy process event and call end process to trigger compilation start.
            wxProcessEvent PE;  // Note: Is initialized with exit code 0.
            OnProcessEnd(PE);

            break;
        }

        case ID_MENU_COMPILE_ABORT:
        {
            if (CurrentProcessID==0)
            {
                wxMessageBox("There is currently no process running that could be aborted.");
                return;
            }

            m_AUIManager.GetPane(m_ConsoleDialog).Show();
            m_AUIManager.Update();

            wxKillError KillErr=wxProcess::Kill(CurrentProcessID, wxSIGKILL);
            m_ConsoleDialog->Print(wxString::Format("The compile process with ID %i has been terminated. Kill result: %i.\n", CurrentProcessID, KillErr), wxRED);

            // The TERM signal triggers a call to CurrentProcess->OnTerminate() and thus a call to ChildFrameT::OnProcessEnd().
            //     wxASSERT(CurrentProcess==NULL && CurrentProcessID==0);
            // doesn't work here though, because the event system must be running so that the event can actually be processed.
            break;
        }

        default:
            // We must also handle the ID_MENU_COMPILE_FLAG_* menu events here, locally in ChildFrameT.
            // Otherwise, the event is propagated to the parent window, where it's ID triggers an entirely unrelated action!
            break;
    }
}


void ChildFrameT::OnWindowActivate(wxActivateEvent& AE)
{
    AE.Skip();

    if (!AE.GetActive()) return;

    // To make sure the proper default material is always set, we reset it here according to current material toolbar MRU.
    ArrayT<EditorMaterialI*> Materials=m_MaterialsToolbar->GetMRUMaterials();

    wxASSERT(Materials.Size()>0);

    m_Doc->GetGameConfig()->GetMatMan().SetDefaultMaterial(Materials[0]);
}


void ChildFrameT::OnAuiPaneClose(wxAuiManagerEvent& AE)
{
    if (AE.GetPane()->window==m_SurfacePropsDialog || AE.GetPane()->window==m_TerrainEditorDialog)
    {
        // Reset the tool back to "Selection", this will close the specific panes.
        m_ToolManager->SetActiveTool(GetToolTIM().FindTypeInfoByName("ToolSelectionT"));
    }
    else
    {
        // Let the default action occur (the pane is closed).
        AE.Skip();
    }
}


/* void ChildFrameT::OnSize(wxSizeEvent& SE)
{
    // wxLogDebug("ChildFrameT::OnSize");
    // HorSplitterRatio=float(SplitterWindowParent->GetSashPosition())/float(GetClientSize().y);
    // VerSplitterRatio=float(SplitterWindowTop   ->GetSashPosition())/float(GetClientSize().x);
    // wxLogDebug("%f %f     %i %i       %i %i", HorSplitterRatio, VerSplitterRatio, GetClientSize().x, SplitterWindowParent->GetClientSize().y, SplitterWindowParent->GetSashPosition(), SplitterWindowTop->GetSashPosition());

    SplitterWindowParent->SetSashPosition(HorSplitterRatio*SplitterWindowParent->GetClientSize().y);
    SplitterWindowTop   ->SetSashPosition(VerSplitterRatio*SplitterWindowTop   ->GetClientSize().x);
    SplitterWindowBottom->SetSashPosition(VerSplitterRatio*SplitterWindowBottom->GetClientSize().x);

    SE.Skip();
}*/


void ChildFrameT::EndCompiling(const wxString& ConsoleMessage, const wxColour* Colour)
{
    delete CurrentProcess;
    CurrentProcess=NULL;
    CurrentProcessID=0;

    PendingCompileCommands.Clear();

    CompileMenu->Enable(ID_MENU_COMPILE_QUICK,   true);
    CompileMenu->Enable(ID_MENU_COMPILE_NORMAL,  true);
    CompileMenu->Enable(ID_MENU_COMPILE_QUALITY, true);
    CompileMenu->Enable(ID_MENU_COMPILE_CUSTOM,  true);
    CompileMenu->Enable(ID_MENU_COMPILE_ABORT,   false);

    m_ConsoleDialog->Print(ConsoleMessage, Colour);
}
