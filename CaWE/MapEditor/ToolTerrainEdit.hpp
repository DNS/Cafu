/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_TOOL_TERRAIN_EDIT_HPP_INCLUDED
#define CAFU_TOOL_TERRAIN_EDIT_HPP_INCLUDED

#include "Tool.hpp"
#include "MapTerrain.hpp"
#include "ObserverPattern.hpp"

#include "Templates/Array.hpp"

#include "wx/bitmap.h"
#include "wx/menu.h"


class MapTerrainT;
class MapDocumentT;
class MapElementT;
class OptionsBar_EditFacePropsToolT;
class TerrainEditorDialogT;


class ToolTerrainEditorT : public ToolT, public ObserverT
{
    public:

    enum ToolModeE
    {
        TOOLMODE_INACTIVE,
        TOOLMODE_ACTIVE,
        TOOLMODE_EYEDROPPER
    };

    enum ExportFileTypeE
    {
        BMP,
        PNG,
        JPG,
        PGM_ASCII,
        PGM_BINARY,
        TER
    };

    ToolTerrainEditorT(MapDocumentT& MapDoc, ToolManagerT& ToolMan, wxWindow* ParentOptionsBar);
    ~ToolTerrainEditorT();

    /// Sets the dialog that is asociated with this tool.
    /// Without a dialog certain tool parameters can't be read (like radius etc.) and the tool
    /// is not functional. This method should be called as soon as possible after the tool has
    /// been created.
    /// @param TerrainEditorDialog The dialog of this tool.
    void SetToolDialog(TerrainEditorDialogT* TerrainEditorDialog);

    /// Checks if the tool has a terrain selected.
    /// @return Whether a terrain is selected by the tool.
    bool IsTerrainSelected() { return (m_TerrainCopy!=NULL); }

    /// Updates the resolution of the terrain currently attached to the tool.
    /// If no terrain is attached to the tool nothing is done.
    void SetResolution(unsigned long Resolution);

    /// Gets the resolution of the terrain currently attached to the tool.
    /// @return Resolution of the currently attached terrain.
    unsigned long GetResolution() { return m_TerrainCopy->GetResolution(); }

    /// Generates the terrains height data using Perlin noise with the given parameters.
    void GenerateTerrain(int Octaves, double Frequency, double Persistence, double Lacunarity, int Seed);

    /// Imports the height data in the passed file into the terrain currently attached to the tool.
    /// If no terrain is attached to the tool nothing is done.
    void ImportHeightMap(const wxString& FileName);

    /// Exports the height data to a file in the specified file format.
    /// @param FileName The full path to the filename in which the height data should be stored.
    /// @param ExportFileType The file format used to store the height data.
    void ExportHeightMap(wxString FileName, ExportFileTypeE ExportFileType);

    /// Updates the modification matrix using the passed radius and hardness.
    void UpdateModifyWeights();

    /// Updates the noise weights according to the passed radius.
    void UpdateNoiseWeights();

    /// Updates the gauss kernel according to the passed tool effect.
    void UpdateGaussWeights();

    // Implementations/overrides of ToolT methods.
    int       GetWxEventID() const { return ChildFrameT::ID_MENU_TOOLS_TOOL_TERRAINEDITOR; }
    wxWindow* GetOptionsBar();
    void      OnActivate(ToolT* OldTool);
    void      OnDeactivate(ToolT* NewTool);

    bool OnKeyDown2D    (ViewWindow2DT& ViewWindow, wxKeyEvent&         KE);
    bool OnKeyUp2D      (ViewWindow2DT& ViewWindow, wxKeyEvent&         KE);
    bool OnLMouseDown2D (ViewWindow2DT& ViewWindow, wxMouseEvent&       ME);
    bool OnLMouseUp2D   (ViewWindow2DT& ViewWindow, wxMouseEvent&       ME);
    bool OnMMouseUp2D   (ViewWindow2DT& ViewWindow, wxMouseEvent&       ME);
    bool OnMouseMove2D  (ViewWindow2DT& ViewWindow, wxMouseEvent&       ME);
    int  OnContextMenu2D(ViewWindow2DT& ViewWindow, wxContextMenuEvent& CE, wxMenu& Menu);

    bool OnKeyDown3D    (ViewWindow3DT& ViewWindow, wxKeyEvent&         KE);
    bool OnKeyUp3D      (ViewWindow3DT& ViewWindow, wxKeyEvent&         KE);
    bool OnLMouseDown3D (ViewWindow3DT& ViewWindow, wxMouseEvent&       ME);
    bool OnLMouseUp3D   (ViewWindow3DT& ViewWindow, wxMouseEvent&       ME);
    bool OnMMouseUp3D   (ViewWindow3DT& ViewWindow, wxMouseEvent&       ME);
    bool OnMouseMove3D  (ViewWindow3DT& ViewWindow, wxMouseEvent&       ME);

    bool IsHiddenByTool(const MapElementT* Elem) const;
    void RenderTool2D(Renderer2DT& Renderer) const;
    void RenderTool3D(Renderer3DT& Renderer) const;

    // ObserverT implementation.
    void NotifySubjectChanged_Deleted(SubjectT* Subject, const ArrayT<MapPrimitiveT*>& Primitives);
    void NotifySubjectChanged_Modified(SubjectT* Subject, const ArrayT<MapElementT*>& MapElements, MapElemModDetailE Detail);
    void NotifySubjectChanged_Modified(SubjectT* Subject, const ArrayT<MapElementT*>& MapElements, MapElemModDetailE Detail, const ArrayT<BoundingBox3fT>& OldBounds);
    void NotifySubjectDies(SubjectT* dyingSubject);

    // The TypeSys related declarations for this class.
    virtual const cf::TypeSys::TypeInfoT* GetType() const { return &TypeInfo; }
    static void* CreateInstance(const cf::TypeSys::CreateParamsT& Params);
    static const cf::TypeSys::TypeInfoT TypeInfo;


    private:

    TerrainEditorDialogT* m_TerrainEditorDialog; ///< The terrain editor dialog that is associated with this tool.
    MapTerrainT*          m_TerrainOrig;         ///< The terrain in the map that is currently being edited by this tool. Note that this terrain is never modified directly, but only via the Command-Pattern by commands that apply changes that were previously made to the local m_TerrainCopy instance: The m_TerrainOrig is hidden in the map and not rendered while the dialog is active.
    MapTerrainT*          m_TerrainCopy;         ///< The "working copy" of the terrain. Always identical to the m_TerrainOrig except during the periods while one of the mouse buttons is being held down (i.e. the tool is active). That is, while a mouse button is being pressed, all changes are only made to this instance (which therefore temporarily diverges from the m_TerrainOrig). When the mouse button is released (i.e. tool becomes inactive) again, a command is submitted that updates the m_TerrainOrig member to the same contents, which in turn makes the m_TerrainOrig and m_TerrainCopy identical again.
    bool                  m_IsRecSelfNotify;     ///< Whether a modification of the terrain has been caused by ourselves.

    mutable bool          m_RenderUpdateBitmap;  ///< Whether the terrain has changed and its render bitmap needs to be updated.
    mutable bool          m_RenderUpdateTool;    ///< Whether the tool position/radius has changed and its render position/radius needs to be updated.
    mutable wxBitmap      m_RenderBitmap;        ///< The bitmap to render in the top down 2D view. This bitmap is updated when the height data changes or the zoom factor/scrollposition of the 2D view changes.

    // Render attributes. These are values that are calculated when the terrain was is rendered in the top down 2D view.
    // To prevent recalculation of the values each time 2D view is rendered (even if the terrains position in the view hasn't changed) they are stored here.
    mutable wxPoint       m_LastRenderPosBL;     ///< Bottom left position of the terrain in client space on last render. This value is needed to determine if the terrain position inside the 2D view has changed and the terrain bitmap needs to be recaclutated.
    mutable wxPoint       m_LastRenderPosTR;     ///< Top right position of the terrain in client space on last render. This value is needed to determine if the terrain position inside the 2D view has changed and the terrain bitmap needs to be recaclutated.
    mutable wxPoint       m_PointTLToolOff;      ///< Used when rendering the terrain in the 2D view.
    mutable int           m_SizeXTool;           ///< Used for tool calculations.
    mutable int           m_SizeYTool;           ///< Used for tool calculations.
    mutable wxPoint       m_ToolRenderPosition;  ///< Used to render the tool in the 2D view.
    mutable int           m_ToolRadiusX;         ///< Used to render the tool in the 2D view.
    mutable int           m_ToolRadiusY;         ///< Used to render the tool in the 2D view.

    wxPoint               m_HeightDataPos;       ///< The current position of the tool cursor in the terrain, in heightmap coordinates. Independent from the current m_ToolMode value.
    ToolModeE             m_ToolMode;            ///< Indicates the current state of the tool, i.e. whether it is inactive or active with one of the mouse buttons.

    // These members are reset each time the tool is newly activated. They are only valid and kept up-to-date while the tool is active.
    // When the tool is or becomes inactive, their contents has no meaning.
    wxRect                m_EditBounds;          ///< Bounds of the terrain modification operations since the activation of the tool.
    wxPoint               m_EditHeigthMapPos;    ///< Last tool cursor position (in heightmap coordinates) at which the last terrain modification occured.
    Plane3fT              m_EditPlane;           ///< When the tool is active in a 3D view, the mouse cursor position is mapped to heightmap coordinates using this plane. (When the tool is *inactive* in a 3D view, a true ray intersection test is performed in order to determine the heightmap coordinates from the current mouse cursor position.)

    ArrayT<wxRect>        m_EditRoadParts;       ///< Array filled when the road tool is active. The road is constructed with these parts, each defining an individual area of the road.

    unsigned short        m_ReferenceHeight;     ///< A reference height value used for by various tools (e.g. the flatten tool), set with a RMB click.
    ArrayT<float>         m_ModifyWeights;       ///< A matrix containing all relevant weights calculated by the modify function for the current tool options. The modify weights are computed each time the radius or hardness changes.
    ArrayT<float>         m_NoiseWeights;        ///< A matrix containing all weights needed to add bumps/holes when using the noise tool.
    int                   m_NoiseWeightsRes;     ///< Resoution of the noise weight array.
    ArrayT<float>         m_GaussWeights;        ///< A matrix containing the wheight used for gaussian operations (e.g. gaussian blur).
    int                   m_GaussWeightsRes;     ///< Resoution of the gauss kernel.

    enum ColorGradientE
    {
        GREY=0,
        RAINBOW,
        DEBUG_COLOR
    };

    ColorGradientE        m_CurrentColorGradient;

    OptionsBar_EditFacePropsToolT* m_OptionsBar;   ///< The options bar for this tool.


    /// Sets the terrain editing mode and calls DoEdit() for initial terrain edit.
    /// Further terrain modifications with this edit mode are done using the DoEdit() method directly.
    /// @param EditMode The edit mode that is used to edit the terrain.
    void SetEditMode(ToolModeE ToolMode);

    /// Modifies the terrain according to current tool state and edit mode.
    /// The terrain is only modified if the tools position has changed since the last call to DoEdit().
    /// @param Force Forces terrain modification ignoring tool position.
    void DoEdit(bool Force=false);

    /// Sets the terrain that is currently affected by the editor.
    /// @param NewTerrain Pointer to the terrain object that is to be edited.
    void SetTerrain(MapTerrainT* NewTerrain, bool UpdateObs_VisChanged=true);

    /// Creates a map command from all changes made up to this point and modifies the original terrain.
    void CommitChanges();

    /// Sets the tools new position inside the height data, after validating it (must be inside height data boundaries).
    /// If the new position is not valid the position is set to (-1, -1, -1).
    void SetHeightDataPos(const wxPoint& HeightDataPos);

    /// Returns the terrain height at the current tool position, in world coordinates.
    float GetToolHeight_World() const;

    /// Returns a scaled bitmap of the terrains height data according to parameters.
    /// This is needed to render a scaled portion of the height data in a 2D view.
    /// @param x The x pos from which height data is read.
    /// @param y The y pos from which height data is read.
    /// @param width The amount of height data to read in x direction.
    /// @param height The amount of height data to read in y direction.
    /// @param NewSizeX The size to scale the original heigth data to in x direction.
    /// @param NewSizeY The size to scale the original heigth data to in y direction.
    /// @return The scaled height data bitmap.
    wxBitmap GetScaledBitmap(float x, float y, float width, float height, unsigned int NewSizeX, unsigned int NewSizeY) const;

    /// Helper method to pick the height value at the tools current position and store it as reference heigth value.
    void PickReHeightValue();

    /// Gets the current radius from the attached dialog and translates it in height data units.
    /// This is necessary since terrains have different size and resolution and makes sure, that a radius with a given
    /// value has the same size on all terrains.
    int GetRadius() const;

    /// Updates the tool information shown in the status bar.
    void UpdateToolInformation();
};

#endif
