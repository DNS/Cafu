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

#ifndef _GUIEDITOR_TOOL_HPP_
#define _GUIEDITOR_TOOL_HPP_


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
