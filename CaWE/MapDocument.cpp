/*
=================================================================================
This file is part of Cafu, the open-source game engine and graphics engine
for multiplayer, cross-platform, real-time 3D action.
Copyright (C) 2002-2013 Carsten Fuchs Software.

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
#include "ChildFrameViewWin.hpp"
#include "Clipboard.hpp"
#include "CompMapEntity.hpp"
#include "DialogEditSurfaceProps.hpp"
#include "DialogInspector.hpp"
#include "EntityClass.hpp"
#include "GameConfig.hpp"
#include "DialogGotoPrimitive.hpp"
#include "DialogMapCheck.hpp"
#include "MapDocument.hpp"
#include "MapBezierPatch.hpp"
#include "MapEntityBase.hpp"
#include "MapEntRepres.hpp"
#include "MapModel.hpp"         // Only needed for some TypeInfo test...
#include "MapPlant.hpp"         // Only needed for some TypeInfo test...
#include "MapTerrain.hpp"       // Only needed for some TypeInfo test...
#include "EntityClassVar.hpp"
#include "DialogMapInfo.hpp"
#include "MapBrush.hpp"
#include "DialogOptions.hpp"
#include "Options.hpp"
#include "OrthoBspTree.hpp"
#include "DialogPasteSpecial.hpp"
#include "DialogReplaceMaterials.hpp"
#include "VarVisitorsLua.hpp"
#include "MapCommands/Transform.hpp"
#include "MapCommands/AddPrim.hpp"
#include "MapCommands/Align.hpp"
#include "MapCommands/ApplyMaterial.hpp"
#include "MapCommands/AssignPrimToEnt.hpp"
#include "MapCommands/Carve.hpp"
#include "MapCommands/Delete.hpp"
#include "MapCommands/Mirror.hpp"
#include "MapCommands/SnapToGrid.hpp"
#include "MapCommands/MakeHollow.hpp"
#include "MapCommands/NewEntity.hpp"
#include "MapCommands/Select.hpp"
#include "MapCommands/Group_Assign.hpp"
#include "MapCommands/Group_Delete.hpp"
#include "MapCommands/Group_New.hpp"
#include "MapCommands/Group_SetVisibility.hpp"

#include "ToolbarMaterials.hpp"     // Only needed for setting the ref to NULL in the dtor.
#include "ToolCamera.hpp"
#include "ToolEditSurface.hpp"
#include "ToolMorph.hpp"
#include "ToolManager.hpp"
#include "ToolOptionsBars.hpp"

#include "EditorMaterial.hpp"
#include "EditorMaterialManager.hpp"
#include "DialogTransform.hpp"
#include "Group.hpp"
#include "Camera.hpp"

#include "GameSys/Entity.hpp"
#include "GameSys/EntityCreateParams.hpp"
#include "GameSys/World.hpp"
#include "Math3D/Misc.hpp"
#include "Templates/Array.hpp"
#include "TextParser/TextParser.hpp"

#include "wx/wx.h"
#include "wx/datetime.h"
#include "wx/file.h"
#include "wx/filename.h"
#include "wx/numdlg.h"

extern "C"
{
    #include <lua.h>
    #include <lualib.h>
    #include <lauxlib.h>
}


#if defined(_WIN32) && defined(_MSC_VER)
    // Turn off warning 4355: "'this' : wird in Initialisierungslisten fuer Basisklasse verwendet".
    #pragma warning(disable:4355)
#endif


using namespace MapEditor;


/*static*/ const unsigned int MapDocumentT::CMAP_FILE_VERSION = 14;


BEGIN_EVENT_TABLE(MapDocumentT, wxEvtHandler)
    EVT_MENU  (wxID_UNDO,                                         MapDocumentT::OnEditUndoRedo)
    EVT_MENU  (wxID_REDO,                                         MapDocumentT::OnEditUndoRedo)
    EVT_MENU  (wxID_CUT,                                          MapDocumentT::OnEditCut)
    EVT_MENU  (wxID_COPY,                                         MapDocumentT::OnEditCopy)
    EVT_MENU  (wxID_PASTE,                                        MapDocumentT::OnEditPaste)
    EVT_MENU  (ChildFrameT::ID_MENU_EDIT_PASTE_SPECIAL,           MapDocumentT::OnEditPasteSpecial)
    EVT_MENU  (ChildFrameT::ID_MENU_EDIT_DELETE,                  MapDocumentT::OnEditDelete)
    EVT_BUTTON(ChildFrameT::ID_MENU_EDIT_DELETE,                  MapDocumentT::OnEditDelete)
    EVT_MENU  (ChildFrameT::ID_MENU_EDIT_SELECT_NONE,             MapDocumentT::OnEditSelectNone)
    EVT_MENU  (wxID_SELECTALL,                                    MapDocumentT::OnEditSelectAll)

    EVT_UPDATE_UI(wxID_UNDO,                                    MapDocumentT::OnUpdateEditUndoRedo)
    EVT_UPDATE_UI(wxID_REDO,                                    MapDocumentT::OnUpdateEditUndoRedo)
    EVT_UPDATE_UI(wxID_CUT,                                     MapDocumentT::OnUpdateEditCutCopyDelete)
    EVT_UPDATE_UI(wxID_COPY,                                    MapDocumentT::OnUpdateEditCutCopyDelete)
    EVT_UPDATE_UI(ChildFrameT::ID_MENU_EDIT_DELETE,             MapDocumentT::OnUpdateEditCutCopyDelete)
    EVT_UPDATE_UI(wxID_PASTE,                                   MapDocumentT::OnUpdateEditPasteSpecial)
    EVT_UPDATE_UI(ChildFrameT::ID_MENU_EDIT_PASTE_SPECIAL,      MapDocumentT::OnUpdateEditPasteSpecial)

    EVT_MENU  (ChildFrameT::ID_MENU_SELECTION_APPLY_MATERIAL,   MapDocumentT::OnSelectionApplyMaterial)
    EVT_BUTTON(ChildFrameT::ID_MENU_SELECTION_APPLY_MATERIAL,   MapDocumentT::OnSelectionApplyMaterial)

    EVT_UPDATE_UI(ChildFrameT::ID_MENU_SELECTION_APPLY_MATERIAL, MapDocumentT::OnUpdateSelectionApplyMaterial)

    EVT_MENU(ChildFrameT::ID_MENU_MAP_SNAP_TO_GRID,             MapDocumentT::OnMapSnapToGrid)
    EVT_MENU(ChildFrameT::ID_MENU_MAP_SHOW_GRID_2D,             MapDocumentT::OnMapToggleGrid2D)
    EVT_MENU(ChildFrameT::ID_MENU_MAP_FINER_GRID,               MapDocumentT::OnMapFinerGrid)
    EVT_MENU(ChildFrameT::ID_MENU_MAP_COARSER_GRID,             MapDocumentT::OnMapCoarserGrid)
    EVT_MENU(ChildFrameT::ID_MENU_MAP_GOTO_PRIMITIVE,           MapDocumentT::OnMapGotoPrimitive)
    EVT_MENU(ChildFrameT::ID_MENU_MAP_SHOW_INFO,                MapDocumentT::OnMapShowInfo)
    EVT_MENU(ChildFrameT::ID_MENU_MAP_CHECK_FOR_PROBLEMS,       MapDocumentT::OnMapCheckForProblems)
    EVT_MENU(ChildFrameT::ID_MENU_MAP_PROPERTIES,               MapDocumentT::OnMapProperties)
    EVT_MENU(ChildFrameT::ID_MENU_MAP_LOAD_POINTFILE,           MapDocumentT::OnMapLoadPointFile)
    EVT_MENU(ChildFrameT::ID_MENU_MAP_UNLOAD_POINTFILE,         MapDocumentT::OnMapUnloadPointFile)

    EVT_MENU  (ChildFrameT::ID_MENU_VIEW_SHOW_ENTITY_INFO,        MapDocumentT::OnViewShowEntityInfo)
    EVT_MENU  (ChildFrameT::ID_MENU_VIEW_SHOW_ENTITY_TARGETS,     MapDocumentT::OnViewShowEntityTargets)
    EVT_MENU  (ChildFrameT::ID_MENU_VIEW_HIDE_SELECTED_OBJECTS,   MapDocumentT::OnViewHideSelectedObjects)
    EVT_BUTTON(ChildFrameT::ID_MENU_VIEW_HIDE_SELECTED_OBJECTS,   MapDocumentT::OnViewHideSelectedObjects)
    EVT_MENU  (ChildFrameT::ID_MENU_VIEW_HIDE_UNSELECTED_OBJECTS, MapDocumentT::OnViewHideUnselectedObjects)
    EVT_BUTTON(ChildFrameT::ID_MENU_VIEW_HIDE_UNSELECTED_OBJECTS, MapDocumentT::OnViewHideUnselectedObjects)
    EVT_MENU  (ChildFrameT::ID_MENU_VIEW_SHOW_HIDDEN_OBJECTS,     MapDocumentT::OnViewShowHiddenObjects)

    EVT_UPDATE_UI(ChildFrameT::ID_MENU_VIEW_SHOW_ENTITY_INFO,     MapDocumentT::OnUpdateViewShowEntityInfo)
    EVT_UPDATE_UI(ChildFrameT::ID_MENU_VIEW_SHOW_ENTITY_TARGETS,  MapDocumentT::OnUpdateViewShowEntityTargets)

    EVT_MENU  (ChildFrameT::ID_MENU_TOOLS_CARVE,                  MapDocumentT::OnToolsCarve)
    EVT_MENU  (ChildFrameT::ID_MENU_TOOLS_MAKE_HOLLOW,            MapDocumentT::OnToolsHollow)
    EVT_MENU  (ChildFrameT::ID_MENU_TOOLS_GROUP,                  MapDocumentT::OnViewHideSelectedObjects)
    EVT_BUTTON(ChildFrameT::ID_MENU_TOOLS_GROUP,                  MapDocumentT::OnViewHideSelectedObjects)
    EVT_MENU  (ChildFrameT::ID_MENU_TOOLS_ASSIGN_PRIM_TO_ENTITY,  MapDocumentT::OnToolsAssignPrimToEntity)
    EVT_BUTTON(ChildFrameT::ID_MENU_TOOLS_ASSIGN_PRIM_TO_ENTITY,  MapDocumentT::OnToolsAssignPrimToEntity)
    EVT_MENU  (ChildFrameT::ID_MENU_TOOLS_ASSIGN_PRIM_TO_WORLD,   MapDocumentT::OnToolsAssignPrimToWorld)
    EVT_BUTTON(ChildFrameT::ID_MENU_TOOLS_ASSIGN_PRIM_TO_WORLD,   MapDocumentT::OnToolsAssignPrimToWorld)
    EVT_MENU  (ChildFrameT::ID_MENU_TOOLS_REPLACE_MATERIALS,      MapDocumentT::OnToolsReplaceMaterials)
    EVT_MENU  (ChildFrameT::ID_MENU_TOOLS_MATERIAL_LOCK,          MapDocumentT::OnToolsMaterialLock)
    EVT_MENU  (ChildFrameT::ID_MENU_TOOLS_SNAP_SELECTION_TO_GRID, MapDocumentT::OnToolsSnapSelectionToGrid)
    EVT_MENU  (ChildFrameT::ID_MENU_TOOLS_TRANSFORM,              MapDocumentT::OnToolsTransform)
    EVT_MENU  (ChildFrameT::ID_MENU_TOOLS_ALIGN_LEFT,             MapDocumentT::OnToolsAlign)
    EVT_MENU  (ChildFrameT::ID_MENU_TOOLS_ALIGN_RIGHT,            MapDocumentT::OnToolsAlign)
    EVT_MENU  (ChildFrameT::ID_MENU_TOOLS_ALIGN_HOR_CENTER,       MapDocumentT::OnToolsAlign)
    EVT_MENU  (ChildFrameT::ID_MENU_TOOLS_ALIGN_TOP,              MapDocumentT::OnToolsAlign)
    EVT_MENU  (ChildFrameT::ID_MENU_TOOLS_ALIGN_BOTTOM,           MapDocumentT::OnToolsAlign)
    EVT_MENU  (ChildFrameT::ID_MENU_TOOLS_ALIGN_VERT_CENTER,      MapDocumentT::OnToolsAlign)
    EVT_MENU  (ChildFrameT::ID_MENU_TOOLS_MIRROR_HOR,             MapDocumentT::OnToolsMirror)
    EVT_MENU  (ChildFrameT::ID_MENU_TOOLS_MIRROR_VERT,            MapDocumentT::OnToolsMirror)

    EVT_UPDATE_UI(ChildFrameT::ID_MENU_TOOLS_MATERIAL_LOCK, MapDocumentT::OnUpdateToolsMaterialLock)
END_EVENT_TABLE()


MapDocumentT::MapDocumentT(GameConfigT* GameConfig)
    : wxEvtHandler(),
      SubjectT(),
      m_ChildFrame(NULL),
      m_FileName("New Map"),
      m_ScriptWorld(NULL),
      m_BspTree(NULL),
      m_GameConfig(GameConfig),
      m_PlantDescrMan(std::string(m_GameConfig->ModDir)),
      m_History(),
      m_Selection(),
      m_SelectionBB(Vector3fT(-64.0f, -64.0f, 0.0f), Vector3fT(64.0f, 64.0f, 64.0f)),
      m_PointFilePoints(),
      m_PointFileColors(),
      m_SnapToGrid(true),
      m_GridSpacing(Options.Grid.InitialSpacing),
      m_ShowGrid(true)
{
    m_ScriptWorld = new cf::GameSys::WorldT(
        "MapEnt = world:new('EntityT', 'Map'); world:SetRootEntity(MapEnt);",
        m_GameConfig->GetModelMan(),
        cf::GameSys::WorldT::InitFlag_InlineCode | cf::GameSys::WorldT::InitFlag_InMapEditor);

    IntrusivePtrT<cf::GameSys::EntityT> ScriptRootEnt = m_ScriptWorld->GetRootEntity();

    MapEntityBaseT* Ent = new MapEntityBaseT(*this);

    const EntityClassT* WorldSpawnClass = GameConfig->FindClass("worldspawn");
    wxASSERT(WorldSpawnClass);
    Ent->SetClass(WorldSpawnClass != NULL ? WorldSpawnClass : FindOrCreateUnknownClass("worldspawn", false /*HasOrigin*/));

    wxASSERT(ScriptRootEnt->GetApp().IsNull());
    ScriptRootEnt->SetApp(new CompMapEntityT(Ent));

    ArrayT<MapElementT*> AllElems;
    GetAllElems(AllElems);

    m_BspTree = new OrthoBspTreeT(AllElems, m_GameConfig->GetMaxMapBB());
}


namespace
{
    /// Returns the entire hierarchy rooted at Entity in depth-first order.
    void GetAll(IntrusivePtrT<cf::GameSys::EntityT> Entity, ArrayT< IntrusivePtrT<cf::GameSys::EntityT> >& Result)
    {
        Result.PushBack(Entity);

        for (unsigned int ChildNr = 0; ChildNr < Entity->GetChildren().Size(); ChildNr++)
            GetAll(Entity->GetChildren()[ChildNr], Result);
    }
}


MapDocumentT::MapDocumentT(GameConfigT* GameConfig, wxProgressDialog* ProgressDialog, const wxString& FileName)
    : wxEvtHandler(),
      SubjectT(),
      m_ChildFrame(NULL),
      m_FileName(FileName),
      m_ScriptWorld(NULL),
      m_BspTree(NULL),
      m_GameConfig(GameConfig),
      m_PlantDescrMan(std::string(m_GameConfig->ModDir)),
      m_History(),
      m_Selection(),
      m_SelectionBB(Vector3fT(-64.0f, -64.0f, 0.0f), Vector3fT(64.0f, 64.0f, 64.0f)),
      m_PointFilePoints(),
      m_PointFileColors(),
      m_SnapToGrid(true),
      m_GridSpacing(Options.Grid.InitialSpacing),
      m_ShowGrid(true)
{
    // This sets the cursor to the busy cursor in its ctor, and back to the default cursor in the dtor.
    wxBusyCursor BusyCursor;
    TextParserT  TP(FileName.c_str(), "({})");

    if (TP.IsAtEOF())
    {
        delete m_ScriptWorld;
        m_ScriptWorld = NULL;

        throw cf::GameSys::WorldT::InitErrorT("The file could not be opened.");
    }

    unsigned int            cmapFileVersion = 0;
    ArrayT<MapEntityBaseT*> AllMapEnts;

    try
    {
        MapEntityBaseT* World = new MapEntityBaseT(*this);

        const EntityClassT* WorldSpawnClass = GameConfig->FindClass("worldspawn");
        wxASSERT(WorldSpawnClass);
        World->SetClass(WorldSpawnClass != NULL ? WorldSpawnClass : FindOrCreateUnknownClass("worldspawn", false /*HasOrigin*/));

        World->Load_cmap(TP, *this, ProgressDialog, 0, cmapFileVersion);
        AllMapEnts.PushBack(World);

        // Load the entities.
        while (!TP.IsAtEOF())
        {
            MapEntityBaseT* Entity = new MapEntityBaseT(*this);

            Entity->Load_cmap(TP, *this, ProgressDialog, AllMapEnts.Size(), cmapFileVersion);
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

        for (unsigned int EntNr = 0; EntNr < AllMapEnts.Size(); EntNr++)
        {
            delete AllMapEnts[EntNr];
            AllMapEnts[EntNr] = NULL;
        }

        delete m_ScriptWorld;   //XXX TODO: Call Cleanup() method instead (same code as dtor).
        m_ScriptWorld = NULL;

        throw cf::GameSys::WorldT::InitErrorT("The file could not be parsed.");
    }


    if (cmapFileVersion >= 14)
    {
        wxString centFileName = FileName;

        if (centFileName.Replace(".cmap", ".cent") == 0)
            centFileName += ".cent";

        m_ScriptWorld = new cf::GameSys::WorldT(
            centFileName.ToStdString(),
            m_GameConfig->GetModelMan(),
            cf::GameSys::WorldT::InitFlag_InMapEditor);
    }
    else
    {
        wxASSERT(cmapFileVersion >= 6);

        // Before `.cmap` file format version 14, related `.cent` files did not exist.
        m_ScriptWorld = new cf::GameSys::WorldT(
            "MapEnt = world:new('EntityT', 'Map'); world:SetRootEntity(MapEnt);",
            m_GameConfig->GetModelMan(),
            cf::GameSys::WorldT::InitFlag_InlineCode | cf::GameSys::WorldT::InitFlag_InMapEditor);
    }


    // Align the entities in the m_ScriptWorld and those in AllMapEnts with each other.
    {
        ArrayT< IntrusivePtrT<cf::GameSys::EntityT> > AllScriptEnts;

        GetAll(m_ScriptWorld->GetRootEntity(), AllScriptEnts);

        unsigned int EntNr = 0;

        // Sync as much as possible.
        while (EntNr < AllScriptEnts.Size() && EntNr < AllMapEnts.Size())
        {
            wxASSERT(AllScriptEnts[EntNr]->GetApp().IsNull());
            AllScriptEnts[EntNr]->SetApp(new CompMapEntityT(AllMapEnts[EntNr]));
            EntNr++;
        }

        if (cmapFileVersion >= 14 && AllScriptEnts.Size() != AllMapEnts.Size())
        {
            const wxString s = wxString::Format(
                "There are %lu entities in the `.cent` file and %lu entities in the `.cmap` file.\n\n"
                "The problem will be fixed automatically, but as this is normally the result of "
                "manual edits to one or both of these files (and can cause unexpected results), "
                "you may wish to manually inspect the files in a text editor yourself.",
                AllScriptEnts.Size(), AllMapEnts.Size());

            wxMessageBox(s, "Mismatching entity counts");
        }

        while (EntNr < AllScriptEnts.Size())
        {
            wxASSERT(AllScriptEnts[EntNr]->GetApp().IsNull());
            AllScriptEnts[EntNr]->SetApp(new CompMapEntityT(new MapEntityBaseT(*this)));
            EntNr++;
        }

        while (EntNr < AllMapEnts.Size())
        {
            IntrusivePtrT<cf::GameSys::EntityT> NewEnt = new cf::GameSys::EntityT(cf::GameSys::EntityCreateParamsT(*m_ScriptWorld));

            NewEnt->SetApp(new CompMapEntityT(AllMapEnts[EntNr]));
            m_ScriptWorld->GetRootEntity()->AddChild(NewEnt);

            EntNr++;
        }
    }


    ArrayT<MapElementT*> AllElems;
    GetAllElems(AllElems);

    m_BspTree = new OrthoBspTreeT(AllElems, m_GameConfig->GetMaxMapBB());
}


/*static*/ MapDocumentT* MapDocumentT::ImportHalfLife1Map(GameConfigT* GameConfig, wxProgressDialog* ProgressDialog, const wxString& FileName)
{
    // This sets the cursor to the busy cursor in its ctor, and back to the default cursor in the dtor.
    wxBusyCursor BusyCursor;
    TextParserT  TP(FileName.c_str(), "({})");

    if (TP.IsAtEOF())
        throw cf::GameSys::WorldT::InitErrorT("The file could not be opened.");

    MapDocumentT* Doc = new MapDocumentT(GameConfig);
    IntrusivePtrT<cf::GameSys::EntityT> ScriptRootEnt = Doc->m_ScriptWorld->GetRootEntity();

    try
    {
        Doc->GetEntities()[0]->Load_HL1_map(TP, *Doc, ProgressDialog, 0);

        // Load the entities.
        while (!TP.IsAtEOF())
        {
            MapEntityBaseT* Entity = new MapEntityBaseT(*Doc);

            Entity->Load_HL1_map(TP, *Doc, ProgressDialog, ScriptRootEnt->GetChildren().Size() + 1);

            IntrusivePtrT<cf::GameSys::EntityT> NewEnt = new cf::GameSys::EntityT(cf::GameSys::EntityCreateParamsT(*Doc->m_ScriptWorld));

            NewEnt->SetApp(new CompMapEntityT(Entity));
            ScriptRootEnt->AddChild(NewEnt);
        }
    }
    catch (const TextParserT::ParseError&)
    {
        wxMessageBox(wxString::Format(
            "I'm sorry, but I was not able to import the map, due to a file error.\n"
            "Worse, I cannot even say where the error occured, except near byte %lu (%.3f%%) of the file.\n"
            "Later versions of CaWE will provide more detailed information.\n"
            "Please use a text editor to make sure that the file you tried to open is a proper HL1 map file,\n"
            "and/or post at the Cafu support forums.", TP.GetReadPosByte(), TP.GetReadPosPercent()*100.0),
            wxString("Could not import HL1 map ")+FileName, wxOK | wxICON_EXCLAMATION);

        delete Doc;
        Doc=NULL;
        throw cf::GameSys::WorldT::InitErrorT("The file could not be parsed.");
    }


    ArrayT<MapElementT*> AllElems;
    Doc->GetAllElems(AllElems);

    delete Doc->m_BspTree;
    Doc->m_BspTree = new OrthoBspTreeT(AllElems, Doc->m_GameConfig->GetMaxMapBB());

    Doc->m_FileName=FileName;
    return Doc;
}


/*static*/ MapDocumentT* MapDocumentT::ImportHalfLife2Vmf(GameConfigT* GameConfig, wxProgressDialog* ProgressDialog, const wxString& FileName)
{
    // This sets the cursor to the busy cursor in its ctor, and back to the default cursor in the dtor.
    wxBusyCursor BusyCursor;
    TextParserT  TP(FileName.c_str(), "{}");

    if (TP.IsAtEOF())
        throw cf::GameSys::WorldT::InitErrorT("The file could not be opened.");

    MapDocumentT* Doc = new MapDocumentT(GameConfig);
    IntrusivePtrT<cf::GameSys::EntityT> ScriptRootEnt = Doc->m_ScriptWorld->GetRootEntity();

    try
    {
        // Read all the chunks.
        while (!TP.IsAtEOF())
        {
            const std::string ChunkName=TP.GetNextToken();

            if (ChunkName=="world")
            {
                Doc->GetEntities()[0]->Load_HL2_vmf(TP, *Doc, ProgressDialog, 0);
            }
            else if (ChunkName=="entity")
            {
                MapEntityBaseT* Entity = new MapEntityBaseT(*Doc);

                Entity->Load_HL2_vmf(TP, *Doc, ProgressDialog, ScriptRootEnt->GetChildren().Size() + 1);

                IntrusivePtrT<cf::GameSys::EntityT> NewEnt = new cf::GameSys::EntityT(cf::GameSys::EntityCreateParamsT(*Doc->m_ScriptWorld));

                NewEnt->SetApp(new CompMapEntityT(Entity));
                ScriptRootEnt->AddChild(NewEnt);
            }
            else
            {
                // It's a chunk that we cannot deal with, so just skip it.
                TP.SkipBlock("{", "}", false);
            }
        }
    }
    catch (const TextParserT::ParseError&)
    {
        wxMessageBox(wxString::Format(
            "I'm sorry, but I was not able to import the map, due to a file error.\n"
            "Worse, I cannot even say where the error occured, except near byte %lu (%.3f%%) of the file.\n"
            "Later versions of CaWE will provide more detailed information.\n"
            "Please use a text editor to make sure that the file you tried to open is a proper HL2 vmf file,\n"
            "and/or post at the Cafu support forums.", TP.GetReadPosByte(), TP.GetReadPosPercent()*100.0),
            wxString("Could not import HL2 map ")+FileName, wxOK | wxICON_EXCLAMATION);

        delete Doc;
        Doc=NULL;
        throw cf::GameSys::WorldT::InitErrorT("The file could not be parsed.");
    }


    ArrayT<MapElementT*> AllElems;
    Doc->GetAllElems(AllElems);

    delete Doc->m_BspTree;
    Doc->m_BspTree = new OrthoBspTreeT(AllElems, Doc->m_GameConfig->GetMaxMapBB());

    Doc->m_FileName=FileName;
    return Doc;
}


/*static*/ MapDocumentT* MapDocumentT::ImportDoom3Map(GameConfigT* GameConfig, wxProgressDialog* ProgressDialog, const wxString& FileName)
{
    // This sets the cursor to the busy cursor in its ctor, and back to the default cursor in the dtor.
    wxBusyCursor BusyCursor;
    TextParserT  TP(FileName.c_str(), "({})");

    if (TP.IsAtEOF())
        throw cf::GameSys::WorldT::InitErrorT("The file could not be opened.");

    MapDocumentT* Doc = new MapDocumentT(GameConfig);
    IntrusivePtrT<cf::GameSys::EntityT> ScriptRootEnt = Doc->m_ScriptWorld->GetRootEntity();

    try
    {
        Doc->GetEntities()[0]->Load_D3_map(TP, *Doc, ProgressDialog, 0);

        // Load the entities.
        while (!TP.IsAtEOF())
        {
            MapEntityBaseT* Entity = new MapEntityBaseT(*Doc);

            Entity->Load_D3_map(TP, *Doc, ProgressDialog, ScriptRootEnt->GetChildren().Size() + 1);

            IntrusivePtrT<cf::GameSys::EntityT> NewEnt = new cf::GameSys::EntityT(cf::GameSys::EntityCreateParamsT(*Doc->m_ScriptWorld));

            NewEnt->SetApp(new CompMapEntityT(Entity));
            ScriptRootEnt->AddChild(NewEnt);
        }
    }
    catch (const TextParserT::ParseError&)
    {
        wxMessageBox(wxString::Format(
            "I'm sorry, but I was not able to import the map, due to a file error.\n"
            "Worse, I cannot even say where the error occured, except near byte %lu (%.3f%%) of the file.\n"
            "Later versions of CaWE will provide more detailed information.\n"
            "Please use a text editor to make sure that the file you tried to open is a proper Doom3 map file,\n"
            "and/or post at the Cafu support forums.", TP.GetReadPosByte(), TP.GetReadPosPercent()*100.0),
            wxString("Could not import Doom3 map ")+FileName, wxOK | wxICON_EXCLAMATION);

        delete Doc;
        Doc=NULL;
        throw cf::GameSys::WorldT::InitErrorT("The file could not be parsed.");
    }


    ArrayT<MapElementT*> AllElems;
    Doc->GetAllElems(AllElems);

    delete Doc->m_BspTree;
    Doc->m_BspTree = new OrthoBspTreeT(AllElems, Doc->m_GameConfig->GetMaxMapBB());

    Doc->m_FileName=FileName;
    return Doc;
}


MapDocumentT::~MapDocumentT()
{
    delete m_BspTree;
    m_BspTree=NULL;

    for (unsigned long GroupNr=0; GroupNr<m_Groups.Size(); GroupNr++) delete m_Groups[GroupNr];
    m_Groups.Clear();

    for (unsigned long uecNr=0; uecNr<m_UnknownEntClasses.Size(); uecNr++) delete m_UnknownEntClasses[uecNr];
    m_UnknownEntClasses.Clear();

    m_Selection.Clear();

    delete m_ScriptWorld;
    m_ScriptWorld = NULL;
}


namespace
{
    // Recursively saves the entity instantiation of the passed entity and all of its children.
    void SaveEntityInstantiation(std::ostream& OutFile, IntrusivePtrT<cf::GameSys::EntityT> Entity, const wxString& ParentName)
    {
        OutFile << ParentName << Entity->GetBasics()->GetEntityName() << " = world:new(\"" << Entity->GetType()->ClassName << "\", \"" << Entity->GetBasics()->GetEntityName() << "\")\n";

        const wxString NewParentName = ParentName + Entity->GetBasics()->GetEntityName() + ".";

        for (unsigned long ChildNr = 0; ChildNr < Entity->GetChildren().Size(); ChildNr++)
            SaveEntityInstantiation(OutFile, Entity->GetChildren()[ChildNr], NewParentName);
    }


    // Recursively saves the entity hierarchy of the passed entity and all of its children.
    void SaveEntityHierarchy(std::ostream& OutFile, IntrusivePtrT<cf::GameSys::EntityT> Entity, const wxString& ParentName)
    {
        if (ParentName != "")
            OutFile << ParentName << ":AddChild(" << ParentName << "." << Entity->GetBasics()->GetEntityName() << ")\n";

        const wxString NewParentName = (ParentName != "" ? ParentName + "." : "") + Entity->GetBasics()->GetEntityName();

        for (unsigned long ChildNr = 0; ChildNr < Entity->GetChildren().Size(); ChildNr++)
            SaveEntityHierarchy(OutFile, Entity->GetChildren()[ChildNr], NewParentName);
    }


    void SaveComponents(std::ostream& OutFile, IntrusivePtrT<cf::GameSys::EntityT> Entity)
    {
        if (!Entity->GetBasics()->IsShown())
            OutFile << "    self:GetBasics():set(\"Show\", false)\n";

        OutFile << "    self:GetTransform():set(\"Origin\", " << Entity->GetTransform()->GetOrigin().x << ", " << Entity->GetTransform()->GetOrigin().y << ", " << Entity->GetTransform()->GetOrigin().z << ")\n";
     // OutFile << "    self:GetTransform():set(\"Size\", " << Entity->GetTransform()->GetSize().x << ", " << Entity->GetTransform()->GetSize().y << ")\n";


        if (Entity->GetComponents().Size() == 0)
            return;

        cf::TypeSys::VarVisitorToLuaCodeT ToLua(OutFile);

        OutFile << "\n";

        for (unsigned int CompNr = 1; CompNr <= Entity->GetComponents().Size(); CompNr++)
        {
            IntrusivePtrT<cf::GameSys::ComponentBaseT> Comp = Entity->GetComponents()[CompNr - 1];
            const ArrayT<cf::TypeSys::VarBaseT*>&      Vars = Comp->GetMemberVars().GetArray();

            OutFile << "    local c" << CompNr << " = world:new(\"" << Comp->GetType()->ClassName << "\")\n";

            for (unsigned int VarNr = 0; VarNr < Vars.Size(); VarNr++)
            {
                const cf::TypeSys::VarBaseT* Var = Vars[VarNr];

                OutFile << "    c" << CompNr << ":set(\"" << Var->GetName() << "\", ";
                Var->accept(ToLua);
                OutFile << ")\n";
            }

            OutFile << "\n";
        }

        OutFile << "    self:AddComponent(";
        for (unsigned int CompNr = 1; CompNr <= Entity->GetComponents().Size(); CompNr++)
        {
            OutFile << "c" << CompNr;
            if (CompNr < Entity->GetComponents().Size()) OutFile << ", ";
        }
        OutFile << ")\n";
    }


    // Recursively saves the entity initialization function of the entity passed and all of its children.
    void SaveEntityInitialization(std::ostream& OutFile, IntrusivePtrT<cf::GameSys::EntityT> Entity, const wxString& ParentName)
    {
        OutFile << "\nfunction " << ParentName + Entity->GetBasics()->GetEntityName() << ":OnInit()\n";

        SaveComponents(OutFile, Entity);

        OutFile << "end\n";

        const wxString NewParentName = ParentName + Entity->GetBasics()->GetEntityName() + ".";

        for (unsigned long ChildNr=0; ChildNr<Entity->GetChildren().Size(); ChildNr++)
            SaveEntityInitialization(OutFile, Entity->GetChildren()[ChildNr], NewParentName);
    }


    void SaveCafuEntities(std::ostream& OutFile, IntrusivePtrT<cf::GameSys::EntityT> RootEntity)
    {
        OutFile << "-- This is a Cafu Entities file, written by the CaWE Map Editor.\n";
        OutFile << "-- The file defines the entity hierarchy and properties of the related game world;\n";
        OutFile << "-- it is used both by the CaWE Map Editor as well as by the Cafu Engine.\n";
        OutFile << "--\n";
        OutFile << "-- You CAN edit this file manually, but note that CaWE may overwrite your changes.\n";
        OutFile << "-- Also note that structural changes to the entity hierarchy will bring this file\n";
        OutFile << "-- out of sync with the related map (cmap) and world (cw) files, effectively\n";
        OutFile << "-- causing LOSS OF WORK (see the documentation for details).\n";
        OutFile << "-- It is therefore recommended that you use CaWE for all changes to this file.\n";
        OutFile << "\n\n";
        OutFile << "-- Instantiation of all entities.\n";
        OutFile << "-- ******************************\n";
        OutFile << "\n";

        SaveEntityInstantiation(OutFile, RootEntity, "");

        OutFile << "\n\n";
        OutFile << "-- Set the worlds root entity.\n";
        OutFile << "-- ***************************\n";
        OutFile << "\n";
        OutFile << "world:SetRootEntity(" << RootEntity->GetBasics()->GetEntityName() << ")\n";

        OutFile << "\n\n";
        OutFile << "-- Setup the entity hierarchy.\n";
        OutFile << "-- ***************************\n";
        OutFile << "\n";

        SaveEntityHierarchy(OutFile, RootEntity, "");

        OutFile << "\n\n";
        OutFile << "-- Initialization of the entity contents (\"constructors\").\n";
        OutFile << "-- *******************************************************\n";

        SaveEntityInitialization(OutFile, RootEntity, "");
    }
}


bool MapDocumentT::OnSaveDocument(const wxString& cmapFileName, bool IsAutoSave)
{
    // if (cmapFileName.Right(4).MakeLower() == ".map") ...;    // Export to different file format.

    if (cmapFileName.Right(5).MakeLower() != ".cmap")
    {
        wxMessageBox(
            "Maps can only be saved as `.cmap` files.\n"
            "Export to other map formats is currently not supported.",
            "File not saved!", wxOK | wxICON_ERROR);
        return false;
    }

    const wxString centFileName = cmapFileName.Left(cmapFileName.length() - 5) + ".cent";

    // Backup the previous file before overwriting it.
    if (!IsAutoSave)
    {
        const char* Msg =
            "Creating the backup file \"%s_bak\" before saving the map to \"%s\" didn't work out.\n"
            "Please check the path and file permissions, or use 'File -> Save As...' to save the current "
            "map elsewhere.";

        if (wxFileExists(cmapFileName) && !wxCopyFile(cmapFileName, cmapFileName + "_bak"))
        {
            wxMessageBox(wxString::Format(Msg, cmapFileName, cmapFileName), "File not saved!", wxOK | wxICON_ERROR);
            return false;
        }

        if (wxFileExists(centFileName) && !wxCopyFile(centFileName, centFileName + "_bak"))
        {
            wxMessageBox(wxString::Format(Msg, centFileName, centFileName), "File not saved!", wxOK | wxICON_ERROR);
            return false;
        }
    }

    std::ofstream cmapOutFile(cmapFileName.fn_str());
    std::ofstream centOutFile(centFileName.fn_str());

    if (!cmapOutFile.is_open())
    {
        wxMessageBox("The file \"" + cmapFileName + "\" could not be opened for writing.\nPlease check the path and file permissions, "
                     "or use 'File -> Save As...' to save the current map elsewhere.", "File not saved!", wxOK | wxICON_ERROR);
        return false;
    }

    if (!centOutFile.is_open())
    {
        wxMessageBox("The file \"" + centFileName + "\" could not be opened for writing.\nPlease check the path and file permissions, "
                     "or use 'File -> Save As...' to save the current map elsewhere.", "File not saved!", wxOK | wxICON_ERROR);
        return false;
    }

    // From MSDN documentation: "digits10 returns the number of decimal digits that the type can represent without loss of precision."
    // For floats, that's usually 6, for doubles, that's usually 15. However, we want to use the number of *significant* decimal digits here,
    // that is, max_digits10. See http://www.open-std.org/JTC1/sc22/wg21/docs/papers/2006/n2005.pdf for details.
    cmapOutFile.precision(std::numeric_limits<float>::digits10 + 3);
    centOutFile.precision(std::numeric_limits<float>::digits10 + 3);

    // This sets the cursor to the busy cursor in its ctor, and back to the default cursor in the dtor.
    wxBusyCursor BusyCursor;

    // Save the `.cmap` file.
    {
        cmapOutFile << "// Cafu Map File\n"
                    << "// Written by CaWE, the Cafu World Editor.\n"
                    << "Version " << CMAP_FILE_VERSION << "\n"
                    << "\n";

        // Save groups.
        for (unsigned long GroupNr=0; GroupNr<m_Groups.Size(); GroupNr++)
            m_Groups[GroupNr]->Save_cmap(cmapOutFile, GroupNr);

        // Save entities.
        const ArrayT<MapEntityBaseT*>& MapEntities = GetEntities();

        for (unsigned long EntNr = 0/*with world*/; EntNr < MapEntities.Size(); EntNr++)
        {
            const BoundingBox3fT* Intersecting = NULL;
            const MapEntityBaseT* Ent = MapEntities[EntNr];

            if (!Intersecting || Ent->GetElemsBB().Intersects(*Intersecting))
            {
                Ent->Save_cmap(*this, cmapOutFile, EntNr, Intersecting);
            }
        }

        if (cmapOutFile.fail())
        {
            wxMessageBox("There was an error saving the file. Please try again.", "File not saved!", wxOK | wxICON_ERROR);
            return false;
        }
    }

    // Save the `.cent` file.
    {
        SaveCafuEntities(centOutFile, m_ScriptWorld->GetRootEntity());

        if (centOutFile.fail())
        {
            wxMessageBox("There was an error saving the file. Please try again.", "File not saved!", wxOK | wxICON_ERROR);
            return false;
        }
    }

    // If this was an auto-save, do not change the filename (nor set the document as "not modified").
    if (IsAutoSave) return true;

    m_FileName = cmapFileName;
    return true;
}


bool MapDocumentT::SaveAs()
{
    static wxString  LastUsedDir=m_GameConfig->ModDir+"/Maps";
    const wxFileName FN(m_FileName);

    wxFileDialog SaveFileDialog(NULL,                               // parent
                                "Save (or Export) File",            // message
                                (FN.IsOk() && wxDirExists(FN.GetPath())) ? FN.GetPath() : LastUsedDir, // default dir
                                "",                                 // default file
                                "Cafu Map Files (*.cmap)|*.cmap",   // wildcard
                             // "|Export Hammer (HL1) Maps (*.map)|*.map"
                             // "|Export Hammer RMFs (*.rmf)|*.rmf"
                                wxFD_SAVE | wxFD_OVERWRITE_PROMPT);

    if (SaveFileDialog.ShowModal()!=wxID_OK) return false;

    LastUsedDir=SaveFileDialog.GetDirectory();


    wxString Path=SaveFileDialog.GetPath();     // directory + filename

    if (!wxFileName(Path).HasExt())
    {
        switch (SaveFileDialog.GetFilterIndex())
        {
            case  0: Path+=".cmap"; break;
         // case  1: Path+=".map";  break;
         // case  2: Path+=".rmf";  break;
            default: Path+=".cmap"; break;
        }
    }

    return OnSaveDocument(Path, false);
}


bool MapDocumentT::Save()
{
    if (m_FileName=="" || m_FileName=="New Map") return SaveAs();
    if (m_FileName.Right(5).MakeLower()!=".cmap") return SaveAs();
    if (!wxFileExists(m_FileName)) return SaveAs();
    if (!wxFile::Access(m_FileName, wxFile::write)) return SaveAs();

    return OnSaveDocument(m_FileName, false);
}


const ArrayT<MapEntityBaseT*>& MapDocumentT::GetEntities() const
{
    /**************************************************************************************/
    /*** TODO: This cannot stay like this!                                              ***/
    /***   The method itself is obsolete (we have the m_ScriptWorld now instead),       ***/
    /***   and the dangerous static variable below is only for backwards-compatibility! ***/
    /**************************************************************************************/
    static ArrayT<MapEntityBaseT*> Entities;

    Entities.Overwrite();

    IntrusivePtrT<cf::GameSys::EntityT> ScriptRootEnt = m_ScriptWorld->GetRootEntity();
    ArrayT< IntrusivePtrT<cf::GameSys::EntityT> > AllChildren;

    AllChildren.PushBack(ScriptRootEnt);
    ScriptRootEnt->GetChildren(AllChildren, true);

    for (unsigned long ChildNr = 0; ChildNr < AllChildren.Size(); ChildNr++)
    {
        IntrusivePtrT<CompMapEntityT> CompMapEnt = dynamic_pointer_cast<CompMapEntityT>(AllChildren[ChildNr]->GetApp());

        Entities.PushBack(CompMapEnt->GetMapEntity());
    }

    return Entities;
}


void MapDocumentT::GetAllElems(ArrayT<MapElementT*>& Elems) const
{
    const ArrayT<MapEntityBaseT*>& MapEntities = GetEntities();

    for (unsigned int EntNr = 0; EntNr < MapEntities.Size(); EntNr++)
    {
        MapEntityBaseT* Ent = MapEntities[EntNr];

        // Add the entity representation...
        Elems.PushBack(Ent->GetRepres());

        // ... and all of its primitives.
        for (unsigned int PrimNr = 0; PrimNr < Ent->GetPrimitives().Size(); PrimNr++)
            Elems.PushBack(Ent->GetPrimitives()[PrimNr]);
    }
}


const EntityClassT* MapDocumentT::FindOrCreateUnknownClass(const wxString& Name, bool HasOrigin)
{
    wxASSERT(m_GameConfig->FindClass(Name)==NULL);

    for (unsigned long ClassNr=0; ClassNr<m_UnknownEntClasses.Size(); ClassNr++)
        if (m_UnknownEntClasses[ClassNr]->GetName()==Name)
        {
            // Somehow inform the user if this happens (it never should).
            // if (!m_UnknownEntClasses[ClassNr]->HasOrigin() && HasOrigin) wxLog("...");

            return m_UnknownEntClasses[ClassNr];
        }

    const EntityClassT* NewUnknownClass=new EntityClassT(*m_GameConfig, Name, HasOrigin);

    m_UnknownEntClasses.PushBack(NewUnknownClass);
    return NewUnknownClass;
}


namespace
{
    IntrusivePtrT<cf::GameSys::EntityT> Find(cf::GameSys::WorldT* ScriptWorld, MapEntityBaseT* FindMapEnt)
    {
        ArrayT< IntrusivePtrT<cf::GameSys::EntityT> > AllChildren;

        AllChildren.PushBack(ScriptWorld->GetRootEntity());
        ScriptWorld->GetRootEntity()->GetChildren(AllChildren, true);

        for (unsigned long ChildNr = 0; ChildNr < AllChildren.Size(); ChildNr++)
        {
            IntrusivePtrT<CompMapEntityT> CompMapEnt = dynamic_pointer_cast<CompMapEntityT>(AllChildren[ChildNr]->GetApp());

            wxASSERT(CompMapEnt != NULL);
            if (CompMapEnt->GetMapEntity() == FindMapEnt)
                return AllChildren[ChildNr];
        }

        return NULL;
    }
}


void MapDocumentT::Insert(MapEntityBaseT* Ent)
{
    wxASSERT(Ent!=NULL);
    if (Ent==NULL) return;

    // Should not have Ent already.
    wxASSERT(Find(m_ScriptWorld, Ent) == NULL);

    // Insert Ent into the m_ScriptWorld.
    IntrusivePtrT<cf::GameSys::EntityT> NewEnt = new cf::GameSys::EntityT(cf::GameSys::EntityCreateParamsT(*m_ScriptWorld));

    NewEnt->SetApp(new CompMapEntityT(Ent));
    m_ScriptWorld->GetRootEntity()->AddChild(NewEnt);

    // Insert all primitives of Ent and Ent itself into the BSP tree.
    for (unsigned long PrimNr=0; PrimNr<Ent->GetPrimitives().Size(); PrimNr++)
        m_BspTree->Insert(Ent->GetPrimitives()[PrimNr]);

    m_BspTree->Insert(Ent->GetRepres());
}


void MapDocumentT::Insert(MapPrimitiveT* Prim, MapEntityBaseT* ParentEnt)
{
    wxASSERT(Prim!=NULL);
    if (Prim==NULL) return;

    if (ParentEnt == NULL)
    {
        IntrusivePtrT<CompMapEntityT> CompMapEnt = dynamic_pointer_cast<CompMapEntityT>(m_ScriptWorld->GetRootEntity()->GetApp());

        wxASSERT(CompMapEnt != NULL);
        ParentEnt = CompMapEnt->GetMapEntity();
    }

    wxASSERT(Find(m_ScriptWorld, ParentEnt) != NULL);

    ParentEnt->AddPrim(Prim);
    m_BspTree->Insert(Prim);
}


void MapDocumentT::Remove(MapEntityBaseT* Ent)
{
    wxASSERT(Ent != NULL);
    if (Ent == NULL) return;

    IntrusivePtrT<cf::GameSys::EntityT> ScriptEnt  = Find(m_ScriptWorld, Ent);
    IntrusivePtrT<CompMapEntityT>       CompMapEnt = dynamic_pointer_cast<CompMapEntityT>(ScriptEnt->GetApp());

    wxASSERT(ScriptEnt != NULL);                // Ent was not found? Should not happen!
    wxASSERT(ScriptEnt->GetParent() != NULL);   // Ent is the world?  Should not happen!

    if (ScriptEnt == NULL) return;
    if (ScriptEnt->GetParent() == NULL) return;

    // Do *NOT* delete the `Ent` instance when ScriptEnt is deleted,
    // because when we get here, the caller (e.g. the Delete command) has taken ownership of `Ent`!
    CompMapEnt->SetMapEntity(NULL);

    ScriptEnt->GetParent()->RemoveChild(ScriptEnt);

    // Remove all primitives of Ent from the BSP tree.
    for (unsigned long PrimNr = 0; PrimNr < Ent->GetPrimitives().Size(); PrimNr++)
        m_BspTree->Remove(Ent->GetPrimitives()[PrimNr]);

    // Remove the representation of Ent from the BSP tree.
    m_BspTree->Remove(Ent->GetRepres());
}


void MapDocumentT::Remove(MapPrimitiveT* Prim)
{
    wxASSERT(Prim != NULL);
    if (Prim == NULL) return;

    MapEntityBaseT* ParentEnt = Prim->GetParent();

    // The first assert is actually redundant in the second, but keep it for clarity.
    wxASSERT(ParentEnt != NULL);
    wxASSERT(Find(m_ScriptWorld, ParentEnt) != NULL);

    if (ParentEnt != NULL)
    {
        ParentEnt->RemovePrim(Prim);
        wxASSERT(Prim->GetParent() == NULL);
    }

    m_BspTree->Remove(Prim);
}


// Hmmm. This would make a nice member function...   (TODO!)
static bool IsElemInBox(const MapElementT* Elem, const BoundingBox3fT& Box, bool InsideOnly, bool CenterOnly)
{
    const BoundingBox3fT ElemBB = Elem->GetBB();

    if (CenterOnly) return Box.Contains(ElemBB.GetCenter());
    if (InsideOnly) return Box.Contains(ElemBB.Min) && Box.Contains(ElemBB.Max);

    return ElemBB.Intersects(Box);
}


ArrayT<MapElementT*> MapDocumentT::GetElementsIn(const BoundingBox3fT& Box, bool InsideOnly, bool CenterOnly) const
{
    const ArrayT<MapEntityBaseT*>& MapEntities = GetEntities();
    ArrayT<MapElementT*> Result;

    for (unsigned int EntNr = 0; EntNr < MapEntities.Size(); EntNr++)
    {
        MapEntityBaseT* Ent = MapEntities[EntNr];

        // Add the entity representation...
        if (IsElemInBox(Ent->GetRepres(), Box, InsideOnly, CenterOnly))
            Result.PushBack(Ent->GetRepres());

        // ... and all of its primitives.
        for (unsigned int PrimNr = 0; PrimNr < Ent->GetPrimitives().Size(); PrimNr++)
            if (IsElemInBox(Ent->GetPrimitives()[PrimNr], Box, InsideOnly, CenterOnly))
                Result.PushBack(Ent->GetPrimitives()[PrimNr]);
    }

    return Result;
}


void MapDocumentT::SetSelection(const ArrayT<MapElementT*>& NewSelection)
{
    if (&m_Selection==&NewSelection) return;

    // Clear the previous selection.
    for (unsigned long SelNr=0; SelNr<m_Selection.Size(); SelNr++)
        m_Selection[SelNr]->SetSelected(false);

    m_Selection.Overwrite();

    for (unsigned long SelNr=0; SelNr<NewSelection.Size(); SelNr++)
    {
        m_Selection.PushBack(NewSelection[SelNr]);
        NewSelection[SelNr]->SetSelected();
    }
}


void MapDocumentT::GetUsedMaterials(ArrayT<EditorMaterialI*>& UsedMaterials) const
{
    const ArrayT<MapEntityBaseT*>& MapEntities = GetEntities();
    std::map<EditorMaterialI*, int> UsedMatMap;

    for (unsigned long EntNr = 0; EntNr < MapEntities.Size(); EntNr++)
    {
        const ArrayT<MapPrimitiveT*>& Primitives = MapEntities[EntNr]->GetPrimitives();

        for (unsigned long PrimNr = 0; PrimNr < Primitives.Size(); PrimNr++)
        {
            MapElementT* Prim = Primitives[PrimNr];

            MapBrushT* Brush=dynamic_cast<MapBrushT*>(Prim);
            if (Brush!=NULL)
            {
                EditorMaterialI* LastMat=NULL;

                for (unsigned long FaceNr=0; FaceNr<Brush->GetFaces().Size(); FaceNr++)
                {
                    EditorMaterialI* Mat=Brush->GetFaces()[FaceNr].GetMaterial();

                    if (Mat!=NULL && Mat!=LastMat)
                    {
                        UsedMatMap[Mat]=1;
                        LastMat=Mat;
                    }
                }
            }

            MapBezierPatchT* BezierPatch=dynamic_cast<MapBezierPatchT*>(Prim);
            if (BezierPatch!=NULL)
            {
                EditorMaterialI* Mat=BezierPatch->GetMaterial();

                if (Mat!=NULL)
                    UsedMatMap[Mat]=1;
            }

            MapTerrainT* Terrain=dynamic_cast<MapTerrainT*>(Prim);
            if (Terrain!=NULL)
            {
                EditorMaterialI* Mat=Terrain->GetMaterial();

                if (Mat!=NULL)
                    UsedMatMap[Mat]=1;
            }
        }
    }

    // Copy the std::map<> into the ArrayT<>.
    UsedMaterials.Overwrite();
    for (std::map<EditorMaterialI*, int>::const_iterator It=UsedMatMap.begin(); It!=UsedMatMap.end(); ++It)
        UsedMaterials.PushBack(It->first);
}


/**********************/
/*** Event Handlers ***/
/**********************/

void MapDocumentT::OnEditUndoRedo(wxCommandEvent& CE)
{
    // The undo system doesn't keep track of selected faces, so clear the face selection just to be safe.
    if (m_ChildFrame->GetToolManager().GetActiveToolType()==&ToolEditSurfaceT::TypeInfo)
        m_ChildFrame->GetSurfacePropsDialog()->ClearSelection();

    // Step forward or backward in the command history.
    if (CE.GetId()==wxID_UNDO) m_History.Undo();
                          else m_History.Redo();
}


void MapDocumentT::OnUpdateEditUndoRedo(wxUpdateUIEvent& UE)
{
    if (UE.GetId()==wxID_UNDO)
    {
        const CommandT* Cmd=m_History.GetUndoCommand();

        UE.SetText(Cmd!=NULL ? "Undo "+Cmd->GetName()+"\tCtrl+Z" : "Cannot Undo\tCtrl+Z");
        UE.Enable(Cmd!=NULL);
    }
    else
    {
        const CommandT* Cmd=m_History.GetRedoCommand();

        UE.SetText(Cmd!=NULL ? "Redo "+Cmd->GetName()+"\tCtrl+Y" : "Cannot Redo\tCtrl+Y");
        UE.Enable(Cmd!=NULL);
    }
}


void MapDocumentT::OnEditCut(wxCommandEvent& CE)
{
    OnEditCopy(CE);
    OnEditDelete(CE);
}


void MapDocumentT::OnEditCopy(wxCommandEvent& CE)
{
    wxBusyCursor BusyCursor;

    m_ChildFrame->GetMapClipboard().CopyFrom(m_Selection);
    m_ChildFrame->GetMapClipboard().SetOriginalCenter(GetMostRecentSelBB().GetCenter());
}


/// Helper function to paste the clipboard contents into the map.
/// @param DeltaTranslation    Translation offset for each pasted copy (only relevant if NumberOfCopies > 1).
/// @param DeltaRotation       Rotation offset for each pasted copy (only relevant if NumberOfCopies > 1).
/// @param NrOfCopies          Number of times the objects are pasted into the world.
/// @param PasteGrouped        Should all pasted objects be grouped?
/// @param CenterAtOriginals   Should pasted objects be centered at position of original objects?
ArrayT<CommandT*> MapDocumentT::CreatePasteCommands(const Vector3fT& DeltaTranslation, const cf::math::AnglesfT& DeltaRotation,
    unsigned int NrOfCopies, bool PasteGrouped, bool CenterAtOriginals)
{
    // Initialize the total translation and rotation for the first paste operation.
    // Note that a TotalTranslation of zero pastes each object in the exact same place as its original.
    Vector3fT          TotalTranslation = DeltaTranslation;
    cf::math::AnglesfT TotalRotation    = DeltaRotation;

    if (!CenterAtOriginals)
    {
        const Vector3fT GoodPastePos    = m_ChildFrame->GuessUserVisiblePoint();
        const Vector3fT OriginalsCenter = m_ChildFrame->GetMapClipboard().GetOriginalCenter();

        static Vector3fT    LastPastePoint(0, 0, 0);
        static unsigned int LastPasteCount = 0;

        if (GoodPastePos != LastPastePoint)
        {
            LastPastePoint = GoodPastePos;
            LastPasteCount = 0;
        }

        int PasteOffset = std::max(GetGridSpacing(), 1);

        while (PasteOffset < 8)
            PasteOffset *= 2;   // Make PasteOffset some multiple of the grid spacing larger than 8.0.

        TotalTranslation = SnapToGrid(LastPastePoint + Vector3fT(((LastPasteCount % 8)+(LastPasteCount/8))*PasteOffset, (LastPasteCount % 8)*PasteOffset, 0.0f) - OriginalsCenter, false, -1 /*Snap all axes.*/);

        LastPasteCount++;
    }


    // FIXME: This should probably be a param to the Trafo*() methods, rather than having these methods query it from the global Options.general.LockingTextures.
    const bool PrevLockMats = Options.general.LockingTextures;
    Options.general.LockingTextures = true;

    const ArrayT<MapEntityBaseT*>& SrcEnts  = m_ChildFrame->GetMapClipboard().GetEntities();
    const ArrayT<MapPrimitiveT*>&  SrcPrims = m_ChildFrame->GetMapClipboard().GetPrimitives();

    ArrayT<MapEntityBaseT*> NewEnts;
    ArrayT<MapPrimitiveT*>  NewPrims;

    for (unsigned int CopyNr = 0; CopyNr < NrOfCopies; CopyNr++)
    {
        for (unsigned long EntNr = 0; EntNr < SrcEnts.Size(); EntNr++)
        {
            MapEntityBaseT* NewEnt = new MapEntityBaseT(*SrcEnts[EntNr]);

            if (TotalTranslation != Vector3fT())
            {
                NewEnt->GetRepres()->TrafoMove(TotalTranslation);

                for (unsigned long PrimNr = 0; PrimNr < NewEnt->GetPrimitives().Size(); PrimNr++)
                    NewEnt->GetPrimitives()[PrimNr]->TrafoMove(TotalTranslation);
            }

            if (TotalRotation != cf::math::AnglesfT())
            {
                NewEnt->GetRepres()->TrafoRotate(NewEnt->GetRepres()->GetBB().GetCenter(), TotalRotation);

                for (unsigned long PrimNr = 0; PrimNr < NewEnt->GetPrimitives().Size(); PrimNr++)
                    NewEnt->GetPrimitives()[PrimNr]->TrafoRotate(NewEnt->GetPrimitives()[PrimNr]->GetBB().GetCenter(), TotalRotation);
            }

            NewEnts.PushBack(NewEnt);
        }

        for (unsigned long PrimNr = 0; PrimNr < SrcPrims.Size(); PrimNr++)
        {
            MapPrimitiveT* NewPrim = SrcPrims[PrimNr]->Clone();

            if (TotalTranslation != Vector3fT())
                NewPrim->TrafoMove(TotalTranslation);

            if (TotalRotation != cf::math::AnglesfT())
                NewPrim->TrafoRotate(NewPrim->GetBB().GetCenter(), TotalRotation);

            NewPrims.PushBack(NewPrim);
        }

        // Advance the total translation and rotation.
        TotalTranslation += DeltaTranslation;
        TotalRotation    += DeltaRotation;
    }

    Options.general.LockingTextures = PrevLockMats;


    ArrayT<CommandT*> SubCommands;

    if (NewEnts.Size() > 0)
    {
        CommandNewEntityT* CmdNewEnt = new CommandNewEntityT(*this, NewEnts, false /*don't select*/);

        CmdNewEnt->Do();
        SubCommands.PushBack(CmdNewEnt);
    }

    if (NewPrims.Size() > 0)
    {
        CommandAddPrimT* CmdAddPrim = new CommandAddPrimT(*this, NewPrims, GetEntities()[0], "insert prims", false /*don't select*/);

        CmdAddPrim->Do();
        SubCommands.PushBack(CmdAddPrim);
    }

    if (SubCommands.Size() > 0)
    {
        CommandSelectT* CmdSel = CommandSelectT::Set(this, NewEnts, NewPrims);

        CmdSel->Do();
        SubCommands.PushBack(CmdSel);

        if (PasteGrouped)
        {
            ArrayT<MapElementT*> NewElems;

            for (unsigned long EntNr = 0; EntNr < NewEnts.Size(); EntNr++)
            {
                MapEntityBaseT* NewEnt = NewEnts[EntNr];

                NewElems.PushBack(NewEnt->GetRepres());

                for (unsigned long PrimNr = 0; PrimNr < NewEnt->GetPrimitives().Size(); PrimNr++)
                    NewElems.PushBack(NewEnt->GetPrimitives()[PrimNr]);
            }

            for (unsigned long PrimNr = 0; PrimNr < NewPrims.Size(); PrimNr++)
                NewElems.PushBack(NewPrims[PrimNr]);


            CommandNewGroupT* CmdNewGroup = new CommandNewGroupT(*this,
                wxString::Format("paste group (%lu element%s)", NewElems.Size(), NewElems.Size() == 1 ? "" : "s"));

            CmdNewGroup->GetGroup()->SelectAsGroup = true;
            CmdNewGroup->Do();
            SubCommands.PushBack(CmdNewGroup);

            CommandAssignGroupT* CmdAssignGroup = new CommandAssignGroupT(*this, NewElems, CmdNewGroup->GetGroup());

            CmdAssignGroup->Do();
            SubCommands.PushBack(CmdAssignGroup);
        }
    }

    return SubCommands;
}


void MapDocumentT::OnEditPaste(wxCommandEvent& CE)
{
    wxBusyCursor      BusyCursor;
    ArrayT<CommandT*> SubCommands = CreatePasteCommands();

    if (SubCommands.Size() > 0)
    {
        // Submit the composite macro command.
        GetHistory().SubmitCommand(new CommandMacroT(SubCommands, "Paste"));
    }

    m_ChildFrame->GetToolManager().SetActiveTool(GetToolTIM().FindTypeInfoByName("ToolSelectionT"));
}


void MapDocumentT::OnEditPasteSpecial(wxCommandEvent& CE)
{
    const ArrayT<MapEntityBaseT*>& SrcEnts  = m_ChildFrame->GetMapClipboard().GetEntities();
    const ArrayT<MapPrimitiveT*>&  SrcPrims = m_ChildFrame->GetMapClipboard().GetPrimitives();

    BoundingBox3fT ClipboardBB;

    for (unsigned long EntNr = 0; EntNr < SrcEnts.Size(); EntNr++)
        ClipboardBB.InsertValid(SrcEnts[EntNr]->GetElemsBB());

    for (unsigned long PrimNr = 0; PrimNr < SrcPrims.Size(); PrimNr++)
        ClipboardBB.InsertValid(SrcPrims[PrimNr]->GetBB());

    if (SrcEnts.Size() == 0 && SrcPrims.Size() == 0) return;
    if (!ClipboardBB.IsInited()) return;

    PasteSpecialDialogT PasteSpecialDialog(ClipboardBB);

    if (PasteSpecialDialog.ShowModal() == wxID_CANCEL) return;

    wxBusyCursor      BusyCursor;
    const Vector3fT   Translation = Vector3fT(PasteSpecialDialog.TranslateX, PasteSpecialDialog.TranslateY, PasteSpecialDialog.TranslateZ);
    const Vector3fT   Rotation    = Vector3fT(PasteSpecialDialog.RotateX,    PasteSpecialDialog.RotateY,    PasteSpecialDialog.RotateZ);
    ArrayT<CommandT*> SubCommands = CreatePasteCommands(Translation, Rotation, PasteSpecialDialog.NrOfCopies,
                                        PasteSpecialDialog.GroupCopies, PasteSpecialDialog.CenterAtOriginal);

    if (SubCommands.Size() > 0)
    {
        // Submit the composite macro command.
        GetHistory().SubmitCommand(new CommandMacroT(SubCommands, "Paste Special"));
    }

    m_ChildFrame->GetToolManager().SetActiveTool(GetToolTIM().FindTypeInfoByName("ToolSelectionT"));
}


void MapDocumentT::OnEditDelete(wxCommandEvent& CE)
{
    // If the camera tool is the currently active tool, delete its active camera.
    ToolCameraT* CameraTool=dynamic_cast<ToolCameraT*>(m_ChildFrame->GetToolManager().GetActiveTool());

    if (CameraTool)
    {
        CameraTool->DeleteActiveCamera();
        return;
    }

    if (m_Selection.Size()>0)
    {
        // Do the actual deletion of the selected elements.
        GetHistory().SubmitCommand(new CommandDeleteT(*this, m_Selection));

        // If there are any empty groups (usually as a result from the deletion), purge them now.
        // We use an explicit command for deleting the groups (instead of putting everything into a macro command)
        // so that the user has the option to undo the purge (separately from the deletion) if he wishes.
        const ArrayT<GroupT*> EmptyGroups=GetAbandonedGroups();

        if (EmptyGroups.Size()>0)
            GetHistory().SubmitCommand(new CommandDeleteGroupT(*this, EmptyGroups));
    }
}


void MapDocumentT::OnEditSelectNone(wxCommandEvent& CE)
{
    if (m_ChildFrame->GetToolManager().GetActiveToolType()!=&ToolEditSurfaceT::TypeInfo)
    {
        GetHistory().SubmitCommand(CommandSelectT::Clear(this));
    }
    else
    {
        m_ChildFrame->GetSurfacePropsDialog()->ClearSelection();
    }
}


void MapDocumentT::OnEditSelectAll(wxCommandEvent& CE)
{
    ArrayT<MapElementT*> NewSelection;
    GetAllElems(NewSelection);

    // This used to remove the world entity (representation) at index 0,
    // but we don't want to make this exception any longer.
    // if (NewSelection.Size()>0)
    // {
    //     wxASSERT(NewSelection[0]->GetType() == &MapEntRepresT::TypeInfo);
    //     NewSelection.RemoveAt(0);
    // }

    // Remove all invisible elements.
    for (unsigned long ElemNr=0; ElemNr<NewSelection.Size(); ElemNr++)
        if (!NewSelection[ElemNr]->IsVisible())
        {
            NewSelection.RemoveAt(ElemNr);
            ElemNr--;
        }

    m_History.SubmitCommand(CommandSelectT::Add(this, NewSelection));
}


void MapDocumentT::OnUpdateSelectionApplyMaterial(wxUpdateUIEvent& UE)
{
    UE.Enable(m_ChildFrame->GetToolManager().GetActiveToolType()!=&ToolEditSurfaceT::TypeInfo);
}


void MapDocumentT::OnSelectionApplyMaterial(wxCommandEvent& CE)
{
    CommandT* Command=new CommandApplyMaterialT(*this, m_Selection, m_GameConfig->GetMatMan().GetDefaultMaterial());

    GetHistory().SubmitCommand(Command);
}


void MapDocumentT::OnMapSnapToGrid(wxCommandEvent& CE)
{
    m_SnapToGrid=CE.IsChecked();
    UpdateAllObservers(UPDATE_GRID);
}


void MapDocumentT::OnMapToggleGrid2D(wxCommandEvent& CE)
{
    m_ShowGrid=CE.IsChecked();
    UpdateAllObservers(UPDATE_GRID);
}


void MapDocumentT::OnMapFinerGrid(wxCommandEvent& CE)
{
    const int NewGridSpacing=m_GridSpacing/2;

    // Make sure that the new grid spacing is invertible: m_GridSpacing must not drop from 1 to 0,
    // and when m_GridSpacing is an odd number like 3 or 5, we cannot have a finer grid either.
    if (NewGridSpacing*2!=m_GridSpacing) return;

    m_GridSpacing=NewGridSpacing;

    UpdateAllObservers(UPDATE_GRID);
}


void MapDocumentT::OnMapCoarserGrid(wxCommandEvent& CE)
{
    if (m_GridSpacing>=512) return;

    m_GridSpacing*=2;
    if (m_GridSpacing<1) m_GridSpacing=1;   // Just in case it was 0 before.

    UpdateAllObservers(UPDATE_GRID);
}


void MapDocumentT::OnMapGotoPrimitive(wxCommandEvent& CE)
{
    GotoPrimitiveDialogT GotoPrimDialog;

    if (GotoPrimDialog.ShowModal()!=wxID_OK) return;

    const ArrayT<MapEntityBaseT*>& MapEntities = GetEntities();

    if (GotoPrimDialog.m_EntityNumber >= int(MapEntities.Size()))
    {
        wxMessageBox("The entity with the given index number does not exist.", "Goto Primitive");
        return;
    }

    if (GotoPrimDialog.m_PrimitiveNumber >= int(MapEntities[GotoPrimDialog.m_EntityNumber]->GetPrimitives().Size()))
    {
        wxMessageBox("The primitive with the given index number does not exist (in the specified entity).", "Goto Primitive");
        return;
    }

    MapPrimitiveT* Prim = MapEntities[GotoPrimDialog.m_EntityNumber]->GetPrimitives()[GotoPrimDialog.m_PrimitiveNumber];

    if (!Prim->IsVisible())
    {
        wxMessageBox("The primitive is currently invisible in group \""+Prim->GetGroup()->Name+"\".", "Goto Primitive");
        return;
    }

    // The primitive was found and is visible, now select it and center the 2D views on it.
    m_History.SubmitCommand(CommandSelectT::Set(this, Prim));
    m_ChildFrame->All2DViews_Center(Prim->GetBB().GetCenter());
}


void MapDocumentT::OnMapShowInfo(wxCommandEvent& CE)
{
    MapInfoDialogT MapInfoDialog(*this);
    MapInfoDialog.ShowModal();
}


void MapDocumentT::OnMapCheckForProblems(wxCommandEvent& CE)
{
    MapCheckDialogT MapCheckDialog(NULL, *this);
    MapCheckDialog.ShowModal();
}


void MapDocumentT::OnMapProperties(wxCommandEvent& CE)
{
    // Select the worldspawn entity, then open the inspector dialog.
    m_History.SubmitCommand(CommandSelectT::Set(this, GetEntities()[0]->GetRepres()));

    GetChildFrame()->GetInspectorDialog()->ChangePage(1);
    GetChildFrame()->ShowPane(GetChildFrame()->GetInspectorDialog());
}


void MapDocumentT::OnMapLoadPointFile(wxCommandEvent& CE)
{
    wxString PointFileName      =m_FileName.BeforeLast('.')+".pts";
    bool     LoadCustomPointFile=true;

    if (PointFileName==".pts") PointFileName=m_FileName+".pts";

    if (wxFileExists(PointFileName))
    {
        wxDateTime DT =wxFileName(PointFileName).GetModificationTime();
        wxString   Ask=wxString("The default pointfile is:\n")+
                       PointFileName+"\n"
                       "last modified on "+DT.FormatISODate()+", "+DT.FormatISOTime()+".\n\n"+
                       "Load the default pointfile?";

        LoadCustomPointFile=(wxMessageBox(Ask, "Load default pointfile?", wxYES_NO | wxICON_QUESTION)==wxNO);
    }

    if (LoadCustomPointFile)
    {
        PointFileName=wxFileSelector("Select Pointfile", "", PointFileName, ".pts", "Pointfile (*.pts)|*.pts", wxFD_OPEN | wxFD_FILE_MUST_EXIST /*, this (parent)*/);
        if (PointFileName.IsEmpty()) return;
    }


    // Create a new Lua state.
    lua_State* LuaState=lua_open();

    try
    {
        if (LuaState==NULL) throw wxString("Couldn't open Lua state.");
        if (!wxFileExists(PointFileName)) throw wxString("The file does not exist.");

        lua_pushcfunction(LuaState, luaopen_base);    lua_pushstring(LuaState, "");              lua_call(LuaState, 1, 0);  // Opens the basic library.
        lua_pushcfunction(LuaState, luaopen_package); lua_pushstring(LuaState, LUA_LOADLIBNAME); lua_call(LuaState, 1, 0);  // Opens the package library.
        lua_pushcfunction(LuaState, luaopen_table);   lua_pushstring(LuaState, LUA_TABLIBNAME);  lua_call(LuaState, 1, 0);  // Opens the table library.
        lua_pushcfunction(LuaState, luaopen_io);      lua_pushstring(LuaState, LUA_IOLIBNAME);   lua_call(LuaState, 1, 0);  // Opens the I/O library.
        lua_pushcfunction(LuaState, luaopen_os);      lua_pushstring(LuaState, LUA_OSLIBNAME);   lua_call(LuaState, 1, 0);  // Opens the OS library.
        lua_pushcfunction(LuaState, luaopen_string);  lua_pushstring(LuaState, LUA_STRLIBNAME);  lua_call(LuaState, 1, 0);  // Opens the string lib.
        lua_pushcfunction(LuaState, luaopen_math);    lua_pushstring(LuaState, LUA_MATHLIBNAME); lua_call(LuaState, 1, 0);  // Opens the math lib.

        // Load and process the Lua program that defines the path.
        if (luaL_loadfile(LuaState, PointFileName.c_str())!=0 || lua_pcall(LuaState, 0, 0, 0)!=0)
            throw wxString("Couldn't load the file:\n")+lua_tostring(LuaState, -1);

        // Read the points.
        wxASSERT(lua_gettop(LuaState)==0);
        m_PointFilePoints.Clear();
        lua_getglobal(LuaState, "Points");
        const size_t NumPoints=lua_objlen(LuaState, 1);

        for (size_t PointNr=1; PointNr<=NumPoints; PointNr++)
        {
            // Put the table of the current point onto the stack (at index 2).
            lua_rawgeti(LuaState, 1, PointNr);
            m_PointFilePoints.PushBackEmpty();

            lua_rawgeti(LuaState, 2, 1);
            m_PointFilePoints[PointNr-1].Time=lua_tonumber(LuaState, 3);
            lua_pop(LuaState, 1);

            for (size_t i=2; i<=4; i++)
            {
                lua_rawgeti(LuaState, 2, i);
                m_PointFilePoints[PointNr-1].Pos[i-2]=lua_tonumber(LuaState, 3);
                lua_pop(LuaState, 1);
            }

            lua_rawgeti(LuaState, 2, 5);
            m_PointFilePoints[PointNr-1].Heading=lua_tonumber(LuaState, 3);
            lua_pop(LuaState, 1);

            lua_rawgeti(LuaState, 2, 6);
            const char* InfoStr=lua_tostring(LuaState, 3);
            m_PointFilePoints[PointNr-1].Info=InfoStr ? InfoStr : "";
            lua_pop(LuaState, 1);

            // Remove the processed points table from the stack again.
            lua_pop(LuaState, 1);
        }

        wxASSERT(lua_gettop(LuaState)==1);
        lua_pop(LuaState, 1);

        // Read the colors.
        wxASSERT(lua_gettop(LuaState)==0);
        m_PointFileColors.Clear();
        lua_getglobal(LuaState, "Colors");
        const size_t NumColors=lua_objlen(LuaState, 1);

        for (size_t ColorNr=1; ColorNr<=NumColors; ColorNr++)
        {
            // Put the string with the current color onto the stack (at index 2).
            lua_rawgeti(LuaState, 1, ColorNr);

            const char* ColorName=lua_tostring(LuaState, 2);
            m_PointFileColors.PushBack(ColorName ? wxColour(ColorName) : wxNullColour);

            // Remove the processed color from the stack again.
            lua_pop(LuaState, 1);
        }

        // If there are profound problems with the colors, fix them.
        while (m_PointFileColors.Size()<4) m_PointFileColors.PushBack(wxNullColour);
        if (!m_PointFileColors[1].IsOk()) m_PointFileColors[1]=(*wxRED);

        wxASSERT(lua_gettop(LuaState)==1);
        lua_pop(LuaState, 1);

        UpdateAllObservers(UPDATE_POINTFILE);
    }
    catch (const wxString& msg)
    {
        wxMessageBox(msg, "Error loading "+PointFileName, wxOK | wxICON_ERROR);
    }

    // Close the Lua state.
    if (LuaState) lua_close(LuaState);
}


void MapDocumentT::OnMapUnloadPointFile(wxCommandEvent& CE)
{
    m_PointFilePoints.Clear();
    m_PointFileColors.Clear();

    UpdateAllObservers(UPDATE_POINTFILE);
}


void MapDocumentT::OnViewShowEntityInfo(wxCommandEvent& CE)
{
    Options.view2d.ShowEntityInfo=!Options.view2d.ShowEntityInfo;
    UpdateAllObservers(UPDATE_GLOBALOPTIONS);
}


void MapDocumentT::OnViewShowEntityTargets(wxCommandEvent& CE)
{
    Options.view2d.ShowEntityTargets=!Options.view2d.ShowEntityTargets;
    UpdateAllObservers(UPDATE_GLOBALOPTIONS);
}


void MapDocumentT::OnViewHideSelectedObjects(wxCommandEvent& CE)
{
    if (m_Selection.Size()==0) return;

    // Warn/inform the user when he tries to group elements with "mixed affiliation"?  (grouped or not, in entity or not)
    // e.g. like "From N selected map elements, there are X in the world, Y in entities and Z in groups."
    // This is pretty difficult to implement though, as there are many cases to distinguish, and might confuse the user.
    // Alternatively, just warn when grouping here *partially* breaks an existing group??
    ArrayT<CommandT*> SubCommands;

    // 1. Create a new group.
    CommandNewGroupT* CmdNewGroup=new CommandNewGroupT(*this, wxString::Format("%lu item%s", m_Selection.Size(), m_Selection.Size()==1 ? "" : "s"));
    GroupT*              NewGroup=CmdNewGroup->GetGroup();

    CmdNewGroup->Do();
    SubCommands.PushBack(CmdNewGroup);

    // 2. Put the m_Selection into the new group.
    CommandAssignGroupT* CmdAssign=new CommandAssignGroupT(*this, m_Selection, NewGroup);

    CmdAssign->Do();
    SubCommands.PushBack(CmdAssign);

    // 3. Hide the new group (set the visibility to false).
    if (CE.GetId()==ChildFrameT::ID_MENU_VIEW_HIDE_SELECTED_OBJECTS)
    {
        CommandGroupSetVisibilityT* CmdVis=new CommandGroupSetVisibilityT(*this, NewGroup, false);

        CmdVis->Do();
        SubCommands.PushBack(CmdVis);
    }

    // 4. Purge all groups that became empty (abandoned) by the (re-)assignment of map elements in step 2.
    CommandDeleteGroupT* CmdPurgeGroups=new CommandDeleteGroupT(*this, GetAbandonedGroups());

    CmdPurgeGroups->Do();
    SubCommands.PushBack(CmdPurgeGroups);

    // 5. Submit the composite macro command.
    m_History.SubmitCommand(new CommandMacroT(SubCommands, "Hide "+NewGroup->Name));
}


void MapDocumentT::OnViewHideUnselectedObjects(wxCommandEvent& CE)
{
    // Find all unselected map elements that are not in a group already.
    ArrayT<MapElementT*> HideElems;
    ArrayT<MapElementT*> Elems;

    GetAllElems(Elems);

    for (unsigned int ElemNr = 0; ElemNr < Elems.Size(); ElemNr++)
    {
        MapElementT* Elem = Elems[ElemNr];

        if (Elem->IsSelected() || Elem->GetGroup()) continue;

        HideElems.PushBack(Elem);
    }

    // If no relevant elements were found, do nothing.
    if (HideElems.Size()==0) return;


    ArrayT<CommandT*> SubCommands;

    // 1. Create a new group.
    CommandNewGroupT* CmdNewGroup=new CommandNewGroupT(*this, wxString::Format("%lu item%s", HideElems.Size(), HideElems.Size()==1 ? "" : "s"));
    GroupT*              NewGroup=CmdNewGroup->GetGroup();

    CmdNewGroup->Do();
    SubCommands.PushBack(CmdNewGroup);

    // 2. Put the HideElems into the new group.
    CommandAssignGroupT* CmdAssign=new CommandAssignGroupT(*this, HideElems, NewGroup);

    CmdAssign->Do();
    SubCommands.PushBack(CmdAssign);

    // 3. Hide the new group (set the visibility to false).
    CommandGroupSetVisibilityT* CmdVis=new CommandGroupSetVisibilityT(*this, NewGroup, false);

    CmdVis->Do();
    SubCommands.PushBack(CmdVis);

    // 4. Purge all groups that became empty (abandoned) by the (re-)assignment of map elements in step 2.
    CommandDeleteGroupT* CmdPurgeGroups=new CommandDeleteGroupT(*this, GetAbandonedGroups());

    CmdPurgeGroups->Do();
    SubCommands.PushBack(CmdPurgeGroups);

    // 5. Submit the composite macro command.
    m_History.SubmitCommand(new CommandMacroT(SubCommands, "Hide "+NewGroup->Name));
}


void MapDocumentT::OnViewShowHiddenObjects(wxCommandEvent& CE)
{
    ArrayT<GroupT*> HiddenGroups;

    for (unsigned long GroupNr=0; GroupNr<m_Groups.Size(); GroupNr++)
        if (!m_Groups[GroupNr]->IsVisible)
            HiddenGroups.PushBack(m_Groups[GroupNr]);

    if (HiddenGroups.Size()==0)
    {
        // wxMessageBox("All groups are already visible.");    // Should be a status bar update.
        return;
    }

    if (HiddenGroups.Size()==1)
    {
        m_History.SubmitCommand(new CommandGroupSetVisibilityT(*this, HiddenGroups[0], true /*NewVis*/));
        return;
    }

    ArrayT<CommandT*> SubCommands;

    for (unsigned long GroupNr=0; GroupNr<HiddenGroups.Size(); GroupNr++)
        SubCommands.PushBack(new CommandGroupSetVisibilityT(*this, HiddenGroups[GroupNr], true /*NewVis*/));

    m_History.SubmitCommand(new CommandMacroT(SubCommands, "Show all groups"));
}


void MapDocumentT::OnToolsCarve(wxCommandEvent& CE)
{
    ArrayT<const MapBrushT*> Carvers;

    for (unsigned long SelNr=0; SelNr<GetSelection().Size(); SelNr++)
    {
        MapBrushT* Brush=dynamic_cast<MapBrushT*>(GetSelection()[SelNr]);

        if (Brush)
        {
            Carvers.PushBack(Brush);
        }
    }

    if (Carvers.Size()==0) return;

    // Do the actual carve.
    GetHistory().SubmitCommand(new CommandCarveT(*this, Carvers));

    // If there are any empty groups (usually as a result from an implicit deletion by the carve), purge them now.
    // We use an explicit command for deleting the groups (instead of putting everything into a macro command)
    // so that the user has the option to undo the purge (separately from the deletion) if he wishes.
    const ArrayT<GroupT*> EmptyGroups=GetAbandonedGroups();

    if (EmptyGroups.Size()>0)
        GetHistory().SubmitCommand(new CommandDeleteGroupT(*this, EmptyGroups));
}


void MapDocumentT::OnToolsHollow(wxCommandEvent& CE)
{
    unsigned long BrushesInSelection=0;

    for (unsigned long SelNr=0; SelNr<m_Selection.Size(); SelNr++)
        if (m_Selection[SelNr]->GetType()==&MapBrushT::TypeInfo)
            BrushesInSelection++;

    if (BrushesInSelection==0)
    {
        wxMessageBox("Only brushes can be made hollow,\nbut there are currently no brushes selected.", "No brushes selected.", wxOK | wxICON_INFORMATION);
        return;
    }

    // Confirm the operation if there is more than one brush selected.
    if (BrushesInSelection>1)
        if (wxMessageBox("Do you want to make each of the selected brushes hollow?", "Multiple brushes selected", wxYES_NO | wxICON_QUESTION)==wxNO)
            return;


    // Prompt for the wall thickness, which is kept as the default value for the next call.
    static int WallWidth=32;

    const int NewWidth=wxGetNumberFromUser("Please enter the thickness of the walls for hollowing the brush(es).\n(Negative numbers to hollow outward do currently not work. Sorry.)", "Wall width:", "Hollow Brushes", WallWidth, 2, 1024);

    if (NewWidth==-1)
    {
        wxMessageBox("You either cancelled the previous dialog,\nor the width of the walls was not between 2 and 1024.\nPlease try again.", "Cannot hollow the brushes");
        return;
    }

    WallWidth=NewWidth;

    // Do the actual hollowing.
    // Note that there is no need to ever purge abandoned groups here: hollowing might replace, but never lessen their members.
    GetHistory().SubmitCommand(new CommandMakeHollowT(*this, WallWidth, m_Selection));
}


void MapDocumentT::OnToolsAssignPrimToEntity(wxCommandEvent& CE)
{
    ToolT*           NewEntityTool=m_ChildFrame->GetToolManager().GetTool(*GetToolTIM().FindTypeInfoByName("ToolNewEntityT")); if (!NewEntityTool) return;
    OptionsBar_NewEntityToolT* Bar=dynamic_cast<OptionsBar_NewEntityToolT*>(NewEntityTool->GetOptionsBar()); if (!Bar) return;

    ArrayT<MapEntityBaseT*> SelEntities;    // All entities   that are in the selection.
    ArrayT<MapPrimitiveT*>  SelPrimitives;  // All primitives that are in the selection.

    for (unsigned long SelNr=0; SelNr<m_Selection.Size(); SelNr++)
    {
        MapElementT*   Elem   = m_Selection[SelNr];
        MapEntRepresT* Repres = dynamic_cast<MapEntRepresT*>(Elem);

        if (Repres)
        {
            SelEntities.PushBack(Repres->GetParent());
        }
        else
        {
            wxASSERT(dynamic_cast<MapPrimitiveT*>(Elem));
            SelPrimitives.PushBack(static_cast<MapPrimitiveT*>(Elem));
        }
    }

    // If there were no primitives among the selected map elements, quit here.
    if (SelPrimitives.Size()==0)
    {
        wxMessageBox("Nothing is selected that could be tied to an entity.");
        return;
    }

    // The details on how to proceed depend on the number of entities in the selection.
    if (SelEntities.Size()==0)
    {
        // Don't query the user, but create (below) a new entity of default class.
        SelEntities.PushBack(NULL);
    }
    else if (SelEntities.Size()==1)
    {
        const int Result=wxMessageBox("Would you like to keep and re-use the selected \""+SelEntities[0]->GetClass()->GetName()+"\" entity?\n\n"
            "If you answer 'No', a new \""+Bar->m_SolidEntityChoice->GetStringSelection()+"\" entity will be created\n"
            "and all selected map elements added to it, including those of the of the selected entity.", "Re-use entity?", wxYES_NO | wxCANCEL | wxICON_QUESTION);

        switch (Result)
        {
            case wxYES: break;                      // Do nothing, SelEntities[0] is ok as-is.
            case wxNO:  SelEntities[0]=NULL; break; // The user wants a new default entity, not this one.
            default:    return;                     // User pressed Cancel.
        }
    }
    else
    {
        wxArrayString EntityNames;

        for (unsigned long EntNr=0; EntNr<SelEntities.Size(); EntNr++)
            EntityNames.Add(SelEntities[EntNr]->GetClass()->GetName()+"   ("+SelEntities[EntNr]->GetClass()->GetDescription()+")");

        const int Choice=wxGetSingleChoiceIndex("There are multiple entities in the selection.\n"
                                                "Please choose the one that you want to keep and that will receive all the map elements of the others.\n"
                                                "Or press Cancel, select only one entity, and start over.",
                                                "Please select the destination entity",
                                                EntityNames);

        if (Choice<0) return;                 // The user pressed Cancel.
        SelEntities[0]=SelEntities[Choice];   // Make sure that the chosen entity is at index 0.
    }

    if (SelEntities[0]!=NULL)
    {
        GetHistory().SubmitCommand(new CommandAssignPrimToEntT(*this, SelPrimitives, SelEntities[0]));
    }
    else
    {
        const EntityClassT* NewEntityClass=m_GameConfig->FindClass(Bar->m_SolidEntityChoice->GetStringSelection());
        if (NewEntityClass==NULL) return;

        ArrayT<CommandT*> SubCommands;

        // 1. Create a new entity.
        MapEntityBaseT* NewEnt = new MapEntityBaseT(*this);

        NewEnt->SetOrigin(SnapToGrid(GetMostRecentSelBB().GetCenter(), false /*Toggle*/, -1 /*AxisNoSnap*/));
        NewEnt->SetClass(NewEntityClass);

        CommandNewEntityT* CmdNewEnt = new CommandNewEntityT(*this, NewEnt);

        CmdNewEnt->Do();
        SubCommands.PushBack(CmdNewEnt);

        // 2. Assign the primitives to the new entity.
        CommandAssignPrimToEntT* CmdAssignToEnt = new CommandAssignPrimToEntT(*this, SelPrimitives, NewEnt);

        CmdAssignToEnt->Do();
        SubCommands.PushBack(CmdAssignToEnt);

        // 3. Submit the composite macro command.
        GetHistory().SubmitCommand(new CommandMacroT(SubCommands, "tie to new entity"));
    }

    // This is very rare - only fires when a parent entity became empty, thus implicitly deleted, and was the last element in its group:
    // If there are any empty groups (usually as a result from the deletion), purge them now.
    // We use an explicit command for deleting the groups (instead of putting everything into a macro command)
    // so that the user has the option to undo the purge (separately from the deletion) if he wishes.
    {
        const ArrayT<GroupT*> EmptyGroups=GetAbandonedGroups();

        if (EmptyGroups.Size()>0)
            GetHistory().SubmitCommand(new CommandDeleteGroupT(*this, EmptyGroups));
    }

    m_ChildFrame->GetToolManager().SetActiveTool(GetToolTIM().FindTypeInfoByName("ToolSelectionT"));
    m_ChildFrame->GetInspectorDialog()->ChangePage(1);
    m_ChildFrame->ShowPane(m_ChildFrame->GetInspectorDialog());
}


void MapDocumentT::OnToolsAssignPrimToWorld(wxCommandEvent& CE)
{
    ArrayT<MapPrimitiveT*> SelPrimitives;   // All primitives that are in the selection.

    for (unsigned long SelNr=0; SelNr<m_Selection.Size(); SelNr++)
    {
        MapPrimitiveT* Prim=dynamic_cast<MapPrimitiveT*>(m_Selection[SelNr]);

        if (Prim)
            SelPrimitives.PushBack(Prim);
    }

    // If there were no primitives among the selected map elements, quit here.
    if (SelPrimitives.Size()==0) return;

    GetHistory().SubmitCommand(new CommandAssignPrimToEntT(*this, SelPrimitives, GetEntities()[0]));

    // This is very rare - only fires when a parent entity became empty, thus implicitly deleted, and was the last element in its group:
    // If there are any empty groups (usually as a result from the deletion), purge them now.
    // We use an explicit command for deleting the groups (instead of putting everything into a macro command)
    // so that the user has the option to undo the purge (separately from the deletion) if he wishes.
    {
        const ArrayT<GroupT*> EmptyGroups=GetAbandonedGroups();

        if (EmptyGroups.Size()>0)
            GetHistory().SubmitCommand(new CommandDeleteGroupT(*this, EmptyGroups));
    }

    m_ChildFrame->GetToolManager().SetActiveTool(GetToolTIM().FindTypeInfoByName("ToolSelectionT"));
}


void MapDocumentT::OnToolsReplaceMaterials(wxCommandEvent& CE)
{
    ReplaceMaterialsDialogT ReplaceMatsDlg(GetSelection().Size()>0, *this, m_GameConfig->GetMatMan().GetDefaultMaterial()->GetName());
    ReplaceMatsDlg.ShowModal();
}


void MapDocumentT::OnToolsMaterialLock(wxCommandEvent& CE)
{
    wxASSERT(m_ChildFrame!=NULL);

    Options.general.LockingTextures=!Options.general.LockingTextures;

    m_ChildFrame->SetStatusText(Options.general.LockingTextures ? "Texture locking on" : "Texture locking off", ChildFrameT::SBP_MENU_HELP);
}


void MapDocumentT::OnToolsSnapSelectionToGrid(wxCommandEvent& CE)
{
    if (!GetSelection().Size()) return;

    CommandT* Command=new CommandSnapToGridT(*this, m_Selection);

    GetHistory().SubmitCommand(Command);
}


void MapDocumentT::OnToolsTransform(wxCommandEvent& CE)
{
    if (m_Selection.Size()==0)
    {
        wxMessageBox("Well, nothing is selected,\nso there is nothing to transform.");
        return;
    }

    TransformDialogT TransDlg;

    if (TransDlg.ShowModal()==wxID_OK)
    {
        CommandTransformT::TransModeT Mode =CommandTransformT::MODE_TRANSLATE;
        Vector3fT                     Value=TransDlg.m_Value.AsVectorOfFloat();
        Vector3fT                     RefPoint;

        // Set the proper transformation mode.
        switch (TransDlg.m_Mode)
        {
            case 1: Mode=CommandTransformT::MODE_SCALE;  break;
            case 2: Mode=CommandTransformT::MODE_ROTATE; break;
        }

        // Check for identity transformations: 1.0 scales, 0.0 translations or 0.0 rotations.
        if (Mode==CommandTransformT::MODE_SCALE)
        {
            if (fabs(Value.x)<0.001f) Value.x=1.0f;
            if (fabs(Value.y)<0.001f) Value.y=1.0f;
            if (fabs(Value.z)<0.001f) Value.z=1.0f;

            if (Value.x==1.0f && Value.y==1.0f && Value.z==1.0f) return;
        }
        else
        {
            // Translation or rotation.
            if (Value.x==0.0f && Value.y==0.0f && Value.z==0.0f) return;
        }

        // For scales and rotations, set the proper reference point.
        if (Mode!=CommandTransformT::MODE_TRANSLATE)
        {
            // Compute the center of the selected elements.
            RefPoint=GetMostRecentSelBB().GetCenter();
        }

        GetHistory().SubmitCommand(
            new CommandTransformT(*this, m_Selection, Mode, RefPoint, Value));
    }
}


void MapDocumentT::OnToolsAlign(wxCommandEvent& CE)
{
    wxASSERT(m_ChildFrame!=NULL);

    const ArrayT<ViewWindowT*>& ViewWindows=m_ChildFrame->GetViewWindows();

    if (ViewWindows.Size()==0)
    {
        wxMessageBox("I'm sorry, but I was not able to determine the most recently used view for this operation.\n"
                     "One view however must be active, so that I precisely know how to perform the operation.\n\n"
                     "You can activate a view by simply moving the mouse over it.\n"
                     "The last view that was touched by the mouse is taken as the active view for the operation.");
        return;
    }

    GetHistory().SubmitCommand(
        new CommandAlignT(*this, m_Selection, ViewWindows[0]->GetAxesInfo(), GetMostRecentSelBB(), CE.GetId()));
}


void MapDocumentT::OnToolsMirror(wxCommandEvent& CE)
{
    wxASSERT(m_ChildFrame!=NULL);

    const ArrayT<ViewWindowT*>& ViewWindows=m_ChildFrame->GetViewWindows();

    if (ViewWindows.Size()==0)
    {
        wxMessageBox("I'm sorry, but I was not able to determine the most recently used view for this operation.\n"
                     "One view however must be active, so that I precisely know how to perform the operation.\n\n"
                     "You can activate a view by simply moving the mouse over it.\n"
                     "The last view that was touched by the mouse is taken as the active view for the operation.");
        return;
    }

    if (!m_Selection.Size()) return;

    const AxesInfoT    AxesInfo  =ViewWindows[0]->GetAxesInfo();
    const unsigned int NormalAxis=(CE.GetId()==ChildFrameT::ID_MENU_TOOLS_MIRROR_HOR) ? AxesInfo.HorzAxis : AxesInfo.VertAxis;
    const float        Dist      =GetMostRecentSelBB().GetCenter()[NormalAxis];

    GetHistory().SubmitCommand(new CommandMirrorT(*this, m_Selection, NormalAxis, Dist));
}


void MapDocumentT::OnUpdateToolsMaterialLock(wxUpdateUIEvent& UE)
{
    UE.Check(Options.general.LockingTextures);
}


void MapDocumentT::OnUpdateEditPasteSpecial(wxUpdateUIEvent& UE)
{
    const ArrayT<MapEntityBaseT*>& SrcEnts  = m_ChildFrame->GetMapClipboard().GetEntities();
    const ArrayT<MapPrimitiveT*>&  SrcPrims = m_ChildFrame->GetMapClipboard().GetPrimitives();

    UE.Enable((SrcEnts.Size() > 0 || SrcPrims.Size() > 0) &&
              m_ChildFrame->GetToolManager().GetActiveToolType() != &ToolEditSurfaceT::TypeInfo);
}


void MapDocumentT::OnUpdateViewShowEntityInfo(wxUpdateUIEvent& UE)
{
    UE.Check(Options.view2d.ShowEntityInfo);
}


void MapDocumentT::OnUpdateViewShowEntityTargets(wxUpdateUIEvent& UE)
{
    UE.Check(Options.view2d.ShowEntityTargets);
}


void MapDocumentT::OnUpdateEditCutCopyDelete(wxUpdateUIEvent& UE)
{
    UE.Enable(m_ChildFrame->GetToolManager().GetActiveToolType()!=&ToolEditSurfaceT::TypeInfo);
}


float MapDocumentT::SnapToGrid(float f, bool Toggle) const
{
    // "!=" implements "xor" for bools.
    const float GridSpacing=(m_SnapToGrid!=Toggle) ? m_GridSpacing : 1.0f;

    return cf::math::round(f/GridSpacing) * GridSpacing;
}


Vector3fT MapDocumentT::SnapToGrid(const Vector3fT& Pos, bool Toggle, int AxisNoSnap) const
{
    Vector3fT SnappedPos=Pos;

    for (int AxisNr=0; AxisNr<3; AxisNr++)
        if (AxisNoSnap!=AxisNr) SnappedPos[AxisNr]=SnapToGrid(Pos[AxisNr], Toggle);

    return SnappedPos;
}


const BoundingBox3fT& MapDocumentT::GetMostRecentSelBB() const
{
    // Must re-determine the bounding-box each time anew, because e.g. a transformation may have caused
    // it to change even if the selection itself (the set of selected elements) did not change.
    BoundingBox3fT SelBB;

    for (unsigned long SelNr=0; SelNr<m_Selection.Size(); SelNr++)
        SelBB.InsertValid(m_Selection[SelNr]->GetBB());

    if (SelBB.IsInited())
    {
        // This is the key: Observe that m_SelectionBB is overwritten only if SelBB is inited!
        // Reversely, if the selection is empty or somehow doesn't yield an inited SelBB,
        // the previous bounding-box is kept!
        m_SelectionBB=SelBB;
    }

    return m_SelectionBB;
}


ArrayT<GroupT*> MapDocumentT::GetAbandonedGroups() const
{
    const ArrayT<MapEntityBaseT*>& MapEntities = GetEntities();
    ArrayT<GroupT*> EmptyGroups;

    for (unsigned long GroupNr=0; GroupNr<m_Groups.Size(); GroupNr++)
    {
        bool IsEmpty=true;

        for (unsigned long EntNr = 0; EntNr < MapEntities.Size(); EntNr++)
        {
            // Check the entity first...
            const MapEntityBaseT* Ent = MapEntities[EntNr];

            if (Ent->GetRepres()->GetGroup() == m_Groups[GroupNr])
            {
                IsEmpty=false;
                break;
            }

            /// ... then all its primitives.
            const ArrayT<MapPrimitiveT*>& Primitives=Ent->GetPrimitives();

            for (unsigned long PrimNr=0; PrimNr<Primitives.Size(); PrimNr++)
            {
                if (Primitives[PrimNr]->GetGroup()==m_Groups[GroupNr])
                {
                    IsEmpty=false;
                    break;
                }
            }

            if (!IsEmpty) break;
        }

        if (IsEmpty) EmptyGroups.PushBack(m_Groups[GroupNr]);
    }

    return EmptyGroups;
}
