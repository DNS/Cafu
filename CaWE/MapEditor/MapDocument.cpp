/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "ChildFrame.hpp"
#include "ChildFrameViewWin.hpp"
#include "CompMapEntity.hpp"
#include "DialogEditSurfaceProps.hpp"
#include "DialogInspector.hpp"
#include "DialogGotoPrimitive.hpp"
#include "DialogMapCheck.hpp"
#include "MapDocument.hpp"
#include "MapBezierPatch.hpp"
#include "MapEntRepres.hpp"
#include "MapModel.hpp"         // Only needed for some TypeInfo test...
#include "MapPlant.hpp"         // Only needed for some TypeInfo test...
#include "MapTerrain.hpp"       // Only needed for some TypeInfo test...
#include "DialogMapInfo.hpp"
#include "MapBrush.hpp"
#include "OrthoBspTree.hpp"
#include "DialogReplaceMaterials.hpp"
#include "DialogTransform.hpp"
#include "Group.hpp"

#include "../Camera.hpp"
#include "../DialogOptions.hpp"
#include "../EditorMaterial.hpp"
#include "../EditorMaterialManager.hpp"
#include "../GameConfig.hpp"
#include "../Options.hpp"

#include "Commands/Transform.hpp"
#include "Commands/AddPrim.hpp"
#include "Commands/Align.hpp"
#include "Commands/ApplyMaterial.hpp"
#include "Commands/ReparentPrimitive.hpp"
#include "Commands/Carve.hpp"
#include "Commands/Delete.hpp"
#include "Commands/Group_Assign.hpp"
#include "Commands/Group_New.hpp"
#include "Commands/Group_SetVisibility.hpp"
#include "Commands/Mirror.hpp"
#include "Commands/MakeHollow.hpp"
#include "Commands/NewEntity.hpp"
#include "Commands/Select.hpp"
#include "Commands/Group_Delete.hpp"

#include "ToolbarMaterials.hpp"     // Only needed for setting the ref to NULL in the dtor.
#include "ToolCamera.hpp"
#include "ToolEditSurface.hpp"
#include "ToolMorph.hpp"
#include "ToolManager.hpp"
#include "ToolOptionsBars.hpp"

#include "ClipSys/CollisionModelMan.hpp"
#include "GameSys/CompCollisionModel.hpp"
#include "GameSys/CompLightPoint.hpp"
#include "GameSys/CompLightRadiosity.hpp"
#include "GameSys/CompModel.hpp"
#include "GameSys/CompMover.hpp"
#include "GameSys/CompPhysics.hpp"
#include "GameSys/CompPlayerPhysics.hpp"
#include "GameSys/CompPlayerStart.hpp"
#include "GameSys/CompScript.hpp"
#include "GameSys/CompSound.hpp"
#include "GameSys/Entity.hpp"
#include "GameSys/EntityCreateParams.hpp"
#include "GameSys/World.hpp"
#include "GuiSys/AllComponents.hpp"
#include "GuiSys/GuiImpl.hpp"
#include "GuiSys/Window.hpp"
#include "Math3D/Matrix3x3.hpp"     // For converting "angles" to Quaternions.
#include "Math3D/Misc.hpp"
#include "Templates/Array.hpp"
#include "TextParser/TextParser.hpp"
#include "String.hpp"
#include "VarVisitorsLua.hpp"

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
    EVT_MENU  (ChildFrameT::ID_MENU_SELECTION_ASSIGN_TO_ENTITY,  MapDocumentT::OnSelectionAssignToEntity)
    EVT_BUTTON(ChildFrameT::ID_MENU_SELECTION_ASSIGN_TO_ENTITY,  MapDocumentT::OnSelectionAssignToEntity)
    EVT_MENU  (ChildFrameT::ID_MENU_SELECTION_GROUP,             MapDocumentT::OnSelectionGroup)
    EVT_BUTTON(ChildFrameT::ID_MENU_SELECTION_GROUP,             MapDocumentT::OnSelectionGroup)
    EVT_MENU  (ChildFrameT::ID_MENU_SELECTION_HIDE,              MapDocumentT::OnSelectionGroup)
    EVT_BUTTON(ChildFrameT::ID_MENU_SELECTION_HIDE,              MapDocumentT::OnSelectionGroup)
    EVT_MENU  (ChildFrameT::ID_MENU_SELECTION_HIDE_OTHER,        MapDocumentT::OnSelectionHideOther)
    EVT_BUTTON(ChildFrameT::ID_MENU_SELECTION_HIDE_OTHER,        MapDocumentT::OnSelectionHideOther)

    EVT_MENU    (ChildFrameT::ID_MENU_MAP_SNAP_TO_GRID,          MapDocumentT::OnMapSnapToGrid)
    EVT_MENU    (ChildFrameT::ID_MENU_MAP_SHOW_GRID_2D,          MapDocumentT::OnMapToggleGrid2D)
    EVT_MENU    (ChildFrameT::ID_MENU_MAP_FINER_GRID,            MapDocumentT::OnMapFinerGrid)
    EVT_MENU    (ChildFrameT::ID_MENU_MAP_COARSER_GRID,          MapDocumentT::OnMapCoarserGrid)
    EVT_MENU    (ChildFrameT::ID_MENU_MAP_AUTO_GROUP_ENTITIES,   MapDocumentT::OnMapAutoGroupEntities)
    EVT_CHECKBOX(ChildFrameT::ID_MENU_MAP_AUTO_GROUP_ENTITIES,   MapDocumentT::OnMapAutoGroupEntities)
    EVT_MENU    (ChildFrameT::ID_MENU_MAP_GOTO_PRIMITIVE,        MapDocumentT::OnMapGotoPrimitive)
    EVT_MENU    (ChildFrameT::ID_MENU_MAP_SHOW_INFO,             MapDocumentT::OnMapShowInfo)
    EVT_MENU    (ChildFrameT::ID_MENU_MAP_CHECK_FOR_PROBLEMS,    MapDocumentT::OnMapCheckForProblems)
    EVT_MENU    (ChildFrameT::ID_MENU_MAP_LOAD_POINTFILE,        MapDocumentT::OnMapLoadPointFile)
    EVT_MENU    (ChildFrameT::ID_MENU_MAP_UNLOAD_POINTFILE,      MapDocumentT::OnMapUnloadPointFile)

    EVT_UPDATE_UI(ChildFrameT::ID_MENU_MAP_AUTO_GROUP_ENTITIES,   MapDocumentT::OnUpdateMapAutoGroupEntities)

    EVT_MENU  (ChildFrameT::ID_MENU_VIEW_SHOW_ENTITY_INFO,        MapDocumentT::OnViewShowEntityInfo)
    EVT_MENU  (ChildFrameT::ID_MENU_VIEW_SHOW_ENTITY_TARGETS,     MapDocumentT::OnViewShowEntityTargets)

    EVT_UPDATE_UI(ChildFrameT::ID_MENU_VIEW_SHOW_ENTITY_INFO,     MapDocumentT::OnUpdateViewShowEntityInfo)
    EVT_UPDATE_UI(ChildFrameT::ID_MENU_VIEW_SHOW_ENTITY_TARGETS,  MapDocumentT::OnUpdateViewShowEntityTargets)

    EVT_MENU  (ChildFrameT::ID_MENU_TOOLS_CARVE,                  MapDocumentT::OnToolsCarve)
    EVT_MENU  (ChildFrameT::ID_MENU_TOOLS_MAKE_HOLLOW,            MapDocumentT::OnToolsHollow)
    EVT_MENU  (ChildFrameT::ID_MENU_TOOLS_APPLY_MATERIAL,         MapDocumentT::OnToolsApplyMaterial)
    EVT_MENU  (ChildFrameT::ID_MENU_TOOLS_REPLACE_MATERIALS,      MapDocumentT::OnToolsReplaceMaterials)
    EVT_MENU  (ChildFrameT::ID_MENU_TOOLS_MATERIAL_LOCK,          MapDocumentT::OnToolsMaterialLock)
    EVT_MENU  (ChildFrameT::ID_MENU_TOOLS_TRANSFORM,              MapDocumentT::OnToolsTransform)
    EVT_MENU  (ChildFrameT::ID_MENU_TOOLS_ALIGN_LEFT,             MapDocumentT::OnToolsAlign)
    EVT_MENU  (ChildFrameT::ID_MENU_TOOLS_ALIGN_RIGHT,            MapDocumentT::OnToolsAlign)
    EVT_MENU  (ChildFrameT::ID_MENU_TOOLS_ALIGN_HOR_CENTER,       MapDocumentT::OnToolsAlign)
    EVT_MENU  (ChildFrameT::ID_MENU_TOOLS_ALIGN_TOP,              MapDocumentT::OnToolsAlign)
    EVT_MENU  (ChildFrameT::ID_MENU_TOOLS_ALIGN_BOTTOM,           MapDocumentT::OnToolsAlign)
    EVT_MENU  (ChildFrameT::ID_MENU_TOOLS_ALIGN_VERT_CENTER,      MapDocumentT::OnToolsAlign)
    EVT_MENU  (ChildFrameT::ID_MENU_TOOLS_MIRROR_HOR,             MapDocumentT::OnToolsMirror)
    EVT_MENU  (ChildFrameT::ID_MENU_TOOLS_MIRROR_VERT,            MapDocumentT::OnToolsMirror)

    EVT_UPDATE_UI(ChildFrameT::ID_MENU_TOOLS_APPLY_MATERIAL, MapDocumentT::OnUpdateToolsApplyMaterial)
    EVT_UPDATE_UI(ChildFrameT::ID_MENU_TOOLS_MATERIAL_LOCK,  MapDocumentT::OnUpdateToolsMaterialLock)
END_EVENT_TABLE()


MapDocumentT::MapDocumentT(GameConfigT* GameConfig)
    : wxEvtHandler(),
      SubjectT(),
      m_ChildFrame(NULL),
      m_FileName("New Map"),
      m_DocAdapter(*this),
      m_ScriptState(),
      m_ScriptWorld(NULL),
      m_BspTree(NULL),
      m_GameConfig(GameConfig),
      m_PlantDescrMan(std::string(m_GameConfig->ModDir)),
      m_Selection(),
      m_SelectionBB(Vector3fT(-64.0f, -64.0f, 0.0f), Vector3fT(64.0f, 64.0f, 64.0f)),
      m_PointFilePoints(),
      m_PointFileColors(),
      m_SnapToGrid(true),
      m_GridSpacing(Options.Grid.InitialSpacing),
      m_ShowGrid(true),
      m_AutoGroupEntities(true)
{
    Init();

    m_ScriptWorld->LoadScript(
        "Map = world:new('EntityT', 'Map')\n"
        "Map:GetBasics():set('Static', true)\n"
        "world:SetRootEntity(Map)\n",
        cf::GameSys::WorldT::InitFlag_InlineCode | cf::GameSys::WorldT::InitFlag_OnlyStatic);
}


MapDocumentT::MapDocumentT(GameConfigT* GameConfig, wxProgressDialog* ProgressDialog, const wxString& FileName)
    : wxEvtHandler(),
      SubjectT(),
      m_ChildFrame(NULL),
      m_FileName(FileName),
      m_DocAdapter(*this),
      m_ScriptState(),
      m_ScriptWorld(NULL),
      m_BspTree(NULL),
      m_GameConfig(GameConfig),
      m_PlantDescrMan(std::string(m_GameConfig->ModDir)),
      m_Selection(),
      m_SelectionBB(Vector3fT(-64.0f, -64.0f, 0.0f), Vector3fT(64.0f, 64.0f, 64.0f)),
      m_PasteParentID(0),
      m_PointFilePoints(),
      m_PointFileColors(),
      m_SnapToGrid(true),
      m_GridSpacing(Options.Grid.InitialSpacing),
      m_ShowGrid(true),
      m_AutoGroupEntities(true)
{
    Init();

    // This sets the cursor to the busy cursor in its ctor, and back to the default cursor in the dtor.
    wxBusyCursor BusyCursor;
    TextParserT  TP(FileName.c_str(), "({})");

    if (TP.IsAtEOF())
    {
        throw cf::GameSys::WorldT::InitErrorT("The file could not be opened.");
    }

    unsigned int cmapFileVersion = 0;
    ArrayT< IntrusivePtrT<CompMapEntityT> > AllMapEnts;

    try
    {
        while (!TP.IsAtEOF())
        {
            IntrusivePtrT<CompMapEntityT> Entity = new CompMapEntityT(*this);

            Entity->Load_cmap(TP, *this, ProgressDialog, AllMapEnts.Size(), cmapFileVersion, false /*IgnoreGroups?*/);
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

        //XXX TODO: Call Cleanup() method here (same code as dtor).

        throw cf::GameSys::WorldT::InitErrorT("The file could not be parsed.");
    }

    if (cmapFileVersion >= 14)
    {
        wxString centFileName = FileName;

        if (centFileName.Replace(".cmap", ".cent") == 0)
            centFileName += ".cent";

        m_ScriptWorld->LoadScript(
            centFileName.ToStdString(),
            cf::GameSys::WorldT::InitFlag_OnlyStatic);
    }
    else
    {
        // Before `.cmap` file format version 14, related `.cent` files did not exist.
        m_ScriptWorld->cf::GameSys::WorldT::LoadScript(
            "Map = world:new('EntityT', 'Map')\n"
            "Map:GetBasics():set('Static', true)\n"
            "world:SetRootEntity(Map)\n",
            cf::GameSys::WorldT::InitFlag_InlineCode | cf::GameSys::WorldT::InitFlag_OnlyStatic);
    }


    PostLoadEntityAlign(cmapFileVersion, AllMapEnts);

    ArrayT<MapElementT*> AllElems;
    GetAllElems(AllElems);

    m_BspTree = new OrthoBspTreeT(AllElems, m_GameConfig->GetMaxMapBB());
}


void MapDocumentT::Init()
{
    cf::GameSys::WorldT::InitScriptState(m_ScriptState);

#if 0
    // We cannot use this method, which in fact is kind of obsolete:
    // It would attempt to re-register the Console and ConsoleInterface libraries,
    // which was already done above in cf::GameSys::WorldT::InitScriptState().
    // (Both InitScriptState() methods should probably be removed / refactored.)
    cf::GuiSys::GuiImplT::InitScriptState(m_ScriptState);
#else
    {
        // For each class that the TypeInfoManTs know about, add a (meta-)table to the registry of the LuaState.
        // The (meta-)table holds the Lua methods that the respective class implements in C++,
        // and is to be used as metatable for instances of this class.
        cf::ScriptBinderT Binder(m_ScriptState.GetLuaState());

        Binder.Init(cf::GuiSys::GetGuiTIM());
        Binder.Init(cf::GuiSys::GetWindowTIM());
        Binder.Init(cf::GuiSys::GetComponentTIM());
    }
#endif

    m_ScriptWorld = new cf::GameSys::WorldT(
        cf::GameSys::WorldT::RealmMapEditor,
        m_ScriptState,
        m_GameConfig->GetModelMan(),
        m_GameConfig->GetGuiResources(),
        *cf::ClipSys::CollModelMan,   // TODO: The CollModelMan should not be a global, but rather be instantiated along with the ModelMan and GuiRes.
        NULL,       // No clip world for this instance.
        NULL);      // No physics world for this instance.
}


/*static*/ MapDocumentT* MapDocumentT::CreateNew(GameConfigT* GameConfig)
{
    MapDocumentT* Doc = new MapDocumentT(GameConfig);

    IntrusivePtrT<cf::GameSys::EntityT> ScriptRootEnt = Doc->m_ScriptWorld->GetRootEntity();
    IntrusivePtrT<CompMapEntityT>       MapEnt        = new CompMapEntityT(*Doc);

    wxASSERT(ScriptRootEnt->GetApp().IsNull());
    ScriptRootEnt->SetApp(MapEnt);

    ArrayT<MapElementT*> AllElems;
    Doc->GetAllElems(AllElems);

    Doc->m_BspTree = new OrthoBspTreeT(AllElems, Doc->m_GameConfig->GetMaxMapBB());

    return Doc;
}


/*static*/ MapDocumentT* MapDocumentT::ImportHalfLife1Map(GameConfigT* GameConfig, wxProgressDialog* ProgressDialog, const wxString& FileName)
{
    // This sets the cursor to the busy cursor in its ctor, and back to the default cursor in the dtor.
    wxBusyCursor BusyCursor;
    TextParserT  TP(FileName.c_str(), "({})");

    if (TP.IsAtEOF())
        throw cf::GameSys::WorldT::InitErrorT("The file could not be opened.");

    MapDocumentT* Doc = new MapDocumentT(GameConfig);
    ArrayT< IntrusivePtrT<CompMapEntityT> > AllMapEnts;

    try
    {
        while (!TP.IsAtEOF())
        {
            IntrusivePtrT<CompMapEntityT> Entity = new CompMapEntityT(*Doc);

            Entity->Load_HL1_map(TP, *Doc, ProgressDialog, AllMapEnts.Size());
            AllMapEnts.PushBack(Entity);
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


    Doc->PostLoadEntityAlign(0, AllMapEnts);

    ArrayT<MapElementT*> AllElems;
    Doc->GetAllElems(AllElems);

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
    ArrayT< IntrusivePtrT<CompMapEntityT> > AllMapEnts;

    try
    {
        // Read all the chunks.
        while (!TP.IsAtEOF())
        {
            const std::string ChunkName = TP.GetNextToken();

            if (ChunkName == "world" || ChunkName == "entity")
            {
                IntrusivePtrT<CompMapEntityT> Entity = new CompMapEntityT(*Doc);

                Entity->Load_HL2_vmf(TP, *Doc, ProgressDialog, AllMapEnts.Size());
                AllMapEnts.PushBack(Entity);
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


    Doc->PostLoadEntityAlign(0, AllMapEnts);

    ArrayT<MapElementT*> AllElems;
    Doc->GetAllElems(AllElems);

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
    ArrayT< IntrusivePtrT<CompMapEntityT> > AllMapEnts;

    try
    {
        while (!TP.IsAtEOF())
        {
            IntrusivePtrT<CompMapEntityT> Entity = new CompMapEntityT(*Doc);

            Entity->Load_D3_map(TP, *Doc, ProgressDialog, AllMapEnts.Size());
            AllMapEnts.PushBack(Entity);
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


    Doc->PostLoadEntityAlign(0, AllMapEnts);

    ArrayT<MapElementT*> AllElems;
    Doc->GetAllElems(AllElems);

    Doc->m_BspTree = new OrthoBspTreeT(AllElems, Doc->m_GameConfig->GetMaxMapBB());

    Doc->m_FileName=FileName;
    return Doc;
}


/// Align the entities in the m_ScriptWorld and those in AllMapEnts with each other.
void MapDocumentT::PostLoadEntityAlign(unsigned int cmapFileVersion, const ArrayT< IntrusivePtrT<CompMapEntityT> >& AllMapEnts)
{
    ArrayT< IntrusivePtrT<cf::GameSys::EntityT> > AllScriptEnts;

    m_ScriptWorld->GetRootEntity()->GetAll(AllScriptEnts);

    unsigned int EntNr = 0;

    // Sync as much as possible.
    while (EntNr < AllScriptEnts.Size() && EntNr < AllMapEnts.Size())
    {
        wxASSERT(AllScriptEnts[EntNr]->GetApp().IsNull());
        AllScriptEnts[EntNr]->SetApp(AllMapEnts[EntNr]);

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
        // There were more entities in the `.cent` file than in the `.cmap` file.
        IntrusivePtrT<CompMapEntityT> MapEnt = new CompMapEntityT(*this);

        wxASSERT(AllScriptEnts[EntNr]->GetApp().IsNull());
        AllScriptEnts[EntNr]->SetApp(MapEnt);

        EntNr++;
    }

    while (EntNr < AllMapEnts.Size())
    {
        // There were fewer entities in the `.cent` file than in the `.cmap` file.
        IntrusivePtrT<cf::GameSys::EntityT> NewEnt = new cf::GameSys::EntityT(cf::GameSys::EntityCreateParamsT(*m_ScriptWorld));
        IntrusivePtrT<CompMapEntityT>       MapEnt = AllMapEnts[EntNr];

        NewEnt->SetApp(MapEnt);
        m_ScriptWorld->GetRootEntity()->AddChild(NewEnt);

        const EntPropertyT* NameProp = MapEnt->FindProperty("name");
        if (NameProp && NameProp->Value != "")
        {
            NewEnt->GetBasics()->SetEntityName(NameProp->Value.ToStdString());
            MapEnt->RemoveProperty("name");
        }
        else
        {
            // If we found no name for the entity above, just use the classname instead.
            if (MapEnt->GetProperty("classname") != "")
                NewEnt->GetBasics()->SetEntityName(MapEnt->GetProperty("classname"));
        }

        if (MapEnt->GetProperty("classname") != "")
        {
            const std::string ClassName = MapEnt->GetProperty("classname");

            // This is a relic from CaBSP's LoadWorld() code, which was hard-coded to move the primitives of
            // entities of these classes into the world entity. Thus, for compatibility, set the "Static" flag here.
            if (ClassName == "func_group" ||
                ClassName == "func_wall" ||
                ClassName == "func_water" ||
                ClassName == "light")
            {
                NewEnt->GetBasics()->SetMember("Static", true);
            }
        }

        const EntPropertyT* OriginProp = MapEnt->FindProperty("origin");
        if (OriginProp)
        {
            NewEnt->GetTransform()->SetOriginWS(OriginProp->GetVector3f());
            MapEnt->RemoveProperty("origin");
        }
        else
        {
            BoundingBox3fT BB;

            for (unsigned long PrimNr = 0; PrimNr < MapEnt->GetPrimitives().Size(); PrimNr++)
                BB += MapEnt->GetPrimitives()[PrimNr]->GetBB();

            if (BB.IsInited())
                NewEnt->GetTransform()->SetOriginWS(BB.GetCenter());
        }

        const EntPropertyT* AnglesProp = MapEnt->FindProperty("angles");
        if (AnglesProp)
        {
            enum { PITCH = 0, YAW, ROLL };  // Nose up/down, Heading, Bank angle.

            // Hard to believe as it is, but this is how we used to obtain an entity's local
            // coordinate-system from a set of angles in the past.
            // I cannot remember when or why the negation of the Pitch angle was introduced,
            // but it was used and is needed in all three forms below, for Q1, Q2 and Q3.
            Vector3fT Angles = AnglesProp->GetVector3f();
            Angles[PITCH] = -Angles[PITCH];

            const cf::math::QuaternionfT Q1(
                cf::math::Matrix3x3fT::GetFromAngles_COMPAT(Angles));

            const cf::math::QuaternionfT Q2(
                cf::math::Matrix3x3fT::GetRotateZMatrix(Angles[YAW  ]) *
                cf::math::Matrix3x3fT::GetRotateYMatrix(Angles[PITCH]) *
                cf::math::Matrix3x3fT::GetRotateXMatrix(Angles[ROLL ]));

            // Convert degrees to radian, as is needed for Euler() below.
            Angles *= cf::math::AnglesfT::PI / 180.0;

            const cf::math::QuaternionfT Q3 =
                cf::math::QuaternionfT::Euler(Angles.x, Angles.y, Angles.z);

            // Assert that Q1, Q2 and Q3 are equivalent to each other,
            // in the sense that they all describe the same orientation.
            wxASSERT(length(Q1 - Q2) < 0.001f || length(Q1 + Q2) < 0.001f);
            wxASSERT(length(Q1 - Q3) < 0.001f || length(Q1 + Q3) < 0.001f);
            wxASSERT(length(Q2 - Q3) < 0.001f || length(Q2 + Q3) < 0.001f);

            NewEnt->GetTransform()->SetQuatWS(Q3);
            MapEnt->RemoveProperty("angles");
        }

        EntNr++;
    }


    // Set door entities of the same team as children of a new "door controller" entity.
    if (cmapFileVersion < 14)
    {
        AllScriptEnts.Overwrite();
        m_ScriptWorld->GetRootEntity()->GetAll(AllScriptEnts);

        std::map<std::string, ArrayT< IntrusivePtrT<cf::GameSys::EntityT> > > Teams;

        for (EntNr = 0; EntNr < AllScriptEnts.Size(); EntNr++)
        {
            IntrusivePtrT<cf::GameSys::EntityT> Ent    = AllScriptEnts[EntNr];
            IntrusivePtrT<CompMapEntityT>       MapEnt = GetMapEnt(Ent);

            if (Ent->GetParent() == m_ScriptWorld->GetRootEntity() && MapEnt->GetProperty("classname") == "func_door")
            {
                const std::string TeamName = MapEnt->FindProperty("team") ? MapEnt->FindProperty("team")->Value.ToStdString() : Ent->GetBasics()->GetEntityName();

                Teams[TeamName].PushBack(Ent);
            }
        }

        // For each team, create a new entity, and assign the parts of the team as children of the entity.
        for (std::map<std::string, ArrayT< IntrusivePtrT<cf::GameSys::EntityT> > >::const_iterator It = Teams.begin(); It != Teams.end(); It++)
        {
            Vector3fT      Origin;
            BoundingBox3fT BB;
            float          triggerPadding = 8.0f;

            for (unsigned int PartNr = 0; PartNr < It->second.Size(); PartNr++)
            {
                Origin += It->second[PartNr]->GetTransform()->GetOriginWS();

                const ArrayT<MapPrimitiveT*>& PartPrims = GetMapEnt(It->second[PartNr])->GetPrimitives();

                for (unsigned int PrimNr = 0; PrimNr < PartPrims.Size(); PrimNr++)
                    BB.Insert(PartPrims[PrimNr]->GetBB());

                triggerPadding = std::max(triggerPadding, float(wxAtof(GetMapEnt(It->second[PartNr])->GetAndRemove("triggerPadding", "8.0"))));
            }

            Origin /= It->second.Size();

            IntrusivePtrT<cf::GameSys::EntityT> DoorEnt = new cf::GameSys::EntityT(cf::GameSys::EntityCreateParamsT(*m_ScriptWorld));
            IntrusivePtrT<CompMapEntityT>       MapEnt  = new CompMapEntityT(*this);

            m_ScriptWorld->GetRootEntity()->AddChild(DoorEnt);

            DoorEnt->GetBasics()->SetEntityName("door");
            DoorEnt->GetTransform()->SetOriginWS(Origin);
            DoorEnt->SetApp(MapEnt);

            for (unsigned int PartNr = 0; PartNr < It->second.Size(); PartNr++)
            {
                IntrusivePtrT<cf::GameSys::EntityT> Part = It->second[PartNr];
                const Vector3fT                     PartPosWS = Part->GetTransform()->GetOriginWS();

                const bool Result1 = m_ScriptWorld->GetRootEntity()->RemoveChild(Part);
                wxASSERT(Result1);

                const bool Result2 = DoorEnt->AddChild(Part);
                wxASSERT(Result2);

                // The part is now positioned relative to DoorEnt, but we want to keep its origin unchanged in world-space:
                Part->GetTransform()->SetOriginWS(PartPosWS);

                // Add a "move_dest" child entity for the part.
                std::istringstream iss(GetMapEnt(Part)->GetAndRemove("moveDir"));
                Vector3fT          MoveDir;

                iss >> MoveDir.x >> MoveDir.y >> MoveDir.z;
                MoveDir = normalizeOr0(MoveDir);

                const BoundingBox3fT PartBB     = GetMapEnt(Part)->GetElemsBB();
                const Vector3fT      PartSize   = PartBB.Max - PartBB.Min;
                const Vector3fT      AbsMoveDir = Vector3fT(fabs(MoveDir.x), fabs(MoveDir.y), fabs(MoveDir.z));
                const double         Lip        = float(wxAtof(GetMapEnt(Part)->GetAndRemove("lip", "0.0")));

                MoveDir *= dot(PartSize, AbsMoveDir) - Lip;
                if (length(MoveDir) < 1.0f) MoveDir = Vector3fT(0.0f, 0.0f, 32.0f);

                IntrusivePtrT<cf::GameSys::EntityT> MoveDestEnt = new cf::GameSys::EntityT(cf::GameSys::EntityCreateParamsT(*m_ScriptWorld));

                const bool Result3 = Part->AddChild(MoveDestEnt);
                wxASSERT(Result3);

                MoveDestEnt->GetBasics()->SetEntityName("move_dest");
                MoveDestEnt->GetTransform()->SetOriginPS(MoveDir);
                MoveDestEnt->SetApp(new CompMapEntityT(*this));
            }

            // Add an explicit trigger brush to the DoorEnt.
            if (!BB.IsInited())
                BB = BoundingBox3fT(Origin).GetEpsilonBox(32.0f);

            unsigned int sa = 0;    // shortest axis

            for (unsigned int i = 1; i < 3; i++)
                if (BB.Max[i] - BB.Min[i] < BB.Max[sa] - BB.Min[sa])
                    sa = i;

            BB.Min[sa] -= triggerPadding;
            BB.Max[sa] += triggerPadding;

            MapEnt->AddPrim(MapBrushT::CreateBlock(BB,
                m_GameConfig->GetMatMan().FindMaterial("Textures/meta/trigger", true)));

            // Add a Mover component to the DoorEnt.
            IntrusivePtrT<cf::GameSys::ComponentMoverT> MoverComp = new cf::GameSys::ComponentMoverT();

            MoverComp->SetMember("destActivated", int(cf::GameSys::ComponentMoverT::VarDestActivatedT::DESTACT_RESET_TIMEOUT));
            MoverComp->SetMember("destTimeout", 2.0f);
            MoverComp->SetMember("trajExp", 2.0f);
            DoorEnt->AddComponent(MoverComp);

            // Add a Script component to the DoorEnt.
            IntrusivePtrT<cf::GameSys::ComponentScriptT> ScriptComp = new cf::GameSys::ComponentScriptT();

            ScriptComp->SetMember("Name", std::string(m_GameConfig->ModDir + "/Scripts/MoverBinary.lua"));
            DoorEnt->AddComponent(ScriptComp);
        }
    }


    // Automatically migrate "old-style" properties (e.g. from cmap files version <= 13, or foreign imported map files)
    // to "new-style" entity components as much as possible.
    // Do this in a manner that allows us a smooth, backwards-compatible transition: It should be possible to account
    // for the existing components today and to develop new components in the future, and to account for them as they
    // become available.
    // The key idea is to examine whether a (new-style) component for an (old-style) property already exists.
    // If a related component does not yet exist, create one, initialize it from the value of the property, then remove
    // the property (so that any old code that might still use properties is made ineffective).
    // Otherwise, assume that the component has been created by this process before, and don't touch it.
    // Obviously, this works with optional components only -- Basics and Transform must immediately be handled above.
    AllScriptEnts.Overwrite();
    m_ScriptWorld->GetRootEntity()->GetAll(AllScriptEnts);

    for (EntNr = 0; EntNr < AllScriptEnts.Size(); EntNr++)
    {
        IntrusivePtrT<cf::GameSys::EntityT> Ent    = AllScriptEnts[EntNr];
        IntrusivePtrT<CompMapEntityT>       MapEnt = GetMapEnt(Ent);

        if (Ent->GetComponents().Size() == 0 && MapEnt->GetProperty("classname") == "info_player_start")
        {
            IntrusivePtrT<cf::GameSys::ComponentPlayerStartT> PlayerStartComp  = new cf::GameSys::ComponentPlayerStartT();

            PlayerStartComp->SetMember("SinglePlayer", true);
            PlayerStartComp->SetMember("MultiPlayer", true);
            Ent->AddComponent(PlayerStartComp);
        }

        if (Ent->GetComponent("Model") == NULL && MapEnt->FindProperty("model") && MapEnt->FindProperty("model")->Value != "")
        {
            IntrusivePtrT<cf::GameSys::ComponentModelT> ModelComp = new cf::GameSys::ComponentModelT();

            std::string ModelName = MapEnt->GetAndRemove("model");
            if (ModelName != "")
            {
                // The "model" value used to be relative to the ModDir, e.g. "Models/Static/Gelaender01.mdl".
                ModelName = m_GameConfig->ModDir + "/" + ModelName;
            }

            std::string GuiName = MapEnt->GetAndRemove("gui");
            if (GuiName != "")
            {
                // The "gui" value used to be relative to the ModDir, e.g. "GUIs/Test.cgui".
                GuiName = m_GameConfig->ModDir + "/" + GuiName;
            }

            ModelComp->SetMember("Name",      ModelName);
            ModelComp->SetMember("Animation", wxAtoi(MapEnt->GetAndRemove("sequence", "0")));
         // ModelComp->SetMember("Skin",      ...);
            ModelComp->SetMember("Scale",     float(wxAtof(MapEnt->GetAndRemove("scale", "1.0"))));
            ModelComp->SetMember("Gui",       GuiName);

            Ent->AddComponent(ModelComp);
        }

        if (Ent->GetComponent("CollisionModel") == NULL && MapEnt->FindProperty("collisionModel") && MapEnt->FindProperty("collisionModel")->Value != "")
        {
            IntrusivePtrT<cf::GameSys::ComponentCollisionModelT> CollMdlComp = new cf::GameSys::ComponentCollisionModelT();

            std::string CollMdlName = MapEnt->GetAndRemove("collisionModel");
            if (CollMdlName != "")
            {
                // The "collisionModel" value used to be relative to the ModDir, e.g. "Models/Static/Tonne01.cmap".
                CollMdlName = m_GameConfig->ModDir + "/" + CollMdlName;
            }

            CollMdlComp->SetMember("Name", CollMdlName);

            Ent->AddComponent(CollMdlComp);
        }

        if (Ent->GetComponents().Size() == 0 && MapEnt->GetProperty("classname") == "monster_butterfly")
        {
            IntrusivePtrT<cf::GameSys::ComponentModelT>  ModelComp  = new cf::GameSys::ComponentModelT();
            IntrusivePtrT<cf::GameSys::ComponentScriptT> ScriptComp = new cf::GameSys::ComponentScriptT();

            ModelComp->SetMember("Name", std::string("Games/DeathMatch/Models/LifeForms/Butterfly/Butterfly.cmdl"));
            Ent->AddComponent(ModelComp);

            ScriptComp->SetMember("Name", std::string(m_GameConfig->ModDir + "/Scripts/Butterfly.lua"));
            Ent->AddComponent(ScriptComp);
        }

        if (Ent->GetComponents().Size() == 0 && MapEnt->GetProperty("classname") == "LifeFormMaker")
        {
            IntrusivePtrT<cf::GameSys::ComponentScriptT> ScriptComp = new cf::GameSys::ComponentScriptT();

            const std::string MnType = MapEnt->GetAndRemove("monstertype", "Butterfly");
            const std::string Count1 = MapEnt->GetAndRemove("monstercount", "-1");
            const std::string Count2 = MapEnt->GetAndRemove("m_imaxlivechildren", "-1");

            std::string Clearance = "16.0";
            if (MnType == "CompanyBot") Clearance = "96.0";
            if (MnType == "Eagle")      Clearance = "32.0";

            ScriptComp->SetMember("Name", std::string(m_GameConfig->ModDir + "/Scripts/EntityFactory.lua"));
            ScriptComp->SetMember("ScriptCode", std::string("local EntFac = ...\n") +
                "EntFac.Type = \"" + MnType + "\"\n" +
                "EntFac.NumTotal = " + (Count1 == "-1" ? Count2 : Count1) + "\n" +
                "EntFac.Delay = " + MapEnt->GetAndRemove("delay", "3.0") + "\n" +
                "EntFac.Clearance = " + Clearance + "\n");

            Ent->AddComponent(ScriptComp);
        }

        if (Ent->GetComponents().Size() == 0 && MapEnt->GetProperty("classname") == "monster_companybot")
        {
            IntrusivePtrT<cf::GameSys::ComponentModelT> ModelComp  = new cf::GameSys::ComponentModelT();
            ModelComp->SetMember("Name", std::string("Games/DeathMatch/Models/Players/Trinity/Trinity.cmdl"));
            Ent->AddComponent(ModelComp);

            IntrusivePtrT<cf::GameSys::ComponentModelT> SubmodelComp  = new cf::GameSys::ComponentModelT();
            SubmodelComp->SetMember("Name", std::string("Games/DeathMatch/Models/Weapons/DesertEagle/DesertEagle_p.cmdl"));
            SubmodelComp->SetMember("IsSubmodel", true);
            Ent->AddComponent(SubmodelComp);

            IntrusivePtrT<cf::GameSys::ComponentPlayerPhysicsT> PlayerPhysicsComp = new cf::GameSys::ComponentPlayerPhysicsT();
            PlayerPhysicsComp->SetMember("Dimensions", BoundingBox3dT(Vector3dT( 12.0,  12.0, 4.0), Vector3dT(-12.0, -12.0, -68.0)));   // Isn't -36 ... 36 the proper heights?
            PlayerPhysicsComp->SetMember("StepHeight", 24.0);
            Ent->AddComponent(PlayerPhysicsComp);

            // Note that a collision model is added by the script code itself.
            IntrusivePtrT<cf::GameSys::ComponentScriptT> ScriptComp = new cf::GameSys::ComponentScriptT();
            ScriptComp->SetMember("Name", std::string(m_GameConfig->ModDir + "/Scripts/CompanyBot.lua"));
            Ent->AddComponent(ScriptComp);

            // Add a child entity with a point light source component (a "lantern").
            IntrusivePtrT<cf::GameSys::EntityT> LanternEnt  = new cf::GameSys::EntityT(cf::GameSys::EntityCreateParamsT(*m_ScriptWorld));
            IntrusivePtrT<CompMapEntityT>       LntMapEnt   = new CompMapEntityT(*this);

            Ent->AddChild(LanternEnt);

            LanternEnt->GetBasics()->SetEntityName("Lantern");
            LanternEnt->GetTransform()->SetOriginPS(Vector3fT(20.0f, 0.0f, 0.0f));
            LanternEnt->SetApp(LntMapEnt);

            IntrusivePtrT<cf::GameSys::ComponentPointLightT> PointLight = new cf::GameSys::ComponentPointLightT();
            PointLight->SetMember("Color", Vector3fT(0.0f, 0.0f, 0.0f));    // The CompanyBot.lua script brings this up to color.
            PointLight->SetMember("On", true);
            PointLight->SetMember("Radius", 400.0f);
            PointLight->SetMember("ShadowType", 1);
            LanternEnt->AddComponent(PointLight);
        }

        if (Ent->GetComponents().Size() == 0 && MapEnt->GetProperty("classname") == "monster_eagle")
        {
            IntrusivePtrT<cf::GameSys::ComponentModelT>  ModelComp  = new cf::GameSys::ComponentModelT();
            IntrusivePtrT<cf::GameSys::ComponentScriptT> ScriptComp = new cf::GameSys::ComponentScriptT();
            IntrusivePtrT<cf::GameSys::ComponentSoundT>  SoundComp  = new cf::GameSys::ComponentSoundT();

            ModelComp->SetMember("Name", std::string("Games/DeathMatch/Models/LifeForms/Eagle/Eagle.cmdl"));
            Ent->AddComponent(ModelComp);

            ScriptComp->SetMember("Name", std::string(m_GameConfig->ModDir + "/Scripts/Eagle.lua"));
            Ent->AddComponent(ScriptComp);

            SoundComp->SetMember("Name", std::string("Ambient/Jungle"));
            SoundComp->SetMember("AutoPlay", true);
            SoundComp->SetMember("Interval", 20.0f);
            Ent->AddComponent(SoundComp);
        }

        if (Ent->GetComponents().Size() == 0 && MapEnt->GetProperty("classname") == "monster_facehugger")
        {
            // Well, I don't think that we have any such entities in any of our maps, because
            // they're normally only created dynamically, when the player throws them by firing
            // the related weapon.
            wxMessageBox("Unexpected \"" + MapEnt->GetProperty("classname") + "\" entity found in the map!");
        }

        if (Ent->GetComponents().Size() == 0 && MapEnt->GetProperty("classname") == "PointLight")
        {
            IntrusivePtrT<cf::GameSys::ComponentRadiosityLightT> RadiosityLight = new cf::GameSys::ComponentRadiosityLightT();

            const Vector3fT I(float(wxAtof(MapEnt->GetAndRemove("intensity_r"))),
                              float(wxAtof(MapEnt->GetAndRemove("intensity_g"))),
                              float(wxAtof(MapEnt->GetAndRemove("intensity_b"))));

            RadiosityLight->SetMember("Color",     I / std::max(0.001f, length(I)));
            RadiosityLight->SetMember("Intensity", length(I));
            RadiosityLight->SetMember("ConeAngle", float(wxAtof(MapEnt->GetAndRemove("opening_angle", "180.0")) * 2.0f));
            Ent->AddComponent(RadiosityLight);

            // Some hack in CaBSP's LoadWorld.cpp file used to force direction (0, 0, -1), downwards, for the radiosity light.
            // With the new ComponentRadiosityLightT's, CaLight assumes that the main light direction is along the x-axis.
            // Thus, for backwards-compatibility, force an orientation here whose x-axis points downwards.
            Ent->GetTransform()->SetQuatWS(cf::math::QuaternionfT(cf::math::Matrix3x3fT::GetRotateYMatrix(90.0f)));
        }

        if (Ent->GetComponents().Size() == 0 && MapEnt->GetProperty("classname") == "PointLightSource")
        {
            IntrusivePtrT<cf::GameSys::ComponentPointLightT> PointLight = new cf::GameSys::ComponentPointLightT();

            const EntPropertyT* ColProp = MapEnt->FindProperty("light_color_diff");
            if (ColProp)
            {
                PointLight->SetMember("Color", ColProp->GetVector3f() / 255.0f);
                MapEnt->RemoveProperty("light_color_diff");
                MapEnt->RemoveProperty("light_color_spec");
            }

            PointLight->SetMember("On", true);
            PointLight->SetMember("Radius", float(wxAtof(MapEnt->GetAndRemove("light_radius"))));
            PointLight->SetMember("ShadowType", wxAtoi(MapEnt->GetAndRemove("light_casts_shadows")));
            Ent->AddComponent(PointLight);
        }

        if (Ent->GetComponents().Size() == 0 && MapEnt->GetProperty("classname") == "Rigid Body")
        {
            IntrusivePtrT<cf::GameSys::ComponentPhysicsT> Physics    = new cf::GameSys::ComponentPhysicsT();
            IntrusivePtrT<cf::GameSys::ComponentScriptT>  ScriptComp = new cf::GameSys::ComponentScriptT();

            Physics->SetMember("Mass", float(wxAtof(MapEnt->GetAndRemove("Mass"))));
            Ent->AddComponent(Physics);

            ScriptComp->SetMember("Name", std::string(m_GameConfig->ModDir + "/Scripts/RigidBody.lua"));
            Ent->AddComponent(ScriptComp);
        }

        if (Ent->GetComponents().Size() == 0 && MapEnt->GetProperty("classname") == "speaker")
        {
            IntrusivePtrT<cf::GameSys::ComponentSoundT> SoundComp = new cf::GameSys::ComponentSoundT();

            SoundComp->SetMember("Name", MapEnt->GetAndRemove("soundshader"));
            SoundComp->SetMember("AutoPlay", wxAtoi(MapEnt->GetAndRemove("autoplay")) != 0);
            SoundComp->SetMember("Interval", float(wxAtof(MapEnt->GetAndRemove("interval", "3.0"))));
            Ent->AddComponent(SoundComp);
        }

        if (Ent->GetComponents().Size() == 0 && MapEnt->GetProperty("classname") == "Trigger")
        {
            IntrusivePtrT<cf::GameSys::ComponentScriptT> ScriptComp = new cf::GameSys::ComponentScriptT();

            ScriptComp->SetMember("ScriptCode", std::string("local Trigger = ...\n\nfunction Trigger:OnTrigger(Other)\n    -- Add your code here.\nend\n"));
            Ent->AddComponent(ScriptComp);
        }

        if (Ent->GetComponents().Size() == 0 && MapEnt->GetProperty("classname") == "func_mover")
        {
            IntrusivePtrT<cf::GameSys::ComponentScriptT> ScriptComp = new cf::GameSys::ComponentScriptT();

            ScriptComp->SetMember("Name", std::string(m_GameConfig->ModDir + "/Scripts/Mover.lua"));
            Ent->AddComponent(ScriptComp);
        }

        if (Ent->GetComponents().Size() == 0 && (MapEnt->GetProperty("classname").substr(0, 5) == "ammo_" || MapEnt->GetProperty("classname").substr(0, 5) == "item_"))
        {
            // This is a perfect example how Prefabs would be highly useful!!!
            IntrusivePtrT<cf::GameSys::ComponentModelT>  ModelComp        = new cf::GameSys::ComponentModelT();
            IntrusivePtrT<cf::GameSys::ComponentScriptT> ScriptComp       = new cf::GameSys::ComponentScriptT();
            IntrusivePtrT<cf::GameSys::ComponentSoundT>  PickupSoundComp  = new cf::GameSys::ComponentSoundT();
            IntrusivePtrT<cf::GameSys::ComponentSoundT>  RespawnSoundComp = new cf::GameSys::ComponentSoundT();

            const wxString cn = MapEnt->GetProperty("classname");
            const char*    mn = "";

                 if (cn == "ammo_9mmclip"        ) mn ="Games/DeathMatch/Models/Items/?.cmdl";
            else if (cn == "ammo_9mmAR"          ) mn ="Games/DeathMatch/Models/Items/?.cmdl";
            else if (cn == "ammo_9mmbox"         ) mn ="Games/DeathMatch/Models/Items/?.cmdl";
            else if (cn == "ammo_ARgrenades"     ) mn ="Games/DeathMatch/Models/Items/?.cmdl";
            else if (cn == "ammo_357"            ) mn ="Games/DeathMatch/Models/Items/Ammo_DesertEagle/Ammo_DesertEagle.cmdl";
            else if (cn == "ammo_shotgun_shells" ) mn ="Games/DeathMatch/Models/Items/?.cmdl";
            else if (cn == "ammo_rpg_rocket"     ) mn ="Games/DeathMatch/Models/Items/?.cmdl";
            else if (cn == "ammo_gausscells"     ) mn ="Games/DeathMatch/Models/Items/Ammo_Gauss/Ammo_Gauss.cmdl";
            else if (cn == "ammo_crossbow_arrows") mn ="Games/DeathMatch/Models/Items/Ammo_DartGun/Ammo_DartGun.cmdl";

            else if (cn == "item_airtank"        ) mn ="Games/DeathMatch/Models/Items/?.cmdl";
            else if (cn == "item_longjump"       ) mn ="Games/DeathMatch/Models/Items/?.cmdl";
            else if (cn == "item_security"       ) mn ="Games/DeathMatch/Models/Items/?.cmdl";
            else if (cn == "item_healthkit_small") mn ="Games/DeathMatch/Models/Items/?.cmdl";
            else if (cn == "item_healthkit_big"  ) mn ="Games/DeathMatch/Models/Items/?.cmdl";
            else if (cn == "item_armor_green"    ) mn ="Games/DeathMatch/Models/Items/?.cmdl";
            else if (cn == "item_armor_yellow"   ) mn ="Games/DeathMatch/Models/Items/?.cmdl";
            else if (cn == "item_armor_red"      ) mn ="Games/DeathMatch/Models/Items/?.cmdl";

            ModelComp->SetMember("Name", std::string(mn));
            Ent->AddComponent(ModelComp);

            ScriptComp->SetMember("Name", std::string(m_GameConfig->ModDir + "/Scripts/Item.lua"));
            Ent->AddComponent(ScriptComp);

            PickupSoundComp->SetMember("Name", std::string("Item/PickUp"));
            PickupSoundComp->SetMember("AutoPlay", false);
            Ent->AddComponent(PickupSoundComp);

            RespawnSoundComp->SetMember("Name", std::string("Item/Respawn"));
            RespawnSoundComp->SetMember("AutoPlay", false);
            Ent->AddComponent(RespawnSoundComp);
        }

        if (Ent->GetComponents().Size() == 0 && MapEnt->GetProperty("classname").substr(0, 7) == "weapon_")
        {
            // This is a perfect example how Prefabs would be highly useful!!!
            IntrusivePtrT<cf::GameSys::ComponentModelT>  ModelComp        = new cf::GameSys::ComponentModelT();
            IntrusivePtrT<cf::GameSys::ComponentScriptT> ScriptComp       = new cf::GameSys::ComponentScriptT();
            IntrusivePtrT<cf::GameSys::ComponentSoundT>  PickupSoundComp  = new cf::GameSys::ComponentSoundT();
            IntrusivePtrT<cf::GameSys::ComponentSoundT>  RespawnSoundComp = new cf::GameSys::ComponentSoundT();

            const wxString cn = MapEnt->GetProperty("classname").substr(7);
            const char*    mn = "";

                 if (cn == "battlescythe") mn = "Games/DeathMatch/Models/Weapons/BattleScythe/BattleScythe_w.cmdl";
            else if (cn == "hornetgun"   ) mn = "Games/DeathMatch/Models/Weapons/BattleScythe/BattleScythe_w.cmdl"; // For b-w compatibility, as our DeathMatch implementation at this time doesn't support Hornet Guns.
            else if (cn == "9mmhandgun"  ) mn = "Games/DeathMatch/Models/Weapons/Beretta/Beretta_w.cmdl";
            else if (cn == "357handgun"  ) mn = "Games/DeathMatch/Models/Weapons/DesertEagle/DesertEagle_w.cmdl";
            else if (cn == "9mmAR"       ) mn = "Games/DeathMatch/Models/Weapons/9mmAR/9mmAR_w.cmdl";
            else if (cn == "shotgun"     ) mn = "Games/DeathMatch/Models/Weapons/Shotgun/Shotgun_w.cmdl";
            else if (cn == "crossbow"    ) mn = "Games/DeathMatch/Models/Weapons/DartGun/DartGun_w.cmdl";
            else if (cn == "rpg"         ) mn = "Games/DeathMatch/Models/Weapons/Bazooka/Bazooka_w.cmdl";
            else if (cn == "gauss"       ) mn = "Games/DeathMatch/Models/Weapons/Gauss/Gauss_w.cmdl";
            else if (cn == "egon"        ) mn = "Games/DeathMatch/Models/Weapons/Egon/Egon_w.cmdl";
            else if (cn == "handgrenade" ) mn = "Games/DeathMatch/Models/Weapons/Grenade/Grenade_w.cmdl";
            else if (cn == "tripmine"    ) mn = "Games/DeathMatch/Models/Weapons/Grenade/Grenade_w.cmdl";           // For b-w compatibility, as our DeathMatch implementation at this time doesn't support Tripmines.
            else if (cn == "facehugger"  ) mn = "Games/DeathMatch/Models/Weapons/FaceHugger/FaceHugger_w.cmdl";

            ModelComp->SetMember("Name", std::string(mn));
            Ent->AddComponent(ModelComp);

            ScriptComp->SetMember("Name", std::string(m_GameConfig->ModDir + "/Scripts/Weapon.lua"));
            Ent->AddComponent(ScriptComp);

            PickupSoundComp->SetMember("Name", std::string("Item/PickUp"));
            PickupSoundComp->SetMember("AutoPlay", false);
            Ent->AddComponent(PickupSoundComp);

            RespawnSoundComp->SetMember("Name", std::string("Item/Respawn"));
            RespawnSoundComp->SetMember("AutoPlay", false);
            Ent->AddComponent(RespawnSoundComp);
        }
    }
}


MapDocumentT::~MapDocumentT()
{
    // This method does what the destructor of the base class of the MapDocumentT, that is, the
    // SubjectT's destructor, would do anyway: It notifies all observers that the map document
    // is about to die.
    //
    // However, in this case, it would happen only *after* our setting of `m_ScriptWorld = NULL`
    // below. An observer that keeps `IntrusivePtrT<cf::GameSys::EntityT>`s, such as the Entity
    // Hierarchy dialog, would then release them, and the related `EntityT` destructor would run.
    // If such a destructor tried access the entity's world, e.g. a component trying to release a
    // shared resource, it would access a world instance that has already been deleted and crash.
    //
    // Therefore, we have to anticipate the de-coupling of the observers here, while the "parent"
    // resources of the map document are still fully available. Note that this is also related to
    // the "unexpected" window destruction order that can occur with wxWidgets: The parent frame
    // (the `ChildFrameT` instance that in turn holds this `MapDocumentT`) is destroyed *before*
    // its child windows (the observers). If the order was opposite, as one would normally expect,
    // the entire problem would not exist in the first place.
    UpdateAllObservers_SubjectDies();

    delete m_BspTree;
    m_BspTree = NULL;

    for (unsigned long GroupNr = 0; GroupNr < m_Groups.Size(); GroupNr++) delete m_Groups[GroupNr];
    m_Groups.Clear();

    m_Selection.Clear();

    m_ScriptWorld = NULL;
}


namespace
{
    const char* StripNamespace(const char* ClassName)
    {
        if (strncmp(ClassName, "GameSys::", 9) == 0)
            return ClassName + 9;

        return ClassName;
    }


    // Recursively saves the entity instantiation of the passed entity and all of its children.
    void SaveEntityInstantiation(std::ostream& OutFile, IntrusivePtrT<cf::GameSys::EntityT> Entity, const wxString& ParentName)
    {
        if (ParentName == "")
        {
            // Don't modify the global script state, so that multiple maps can be loaded into the same world without
            // interfering with each other (that is, without overwriting each other's global variables).
            // This is especially useful with "prefabs", which we implement as small but normal map files.
            OutFile << "local ";
        }

        OutFile << ParentName << Entity->GetBasics()->GetEntityName() << " = world:new(\"" << StripNamespace(Entity->GetType()->ClassName) << "\", \"" << Entity->GetBasics()->GetEntityName() << "\")\n";

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
        cf::TypeSys::VarVisitorToLuaCodeT ToLua(OutFile);

        if (Entity->GetBasics()->IsStatic())
            OutFile << "    self:GetBasics():set(\"Static\", true)\n";

        const ArrayT<cf::TypeSys::VarBaseT*>& TrafoVars = Entity->GetTransform()->GetMemberVars().GetArray();

        for (unsigned int VarNr = 0; VarNr < TrafoVars.Size(); VarNr++)
        {
            OutFile << "    self:GetTransform():set(\"" << TrafoVars[VarNr]->GetName() << "\", ";
            TrafoVars[VarNr]->accept(ToLua);
            OutFile << ")\n";
        }

        if (Entity->GetComponents().Size() == 0)
            return;

        for (unsigned int CompNr = 1; CompNr <= Entity->GetComponents().Size(); CompNr++)
        {
            IntrusivePtrT<cf::GameSys::ComponentBaseT> Comp = Entity->GetComponents()[CompNr - 1];
            const ArrayT<cf::TypeSys::VarBaseT*>&      Vars = Comp->GetMemberVars().GetArray();

            OutFile << "\n";
            OutFile << "    local c" << CompNr << " = world:new(\"" << StripNamespace(Comp->GetType()->ClassName) << "\")\n";

            for (unsigned int VarNr = 0; VarNr < Vars.Size(); VarNr++)
            {
                const cf::TypeSys::VarBaseT* Var = Vars[VarNr];

                // This is the same test as in WriteDoxyVars() in AppCaWE.cpp, see there for details.
                wxASSERT(Var->GetName() == cf::String::ToLuaIdentifier(Var->GetName()));

                OutFile << "    c" << CompNr << ":set(\"" << Var->GetName() << "\", ";
                Var->accept(ToLua);
                OutFile << ")\n";
            }
        }

        OutFile << "\n";
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
}


/*static*/ void MapDocumentT::SaveEntities(std::ostream& OutFile, IntrusivePtrT<cf::GameSys::EntityT> RootEntity)
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


bool MapDocumentT::OnSaveDocument(const wxString& cmapFileName, bool IsAutoSave, IntrusivePtrT<cf::GameSys::EntityT> RootEntity)
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

    // This sets the cursor to the busy cursor in its ctor, and back to the default cursor in the dtor.
    wxBusyCursor BusyCursor;
    const bool   UpdateFileName = (RootEntity == NULL);     // We're saving a map if RootEntity == NULL, otherwise a prefab.

    if (RootEntity == NULL)
        RootEntity = m_ScriptWorld->GetRootEntity();

    // Save the `.cmap` file.
    {
        cmapOutFile << "// Cafu Map File\n"
                    << "// Written by CaWE, the Cafu World Editor.\n"
                    << "Version " << CMAP_FILE_VERSION << "\n"
                    << "\n";

        // Save groups.
        for (unsigned long GroupNr=0; GroupNr<m_Groups.Size(); GroupNr++)
            m_Groups[GroupNr]->Save_cmap(cmapOutFile, GroupNr);

        // Save entities (in depth-first order, as in the .cent file).
        ArrayT< IntrusivePtrT<cf::GameSys::EntityT> > AllScriptEnts;

        RootEntity->GetAll(AllScriptEnts);

        for (unsigned long EntNr = 0/*with world*/; EntNr < AllScriptEnts.Size(); EntNr++)
        {
            const BoundingBox3fT* Intersecting = NULL;
            IntrusivePtrT<const CompMapEntityT> Ent = GetMapEnt(AllScriptEnts[EntNr]);

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
        SaveEntities(centOutFile, RootEntity);

        if (centOutFile.fail())
        {
            wxMessageBox("There was an error saving the file. Please try again.", "File not saved!", wxOK | wxICON_ERROR);
            return false;
        }
    }

    // If this was an auto-save, do not change the filename (nor set the document as "not modified").
    if (IsAutoSave) return true;

    // If we were saving a prefab, do not change the filename (nor set the document as "not modified").
    if (!UpdateFileName) return true;

    m_FileName = cmapFileName;
    return true;
}


bool MapDocumentT::SaveAs()
{
    static wxString  LastUsedDir=m_GameConfig->ModDir+"/Maps";
    const wxFileName FN(m_FileName);

    wxFileDialog SaveFileDialog(NULL,                               // parent
                                "Save (or Export) Cafu Map File",   // message
                                (FN.IsOk() && wxDirExists(FN.GetPath())) ? FN.GetPath() : LastUsedDir, // default dir
                                (FN.IsOk() && FN.GetExt() == "cmap") ? FN.GetFullName() : "", // default file
                                "Cafu Map Files (*.cmap)|*.cmap"    // wildcard
                             // "|Export Hammer (HL1) Maps (*.map)|*.map"
                             // "|Export Hammer RMFs (*.rmf)|*.rmf"
                                "|All Files (*.*)|*.*",
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


bool MapDocumentT::CompatSubmitCommand(CommandT* Command)
{
    return m_ChildFrame->SubmitCommand(Command);
}


IntrusivePtrT<MapEditor::CompMapEntityT> MapDocumentT::GetRootMapEntity() const
{
    return GetMapEnt(m_ScriptWorld->GetRootEntity());
}


void MapDocumentT::GetAllElems(ArrayT<MapElementT*>& Elems) const
{
    ArrayT< IntrusivePtrT<cf::GameSys::EntityT> > AllEnts;
    m_ScriptWorld->GetRootEntity()->GetAll(AllEnts);

    for (unsigned int EntNr = 0; EntNr < AllEnts.Size(); EntNr++)
    {
        IntrusivePtrT<CompMapEntityT> Ent = GetMapEnt(AllEnts[EntNr]);

        // Add the entity representation...
        Elems.PushBack(Ent->GetRepres());

        // ... and all of its primitives.
        for (unsigned int PrimNr = 0; PrimNr < Ent->GetPrimitives().Size(); PrimNr++)
            Elems.PushBack(Ent->GetPrimitives()[PrimNr]);
    }
}


void MapDocumentT::Insert(IntrusivePtrT<cf::GameSys::EntityT> Entity, IntrusivePtrT<cf::GameSys::EntityT> Parent, unsigned long Pos)
{
    wxASSERT(Entity != NULL);
    wxASSERT(Entity->GetParent() == NULL);
    wxASSERT(Entity != m_ScriptWorld->GetRootEntity());

    wxASSERT(Parent != NULL);
    wxASSERT(Parent->GetRoot() == m_ScriptWorld->GetRootEntity());

    Parent->AddChild(Entity, Pos);

    // Insert all map elements of Entity and of all of its children into the BSP tree.
    const ArrayT<MapElementT*> MapElements = GetMapEnt(Entity)->GetAllMapElements();

    for (unsigned long ElemNr = 0; ElemNr < MapElements.Size(); ElemNr++)
        m_BspTree->Insert(MapElements[ElemNr]);
}


void MapDocumentT::Insert(MapPrimitiveT* Prim, IntrusivePtrT<CompMapEntityT> ParentEnt)
{
    wxASSERT(Prim != NULL);

    wxASSERT(ParentEnt != NULL);
    wxASSERT(ParentEnt->GetEntity()->GetRoot() == m_ScriptWorld->GetRootEntity());

    ParentEnt->AddPrim(Prim);
    wxASSERT(ParentEnt == Prim->GetParent());

    m_BspTree->Insert(Prim);
}


void MapDocumentT::Remove(IntrusivePtrT<cf::GameSys::EntityT> Entity)
{
    wxASSERT(Entity != NULL);
    wxASSERT(Entity->GetParent() != NULL);
    wxASSERT(Entity->GetRoot() == m_ScriptWorld->GetRootEntity());
    wxASSERT(Entity != m_ScriptWorld->GetRootEntity());

    // The caller (e.g. the Delete command) has taken ownership of `Entity`,
    // that is, keeps another `IntrusivePtrT<cf::GameSys::EntityT>` to `Entity`.
    Entity->GetParent()->RemoveChild(Entity);

    // Remove all map elements of Entity and of all of its children from the BSP tree.
    const ArrayT<MapElementT*> MapElements = GetMapEnt(Entity)->GetAllMapElements();

    for (unsigned long ElemNr = 0; ElemNr < MapElements.Size(); ElemNr++)
        m_BspTree->Remove(MapElements[ElemNr]);
}


void MapDocumentT::Remove(MapPrimitiveT* Prim)
{
    wxASSERT(Prim != NULL);

    IntrusivePtrT<CompMapEntityT> ParentEnt = Prim->GetParent();

    wxASSERT(ParentEnt != NULL);
    wxASSERT(ParentEnt->GetEntity()->GetRoot() == m_ScriptWorld->GetRootEntity());

    ParentEnt->RemovePrim(Prim);
    wxASSERT(Prim->GetParent() == NULL);

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
    ArrayT<MapElementT*> Result;

    ArrayT< IntrusivePtrT<cf::GameSys::EntityT> > AllEnts;
    m_ScriptWorld->GetRootEntity()->GetAll(AllEnts);

    for (unsigned int EntNr = 0; EntNr < AllEnts.Size(); EntNr++)
    {
        IntrusivePtrT<CompMapEntityT> Ent = GetMapEnt(AllEnts[EntNr]);

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

    // Update the m_SelectionBB member whenever the selection is changed.
    // Updating the value only when `GetMostRecentSelBB()` is called by the user is not enough, because selecting
    // something, then clearing the selection again would not be accounted for if the m_SelectionBB member was not
    // also updated immediately when a new selection is set.
    GetMostRecentSelBB();

    if (m_Selection.Size() > 0)
    {
        MapElementT*                        SelElem = m_Selection[m_Selection.Size() - 1];
        IntrusivePtrT<cf::GameSys::EntityT> SelEnt  = SelElem->GetParent()->GetEntity();

        // As long as the parent entity is selected, too, bubble up.
        while (SelEnt->GetParent() != NULL && GetMapEnt(SelEnt->GetParent())->GetRepres()->IsSelected())
            SelEnt = SelEnt->GetParent();

        // If the new selection is empty, the m_PasteParentID is (intentionally) not updated.
        // If SelElem is a primitive, a "Paste" will insert the new objects *next* to it.
        // If SelElem is an entity,   a "Paste" will insert the new objects *into* to it.
        m_PasteParentID = SelEnt->GetID();
    }
}


ArrayT< IntrusivePtrT<cf::GameSys::EntityT> > MapDocumentT::GetSelectedEntities() const
{
    ArrayT< IntrusivePtrT<cf::GameSys::EntityT> > SelEnts;

    for (unsigned long SelNr = 0; SelNr < m_Selection.Size(); SelNr++)
        if (m_Selection[SelNr]->GetType() == &MapEntRepresT::TypeInfo)
            SelEnts.PushBack(m_Selection[SelNr]->GetParent()->GetEntity());

    return SelEnts;
}


void MapDocumentT::GetUsedMaterials(ArrayT<EditorMaterialI*>& UsedMaterials) const
{
    std::map<EditorMaterialI*, int> UsedMatMap;

    ArrayT< IntrusivePtrT<cf::GameSys::EntityT> > AllEnts;
    m_ScriptWorld->GetRootEntity()->GetAll(AllEnts);

    for (unsigned long EntNr = 0; EntNr < AllEnts.Size(); EntNr++)
    {
        const ArrayT<MapPrimitiveT*>& Primitives = GetMapEnt(AllEnts[EntNr])->GetPrimitives();

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


void MapDocumentT::OnMapAutoGroupEntities(wxCommandEvent& CE)
{
    m_AutoGroupEntities = !m_AutoGroupEntities;
}


void MapDocumentT::OnUpdateMapAutoGroupEntities(wxUpdateUIEvent& UE)
{
    UE.Check(m_AutoGroupEntities);
}


void MapDocumentT::OnMapGotoPrimitive(wxCommandEvent& CE)
{
    GotoPrimitiveDialogT GotoPrimDialog;

    if (GotoPrimDialog.ShowModal()!=wxID_OK) return;

    ArrayT< IntrusivePtrT<cf::GameSys::EntityT> > AllEnts;
    m_ScriptWorld->GetRootEntity()->GetAll(AllEnts);

    if (GotoPrimDialog.m_EntityNumber >= int(AllEnts.Size()))
    {
        wxMessageBox("The entity with the given index number does not exist.", "Goto Primitive");
        return;
    }

    if (GotoPrimDialog.m_PrimitiveNumber >= int(GetMapEnt(AllEnts[GotoPrimDialog.m_EntityNumber])->GetPrimitives().Size()))
    {
        wxMessageBox("The primitive with the given index number does not exist (in the specified entity).", "Goto Primitive");
        return;
    }

    MapPrimitiveT* Prim = GetMapEnt(AllEnts[GotoPrimDialog.m_EntityNumber])->GetPrimitives()[GotoPrimDialog.m_PrimitiveNumber];

    if (!Prim->IsVisible())
    {
        wxMessageBox("The primitive is currently invisible in group \"" + Prim->GetGroup()->Name + "\".", "Goto Primitive");
        return;
    }

    // The primitive was found and is visible, now select it and center the 2D views on it.
    CompatSubmitCommand(CommandSelectT::Set(this, Prim));
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
    lua_State* LuaState = luaL_newstate();

    try
    {
        if (LuaState==NULL) throw wxString("Couldn't open Lua state.");
        if (!wxFileExists(PointFileName)) throw wxString("The file does not exist.");

        luaL_requiref(LuaState, "_G",            luaopen_base,      1); lua_pop(LuaState, 1);
        luaL_requiref(LuaState, LUA_LOADLIBNAME, luaopen_package,   1); lua_pop(LuaState, 1);
        luaL_requiref(LuaState, LUA_COLIBNAME,   luaopen_coroutine, 1); lua_pop(LuaState, 1);
        luaL_requiref(LuaState, LUA_TABLIBNAME,  luaopen_table,     1); lua_pop(LuaState, 1);
        luaL_requiref(LuaState, LUA_IOLIBNAME,   luaopen_io,        1); lua_pop(LuaState, 1);
        luaL_requiref(LuaState, LUA_OSLIBNAME,   luaopen_os,        1); lua_pop(LuaState, 1);
        luaL_requiref(LuaState, LUA_STRLIBNAME,  luaopen_string,    1); lua_pop(LuaState, 1);
        luaL_requiref(LuaState, LUA_BITLIBNAME,  luaopen_bit32,     1); lua_pop(LuaState, 1);
        luaL_requiref(LuaState, LUA_MATHLIBNAME, luaopen_math,      1); lua_pop(LuaState, 1);

        // Load and process the Lua program that defines the path.
        if (luaL_loadfile(LuaState, PointFileName.c_str())!=0 || lua_pcall(LuaState, 0, 0, 0)!=0)
            throw wxString("Couldn't load the file:\n")+lua_tostring(LuaState, -1);

        // Read the points.
        wxASSERT(lua_gettop(LuaState)==0);
        m_PointFilePoints.Clear();
        lua_getglobal(LuaState, "Points");
        const size_t NumPoints=lua_rawlen(LuaState, 1);

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
        const size_t NumColors=lua_rawlen(LuaState, 1);

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


void MapDocumentT::OnSelectionGroup(wxCommandEvent& CE)
{
    if (m_Selection.Size()==0) return;

    // Warn/inform the user when he tries to group elements with "mixed affiliation"?  (grouped or not, in entity or not)
    // e.g. like "From N selected map elements, there are X in the world, Y in entities and Z in groups."
    // This is pretty difficult to implement though, as there are many cases to distinguish, and might confuse the user.
    // Alternatively, just warn when grouping here *partially* breaks an existing group??
    ArrayT<CommandT*> SubCommands;

    // 1. Create a new group.
    CommandNewGroupT* CmdNewGroup = new CommandNewGroupT(*this, wxString::Format("%lu item%s", m_Selection.Size(), m_Selection.Size()==1 ? "" : "s"));
    GroupT*              NewGroup = CmdNewGroup->GetGroup();

    CmdNewGroup->Do();
    SubCommands.PushBack(CmdNewGroup);

    // 2. Put the m_Selection into the new group.
    CommandAssignGroupT* CmdAssign = new CommandAssignGroupT(*this, m_Selection, NewGroup);

    CmdAssign->Do();
    SubCommands.PushBack(CmdAssign);

    // 3. Hide the new group (set the visibility to false).
    if (CE.GetId() == ChildFrameT::ID_MENU_SELECTION_HIDE)
    {
        CommandGroupSetVisibilityT* CmdVis = new CommandGroupSetVisibilityT(*this, NewGroup, false);

        CmdVis->Do();
        SubCommands.PushBack(CmdVis);
    }

    // 4. Purge all groups that became empty (abandoned) by the (re-)assignment of map elements in step 2.
    CommandDeleteGroupT* CmdPurgeGroups = new CommandDeleteGroupT(*this, GetAbandonedGroups());

    CmdPurgeGroups->Do();
    SubCommands.PushBack(CmdPurgeGroups);

    // 5. Submit the composite macro command.
    CompatSubmitCommand(new CommandMacroT(SubCommands, "Hide "+NewGroup->Name));
}


void MapDocumentT::OnSelectionHideOther(wxCommandEvent& CE)
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
    if (HideElems.Size() == 0) return;


    ArrayT<CommandT*> SubCommands;

    // 1. Create a new group.
    CommandNewGroupT* CmdNewGroup = new CommandNewGroupT(*this, wxString::Format("%lu item%s", HideElems.Size(), HideElems.Size() == 1 ? "" : "s"));
    GroupT*              NewGroup = CmdNewGroup->GetGroup();

    CmdNewGroup->Do();
    SubCommands.PushBack(CmdNewGroup);

    // 2. Put the HideElems into the new group.
    CommandAssignGroupT* CmdAssign = new CommandAssignGroupT(*this, HideElems, NewGroup);

    CmdAssign->Do();
    SubCommands.PushBack(CmdAssign);

    // 3. Hide the new group (set the visibility to false).
    CommandGroupSetVisibilityT* CmdVis = new CommandGroupSetVisibilityT(*this, NewGroup, false);

    CmdVis->Do();
    SubCommands.PushBack(CmdVis);

    // 4. Purge all groups that became empty (abandoned) by the (re-)assignment of map elements in step 2.
    CommandDeleteGroupT* CmdPurgeGroups = new CommandDeleteGroupT(*this, GetAbandonedGroups());

    CmdPurgeGroups->Do();
    SubCommands.PushBack(CmdPurgeGroups);

    // 5. Submit the composite macro command.
    CompatSubmitCommand(new CommandMacroT(SubCommands, "Hide "+NewGroup->Name));
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
    CompatSubmitCommand(new CommandCarveT(*this, Carvers));

    // If there are any empty groups (usually as a result from an implicit deletion by the carve), purge them now.
    // We use an explicit command for deleting the groups (instead of putting everything into a macro command)
    // so that the user has the option to undo the purge (separately from the deletion) if he wishes.
    const ArrayT<GroupT*> EmptyGroups = GetAbandonedGroups();

    if (EmptyGroups.Size() > 0)
        CompatSubmitCommand(new CommandDeleteGroupT(*this, EmptyGroups));
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
    CompatSubmitCommand(new CommandMakeHollowT(*this, WallWidth, m_Selection));
}


// This method assigns all selected primitives (brushes, Bezier patches, ...) to a single entity.
// If along with the primitives
//   - *no* entity is selected, a new entity is created;
//   - exactly *one* entity is selected, all primitives are assigned to this entity;
//   - *multiple* entities are selected, the user is asked to choose the desired target entity.
void MapDocumentT::OnSelectionAssignToEntity(wxCommandEvent& CE)
{
    // Split the selection into entities and primitives.
    ArrayT< IntrusivePtrT<CompMapEntityT> > SelEntities;    // All entities   that are in the selection.
    ArrayT<MapPrimitiveT*>                  SelPrimitives;  // All primitives that are in the selection.

    for (unsigned long SelNr = 0; SelNr < m_Selection.Size(); SelNr++)
    {
        MapElementT* Elem = m_Selection[SelNr];

        if (Elem->GetParent()->GetRepres() == Elem)
        {
            wxASSERT(dynamic_cast<MapEntRepresT*>(Elem));
            SelEntities.PushBack(Elem->GetParent());
        }
        else
        {
            wxASSERT(dynamic_cast<MapPrimitiveT*>(Elem));
            SelPrimitives.PushBack(static_cast<MapPrimitiveT*>(Elem));
        }
    }

    // If there were no primitives among the selected map elements, quit here.
    if (SelPrimitives.Size() == 0)
    {
        wxMessageBox(
            "No map primitives are currently selected.\n\n"
            "Select at least one map primitive (and optionally the desired target entity) "
            "in order to have the selected map primitives re-assigned to a new or existing entity.",
            "Assignment is empty");

        return;
    }

    // Determine the target entity.
    ArrayT<CommandT*>             SubCommands;
    IntrusivePtrT<CompMapEntityT> TargetEntity = NULL;

    if (SelEntities.Size() == 0)
    {
        // Create a new entity.
        IntrusivePtrT<cf::GameSys::EntityT> NewEnt = new cf::GameSys::EntityT(cf::GameSys::EntityCreateParamsT(*m_ScriptWorld));
        IntrusivePtrT<CompMapEntityT>       MapEnt = new CompMapEntityT(*this);

        NewEnt->GetBasics()->SetMember("Static", true);
        NewEnt->GetTransform()->SetOriginWS(SnapToGrid(GetMostRecentSelBB().GetCenter(), false /*Toggle*/, -1 /*AxisNoSnap*/));
        NewEnt->SetApp(MapEnt);

        CommandNewEntityT* CmdNewEnt = new CommandNewEntityT(
            *this, NewEnt, SelPrimitives[0]->GetParent()->GetEntity(), false /*SetSelection?*/);

        CmdNewEnt->Do();
        SubCommands.PushBack(CmdNewEnt);

        CommandSelectT* CmdAddSel = CommandSelectT::Add(this, MapEnt->GetRepres());

        CmdAddSel->Do();
        SubCommands.PushBack(CmdAddSel);

        TargetEntity = MapEnt;
    }
    else if (SelEntities.Size() == 1)
    {
        TargetEntity = SelEntities[0];

        // Check if all map primitives are assigned to the single selected entity already.
        // If so, extra considerations are warranted.
        unsigned long PrimNr = 0;

        for (PrimNr = 0; PrimNr < SelPrimitives.Size(); PrimNr++)
            if (SelEntities[0] != SelPrimitives[PrimNr]->GetParent())
                break;

        if (PrimNr >= SelPrimitives.Size())
        {
            if (SelEntities[0] == GetRootMapEntity())
            {
                wxMessageBox(
                    "All selected map primitives are assigned to the selected entity already.\n\n"
                    "Select a different entity (or none, in order to have automatically created a new one) "
                    "in order to have the selected map primitives re-assigned.",
                    "Assignment has no effect");

                return;
            }

            if (wxMessageBox(
                "Assign to root entity \"" + GetRootMapEntity()->GetEntity()->GetBasics()->GetEntityName() + "\" instead?\n\n"
                "All selected map primitives are assigned to the selected entity already, "
                "so the assignment as-is has no effect.\n\n"
                "Click \"Yes\" in order to assign the selected map primitives to the map's root entity \"" + GetRootMapEntity()->GetEntity()->GetBasics()->GetEntityName() + "\" instead.\n\n"
                "Click \"No\" to cancel. You can then select a different entity "
                "(or none, in order to have automatically created a new one) "
                "in order to have the selected map primitives re-assigned.",
                "Assignment has no effect", wxYES_NO | wxCENTRE) != wxYES)
            {
                return;
            }

            TargetEntity = GetRootMapEntity();
        }
    }
    else
    {
        wxArrayString EntityNames;

        for (unsigned long EntNr = 0; EntNr < SelEntities.Size(); EntNr++)
        {
            wxString            Label    = SelEntities[EntNr]->GetEntity()->GetBasics()->GetEntityName();
            const unsigned long NumPrims = SelEntities[EntNr]->GetPrimitives().Size();

            if (NumPrims > 0)
                Label += wxString::Format(" (%lu)", NumPrims);

            EntityNames.Add(Label);
        }

        const int Choice = wxGetSingleChoiceIndex(
            "Multiple entities are currently selected. In the following list,\n"
            "choose the one that the map primitives are to be assigned to\n"
            "(or press Cancel, select only one entity, and start over).",
            "Assignment is ambiguous",
            EntityNames);

        if (Choice < 0) return;               // The user pressed Cancel.

        TargetEntity = SelEntities[Choice];   // Make sure that the chosen entity is at index 0.
    }

    // Assign the primitives to the target entity.
    CommandReparentPrimitiveT* CmdRepPrim = new CommandReparentPrimitiveT(*this, SelPrimitives, TargetEntity);

    CmdRepPrim->Do();
    SubCommands.PushBack(CmdRepPrim);

    // Submit the macro command.
    CompatSubmitCommand(new CommandMacroT(SubCommands, "Assign primitives"));

    // Purge empty entities.
    // This is intentionally not submitted along with the macro command above, so that the user
    // has the option to undo the purge (separately from the assignment) if he wishes.
    ArrayT<MapElementT*> EmptyEntities;
    wxString Msg = "Delete empty entities?\n\nThe following entities don't have any primitives:\n\n";

    for (unsigned long EntNr = 0; EntNr < SelEntities.Size(); EntNr++)
    {
        IntrusivePtrT<cf::GameSys::EntityT> SelEnt = SelEntities[EntNr]->GetEntity();

        if (SelEntities[EntNr]->GetPrimitives().Size() == 0 && !SelEnt->Has(TargetEntity->GetEntity()))
        {
            EmptyEntities.PushBack(SelEntities[EntNr]->GetRepres());

            Msg += SelEnt->GetBasics()->GetEntityName();

            for (unsigned int CompNr = 0; CompNr < SelEnt->GetComponents().Size(); CompNr++)
            {
                Msg += (CompNr == 0) ? " (": "";
                Msg += SelEnt->GetComponents()[CompNr]->GetName();
                Msg += (CompNr+1 < SelEnt->GetComponents().Size()) ? ", " : ")";
            }

            Msg += "\n";
        }
    }

    if (EmptyEntities.Size() > 0)
    {
        Msg += "\nShould these entities be deleted now?";

        if (wxMessageBox(Msg, "Assignment left entities without primitives", wxYES_NO) == wxYES)
        {
            CompatSubmitCommand(new CommandDeleteT(*this, EmptyEntities));
        }
    }

    // Purge empty groups.
    // This is very rare: it only fires when a parent entity became empty, was thus deleted above, and was the last element in its group:
    // If there are any empty groups (usually as a result from the deletion), purge them now.
    // We use an explicit command for deleting the groups (instead of putting everything into a macro command)
    // so that the user has the option to undo the purge (separately from the deletion) if he wishes.
    {
        const ArrayT<GroupT*> EmptyGroups = GetAbandonedGroups();

        if (EmptyGroups.Size() > 0)
            CompatSubmitCommand(new CommandDeleteGroupT(*this, EmptyGroups));
    }

    m_ChildFrame->GetToolManager().SetActiveTool(GetToolTIM().FindTypeInfoByName("ToolSelectionT"));
    m_ChildFrame->ShowPane(m_ChildFrame->GetInspectorDialog());
}


void MapDocumentT::OnUpdateToolsApplyMaterial(wxUpdateUIEvent& UE)
{
    UE.Enable(m_ChildFrame->GetToolManager().GetActiveToolType() != &ToolEditSurfaceT::TypeInfo);
}


void MapDocumentT::OnToolsApplyMaterial(wxCommandEvent& CE)
{
    CommandT* Command = new CommandApplyMaterialT(*this, m_Selection, m_GameConfig->GetMatMan().GetDefaultMaterial());

    CompatSubmitCommand(Command);
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

        CompatSubmitCommand(
            new CommandTransformT(*this, m_Selection, Mode, RefPoint, Value, Options.general.LockingTextures));
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

    CompatSubmitCommand(
        new CommandAlignT(*this, m_Selection, ViewWindows[0]->GetAxesInfo(), GetMostRecentSelBB(), CE.GetId(), Options.general.LockingTextures));
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

    CompatSubmitCommand(new CommandMirrorT(*this, m_Selection, NormalAxis, Dist, Options.general.LockingTextures));
}


void MapDocumentT::OnUpdateToolsMaterialLock(wxUpdateUIEvent& UE)
{
    UE.Check(Options.general.LockingTextures);
}


void MapDocumentT::OnUpdateViewShowEntityInfo(wxUpdateUIEvent& UE)
{
    UE.Check(Options.view2d.ShowEntityInfo);
}


void MapDocumentT::OnUpdateViewShowEntityTargets(wxUpdateUIEvent& UE)
{
    UE.Check(Options.view2d.ShowEntityTargets);
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


IntrusivePtrT<cf::GameSys::EntityT> MapDocumentT::GetPasteParent() const
{
    IntrusivePtrT<cf::GameSys::EntityT> Ent = m_ScriptWorld->GetRootEntity()->FindID(m_PasteParentID);

    return (Ent != NULL) ? Ent : m_ScriptWorld->GetRootEntity();
}


void MapDocumentT::SetPasteParent(unsigned int ID)
{
    m_PasteParentID = ID;
}


/*static*/
void MapDocumentT::Reduce(ArrayT<MapElementT*>& Elems)
{
    for (unsigned int ElemNr = 0; ElemNr < Elems.Size(); ElemNr++)
    {
        // If this element's entity is part of a tree, the element will be removed.
        IntrusivePtrT<cf::GameSys::EntityT> Entity = Elems[ElemNr]->GetParent()->GetEntity();

        for (unsigned int TreeNr = 0; TreeNr < Elems.Size(); TreeNr++)
        {
            // A tree can only be the entity of an element that is a MapEntRepresT
            // and is different from the currently considered element itself.
            if (TreeNr == ElemNr) continue;

            MapElementT* TreeElem = Elems[TreeNr];

            // There should never be any duplicates in Elems.
            wxASSERT(TreeElem != Elems[ElemNr]);

            if (TreeElem->GetType() != &MapEntRepresT::TypeInfo) continue;

            // Double-check that this is really a MapEntRepresT.
            wxASSERT(TreeElem->GetParent()->GetRepres() == TreeElem);

            IntrusivePtrT<cf::GameSys::EntityT> Tree = TreeElem->GetParent()->GetEntity();

            if (Tree->Has(Entity))
            {
                Elems.RemoveAt(ElemNr);
                ElemNr--;
                break;
            }
        }
    }
}


ArrayT<GroupT*> MapDocumentT::GetAbandonedGroups() const
{
    ArrayT<GroupT*>      EmptyGroups;
    ArrayT<MapElementT*> AllElems;

    GetAllElems(AllElems);

    for (unsigned long GroupNr = 0; GroupNr < m_Groups.Size(); GroupNr++)
    {
        bool IsEmpty = true;

        for (unsigned int ElemNr = 0; ElemNr < AllElems.Size(); ElemNr++)
        {
            if (AllElems[ElemNr]->GetGroup() == m_Groups[GroupNr])
            {
                IsEmpty = false;
                break;
            }
        }

        if (IsEmpty) EmptyGroups.PushBack(m_Groups[GroupNr]);
    }

    return EmptyGroups;
}
