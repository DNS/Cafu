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
#include "ChildFrameViewWin.hpp"
#include "DialogEditSurfaceProps.hpp"
#include "DialogInspector.hpp"
#include "EntityClass.hpp"
#include "GameConfig.hpp"
#include "DialogGotoPrimitive.hpp"
#include "ParentFrame.hpp"
#include "DialogMapCheck.hpp"
#include "MapDocument.hpp"
#include "MapBezierPatch.hpp"
#include "MapEntity.hpp"
#include "MapModel.hpp"         // Only needed for some TypeInfo test...
#include "MapPlant.hpp"         // Only needed for some TypeInfo test...
#include "MapTerrain.hpp"       // Only needed for some TypeInfo test...
#include "EntityClassVar.hpp"
#include "DialogMapInfo.hpp"
#include "MapBrush.hpp"
#include "MapWorld.hpp"
#include "DialogOptions.hpp"
#include "Options.hpp"
#include "OrthoBspTree.hpp"
#include "DialogPasteSpecial.hpp"
#include "DialogReplaceMaterials.hpp"
#include "MapCommands/Transform.hpp"
#include "MapCommands/Align.hpp"
#include "MapCommands/ApplyMaterial.hpp"
#include "MapCommands/AssignPrimToEnt.hpp"
#include "MapCommands/Carve.hpp"
#include "MapCommands/Delete.hpp"
#include "MapCommands/Mirror.hpp"
#include "MapCommands/SnapToGrid.hpp"
#include "MapCommands/MakeHollow.hpp"
#include "MapCommands/NewEntity.hpp"
#include "MapCommands/Paste.hpp"
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
    #if (_MSC_VER<1300)
        #define for if (false) ; else for
    #endif

    // Turn off warning 4355: "'this' : wird in Initialisierungslisten fuer Basisklasse verwendet".
    #pragma warning(disable:4355)
#endif


/// The class represents the "clipboard".
/// The clipboard is a singleton, global to the application,
/// in order to allow users to copy elements from one document and paste them into another.
class ClipboardT
{
    public:

    // TODO: Fully and properly implement Singleton pattern.
    ~ClipboardT();

    ArrayT<MapElementT*> Objects;
    Vector3fT            OriginalCenter;
};

ClipboardT::~ClipboardT()
{
    // Make sure that we don't leak memory at application exit.
    for (unsigned long ElemNr=0; ElemNr<Objects.Size(); ElemNr++)
        delete Objects[ElemNr];
}

static ClipboardT s_Clipboard;


/*static*/ const unsigned int MapDocumentT::CMAP_FILE_VERSION=13;


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
      m_Entities(),
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
    m_Entities.PushBack(new MapWorldT(*this));

    ArrayT<MapElementT*> AllElems;
    GetAllElems(AllElems);    // There are none at this time...
    m_BspTree=new OrthoBspTreeT(AllElems, m_GameConfig->GetMaxMapBB());
}


MapDocumentT::MapDocumentT(GameConfigT* GameConfig, wxProgressDialog* ProgressDialog, const wxString& FileName)
    : wxEvtHandler(),
      SubjectT(),
      m_ChildFrame(NULL),
      m_FileName(FileName),
      m_Entities(),
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

    MapWorldT* World=new MapWorldT(*this);
    m_Entities.PushBack(World);

    TextParserT TP(FileName.c_str(), "({})");

    if (TP.IsAtEOF())
    {
        delete m_Entities[0];
        throw LoadErrorT();
    }

    try
    {
        World->Load_cmap(TP, *this, ProgressDialog, 0);

        // Load the entities.
        while (!TP.IsAtEOF())
        {
            MapEntityT* Entity=new MapEntityT;

            Entity->Load_cmap(TP, *this, ProgressDialog, m_Entities.Size());
            m_Entities.PushBack(Entity);
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

        delete m_Entities[0];   //XXX TODO: Call Cleanup() method instead (same code as dtor).
        throw LoadErrorT();
    }


    ArrayT<MapElementT*> AllElems;
    GetAllElems(AllElems);

    if (AllElems.Size()>0)
    {
        // The world itself is never inserted into the BSP tree.
        wxASSERT(AllElems[0]==World);
        wxASSERT(AllElems[0]->GetType()==&MapWorldT::TypeInfo);
        AllElems.RemoveAt(0);
    }

    m_BspTree=new OrthoBspTreeT(AllElems, m_GameConfig->GetMaxMapBB());
}


/*static*/ MapDocumentT* MapDocumentT::ImportHalfLife1Map(GameConfigT* GameConfig, wxProgressDialog* ProgressDialog, const wxString& FileName)
{
    // This sets the cursor to the busy cursor in its ctor, and back to the default cursor in the dtor.
    wxBusyCursor BusyCursor;

    MapDocumentT* Doc=new MapDocumentT(GameConfig);
    TextParserT   TP(FileName.c_str(), "({})");

    if (TP.IsAtEOF())
    {
        delete Doc;
        Doc=NULL;
        throw LoadErrorT();
    }

    try
    {
        Doc->GetEntities()[0]->Load_HL1_map(TP, *Doc, ProgressDialog, 0);

        // Load the entities.
        while (!TP.IsAtEOF())
        {
            MapEntityT* Entity=new MapEntityT;

            Entity->Load_HL1_map(TP, *Doc, ProgressDialog, Doc->m_Entities.Size());
            Doc->m_Entities.PushBack(Entity);
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
        throw LoadErrorT();
    }


    ArrayT<MapElementT*> AllElems;
    Doc->GetAllElems(AllElems);

    if (AllElems.Size()>0)
    {
        // The world itself is never inserted into the BSP tree.
        wxASSERT(AllElems[0]==Doc->GetEntities()[0]);
        wxASSERT(AllElems[0]->GetType()==&MapWorldT::TypeInfo);
        AllElems.RemoveAt(0);
    }

    delete Doc->m_BspTree;
    Doc->m_BspTree=new OrthoBspTreeT(AllElems, Doc->m_GameConfig->GetMaxMapBB());

    Doc->m_FileName=FileName;
    return Doc;
}


/*static*/ MapDocumentT* MapDocumentT::ImportHalfLife2Vmf(GameConfigT* GameConfig, wxProgressDialog* ProgressDialog, const wxString& FileName)
{
    // This sets the cursor to the busy cursor in its ctor, and back to the default cursor in the dtor.
    wxBusyCursor BusyCursor;

    MapDocumentT* Doc=new MapDocumentT(GameConfig);
    TextParserT TP(FileName.c_str(), "{}");

    if (TP.IsAtEOF())
    {
        delete Doc;
        Doc=NULL;
        throw LoadErrorT();
    }

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
                MapEntityT* Entity=new MapEntityT;

                Entity->Load_HL2_vmf(TP, *Doc, ProgressDialog, Doc->m_Entities.Size());
                Doc->m_Entities.PushBack(Entity);
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
        throw LoadErrorT();
    }


    ArrayT<MapElementT*> AllElems;
    Doc->GetAllElems(AllElems);

    if (AllElems.Size()>0)
    {
        // The world itself is never inserted into the BSP tree.
        wxASSERT(AllElems[0]==Doc->GetEntities()[0]);
        wxASSERT(AllElems[0]->GetType()==&MapWorldT::TypeInfo);
        AllElems.RemoveAt(0);
    }

    delete Doc->m_BspTree;
    Doc->m_BspTree=new OrthoBspTreeT(AllElems, Doc->m_GameConfig->GetMaxMapBB());

    Doc->m_FileName=FileName;
    return Doc;
}


/*static*/ MapDocumentT* MapDocumentT::ImportDoom3Map(GameConfigT* GameConfig, wxProgressDialog* ProgressDialog, const wxString& FileName)
{
    // This sets the cursor to the busy cursor in its ctor, and back to the default cursor in the dtor.
    wxBusyCursor BusyCursor;

    MapDocumentT* Doc=new MapDocumentT(GameConfig);
    TextParserT   TP(FileName.c_str(), "({})");

    if (TP.IsAtEOF())
    {
        delete Doc;
        Doc=NULL;
        throw LoadErrorT();
    }

    try
    {
        Doc->GetEntities()[0]->Load_D3_map(TP, *Doc, ProgressDialog, 0);

        // Load the entities.
        while (!TP.IsAtEOF())
        {
            MapEntityT* Entity=new MapEntityT;

            Entity->Load_D3_map(TP, *Doc, ProgressDialog, Doc->m_Entities.Size());
            Doc->m_Entities.PushBack(Entity);
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
        throw LoadErrorT();
    }


    ArrayT<MapElementT*> AllElems;
    Doc->GetAllElems(AllElems);

    if (AllElems.Size()>0)
    {
        // The world itself is never inserted into the BSP tree.
        wxASSERT(AllElems[0]==Doc->GetEntities()[0]);
        wxASSERT(AllElems[0]->GetType()==&MapWorldT::TypeInfo);
        AllElems.RemoveAt(0);
    }

    delete Doc->m_BspTree;
    Doc->m_BspTree=new OrthoBspTreeT(AllElems, Doc->m_GameConfig->GetMaxMapBB());

    Doc->m_FileName=FileName;
    return Doc;
}


MapDocumentT::~MapDocumentT()
{
    delete m_BspTree;
    m_BspTree=NULL;

    for (unsigned long EntNr=0; EntNr<m_Entities.Size(); EntNr++) delete m_Entities[EntNr];
    m_Entities.Clear();

    for (unsigned long GroupNr=0; GroupNr<m_Groups.Size(); GroupNr++) delete m_Groups[GroupNr];
    m_Groups.Clear();

    for (unsigned long uecNr=0; uecNr<m_UnknownEntClasses.Size(); uecNr++) delete m_UnknownEntClasses[uecNr];
    m_UnknownEntClasses.Clear();

    m_Selection.Clear();
}


bool MapDocumentT::OnSaveDocument(const wxString& FileName, bool IsAutoSave)
{
    // This sets the cursor to the busy cursor in its ctor, and back to the default cursor in the dtor.
    wxBusyCursor BusyCursor;

    // Backup the previous file before overwriting it.
    if (!IsAutoSave && wxFileExists(FileName))
        if (!wxCopyFile(FileName, FileName+"_bak"))
        {
            wxMessageBox("Sorry, creating the backup file \""+FileName+"_bak\" before saving the map to \""+FileName+"\" didn't work out.\n"
                         "Please check the path and file permissions, "
                         "or use 'File -> Save As...' to save the current map elsewhere.", "File not saved!", wxOK | wxICON_ERROR);
            return false;
        }

    if (FileName.Right(5).MakeLower()==".cmap")
    {
        std::ofstream OutFile(FileName.fn_str());

        if (!OutFile.is_open())
        {
            wxMessageBox("The file \""+FileName+"\" could not be opened for writing.\nPlease check the path and file permissions, "
                         "or use 'File -> Save As...' to save the current map elsewhere.", "File not saved!", wxOK | wxICON_ERROR);
            return false;
        }

        // From MSDN documentation: "digits10 returns the number of decimal digits that the type can represent without loss of precision."
        // For floats, that's usually 6, for doubles, that's usually 15. However, we want to use the number of *significant* decimal digits here,
        // that is, max_digits10. See http://www.open-std.org/JTC1/sc22/wg21/docs/papers/2006/n2005.pdf for details.
        OutFile.precision(std::numeric_limits<float>::digits10 + 3);

        OutFile << "// Cafu Map File\n"
                << "// Written by CaWE, the Cafu World Editor.\n"
                << "Version " << CMAP_FILE_VERSION << "\n"
                << "\n";

        // Save groups.
        for (unsigned long GroupNr=0; GroupNr<m_Groups.Size(); GroupNr++)
            m_Groups[GroupNr]->Save_cmap(OutFile, GroupNr);

        // Save entities.
        for (unsigned long EntNr=0/*with world*/; EntNr<m_Entities.Size(); EntNr++)
        {
            const BoundingBox3fT* Intersecting=NULL;
            const MapEntityBaseT* Ent=m_Entities[EntNr];

            if (!Intersecting || Ent->GetBB().Intersects(*Intersecting))
            {
                Ent->Save_cmap(*this, OutFile, EntNr, Intersecting);
            }
        }

        if (OutFile.fail())
        {
            wxMessageBox("There was an error when saving the file. Please try again.", "File not saved!", wxOK | wxICON_ERROR);
            return false;
        }

        // If this was an auto-save, do not change the filename (nor set the document as "not modified").
        if (IsAutoSave) return true;

        m_FileName=FileName;
        return true;
    }

    // if (FileName.Right(4).MakeLower()==".map") ...;      // Export to different file format.

    wxMessageBox("Sorry, extension of this filename not recognized.", FileName);
    return false;
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


void MapDocumentT::GetAllElems(ArrayT<MapElementT*>& Elems) const
{
    for (unsigned long EntNr=0; EntNr<m_Entities.Size(); EntNr++)
    {
        MapEntityBaseT*               Ent=m_Entities[EntNr];
        const ArrayT<MapPrimitiveT*>& Primitives=Ent->GetPrimitives();

        // Add the entity itself...
        Elems.PushBack(Ent);

        // ... and all of its primitives.
        for (unsigned long PrimNr=0; PrimNr<Primitives.Size(); PrimNr++)
            Elems.PushBack(Primitives[PrimNr]);
    }
}


bool MapDocumentT::IterateElems(IterationHandlerI& IH)
{
    for (unsigned long EntNr=0; EntNr<m_Entities.Size(); EntNr++)
    {
        MapEntityBaseT*               Ent=m_Entities[EntNr];
        const ArrayT<MapPrimitiveT*>& Primitives=Ent->GetPrimitives();

        // If not the world, have the entity itself handled...
        if (EntNr>0)
            if (!IH.Handle(Ent)) return false;

        // ... then all of its primitives.
        for (unsigned long PrimNr=0; PrimNr<Primitives.Size(); PrimNr++)
            if (!IH.Handle(Primitives[PrimNr])) return false;
    }

    return true;
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


void MapDocumentT::Insert(MapEntityT* Ent)
{
    wxASSERT(Ent!=NULL);
    if (Ent==NULL) return;

    // FIXME: Just drop this here, and add another check into the MapCheckDialogT.
    Ent->CheckUniqueValues(*this);

    // Should not have Ent already.
    wxASSERT(m_Entities.Find(Ent)==-1);
    m_Entities.PushBack(Ent);

    // Insert all primitives of Ent and Ent itself into the BSP tree.
    for (unsigned long PrimNr=0; PrimNr<Ent->GetPrimitives().Size(); PrimNr++)
        m_BspTree->Insert(Ent->GetPrimitives()[PrimNr]);

    m_BspTree->Insert(Ent);
}


void MapDocumentT::Insert(MapPrimitiveT* Prim, MapEntityBaseT* ParentEnt)
{
    wxASSERT(Prim!=NULL);
    if (Prim==NULL) return;

    if (ParentEnt==NULL) ParentEnt=m_Entities[0];
    wxASSERT(m_Entities.Find(ParentEnt)>=0);

    ParentEnt->AddPrim(Prim);
    m_BspTree->Insert(Prim);
}


void MapDocumentT::Remove(MapElementT* Elem)
{
    wxASSERT(Elem!=NULL);
    if (Elem==NULL) return;

    MapPrimitiveT* Prim=dynamic_cast<MapPrimitiveT*>(Elem);
    if (Prim)
    {
        MapEntityBaseT* ParentEnt=Prim->GetParent();

        // The first assert is actually redundant in the second, but keep it for clarity.
        wxASSERT(ParentEnt!=NULL);
        wxASSERT(m_Entities.Find(ParentEnt)>=0);

        if (ParentEnt!=NULL)
        {
            ParentEnt->RemovePrim(Prim);
            wxASSERT(Prim->GetParent()==NULL);
        }

        m_BspTree->Remove(Prim);
        return;
    }

    MapEntityT* Ent=dynamic_cast<MapEntityT*>(Elem);
    if (Ent)
    {
        const int Index=m_Entities.Find(Ent);

        // -1 means not found, 0 means the world. Both should not happen.
        wxASSERT(Index>0);
        if (Index>0)
        {
            // Keeping the order helps when map files are diff'ed or manually compared.
            m_Entities.RemoveAtAndKeepOrder(Index);
        }

        // Remove all primitives of Ent and Ent itself from the BSP tree.
        for (unsigned long PrimNr=0; PrimNr<Ent->GetPrimitives().Size(); PrimNr++)
            m_BspTree->Remove(Ent->GetPrimitives()[PrimNr]);

        m_BspTree->Remove(Ent);
        return;
    }

    // We should never get here, because then Elem is neither a MapPrimitiveT nor a MapEntityT.
    wxASSERT(false);
}


// Hmmm. This would make a wonderful member function...   (TODO!)
static bool IsElemInBox(const MapElementT* Elem, const BoundingBox3fT& Box, bool InsideOnly, bool CenterOnly)
{
    const BoundingBox3fT ElemBB=Elem->GetBB();

    if (CenterOnly) return Box.Contains(ElemBB.GetCenter());
    if (InsideOnly) return Box.Contains(ElemBB.Min) && Box.Contains(ElemBB.Max);

    return ElemBB.Intersects(Box);
}


ArrayT<MapElementT*> MapDocumentT::GetElementsIn(const BoundingBox3fT& Box, bool InsideOnly, bool CenterOnly) const
{
    ArrayT<MapElementT*> Result;

    for (unsigned long EntNr=0; EntNr<m_Entities.Size(); EntNr++)
    {
        MapEntityBaseT*               Ent=m_Entities[EntNr];
        const ArrayT<MapPrimitiveT*>& Primitives=Ent->GetPrimitives();

        // If not the world, have the entity itself handled...
        if (EntNr>0)
            if (IsElemInBox(Ent, Box, InsideOnly, CenterOnly)) Result.PushBack(Ent);

        // ... then all of its primitives.
        for (unsigned long PrimNr=0; PrimNr<Primitives.Size(); PrimNr++)
            if (IsElemInBox(Primitives[PrimNr], Box, InsideOnly, CenterOnly)) Result.PushBack(Primitives[PrimNr]);
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
    std::map<EditorMaterialI*, int> UsedMatMap;

    for (unsigned long EntNr=0; EntNr<m_Entities.Size(); EntNr++)
    {
        const ArrayT<MapPrimitiveT*>& Primitives=m_Entities[EntNr]->GetPrimitives();

        for (unsigned long PrimNr=0; PrimNr<Primitives.Size(); PrimNr++)
        {
            MapElementT* Prim=Primitives[PrimNr];

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

    // First delete the previous contents of the clipboard.
    for (unsigned long SelNr=0; SelNr<s_Clipboard.Objects.Size(); SelNr++)
        delete s_Clipboard.Objects[SelNr];

    s_Clipboard.Objects.Clear();
    s_Clipboard.OriginalCenter=GetMostRecentSelBB().GetCenter();

    // Assign a copy of the current selection as the new clipboard contents.
    for (unsigned long SelNr=0; SelNr<m_Selection.Size(); SelNr++)
        s_Clipboard.Objects.PushBack(m_Selection[SelNr]->Clone());
}


void MapDocumentT::OnEditPaste(wxCommandEvent& CE)
{
    wxBusyCursor BusyCursor;

    GetHistory().SubmitCommand(new CommandPasteT(*this, s_Clipboard.Objects, s_Clipboard.OriginalCenter, m_ChildFrame->GuessUserVisiblePoint()));

    m_ChildFrame->GetToolManager().SetActiveTool(GetToolTIM().FindTypeInfoByName("ToolSelectionT"));
}


void MapDocumentT::OnEditPasteSpecial(wxCommandEvent& CE)
{
    BoundingBox3fT ClipboardBB;

    for (unsigned long ElemNr=0; ElemNr<s_Clipboard.Objects.Size(); ElemNr++)
        ClipboardBB.InsertValid(s_Clipboard.Objects[ElemNr]->GetBB());

    if (s_Clipboard.Objects.Size()==0) return;
    if (!ClipboardBB.IsInited()) return;

    PasteSpecialDialogT PasteSpecialDialog(ClipboardBB);

    if (PasteSpecialDialog.ShowModal()==wxID_CANCEL) return;

    wxBusyCursor BusyCursor;

    const Vector3fT Translation=Vector3fT(PasteSpecialDialog.TranslateX, PasteSpecialDialog.TranslateY, PasteSpecialDialog.TranslateZ);
    const Vector3fT Rotation   =Vector3fT(PasteSpecialDialog.RotateX,    PasteSpecialDialog.RotateY,    PasteSpecialDialog.RotateZ);

    CommandT* Command=new CommandPasteT(*this, s_Clipboard.Objects, s_Clipboard.OriginalCenter, m_ChildFrame->GuessUserVisiblePoint(),
                                        Translation, Rotation, PasteSpecialDialog.NrOfCopies, PasteSpecialDialog.GroupCopies,
                                        PasteSpecialDialog.CenterAtOriginal);

    GetHistory().SubmitCommand(Command);

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

    // Remove the world entity at index 0.
    if (NewSelection.Size()>0)
    {
        wxASSERT(NewSelection[0]->GetType()==&MapWorldT::TypeInfo);
        NewSelection.RemoveAt(0);
    }

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

    if (GotoPrimDialog.m_EntityNumber>=int(m_Entities.Size()))
    {
        wxMessageBox("The entity with the given index number does not exist.", "Goto Primitive");
        return;
    }

    if (GotoPrimDialog.m_PrimitiveNumber>=int(m_Entities[GotoPrimDialog.m_EntityNumber]->GetPrimitives().Size()))
    {
        wxMessageBox("The primitive with the given index number does not exist (in the specified entity).", "Goto Primitive");
        return;
    }

    MapPrimitiveT* Prim=m_Entities[GotoPrimDialog.m_EntityNumber]->GetPrimitives()[GotoPrimDialog.m_PrimitiveNumber];

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
    m_History.SubmitCommand(CommandSelectT::Set(this, m_Entities[0]));

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


/// An iteration handler that collects all map elements that are unselected (and not in a group).
class CollectUnselectedT : public IterationHandlerI
{
    public:

    CollectUnselectedT()
        : m_Unselected()
    {
    }

    bool Handle(MapElementT* Child)
    {
        if (Child->IsSelected() || Child->GetGroup()) return true;

        m_Unselected.PushBack(Child);
        return true;
    }

    const ArrayT<MapElementT*>& GetUnselected() const
    {
        return m_Unselected;
    }


    private:

    ArrayT<MapElementT*> m_Unselected;
};


void MapDocumentT::OnViewHideUnselectedObjects(wxCommandEvent& CE)
{
    // Find all unselected map elements that are not in a group already.
    CollectUnselectedT CollectUnselectedCallBack;

    IterateElems(CollectUnselectedCallBack);

    const ArrayT<MapElementT*>& HideElems=CollectUnselectedCallBack.GetUnselected();

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

    ArrayT<MapEntityT*>    SelEntities;     // All entities   that are in the selection.
    ArrayT<MapPrimitiveT*> SelPrimitives;   // All primitives that are in the selection.

    for (unsigned long SelNr=0; SelNr<m_Selection.Size(); SelNr++)
    {
        MapElementT* Elem=m_Selection[SelNr];

        if (Elem->GetType()==&MapEntityT::TypeInfo)
        {
            SelEntities.PushBack(static_cast<MapEntityT*>(Elem));
        }
        else if (Elem->GetType()==&MapPrimitiveT::TypeInfo)
        {
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
        CommandNewEntityT* CmdNewEnt=new CommandNewEntityT(*this, NewEntityClass, SnapToGrid(GetMostRecentSelBB().GetCenter(), false /*Toggle*/, -1 /*AxisNoSnap*/));

        CmdNewEnt->Do();
        SubCommands.PushBack(CmdNewEnt);

        // 2. Assign the primitives to the new entity.
        CommandAssignPrimToEntT* CmdAssignToEnt=new CommandAssignPrimToEntT(*this, SelPrimitives, CmdNewEnt->GetEntity());

        CmdNewEnt->Do();
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

    GetHistory().SubmitCommand(new CommandAssignPrimToEntT(*this, SelPrimitives, m_Entities[0]));

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
            new CommandTransformT(*this, m_Selection, Mode, RefPoint, Value, false /*don't clone*/));
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
    UE.Enable(s_Clipboard.Objects.Size()>0 && m_ChildFrame->GetToolManager().GetActiveToolType()!=&ToolEditSurfaceT::TypeInfo);
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
    ArrayT<GroupT*> EmptyGroups;

    for (unsigned long GroupNr=0; GroupNr<m_Groups.Size(); GroupNr++)
    {
        bool IsEmpty=true;

        for (unsigned long EntNr=0; EntNr<m_Entities.Size(); EntNr++)
        {
            // Check the entity first...
            const MapEntityBaseT* Ent=m_Entities[EntNr];

            if (Ent->GetGroup()==m_Groups[GroupNr])
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
