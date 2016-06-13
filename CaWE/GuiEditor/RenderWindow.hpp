/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_GUIEDITOR_RENDER_WINDOW_HPP_INCLUDED
#define CAFU_GUIEDITOR_RENDER_WINDOW_HPP_INCLUDED

#include "ObserverPattern.hpp"
#include "Math3D/Vector2.hpp"
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
        void Notify_Changed(SubjectT* Subject, const cf::TypeSys::VarBaseT& Var);
        void NotifySubjectDies(SubjectT* dyingSubject);

        Vector2fT ClientToGui(int x, int y) const;
        wxPoint   GuiToClient(const Vector2fT& Pos) const;

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
