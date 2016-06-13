/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_GUIEDITOR_TOOL_HPP_INCLUDED
#define CAFU_GUIEDITOR_TOOL_HPP_INCLUDED


class wxKeyEvent;
class wxMouseEvent;
class wxContextMenuEvent;


namespace GuiEditor
{
    class RenderWindowT;

    // Tool IDs.
    enum ToolID
    {
        TOOL_SELECTION,
        TOOL_NEW_WINDOW
    };


    class ToolI
    {
        public:

        ToolI() {}
        virtual ~ToolI() {}

        virtual ToolID GetID() const=0;

        virtual void Activate() {}
        virtual void Deactivate() {}

        virtual void RenderTool(RenderWindowT* RenderWindow) {}

        virtual bool OnKeyDown(RenderWindowT* RenderWindow, wxKeyEvent& KE) { return false; }
        virtual bool OnKeyUp  (RenderWindowT* RenderWindow, wxKeyEvent& KE) { return false; }
        virtual bool OnChar   (RenderWindowT* RenderWindow, wxKeyEvent& KE) { return false; }

        virtual bool OnLMouseDown(RenderWindowT* RenderWindow, wxMouseEvent& ME) { return false; }
        virtual bool OnLMouseUp  (RenderWindowT* RenderWindow, wxMouseEvent& ME) { return false; }
        virtual bool OnRMouseDown(RenderWindowT* RenderWindow, wxMouseEvent& ME) { return false; }
        virtual bool OnRMouseUp  (RenderWindowT* RenderWindow, wxMouseEvent& ME) { return false; }
        virtual bool OnMouseWheel(RenderWindowT* RenderWindow, wxMouseEvent& ME) { return false; }
        virtual bool OnMouseMove (RenderWindowT* RenderWindow, wxMouseEvent& ME) { return false; }

        virtual bool OnContextMenu(RenderWindowT* RenderWindow, wxContextMenuEvent& CE) { return false; }
    };
}

#endif
