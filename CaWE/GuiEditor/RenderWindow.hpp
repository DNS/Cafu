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

#ifndef CAFU_GUIEDITOR_RENDER_WINDOW_HPP_INCLUDED
#define CAFU_GUIEDITOR_RENDER_WINDOW_HPP_INCLUDED

#include "ObserverPattern.hpp"

#include "Math3D/Vector3.hpp"

#include "wx/glcanvas.h"


namespace GuiEditor
{
    class ChildFrameT;
    class GuiDocumentT;

    class RenderWindowT : public wxGLCanvas, public ObserverT
    {
        public:

        RenderWindowT(ChildFrameT* Parent);

        // ObserverT implementation.
        void NotifySubjectChanged_Selection(SubjectT* Subject, const ArrayT< IntrusivePtrT<cf::GuiSys::WindowT> >& OldSelection, const ArrayT< IntrusivePtrT<cf::GuiSys::WindowT> >& NewSelection);
        void NotifySubjectChanged_Created(SubjectT* Subject, const ArrayT< IntrusivePtrT<cf::GuiSys::WindowT> >& Windows);
        void NotifySubjectChanged_Deleted(SubjectT* Subject, const ArrayT< IntrusivePtrT<cf::GuiSys::WindowT> >& Windows);
        void NotifySubjectChanged_Modified(SubjectT* Subject, const ArrayT< IntrusivePtrT<cf::GuiSys::WindowT> >& Windows, WindowModDetailE Detail);
        void NotifySubjectChanged_Modified(SubjectT* Subject, const ArrayT< IntrusivePtrT<cf::GuiSys::WindowT> >& Windows, WindowModDetailE Detail, const wxString& PropertyName);
        void Notify_WinChanged(SubjectT* Subject, const EditorWindowT* Win, const wxString& PropName);
        void NotifySubjectDies(SubjectT* dyingSubject);

        Vector3fT ClientToGui(int x, int y);
        wxPoint   GuiToClient(float x, float y);

        void ZoomIn();
        void ZoomOut();
        void ZoomFit();
        void ZoomSet(float ZoomFactor);


        private:

        GuiDocumentT* m_GuiDocument;
        ChildFrameT*  m_Parent;
        unsigned long m_TimeLastFrame;
        float         m_Zoom;

        float m_OffsetX;
        float m_OffsetY;

        void UpdateScrollbars();                     ///< Helper to calculate scroll ranges based on client size and zoom factor.
        void CalcViewOffsets();                      ///< Helper to calculate the view offsets based on client size and zoom factor.
        void ScrollWindow(int AmountX, int AmountY); ///< Helper to scroll the window.
        void CenterView();                           ///< Helper to center the GUI in the render window.

        void OnPaint     (wxPaintEvent&     PE);
        void OnMouseWheel(wxMouseEvent&     ME);
        void OnSize      (wxSizeEvent&      SE);
        void OnMouseMove (wxMouseEvent&     ME);
        void OnLMouseDown(wxMouseEvent&     ME);
        void OnLMouseUp  (wxMouseEvent&     ME);
        void OnRMouseUp  (wxMouseEvent&     ME);
        void OnKeyDown   (wxKeyEvent&       KE);
        void OnScroll    (wxScrollWinEvent& SE);

        DECLARE_EVENT_TABLE()
    };
}


#endif
