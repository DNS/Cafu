/*
=================================================================================
This file is part of Cafu, the open-source game engine and graphics engine
for multiplayer, cross-platform, real-time 3D action.
Copyright (C) 2002-2011 Carsten Fuchs Software.

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

#ifndef _GUIEDITOR_TOOL_SELECTION_HPP_
#define _GUIEDITOR_TOOL_SELECTION_HPP_

#include "Tool.hpp"

#include "Math3D/Vector3.hpp"

#include "Templates/Array.hpp"


namespace GuiEditor
{
    class GuiDocumentT;
    class ChildFrameT;

    /// Through the selection tool the user can select, move, resize or rotate windows.
    /// The selection tool is basically a state machine:
    /// Initially the tool in in an idle state where the user can move the mouse around while
    /// nothing happens.
    /// In this state the user can select windows by clicking them and thus change the selection.
    /// If the mouse cursor is moved over an already selected window the TransformationState of
    /// the tool changes according to the position over the window to TRANSLATE or SCALE* (whereas
    /// * is the scale direction).
    /// Left clicking on an already selected window (valid TransformationState) sets the tool into
    /// transformation mode and following mouse movements result in transformation of the currently
    /// selected windows according to TransformationState.
    class ToolSelectionT : public ToolI
    {
        public:

        enum TransformationStateE
        {
            NONE,
            TRANSLATE,
            SCALE_N,
            SCALE_NE,
            SCALE_E,
            SCALE_SE,
            SCALE_S,
            SCALE_SW,
            SCALE_W,
            SCALE_NW,
            ROTATE
        };

        ToolSelectionT(GuiDocumentT* GuiDocument, ChildFrameT* Parent);

        ToolID GetID() const { return TOOL_SELECTION; }

        void Activate();
        void Deactivate();

        void RenderTool(RenderWindowT* RenderWindow);

        bool OnKeyDown(RenderWindowT* RenderWindow, wxKeyEvent& KE);

        bool OnLMouseDown(RenderWindowT* RenderWindow, wxMouseEvent& ME);
        bool OnLMouseUp  (RenderWindowT* RenderWindow, wxMouseEvent& ME);
        bool OnMouseMove (RenderWindowT* RenderWindow, wxMouseEvent& ME);
        bool OnRMouseUp  (RenderWindowT* RenderWindow, wxMouseEvent& ME);


        private:

        /// Window attributes that are changed by a transformation.
        /// Needed for command handling.
        struct WindowStateT
        {
            Vector3fT Position;
            Vector3fT Size;
            float     Rotation;
        };

        ArrayT<WindowStateT> m_WindowStates; ///< Holds all original window states of the currently transformed windows.

        GuiDocumentT*        m_GuiDocument;
        ChildFrameT*         m_Parent;

        bool                 m_TransformSelection;  ///< Whether the selection is currently transformed (mouse moved transforms selection).
        bool                 m_TransformationStart; ///< Whether a transformation has been triggered (left mouse down on selected window).
        TransformationStateE m_TransState;

        // Rotation start angle.
        float m_RotStartAngle;

        // Last known mouse position in GUI coordinates.
        float m_LastMousePosX;
        float m_LastMousePosY;
    };
}

#endif
