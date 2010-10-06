/*
=================================================================================
This file is part of Cafu, the open-source game and graphics engine for
multiplayer, cross-platform, real-time 3D action.
$Id$

Copyright (C) 2002-2010 Carsten Fuchs Software.

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

#ifndef _CHILDFRAME_VIEW_WIN_2D_HPP_
#define _CHILDFRAME_VIEW_WIN_2D_HPP_

#include "AxesInfo.hpp"
#include "ChildFrameViewWin.hpp"
#include "Math3D/Vector3.hpp"
#include "Templates/Array.hpp"
#include "wx/wx.h"


class MapElementT;
class ViewWindow2DT;


class MouseDragTimerT : public wxTimer
{
    public:

    MouseDragTimerT(ViewWindow2DT& ViewWin2D);

    void Notify();

    ViewWindow2DT& m_ViewWin2D;
    wxPoint        m_MouseLeftDownPoint;
    wxPoint        m_ScrollDist;
};


class ViewWindow2DT : public wxWindow, public ViewWindowT
{
    public:

    /// The constructor.
    ViewWindow2DT(wxWindow* Parent, ChildFrameT* ChildFrame, ViewTypeT InitialViewType);

    // Methods inherited from ObserverT.
    void NotifySubjectChanged(SubjectT* Subject, MapDocOtherDetailT OtherDetail);
    void NotifySubjectChanged_Selection(SubjectT* Subject, const ArrayT<MapElementT*>& OldSelection, const ArrayT<MapElementT*>& NewSelection);
    void NotifySubjectChanged_Created(const ArrayT<MapElementT*>& MapElements);
    void NotifySubjectChanged_Deleted(SubjectT* Subject, const ArrayT<MapElementT*>& MapElements);
    void NotifySubjectChanged_Modified(SubjectT* Subject, const ArrayT<MapElementT*>& MapElements, MapElemModDetailE Detail);
    void NotifySubjectChanged_Modified(SubjectT* Subject, const ArrayT<MapElementT*>& MapElements, MapElemModDetailE Detail, const ArrayT<BoundingBox3fT>& OldBounds);
    void NotifySubjectChanged_Modified(SubjectT* Subject, const ArrayT<MapElementT*>& MapElements, MapElemModDetailE Detail, const wxString& Key);
 // void NotifySubjectDies(SubjectT* dyingSubject);     // Already implemented by ViewWindowT.

    // Methods inherited from ToolsObserverT.
    void NotifySubjectChanged(ToolsSubjectT* Subject, ToolT* Tool, ToolsUpdatePriorityT Priority);

    // Inherited methods from the ViewWindowT base class.
    wxWindow* GetWindow();
    ViewTypeT GetViewType() const;
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

    friend class MouseDragTimerT;

    ViewWindow2DT(const ViewWindow2DT&);    ///< Use of the Copy    Constructor is not allowed.
    void operator = (const ViewWindow2DT&); ///< Use of the Assignment Operator is not allowed.

    ViewTypeT       m_ViewType;             ///< The type of this 2D view (top, front, side).
    AxesInfoT       m_AxesInfo;             ///< Describes how the three world-space axes map to our two window-space axes.
    wxBitmap        m_BitmapMapOnly;        ///< For the 2D view types.
    wxBitmap        m_BitmapMapAndTools;    ///< For the 2D view types.
    float           m_ZoomFactor;           ///< The zoom factor.
    MouseDragTimerT m_MouseDragTimer;       ///< The timer for mouse-dragging the view.

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
    void OnEraseBackground (wxEraseEvent&            EE);
    void OnSize            (wxSizeEvent&             SE);
    void OnMouseCaptureLost(wxMouseCaptureLostEvent& ME);

    DECLARE_EVENT_TABLE()
};

#endif
