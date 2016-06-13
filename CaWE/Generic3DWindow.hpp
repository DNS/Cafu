/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_GENERIC_3D_WINDOW_HPP_INCLUDED
#define CAFU_GENERIC_3D_WINDOW_HPP_INCLUDED

#include "Math3D/Plane3.hpp"
#include "wx/glcanvas.h"


class AxesInfoT;
class CameraT;


/// This class implements a generic 3D window.
/// It provides basic, common 3D window functionality that is independent from any editor.
class Generic3DWindowT : public wxGLCanvas
{
    public:

    /// This class defines if and how the camera of the associated window is currently being controlled with the mouse.
    class MouseControlT
    {
        public:

        enum StateT { NOT_ACTIVE, ACTIVE_NORMAL, ACTIVE_ORBIT };

        MouseControlT(Generic3DWindowT& Win);

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

        Generic3DWindowT& m_Win;        ///< The 3D window that this is the mouse control for.
        StateT            m_State;      ///< The current state of the mouse control.
        wxPoint           m_RefPtWin;   ///< The position of the reference point in window coordinates as set when the mouse control was activated.
        Vector3fT         m_RefPtWorld; ///< The position of the reference point in world  coordinates as set when the mouse control was activated.
    };


    /// The constructor.
    Generic3DWindowT(wxWindow* Parent, CameraT* InitialCamera);

    /// The destructor.
    ~Generic3DWindowT();

    /// Returns the set of axes that the camera orientation is currently the closest to.
    AxesInfoT GetAxesInfo() const;

    /// Returns the camera that is currently associated with this window.
    const CameraT& GetCamera() const { return *m_Camera; }

    /// Sets Camera as the new camera to use for this window.
    void SetCamera(CameraT* Camera);

    /// Moves the camera that is currently associated with this window to the given new position.
    void MoveCamera(const Vector3fT& NewPos);

    /// Processes the user input for the (last) frame with the given duration and updates the camera accordingly.
    void ProcessInput(float FrameTime);

    /// Returns the mouse control instance of this window.
    const MouseControlT& GetMouseControl() const { return m_MouseControl; }

    /// Returns the view frustum for this window, based on its current window dimensions and camera setting.
    void GetViewFrustum(Plane3fT* Planes, unsigned int NumPlanes=6) const;

    /// Transforms (unprojects) the given pixel from window space to the related point in world space.
    /// The returned point is located on the near clipping plane of this windows view pyramid/frustum.
    /// Therefore, "camera rays" are conveniently built through the two points GetCamera().Pos and WindowToWorld(Pixel).
    /// This method is "roughly" the inverse of WorldToWindow().
    /// @param Pixel   The pixel to convert from window to world space.
    /// @returns the pixel in world space coordinates.
    Vector3fT WindowToWorld(const wxPoint& Pixel) const;

    /// Transforms (projects) the given point from world space to the related pixel in the 3D window.
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

    // Methods that derived classes (cannot call, but) must implement.
    virtual Vector3fT GetRefPtWorld(const wxPoint& RefPtWin) const=0;
    virtual void      InfoCameraChanged()=0;
    virtual void      InfoRightMouseClick(wxMouseEvent& ME)=0;

    enum RightMBStateT { RMB_UP_IDLE, RMB_DOWN_UNDECIDED, RMB_DOWN_DRAGGING };  ///< This enumeration describes the states that the right mouse button can take.

    CameraT*      m_Camera;         ///< Pointer to the camera that is currently used for this 3D window. The actual instance of the camera is kept in and owned by the caller (i.e. the owner of the window instance).
    Vector3fT     m_CameraVel;      ///< The cameras current velocity, in camera space. Positive values for m_CameraVel.y mean forward movement, etc.
    MouseControlT m_MouseControl;   ///< If and how the camera of this window is currently being controlled with the mouse.
    RightMBStateT m_RightMBState;   ///< The state of the right mouse button. This is required because the RMB has a dual function: a click can bring up the context menu, or initiate mouse-looking for the 3D window.
    wxPoint       m_RDownPosWin;    ///< The point where the RMB went down, in window coordinates.

    // Event handlers.
    void OnKeyDown         (wxKeyEvent&              ME);
    void OnKeyUp           (wxKeyEvent&              ME);
    void OnMouseMiddleDown (wxMouseEvent&            ME);   ///< We also handle "double-click" events in this method (use ME.ButtonDClick() for distinction).
    void OnMouseMiddleUp   (wxMouseEvent&            ME);
    void OnMouseRightDown  (wxMouseEvent&            ME);   ///< We also handle "double-click" events in this method (use ME.ButtonDClick() for distinction).
    void OnMouseRightUp    (wxMouseEvent&            ME);
    void OnMouseWheel      (wxMouseEvent&            ME);
    void OnMouseMove       (wxMouseEvent&            ME);
    void OnKillFocus       (wxFocusEvent&            FE);
    void OnMouseCaptureLost(wxMouseCaptureLostEvent& ME);

    DECLARE_EVENT_TABLE()
};

#endif
