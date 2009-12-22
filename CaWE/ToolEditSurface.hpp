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

#ifndef _TOOL_EDIT_SURFACE_HPP_
#define _TOOL_EDIT_SURFACE_HPP_

#include "Tool.hpp"


class MapDocumentT;
class OptionsBar_EditFacePropsToolT;


class ToolEditSurfaceT : public ToolT
{
    public:

    ToolEditSurfaceT(MapDocumentT& MapDoc, ToolManagerT& ToolMan, wxWindow* ParentOptionsBar);

    // Implementations/overrides of ToolT methods.
    int       GetWxEventID() { return ChildFrameT::ID_MENU_TOOLS_TOOL_EDITSURFACEPROPERTIES; }
    wxWindow* GetOptionsBar();
    void      OnActivate(ToolT* OldTool);
    void      OnDeactivate(ToolT* NewTool);

    bool OnLMouseDown2D(ViewWindow2DT& ViewWindow, wxMouseEvent& ME);
    bool OnKeyDown2D   (ViewWindow2DT& ViewWindow, wxKeyEvent&   KE);
    bool OnKeyUp2D     (ViewWindow2DT& ViewWindow, wxKeyEvent&   KE);

    bool OnKeyDown3D   (ViewWindow3DT& ViewWindow, wxKeyEvent&   KE);
    bool OnKeyUp3D     (ViewWindow3DT& ViewWindow, wxKeyEvent&   KE);
    bool OnLMouseDown3D(ViewWindow3DT& ViewWindow, wxMouseEvent& ME);
    bool OnRMouseDown3D(ViewWindow3DT& ViewWindow, wxMouseEvent& ME);
    bool OnMouseMove3D (ViewWindow3DT& ViewWindow, wxMouseEvent& ME);

    bool UpdateStatusBar(ChildFrameT* ChildFrame) const;

    // The TypeSys related declarations for this class.
    virtual const cf::TypeSys::TypeInfoT* GetType() const { return &TypeInfo; }
    static void* CreateInstance(const cf::TypeSys::CreateParamsT& Params);
    static const cf::TypeSys::TypeInfoT TypeInfo;


    private:

    bool m_EyeDropperActive;

    OptionsBar_EditFacePropsToolT* m_OptionsBar;   ///< The options bar for this tool.
};

#endif
