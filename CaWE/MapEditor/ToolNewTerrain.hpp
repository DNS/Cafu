/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_TOOL_NEW_TERRAIN_HPP_INCLUDED
#define CAFU_TOOL_NEW_TERRAIN_HPP_INCLUDED

#include "Tool.hpp"
#include "Math3D/Vector3.hpp"


class MapBrushT;
class OptionsBar_NewTerrainToolT;
class ViewWindowT;


class ToolNewTerrainT : public ToolT
{
    public:

    ToolNewTerrainT(MapDocumentT& MapDoc, ToolManagerT& ToolMan, wxWindow* ParentOptionsBar);

    ~ToolNewTerrainT();

    // Implementations/overrides of ToolT methods.
    int       GetWxEventID() const { return ChildFrameT::ID_MENU_TOOLS_TOOL_NEWTERRAIN; }
    wxWindow* GetOptionsBar();

    bool OnKeyDown2D   (ViewWindow2DT& ViewWindow, wxKeyEvent&   KE);
    bool OnLMouseDown2D(ViewWindow2DT& ViewWindow, wxMouseEvent& ME);
    bool OnLMouseUp2D  (ViewWindow2DT& ViewWindow, wxMouseEvent& ME);
    bool OnMouseMove2D (ViewWindow2DT& ViewWindow, wxMouseEvent& ME);

    bool OnKeyDown3D   (ViewWindow3DT& ViewWindow, wxKeyEvent&   KE);
    bool OnLMouseDown3D(ViewWindow3DT& ViewWindow, wxMouseEvent& ME);
    bool OnMouseMove3D (ViewWindow3DT& ViewWindow, wxMouseEvent& ME);

    void RenderTool2D(Renderer2DT& Renderer) const;
    void RenderTool3D(Renderer3DT& Renderer) const;

    // The TypeSys related declarations for this class.
    virtual const cf::TypeSys::TypeInfoT* GetType() const { return &TypeInfo; }
    static void* CreateInstance(const cf::TypeSys::CreateParamsT& Params);
    static const cf::TypeSys::TypeInfoT TypeInfo;


    private:

    bool OnKeyDown(ViewWindowT& ViewWindow, wxKeyEvent& KE);
    void UpdateNewBrush(ViewWindow2DT& ViewWindow);     ///< Updates the m_NewBrush member according to the given drag rectangle.

    MapBrushT* m_NewBrush;      ///< The new brush that is currently being dragged, or NULL when no dragging is in progress.
    Vector3fT  m_DragBegin;     ///< The point in world space where the dragging began (first corner of the dragging rectangle).
    Vector3fT  m_DragCurrent;   ///< The current point in world space of the drag (second corner of the dragging rectangle).

    OptionsBar_NewTerrainToolT* m_OptionsBar;   ///< The options bar for this tool.
};

#endif
