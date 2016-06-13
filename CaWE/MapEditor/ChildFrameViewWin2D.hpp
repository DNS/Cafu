/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_CHILDFRAME_VIEW_WIN_2D_HPP_INCLUDED
#define CAFU_CHILDFRAME_VIEW_WIN_2D_HPP_INCLUDED

#include "ChildFrameViewWin.hpp"

#include "../AxesInfo.hpp"

#include "Math3D/Vector3.hpp"
#include "Templates/Array.hpp"

#include "wx/wx.h"


class MapElementT;
class ViewWindow2DT;


class ViewWindow2DT : public wxWindow, public ViewWindowT
{
    public:

    /// This class defines if the associated view is currently being grabbed (for scrolling) with the mouse.
    /// It is similar to class ViewWindow3DT::MouseControlT, but much simpler as grabbing is the only type of control.
    class MouseGrabT
    {
        public:

        MouseGrabT(ViewWindow2DT& ViewWin);

        /// Activates mouse grabbing at the given reference point,
        /// or updates the reference point when mouse grabbing is already active.
        /// Note that activating can fail, especially if some other code has already captured the mouse pointer.
        void Activate(const wxPoint& RefPt);

        /// Deactivates the mouse control.
        void Deactivate();

        /// Returns whether the mouse control is active.
        bool IsActive() const { return m_IsActive; }

        /// Returns the position of the reference point in window coordinates.
        const wxPoint& GetRefPt() const { return m_RefPtWin; }


        private:

        ViewWindow2DT& m_ViewWin;
        bool           m_IsActive;
        wxPoint        m_RefPtWin;  ///< The position of the reference point in window coordinates.
    };


    /// The constructor.
    ViewWindow2DT(wxWindow* Parent, ChildFrameT* ChildFrame, ViewTypeT InitialViewType);

    // Methods inherited from ObserverT.
    void NotifySubjectChanged(SubjectT* Subject, MapDocOtherDetailT OtherDetail);
    void NotifySubjectChanged_Selection(SubjectT* Subject, const ArrayT<MapElementT*>& OldSelection, const ArrayT<MapElementT*>& NewSelection);
    void NotifySubjectChanged_Created(SubjectT* Subject, const ArrayT< IntrusivePtrT<cf::GameSys::EntityT> >& Entities);
    void NotifySubjectChanged_Created(SubjectT* Subject, const ArrayT<MapPrimitiveT*>& Primitives);
    void NotifySubjectChanged_Deleted(SubjectT* Subject, const ArrayT< IntrusivePtrT<cf::GameSys::EntityT> >& Entities);
    void NotifySubjectChanged_Deleted(SubjectT* Subject, const ArrayT<MapPrimitiveT*>& Primitives);
    void NotifySubjectChanged_Modified(SubjectT* Subject, const ArrayT<MapElementT*>& MapElements, MapElemModDetailE Detail);
    void NotifySubjectChanged_Modified(SubjectT* Subject, const ArrayT<MapElementT*>& MapElements, MapElemModDetailE Detail, const ArrayT<BoundingBox3fT>& OldBounds);
    void Notify_EntChanged(SubjectT* Subject, const ArrayT< IntrusivePtrT<MapEditor::CompMapEntityT> >& Entities, EntityModDetailE Detail);
    void Notify_VarChanged(SubjectT* Subject, const cf::TypeSys::VarBaseT& Var);
 // void NotifySubjectDies(SubjectT* dyingSubject);     // Already implemented by ViewWindowT.

    // Methods inherited from ToolsObserverT.
    void NotifySubjectChanged(ToolsSubjectT* Subject, ToolT* Tool, ToolsUpdatePriorityT Priority);

    // Inherited methods from the ViewWindowT base class.
    wxWindow* GetWindow();
    ViewTypeT GetViewType() const { return m_ViewType; }
    AxesInfoT GetAxesInfo() const { return m_AxesInfo; }

    // Helper methods (all for public and some also for private use).
    void  SetZoom(float NewZoom);
    float GetZoom() const { return m_ZoomFactor; }
    void  CenterView(const Vector3fT& Point);

    /// This method returns all visible map elements at the given pixel in the 2D view window.
    /// More precisely, all visible map elements that intersect the disc centered around Pixel with radius Radius are returned.
    /// @param Pixel    The pixel in window coordinates for which the map elements are to be found.
    /// @param Radius   The error tolerance around Pixel within which map elements also count as "at Pixel".
    /// @returns The array of visible map elements that are at the given disc (in no particular order).
    ArrayT<MapElementT*> GetElementsAt(const wxPoint& Pixel, int Radius=3) const;

    wxPoint GetScrollPosXY()   const { return wxPoint(GetScrollPos  (wxHORIZONTAL), GetScrollPos  (wxVERTICAL)); }  ///< Conveniently return the positions of both scroll bars in a wxPoint.
    wxPoint GetScrollRangeXY() const { return wxPoint(GetScrollRange(wxHORIZONTAL), GetScrollRange(wxVERTICAL)); }  ///< Conveniently return the ranges    of both scroll bars in a wxPoint.
    wxPoint GetClientCenter()  const { const wxSize Size=GetClientSize(); return wxPoint(Size.x/2, Size.y/2); }     ///< Returns the center point of the client area of this window.

    // Methods for converting points between the coordinate systems.
    // TODO: Should window and tool coordinates also be of type float rather than int???
    // Floats would avoid the forced round-off error with intermediate results and work well when OpenGL is used also for 2D rendering.
    // Moreover, I'd even suggest to just say   typedef CoordT Vector3fT;   because we could even convert the third axis in both directions.
    wxPoint   WindowToTool (const wxPoint&   p) const;  ///< Transforms point p from window to tool coordinates/space. This is the inverse of ToolToWindow().
    wxPoint   ToolToWindow (const wxPoint&   p) const;  ///< Transforms point p from tool to window coordinates/space. This is the inverse of WindowToTool().

    Vector3fT ToolToWorld  (const wxPoint&   p, float Third) const;  ///< Transforms point  p from tool  to world coordinates/space. This is the inverse of WorldToTool(). The value for the third axis of the returned vector is set to Third.
    wxPoint   WorldToTool  (const Vector3fT& v) const;  ///< Transforms vector v from world to tool  coordinates/space. This is the inverse of ToolToWorld().

    Vector3fT WindowToWorld(const wxPoint&   p, float Third) const;  ///< Transforms point  p from window to world  coordinates/space. This is the inverse of WorldToWindow(), and equivalent to calling ToolToWorld(WindowToTool(p)). The value for the third axis of the returned vector is set to Third.
    wxPoint   WorldToWindow(const Vector3fT& v) const;  ///< Transforms vector v from world  to window coordinates/space. This is the inverse of WindowToWorld(), and equivalent to calling ToolToWindow(WorldToTool(p)).

    /// Determines whether the given rectangle Rect is intersected by the line segment through points A and B.
    static bool RectIsIntersected(const wxRect& Rect, const wxPoint& A, const wxPoint& B);


    private:

    enum RightMBStateT { RMB_UP_IDLE, RMB_DOWN_UNDECIDED, RMB_DOWN_DRAGGING };  ///< This enumeration describes the states that the right mouse button can take.

    ViewWindow2DT(const ViewWindow2DT&);    ///< Use of the Copy    Constructor is not allowed.
    void operator = (const ViewWindow2DT&); ///< Use of the Assignment Operator is not allowed.

    ViewTypeT     m_ViewType;           ///< The type of this 2D view (top, front, side).
    AxesInfoT     m_AxesInfo;           ///< Describes how the three world-space axes map to our two window-space axes.
    wxBitmap      m_BitmapMapOnly;      ///< This bitmap caches our 2D rendering of the map only.
    wxBitmap      m_BitmapMapAndTools;  ///< This bitmap caches our 2D rendering of the map and the tools overlay.
    float         m_ZoomFactor;         ///< The zoom factor.
    MouseGrabT    m_MouseGrab;          ///< If this view is currently being grabbed and scrolled with the mouse.
    RightMBStateT m_RightMBState;       ///< The state of the right mouse button. This is required because the RMB has a dual function: a click can bring up the context menu, or initiate mouse-grabbing.
    wxPoint       m_RDownPosWin;        ///< The point where the RMB went down, in window coordinates.

    // Private helper methods.
    void SetViewType(ViewTypeT NewViewType);
    void ScrollWindow(int AmountX, int AmountY, const wxRect* Rect=NULL);   ///< Overridden method of base class wxWindow.
    void UpdateScrollbars();                                                ///< Updates the scrollbars (and thus the virtual area of the window) according to window client size and map zoom factor.
    int  GetBestGridSpacing() const;

    void Render(const wxRect& UpdateRect);                                  ///< Called by DoPaint().
    enum LayerT { TOOL_LAYER, MAP_AND_TOOL_LAYER };                         ///< The bitmap layers to repaint in DoPaint(). Note that there is no MAP_LAYER, because re-painting the map layer requires re-painting the tool layer (an inherent property of our implementation).
    void DoPaint(LayerT Layer, wxDC& dc, wxRegion* UpdateRegion=NULL);      ///< Called by OnPaint() and NotifySubjectChanged(Tool).

    // Event handlers.
    void OnKeyDown         (wxKeyEvent&              KE);
    void OnKeyUp           (wxKeyEvent&              KE);
    void OnKeyChar         (wxKeyEvent&              KE);
    void OnMouseLeftDown   (wxMouseEvent&            ME);   ///< We also handle "double-click" events in this method (use ME.ButtonDClick() for distinction).
    void OnMouseLeftUp     (wxMouseEvent&            ME);
    void OnMouseMiddleDown (wxMouseEvent&            ME);   ///< We also handle "double-click" events in this method (use ME.ButtonDClick() for distinction).
    void OnMouseMiddleUp   (wxMouseEvent&            ME);
    void OnMouseRightDown  (wxMouseEvent&            ME);   ///< We also handle "double-click" events in this method (use ME.ButtonDClick() for distinction).
    void OnMouseRightUp    (wxMouseEvent&            ME);
    void OnMouseWheel      (wxMouseEvent&            ME);
    void OnMouseMove       (wxMouseEvent&            ME);
    void OnScroll          (wxScrollWinEvent&        SE);
    void OnContextMenu     (wxContextMenuEvent&      CE);
    void OnPaint           (wxPaintEvent&            PE);
    void OnSize            (wxSizeEvent&             SE);
    void OnKillFocus       (wxFocusEvent&            FE);
    void OnMouseCaptureLost(wxMouseCaptureLostEvent& ME);

    DECLARE_EVENT_TABLE()
};

#endif
