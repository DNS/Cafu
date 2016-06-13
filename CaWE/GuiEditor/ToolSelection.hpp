/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_GUIEDITOR_TOOL_SELECTION_HPP_INCLUDED
#define CAFU_GUIEDITOR_TOOL_SELECTION_HPP_INCLUDED

#include "Tool.hpp"

#include "Math3D/Vector2.hpp"
#include "Templates/Array.hpp"
#include "wx/gdicmn.h"


namespace GuiEditor
{
    class GuiDocumentT;
    class ChildFrameT;


    class ToolSelectionT : public ToolI
    {
        public:

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

        /// This enumeration defines the essential states of this tool.
        /// Note that in each tool state, the selection can contain any number of map elements.
        enum ToolStateT
        {
            /// The LMB is up and nothing is currently happening.
            /// The mouse cursor however is updated to indicate a likely next state if the button is pressed at the current position.
            TS_IDLE,

            /// One of the window handles is currently being dragged, transforming the selected window(s).
            TS_DRAG_HANDLE
        };

        enum TrafoHandleT
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

        /// Window attributes that are changed by a transformation.
        struct WindowStateT
        {
            Vector2fT Position;
            Vector2fT Size;
            float     Rotation;
        };


        TrafoHandleT GetHandle(const Vector2fT& GuiPos) const;
        wxCursor     SuggestCursor(TrafoHandleT TrafoHandle) const;

        GuiDocumentT*        m_GuiDocument;
        ChildFrameT*         m_Parent;

        ToolStateT           m_ToolState;
        TrafoHandleT         m_DragState;

        ArrayT<WindowStateT> m_WindowStates;    ///< Holds all original window states of the currently transformed windows.
        float                m_RotStartAngle;   ///< The rotation angle at which the transform was started.
        Vector2fT            m_LastMousePos;    ///< Last known mouse position, in GUI coordinates.
    };
}

#endif
