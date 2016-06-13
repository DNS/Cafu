/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_TOOL_HPP_INCLUDED
#define CAFU_TOOL_HPP_INCLUDED

#include "ChildFrame.hpp"   // Make it easy for ToolT derivatives to implement the GetWxEventID() method.
#include "TypeSys.hpp"


class MapDocumentT;
class MapElementT;
class Renderer2DT;
class Renderer3DT;
class ViewWindow2DT;
class ViewWindow3DT;
class wxContextMenuEvent;
class wxKeyEvent;
class wxMouseEvent;
class wxPoint;
class wxWindow;


/// The TypeInfoTs of all ToolT derived classes must register with this TypeInfoManT instance.
cf::TypeSys::TypeInfoManT& GetToolTIM();


/// This is the base class for all tools in the map editor.
/// The 2D and 3D views keep a ToolT reference to the currently active tool,
/// forwarding their key events, mouse events and render requests to it.
class ToolT
{
    public:

    /// Parameters for creating a tool via the TypeSys.
    class ToolCreateParamsT : public cf::TypeSys::CreateParamsT
    {
        public:

        /// The constructor.
        /// @param MapDoc_             The map document for which the tool will be created.
        /// @param ToolMan_            The tool manager that manages the tool.
        /// @param ParentOptionsBar_   The window that is the designated parent for the tools options bar.
        ToolCreateParamsT(MapDocumentT& MapDoc_, ToolManagerT& ToolMan_, wxWindow* ParentOptionsBar_);

        MapDocumentT& MapDoc;           ///< The map document for which the tool will be created.
        ToolManagerT& ToolMan;          ///< The tool manager that manages the tool.
        wxWindow*     ParentOptionsBar; ///< The window that is the designated parent for the tools options bar.
    };


    /// The constructor.
    ToolT(MapDocumentT& MapDoc, ToolManagerT& ToolMan);

    /// The destructor.
    virtual ~ToolT() { }

    /// Returns the ID of the wxWidgets event (menu selection or toolbar button click) that is associated with activating this tool.
    virtual int GetWxEventID() const=0;

    /// Returns the options bar window associated with this tool. NULL if no options bar has been assigned.
    virtual wxWindow* GetOptionsBar() { return NULL; }

    void Activate(ToolT* OldTool);
    void Deactivate(ToolT* NewTool);
    bool IsActiveTool() const { return m_IsActiveTool; }

    virtual bool CanDeactivate() { return true; }
    virtual bool IsHiddenByTool(const MapElementT* Elem) const { return false; }    ///< The caller calls this method in order to learn whether it should exempt the given map element Elem from normal 2D and 3D rendering. This is usually true when Elem is currently being modified by the tool and thus rendered (in a special way) by the tool itself. Examples include brushes being morphed and terrains being edited.
    virtual void RenderTool2D(Renderer2DT& Renderer) const { }
    virtual void RenderTool3D(Renderer3DT& Renderer) const { }
    virtual bool UpdateStatusBar(ChildFrameT* ChildFrame) const { return false; }

    // Event handlers for events chain-forwarded by the 2D view windows.
    virtual bool OnKeyDown2D    (ViewWindow2DT& ViewWindow, wxKeyEvent&         KE) { return false; }
    virtual bool OnKeyUp2D      (ViewWindow2DT& ViewWindow, wxKeyEvent&         KE) { return false; }
    virtual bool OnChar2D       (ViewWindow2DT& ViewWindow, wxKeyEvent&         KE) { return false; }
    virtual bool OnLMouseDown2D (ViewWindow2DT& ViewWindow, wxMouseEvent&       ME) { return false; }   ///< Also used for LMB "double-click" events (use ME.ButtonDClick() for distinction).
    virtual bool OnLMouseUp2D   (ViewWindow2DT& ViewWindow, wxMouseEvent&       ME) { return false; }
    virtual bool OnMMouseDown2D (ViewWindow2DT& ViewWindow, wxMouseEvent&       ME) { return false; }   ///< Also used for MMB "double-click" events (use ME.ButtonDClick() for distinction).
    virtual bool OnMMouseUp2D   (ViewWindow2DT& ViewWindow, wxMouseEvent&       ME) { return false; }
    virtual bool OnRMouseClick2D(ViewWindow2DT& ViewWindow, wxMouseEvent&       ME) { return false; }   ///< For the RMB, only a "click" event is available, because the RMB is also used for mouse-looking and the context menu.
    virtual bool OnMouseWheel2D (ViewWindow2DT& ViewWindow, wxMouseEvent&       ME) { return false; }
    virtual bool OnMouseMove2D  (ViewWindow2DT& ViewWindow, wxMouseEvent&       ME) { return false; }
    virtual int  OnContextMenu2D(ViewWindow2DT& ViewWindow, wxContextMenuEvent& CE, wxMenu& Menu);

    // Event handlers for events chain-forwarded by the 3D view windows.
    virtual bool OnKeyDown3D    (ViewWindow3DT& ViewWindow, wxKeyEvent&         KE) { return false; }
    virtual bool OnKeyUp3D      (ViewWindow3DT& ViewWindow, wxKeyEvent&         KE) { return false; }
    virtual bool OnChar3D       (ViewWindow3DT& ViewWindow, wxKeyEvent&         KE) { return false; }
    virtual bool OnLMouseDown3D (ViewWindow3DT& ViewWindow, wxMouseEvent&       ME) { return false; }   ///< Also used for LMB "double-click" events (use ME.ButtonDClick() for distinction).
    virtual bool OnLMouseUp3D   (ViewWindow3DT& ViewWindow, wxMouseEvent&       ME) { return false; }
    virtual bool OnMMouseDown3D (ViewWindow3DT& ViewWindow, wxMouseEvent&       ME) { return false; }   ///< Also used for MMB "double-click" events (use ME.ButtonDClick() for distinction).
    virtual bool OnMMouseUp3D   (ViewWindow3DT& ViewWindow, wxMouseEvent&       ME) { return false; }
    virtual bool OnRMouseClick3D(ViewWindow3DT& ViewWindow, wxMouseEvent&       ME) { return false; }   ///< For the RMB, only a "click" event is available, because the RMB is also used for mouse-looking and the context menu.
    virtual bool OnMouseWheel3D (ViewWindow3DT& ViewWindow, wxMouseEvent&       ME) { return false; }
    virtual bool OnMouseMove3D  (ViewWindow3DT& ViewWindow, wxMouseEvent&       ME) { return false; }
    virtual int  OnContextMenu3D(ViewWindow3DT& ViewWindow, wxContextMenuEvent& CE, wxMenu& Menu);
    virtual bool OnIdle3D       (ViewWindow3DT& ViewWindow, const wxPoint&   Point) { return false; }

    // The TypeSys related declarations for this class.
    virtual const cf::TypeSys::TypeInfoT* GetType() const { return &TypeInfo; }
    static void* CreateInstance(const cf::TypeSys::CreateParamsT& Params);
    static const cf::TypeSys::TypeInfoT TypeInfo;


    protected:

    MapDocumentT& m_MapDoc;     ///< The document that is modified by this tool.
    ToolManagerT& m_ToolMan;    ///< The tool manager that manages this tool.


    private:

    ToolT(const ToolT&);                ///< Use of the Copy    Constructor is not allowed.
    void operator = (const ToolT&);     ///< Use of the Assignment Operator is not allowed.

    // These methods employ the "Non-Virtual Interface Idiom" as described by Scott Meyers in his
    // book "Effective C++, 3rd Edition" in item 35 ("Consider alternatives to virtual functions.").
    virtual void OnActivate(ToolT* OldTool) { }
    virtual void OnDeactivate(ToolT* NewTool) { }

    bool m_IsActiveTool;    ///< Indicates whether this tool is the currently active tool.
};

#endif
