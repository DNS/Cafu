/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_TOOL_NEW_ENTITY_HPP_INCLUDED
#define CAFU_TOOL_NEW_ENTITY_HPP_INCLUDED

#include "Tool.hpp"
#include "wx/wx.h"


class OptionsBar_NewEntityToolT;


/// This class implements the "New Entity" tool.
class ToolNewEntityT : public ToolT
{
    public:

    /// The constructor.
    ToolNewEntityT(MapDocumentT& MapDoc, ToolManagerT& ToolMan, wxWindow* ParentOptionsBar, bool CreateOptionsBar=true);


    // Implementations/overrides of ToolT methods.
    int       GetWxEventID() const { return ChildFrameT::ID_MENU_TOOLS_TOOL_NEWENTITY; }
    wxWindow* GetOptionsBar();

    bool OnKeyDown2D   (ViewWindow2DT& ViewWindow, wxKeyEvent&   KE);
    bool OnLMouseDown2D(ViewWindow2DT& ViewWindow, wxMouseEvent& ME);
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


    protected:

    OptionsBar_NewEntityToolT* m_OptionsBar;   ///< The options bar for this tool.
};

#endif
