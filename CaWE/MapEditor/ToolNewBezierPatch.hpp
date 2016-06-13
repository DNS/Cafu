/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_TOOL_NEW_BEZIER_PATCH_HPP_INCLUDED
#define CAFU_TOOL_NEW_BEZIER_PATCH_HPP_INCLUDED

#include "Tool.hpp"
#include "Math3D/Vector3.hpp"
#include "Templates/Array.hpp"


class MapBezierPatchT;
class OptionsBar_NewBezierPatchToolT;
class ViewWindowT;


class ToolNewBezierPatchT : public ToolT
{
    public:

    ToolNewBezierPatchT(MapDocumentT& MapDoc, ToolManagerT& ToolMan, wxWindow* ParentOptionsBar);

    ~ToolNewBezierPatchT();

    // Implementations/overrides of ToolT methods.
    int       GetWxEventID() const { return ChildFrameT::ID_MENU_TOOLS_TOOL_NEWBEZIERPATCH; }
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
    void UpdateNewBPs(ViewWindow2DT& ViewWindow);   ///< Updates the m_NewBPs member according to the given drag rectangle.
    void DeleteNewBPs();                            ///< Deletes the bezier patches kept in m_NewBPs and clears the array.

    ArrayT<MapBezierPatchT*> m_NewBPs;      ///< The new bezier patch(es) that is/are currently being dragged. The array is empty when no dragging is in progress.
 // int                      m_NewBPType;   ///< Describes the type of bezier patch(es) that we are currently creating: simple, cylinder, cone, spheroid, ...
    Vector3fT                m_DragBegin;   ///< The point in world space where the dragging began (first corner of the dragging rectangle).
    Vector3fT                m_DragCurrent; ///< The current point in world space of the drag (second corner of the dragging rectangle).

    OptionsBar_NewBezierPatchToolT* m_OptionsBar;   ///< The options bar for this tool.
};

#endif
