/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_TOOL_EDIT_SURFACE_HPP_INCLUDED
#define CAFU_TOOL_EDIT_SURFACE_HPP_INCLUDED

#include "Tool.hpp"


class MapDocumentT;
class OptionsBar_EditFacePropsToolT;


class ToolEditSurfaceT : public ToolT
{
    public:

    ToolEditSurfaceT(MapDocumentT& MapDoc, ToolManagerT& ToolMan, wxWindow* ParentOptionsBar);

    // Implementations/overrides of ToolT methods.
    int       GetWxEventID() const { return ChildFrameT::ID_MENU_TOOLS_TOOL_EDITSURFACEPROPERTIES; }
    wxWindow* GetOptionsBar();
    void      OnActivate(ToolT* OldTool);
    void      OnDeactivate(ToolT* NewTool);

    bool OnLMouseDown2D (ViewWindow2DT& ViewWindow, wxMouseEvent& ME);
    bool OnKeyDown2D    (ViewWindow2DT& ViewWindow, wxKeyEvent&   KE);
    bool OnKeyUp2D      (ViewWindow2DT& ViewWindow, wxKeyEvent&   KE);

    bool OnKeyDown3D    (ViewWindow3DT& ViewWindow, wxKeyEvent&   KE);
    bool OnKeyUp3D      (ViewWindow3DT& ViewWindow, wxKeyEvent&   KE);
    bool OnLMouseDown3D (ViewWindow3DT& ViewWindow, wxMouseEvent& ME);
    bool OnRMouseClick3D(ViewWindow3DT& ViewWindow, wxMouseEvent& ME);
    bool OnMouseMove3D  (ViewWindow3DT& ViewWindow, wxMouseEvent& ME);

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
