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

#ifndef _MAP_DOCUMENT_HPP_
#define _MAP_DOCUMENT_HPP_

#include "ObserverPattern.hpp"
#include "CommandHistory.hpp"
#include "Plants/PlantDescrMan.hpp"
#include "Templates/Array.hpp"
#include "SceneGraph/LightMapMan.hpp"


class ChildFrameT;
class EditorMaterialI;
class EntityClassT;
class GameConfigT;
class GroupT;
class MapEntityBaseT;
class MapEntityT;
class MapPrimitiveT;
class OrthoBspTreeT;
class wxProgressDialog;


/// This class handles each iteration element when the children of a MapElementT object are iterated.
/// It quasi serves as a call-back function that is called on the element in each iteration.
/// User code is supposed to derive from this class in order to implement custom behaviour.
class IterationHandlerI
{
    public:

    /// The actual method that is called back on each element of the iteration.
    virtual bool Handle(MapElementT* Child)=0;

    /// The virtual destructor.
    virtual ~IterationHandlerI() { }
};


/// This class represents a CaWE "map" document.
class MapDocumentT : public wxEvtHandler, public SubjectT
{
    public:

    static const unsigned int CMAP_FILE_VERSION;

    // A class for throwing exceptions on load errors.
    class LoadErrorT { };


    /// The constructor for creating a new, clean, empty map.
    /// @param GameConfig   The game configuration that will be used for the map.
    MapDocumentT(GameConfigT* GameConfig);

    /// The regular constructor for loading a cmap file from disk.
    /// @param GameConfig   The game configuration that will be used for the map.
    MapDocumentT(GameConfigT* GameConfig, wxProgressDialog* ProgressDialog, const wxString& FileName);

    /// A named constructor for importing a map in HL1 map file format.
    /// @param GameConfig   The game configuration that will be used for the map.
    static MapDocumentT* ImportHalfLife1Map(GameConfigT* GameConfig, wxProgressDialog* ProgressDialog, const wxString& FileName);

    /// A named constructor for importing a map in HL2 vmf file format.
    /// @param GameConfig   The game configuration that will be used for the map.
    static MapDocumentT* ImportHalfLife2Vmf(GameConfigT* GameConfig, wxProgressDialog* ProgressDialog, const wxString& FileName);

    /// A named constructor for importing a map in Doom3 map file format.
    /// @param GameConfig   The game configuration that will be used for the map.
    static MapDocumentT* ImportDoom3Map(GameConfigT* GameConfig, wxProgressDialog* ProgressDialog, const wxString& FileName);

    /// The destructor.
    ~MapDocumentT();


    // Inherited methods from the wxDocument base class.
    bool OnSaveDocument(const wxString& FileName, bool IsAutoSave); ///< Saves the document. Non-const, as it updates the m_FileName member.
    bool SaveAs();                                                  ///< Calls OnSaveDocument().
    bool Save();                                                    ///< Calls OnSaveDocument().

    void                           SetChildFrame(ChildFrameT* ChildFrame) { m_ChildFrame=ChildFrame; }   // This should be in the ctor!
    ChildFrameT*                   GetChildFrame() const { return m_ChildFrame; }
    const wxString&                GetFileName() const   { return m_FileName; }

    /// Returns all entities in the map. The world is always at index 0, followed by the "regular" entities.
    const ArrayT<MapEntityBaseT*>& GetEntities() const { return m_Entities; }

    /// Adds all elements (entities and primitives) in this map to the given array.
    /// Note that the world entity is always the first element that is added to the list.
    void GetAllElems(ArrayT<MapElementT*>& Elems) const;

    /// Iterates over all elements (entities and primitives) in this map, calling the IH.Handle() call-back method for each.
    /// Note that for backwards-compatibility, the world entity itself is *skipped*: IH.Handle() is called for all its
    /// primitives, but not the world entity itself.
    /// @param IH   The iteration handler whose Handle() method is called once per map element.
    ///             User code is expected to pass a derived class instance in order to achieve custom behaviour.
    /// @returns true if the iteration completed fully, or false if it was aborted early.
    bool IterateElems(IterationHandlerI& IH);

    /// Inserts the given entity into the map.
    /// Callers should never attempt to insert an element into the world in a way other than calling this method,
    /// as it also inserts the element into the internal BSP tree that is used for rendering and culling.
    void Insert(MapEntityT* Ent);

    /// Inserts the given primitive into the map, as a child of the given entity (the world or a custom entity).
    /// Callers should never attempt to insert an element into the world in a way other than calling this method,
    /// as it also inserts the element into the internal BSP tree that is used for rendering and culling.
    void Insert(MapPrimitiveT* Prim, MapEntityBaseT* ParentEnt=NULL);

    /// Removes the given element from the map.
    /// The element can be any primitive or custom entity (but never the MapWorldT instance).
    void Remove(MapElementT* Elem);

    ArrayT<MapElementT*> GetElementsIn(const BoundingBox3fT& Box, bool InsideOnly, bool CenterOnly) const;

    /// Determines all materials that are currently being used in the world (in brushes, Bezier patches and terrains),
    /// and returns the whole list via the UsedMaterials reference parameter.
    void GetUsedMaterials(ArrayT<EditorMaterialI*>& UsedMaterials) const;

    OrthoBspTreeT*                 GetBspTree() const    { return m_BspTree; }
    GameConfigT*                   GetGameConfig() const { return m_GameConfig; }
    const EntityClassT*            FindOrCreateUnknownClass(const wxString& Name, bool HasOrigin);
    cf::SceneGraph::LightMapManT&  GetLightMapMan()      { return m_LightMapMan; }
    PlantDescrManT&                GetPlantDescrMan()    { return m_PlantDescrMan; }

    CommandHistoryT&         GetHistory()               { return m_History; }
    const ArrayT<Vector3fT>& GetPointFilePoints() const { return m_PointFilePoints; }

    bool      IsSnapEnabled() const   { return m_SnapToGrid; }      ///< Returns whether or not grid snap is enabled. Called by the tools and views to determine snap behavior.
    int       GetGridSpacing() const  { return m_GridSpacing>0 ? m_GridSpacing : 1; }
    bool      Is2DGridEnabled() const { return m_ShowGrid; }
    float     SnapToGrid(float f, bool Toggle) const;                               ///< Returns the given number f   snapped to the grid if the grid is active, rounded to the nearest integer otherwise. If Toggle is true, the grid activity is considered toggled.
    Vector3fT SnapToGrid(const Vector3fT& Pos, bool Toggle, int AxisNoSnap) const;  ///< Returns the given vector Pos snapped to the grid if the grid is active, rounded to the nearest integer otherwise. If Toggle is true, the grid activity is considered toggled. If AxisNoSnap is -1, all components of Pos are snapped to the grid. If AxisNoSnap is 0, 1 or 2, the corresponding component of Pos is returned unchanged.

    /// Methods for managing the set of currently selected map elements.
    //@{
    void                        SetSelection(const ArrayT<MapElementT*>& NewSelection);
    const ArrayT<MapElementT*>& GetSelection() const { return m_Selection; }
    const BoundingBox3fT&       GetMostRecentSelBB() const;     ///< Returns the most recent bounding-box of the selection. That is, it returns the bounding-box of the current selection, or (if nothing is selected) the bounding-box of the previous selection.
    //@}

    /// Methods for managing the groups.
    //@{
    const ArrayT<GroupT*>& GetGroups() const { return m_Groups; }
    ArrayT<GroupT*>& GetGroups() { return m_Groups; }
    ArrayT<GroupT*> GetAbandonedGroups() const; ///< Returns only the groups that are empty (have no members).
    //@}


    private:

    MapDocumentT(const MapDocumentT&);          ///< Use of the Copy    Constructor is not allowed.
    void operator = (const MapDocumentT&);      ///< Use of the Assignment Operator is not allowed.

    ChildFrameT*                 m_ChildFrame;          ///< The child frame within which this document lives.
    wxString                     m_FileName;            ///< This documents file name.
    ArrayT<MapEntityBaseT*>      m_Entities;            ///< All the entities that exist in this map. The world entity is always first at index 0, followed by an arbitrary number of regular entities.
    OrthoBspTreeT*               m_BspTree;             ///< The BSP tree that spatially organizes the map elements in the m_MapWorld.
    GameConfigT*                 m_GameConfig;          ///< The game configuration that is used with this map.
    ArrayT<const EntityClassT*>  m_UnknownEntClasses;   ///< The entity classes that are used by entities loaded into this map but who are unknown/undefined in this game config. This list complements GameConfigT::m_EntityClasses.
    cf::SceneGraph::LightMapManT m_LightMapMan;         ///< The light map manager that is used with this map.
    PlantDescrManT               m_PlantDescrMan;       ///< The plant description manager that is used with this map.

    CommandHistoryT              m_History;             ///< The command history.

    ArrayT<MapElementT*>         m_Selection;           ///< The currently selected map elements.
    mutable BoundingBox3fT       m_SelectionBB;         ///< The bounding-box of the current selection, or if there is no selection, the bounding-box of the previous selection.
    ArrayT<GroupT*>              m_Groups;              ///< The list of groups in this document.
    ArrayT<Vector3fT>            m_PointFilePoints;     ///< The points of the currently loaded point file.

    bool                         m_SnapToGrid;          ///< Snap things to grid.      Kept here because the other two are kept here as well. ;-)
    int                          m_GridSpacing;         ///< The spacing of the grid.  Could also be kept in the related ChildFrameT, but as the observers depent on it, its properly stored here in the MapDocumentT.
    bool                         m_ShowGrid;            ///< Show or hide the 2D grid. Could also be kept in the related ChildFrameT, but as the observers depent on it, its properly stored here in the MapDocumentT.


    /*************************************************************/
    /*** Event handlers for >>document specific<< menu events. ***/
    /*************************************************************/

    void OnEditUndoRedo                (wxCommandEvent& CE);
    void OnEditCut                     (wxCommandEvent& CE);
    void OnEditCopy                    (wxCommandEvent& CE);
    void OnEditPaste                   (wxCommandEvent& CE);
    void OnEditPasteSpecial            (wxCommandEvent& CE);
    void OnEditDelete                  (wxCommandEvent& CE);
    void OnEditSelectNone              (wxCommandEvent& CE);
    void OnEditSelectAll               (wxCommandEvent& CE);

    void OnUpdateEditUndoRedo          (wxUpdateUIEvent& UE);   // For Undo and Redo.
    void OnUpdateEditCutCopyDelete     (wxUpdateUIEvent& UE);   // For Cut, Copy and Delete.
    void OnUpdateEditPasteSpecial      (wxUpdateUIEvent& UE);   // For Paste and PasteSpecial.

    void OnSelectionApplyMaterial      (wxCommandEvent& CE);

    void OnUpdateSelectionApplyMaterial(wxUpdateUIEvent& UE);

    void OnMapSnapToGrid               (wxCommandEvent& CE);
    void OnMapToggleGrid2D             (wxCommandEvent& CE);
    void OnMapFinerGrid                (wxCommandEvent& CE);
    void OnMapCoarserGrid              (wxCommandEvent& CE);
    void OnMapGotoPrimitive            (wxCommandEvent& CE);
    void OnMapShowInfo                 (wxCommandEvent& CE);
    void OnMapCheckForProblems         (wxCommandEvent& CE);
    void OnMapProperties               (wxCommandEvent& CE);
    void OnMapLoadPointFile            (wxCommandEvent& CE);
    void OnMapUnloadPointFile          (wxCommandEvent& CE);

    void OnViewShowEntityInfo          (wxCommandEvent& CE);
    void OnViewShowEntityTargets       (wxCommandEvent& CE);
    void OnViewHideSelectedObjects     (wxCommandEvent& CE);
    void OnViewHideUnselectedObjects   (wxCommandEvent& CE);
    void OnViewShowHiddenObjects       (wxCommandEvent& CE);

    void OnUpdateViewShowEntityInfo    (wxUpdateUIEvent& UE);
    void OnUpdateViewShowEntityTargets (wxUpdateUIEvent& UE);

    void OnToolsCarve                  (wxCommandEvent& CE);
    void OnToolsHollow                 (wxCommandEvent& CE);
    void OnToolsAssignPrimToEntity     (wxCommandEvent& CE);
    void OnToolsAssignPrimToWorld      (wxCommandEvent& CE);
    void OnToolsReplaceMaterials       (wxCommandEvent& CE);
    void OnToolsMaterialLock           (wxCommandEvent& CE);
    void OnToolsSnapSelectionToGrid    (wxCommandEvent& CE);
    void OnToolsTransform              (wxCommandEvent& CE);
    void OnToolsAlign                  (wxCommandEvent& CE);
    void OnToolsMirror                 (wxCommandEvent& CE);

    void OnUpdateToolsMaterialLock     (wxUpdateUIEvent& UE);

    DECLARE_EVENT_TABLE()
};

#endif
