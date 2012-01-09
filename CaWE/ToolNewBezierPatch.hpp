/*
=================================================================================
This file is part of Cafu, the open-source game engine and graphics engine
for multiplayer, cross-platform, real-time 3D action.
Copyright (C) 2002-2012 Carsten Fuchs Software.

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

#ifndef _TOOL_NEW_BEZIER_PATCH_HPP_
#define _TOOL_NEW_BEZIER_PATCH_HPP_

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
