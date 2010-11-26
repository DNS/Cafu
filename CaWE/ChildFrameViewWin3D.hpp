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

#include "AxesInfo.hpp"
#include "ChildFrameViewWin.hpp"
#include "Generic3DWindow.hpp"
#include "Renderer3D.hpp"


class MapElementT;
class ToolCameraT;


class ViewWindow3DT : public Generic3DWindowT, public ViewWindowT
{
    public:

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

    // Methods inherited from ObserverT.
    // Note that the 3D view is updated on idle anyway, so no observer messages are implemented at this point.
    // void NotifySubjectDies(SubjectT* dyingSubject);     // Already implemented in base class ViewWindowT.

    // Methods inherited from ToolsObserverT.
    void NotifySubjectChanged(ToolsSubjectT* Subject, ToolT* Tool, ToolsUpdatePriorityT Priority);

    // Inherited methods from the ViewWindowT base class.
    wxWindow* GetWindow();
    ViewTypeT GetViewType() const { return m_ViewType; }
    AxesInfoT GetAxesInfo() const { return Generic3DWindowT::GetAxesInfo(); }

    /// This method returns visible all map elements at a given pixel in the 3D view window.
    /// @param Pixel   The pixel in window coordinates for which the map elements are to be found.
    /// @returns The array of visible map elements that intersect the ray from the camera position through the pixel.
    ///          The elements are returned in front-to-back order, i.e. the nearest object is at array index 0.
    ArrayT<HitInfoT> GetElementsAt(const wxPoint& Pixel) const;


    private:

    // Implement virtual methods of Generic3DViewT base class.
    virtual Vector3fT GetRefPtWorld(const wxPoint& RefPtWin) const;
    virtual void      InfoCameraChanged();
    virtual void      InfoRightMouseClick(wxMouseEvent& ME);

    ViewTypeT     m_ViewType;           ///< The type of this 3D view (wireframe, flat, materials, ...).
    Renderer3DT   m_Renderer;           ///< Performs the 3D rendering in our window.
    unsigned long m_TimeOfLastPaint;    ///< The time at which the OnPaint() event handler was last called.
    ToolCameraT*  m_CameraTool;         ///< The camera tool that manages all cameras. The camera of this 3D view is always among the cameras in the tool.

    // Event handlers.
    void OnKeyDown        (wxKeyEvent&         KE);
    void OnKeyUp          (wxKeyEvent&         KE);
    void OnKeyChar        (wxKeyEvent&         KE);
    void OnMouseLeftDown  (wxMouseEvent&       ME); ///< We also handle "double-click" events in this method (use ME.ButtonDClick() for distinction).
    void OnMouseLeftUp    (wxMouseEvent&       ME);
    void OnMouseMiddleDown(wxMouseEvent&       ME); ///< We also handle "double-click" events in this method (use ME.ButtonDClick() for distinction).
    void OnMouseMiddleUp  (wxMouseEvent&       ME);
    void OnMouseWheel     (wxMouseEvent&       ME);
    void OnMouseMove      (wxMouseEvent&       ME);
    void OnContextMenu    (wxContextMenuEvent& CE);
    void OnPaint          (wxPaintEvent&       PE);

    DECLARE_EVENT_TABLE()
};

#endif
