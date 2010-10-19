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

#ifndef _CHILDFRAME_VIEW_WIN_3D_HPP_
#define _CHILDFRAME_VIEW_WIN_3D_HPP_

#include "ChildFrameViewWin.hpp"
#include "Renderer3D.hpp"
#include "Math3D/Plane3.hpp"
#include "wx/glcanvas.h"


class AxesInfoT;
class CameraT;
class MapElementT;
class ToolCameraT;


class ViewWindow3DT : public wxGLCanvas, public ViewWindowT
{
    public:

    /// This class defines if and how the camera of the associated view is currently being controlled with the mouse.
    class MouseControlT
    {
        public:

        enum StateT { NOT_ACTIVE, ACTIVE_NORMAL, ACTIVE_ORBIT };

        MouseControlT(ViewWindow3DT& ViewWin);

        /// Activates the mouse control in the given state.
        /// @param NewState   The state in which the mouse control should be activated.
        /// @param RefPt      The position of the reference point in window coordinates.
        /// Note that if the mouse control is already active, it can not be reactivated in a different ACTIVE_* state but must be deactivated first.
        void Activate(StateT NewState, const wxPoint& RefPt=wxDefaultPosition);

        /// Deactivates the mouse control.
        void Deactivate();

        /// Returns the state that the mouse control is currently in.
        StateT GetState() const { return m_State; }

        /// Returns whether the mouse control is active; a shortcut for GetState()!=NOT_ACTIVE.
        bool IsActive() const { return m_State!=NOT_ACTIVE; }

        /// Return the position of the reference point in window coordinates as set when the mouse control was activated.
        const wxPoint& GetRefPtWin() const { return m_RefPtWin; }

        /// Return the position of the reference point in world coordinates as set when the mouse control was activated.
        const Vector3fT& GetRefPtWorld() const { return m_RefPtWorld; }


        private:

        ViewWindow3DT& m_ViewWin;
        StateT         m_State;
        wxPoint        m_RefPtWin;    ///< The position of the reference point in window coordinates as set when the mouse control was activated.
        Vector3fT      m_RefPtWorld;  ///< The position of the reference point in world  coordinates as set when the mouse control was activated.
    };

    /// This struct describes a hit (an intersection of a map element with a view ray through a given pixel) as returned by the GetElementsAt() method.
    struct HitInfoT
    {
        MapElementT*  Object;   ///< Pointer to the intersected map element.
        unsigned long FaceNr;   ///< If Object is a map brush, this is the number of its face that was hit.
        float         Depth;    ///< Depth value (distance from ray origin (on near clip plane) along the ray) of the hit object.
        Vector3fT     Pos;      ///< The point in the world where the object was hit.
    };


    /// The constructor.
    ViewWindow3DT(wxWindow* Parent, ChildFrameT* ChildFrame, CameraT* InitialCamera, ViewTypeT InitialViewType);

    /// The destructor.
    ~ViewWindow3DT();

    // Methods inherited from ObserverT.
    // Note that the 3D view is updated on idle anyway, so no observer messages are implemented at this point.
    // void NotifySubjectDies(SubjectT* dyingSubject);     // Already implemented in base class ViewWindowT.

    // Methods inherited from ToolsObserverT.
    void NotifySubjectChanged(ToolsSubjectT* Subject, ToolT* Tool, ToolsUpdatePriorityT Priority);

    // Inherited methods from the ViewWindowT base class.
    wxWindow* GetWindow();
    ViewTypeT GetViewType() const;
    AxesInfoT GetAxesInfo() const;


    /// Returns the camera that is currently associated with this view.
    const CameraT& GetCamera() const { return *m_Camera; }

    /// Returns the view frustum for this view, based on its current window dimensions and camera setting.
    void GetViewFrustum(Plane3fT* Planes, unsigned int NumPlanes=6) const;

    /// Moves the camera that is currently associated with this view to the given new position.
    void MoveCamera(const Vector3fT& NewPos);

    /// This method returns visible all map elements at a given pixel in the 3D view window.
    /// @param Pixel   The pixel in window coordinates for which the map elements are to be found.
    /// @returns The array of visible map elements that intersect the ray from the camera position through the pixel.
    ///          The elements are returned in front-to-back order, i.e. the nearest object is at array index 0.
    ArrayT<HitInfoT> GetElementsAt(const wxPoint& Pixel) const;

    /// Transforms (unprojects) the given pixel from window space to the related point in world space.
    /// The returned point is located on the near clipping plane of this windows view pyramid/frustum.
    /// Therefore, "camera rays" are conveniently built through the two points GetCamera().Pos and WindowToWorld(Pixel).
    /// This method is "roughly" the inverse of WorldToWindow().
    /// @param Pixel   The pixel to convert from window to world space.
    /// @returns the pixel in world space coordinates.
    Vector3fT WindowToWorld(const wxPoint& Pixel) const;

    /// Transforms (projects) the given point from world space to the related pixel in the 3D view window.
    /// The transformation is not always reasonably possible, and therefore, if CheckFrustum is true (recommended),
    /// the method checks whether v is inside the view frustum first and returns (-1, -1) otherwise.
    /// If CheckFrustum is false, no view-frustum check is performed, with the potential negative consequences
    /// that can occur when performing the transformation stubbornly (divisions-by-zero, point behind the
    /// viewer yields valid window coordinates, etc.). This method is "roughly" the inverse of WindowToWorld().
    /// @param v   The point to be transformed from world to window coordinates.
    /// @param CheckFrustum   Whether v should be tested against the current view frustum before the transformation.
    /// @returns the transformed point, or (-1, -1) if CheckFrustum was true and the v failed the test.
    wxPoint WorldToWindow(const Vector3fT& v, bool CheckFrustum) const;


    private:

    friend class ToolCameraT;
    enum RightMBStateT { RMB_UP_IDLE, RMB_DOWN_UNDECIDED, RMB_DOWN_DRAGGING };  ///< This enumeration describes the states that the right mouse button can take.

    ViewTypeT     m_ViewType;           ///< The type of this 3D view (wireframe, flat, materials, ...).
    Renderer3DT   m_Renderer;           ///< Performs the 3D rendering in our window.
    unsigned long m_TimeOfLastPaint;    ///< The time at which the OnPaint() event handler was last called.
    ToolCameraT*  m_CameraTool;         ///< The camera tool that manages all cameras. The camera of this 3D view is always among the cameras in the tool.
    CameraT*      m_Camera;             ///< Pointer to the camera that is currently used for this 3D view. The actual instance of the camera is kept in the camera tool.
    Vector3fT     m_CameraVel;          ///< The cameras current velocity, in camera space. Positive values for m_CameraVel.y mean forward movement, etc.
    MouseControlT m_MouseControl;       ///< If and how the camera of the associated view is currently being controlled with the mouse.
    RightMBStateT m_RightMBState;       ///< The state of the right mouse button. This is required because the RMB has a dual function: a click can bring up the context menu, or initiate mouse-looking for the 3D view.
    wxPoint       m_RDownPosWin;        ///< The point where the RMB went down, in window coordinates.

    // Event handlers.
    void OnKeyDown         (wxKeyEvent&              ME);
    void OnKeyUp           (wxKeyEvent&              ME);
    void OnKeyChar         (wxKeyEvent&              ME);
    void OnMouseLeftDown   (wxMouseEvent&            ME);   ///< We also handle "double-click" events in this method (use ME.ButtonDClick() for distinction).
    void OnMouseLeftUp     (wxMouseEvent&            ME);
    void OnMouseMiddleDown (wxMouseEvent&            ME);   ///< We also handle "double-click" events in this method (use ME.ButtonDClick() for distinction).
    void OnMouseMiddleUp   (wxMouseEvent&            ME);
    void OnMouseRightDown  (wxMouseEvent&            ME);   ///< We also handle "double-click" events in this method (use ME.ButtonDClick() for distinction).
    void OnMouseRightUp    (wxMouseEvent&            ME);
    void OnMouseWheel      (wxMouseEvent&            ME);
    void OnMouseMove       (wxMouseEvent&            ME);
    void OnContextMenu     (wxContextMenuEvent&      CE);
    void OnPaint           (wxPaintEvent&            PE);
    void OnSize            (wxSizeEvent&             SE);
    void OnSetFocus        (wxFocusEvent&            FE);
    void OnKillFocus       (wxFocusEvent&            FE);
    void OnMouseCaptureLost(wxMouseCaptureLostEvent& ME);

    DECLARE_EVENT_TABLE()
};

#endif
