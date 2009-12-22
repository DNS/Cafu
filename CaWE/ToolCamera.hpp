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

#ifndef _TOOL_CAMERA_HPP_
#define _TOOL_CAMERA_HPP_

#include "Tool.hpp"
#include "Math3D/Vector3.hpp"
#include "Templates/Array.hpp"


class CameraT;
class OptionsBar_CameraToolT;
class ViewWindowT;


class ToolCameraT : public ToolT
{
    public:

    ToolCameraT(MapDocumentT& MapDoc, ToolManagerT& ToolMan, wxWindow* ParentOptionsBar);
    ~ToolCameraT();

    const ArrayT<CameraT*>& GetCameras() const { return m_Cameras; }
    CameraT* GetActiveCamera() const { return m_Cameras[m_ActiveCameraNr]; }
    void AddCamera(CameraT* Camera);
    void DeleteActiveCamera();
    void NotifyCameraChanged(CameraT* Camera);


    // Implementations/overrides of ToolT methods.
    int       GetWxEventID() { return ChildFrameT::ID_MENU_TOOLS_TOOL_CAMERA; }
    wxWindow* GetOptionsBar();

    bool OnKeyDown2D   (ViewWindow2DT& ViewWindow, wxKeyEvent&   KE);
    bool OnLMouseDown2D(ViewWindow2DT& ViewWindow, wxMouseEvent& ME);
    bool OnLMouseUp2D  (ViewWindow2DT& ViewWindow, wxMouseEvent& ME);
    bool OnMouseMove2D (ViewWindow2DT& ViewWindow, wxMouseEvent& ME);

    bool OnKeyDown3D   (ViewWindow3DT& ViewWindow, wxKeyEvent&   KE);
    bool OnLMouseDown3D(ViewWindow3DT& ViewWindow, wxMouseEvent& ME);
    bool OnLMouseUp3D  (ViewWindow3DT& ViewWindow, wxMouseEvent& ME);
    bool OnRMouseDown3D(ViewWindow3DT& ViewWindow, wxMouseEvent& ME);
    bool OnRMouseUp3D  (ViewWindow3DT& ViewWindow, wxMouseEvent& ME);
    bool OnMouseMove3D (ViewWindow3DT& ViewWindow, wxMouseEvent& ME);

    void RenderTool2D(Renderer2DT& Renderer) const;

    // The TypeSys related declarations for this class.
    virtual const cf::TypeSys::TypeInfoT* GetType() const { return &TypeInfo; }
    static void* CreateInstance(const cf::TypeSys::CreateParamsT& Params);
    static const cf::TypeSys::TypeInfoT TypeInfo;


    private:

    enum CameraPartT
    {
        POSITION,
        LOOKAT
    };

    enum ToolStateT
    {
        IDLE,
        DRAG_CAM_POS,
        DRAG_CAM_DIR
    };


    /// Determines if in ViewWindow at PointWS (in window space) is one of our cameras (i.e. the position or look-at point of one of our cameras).
    bool FindCamera(ViewWindow2DT& ViewWindow, const wxPoint& PointWS, unsigned long& CamNr, CameraPartT& CamPart) const;

    /// Handles key down events that are common to the 2D and 3D views.
    bool OnKeyDown(ViewWindowT& ViewWindow, wxKeyEvent& KE);


    ArrayT<CameraT*> m_Cameras;         ///< The cameras that have been created, there is always at least one.
    unsigned long    m_ActiveCameraNr;  ///< The index number of the camera that is currently active.
    ToolStateT       m_ToolState;       ///< The state this tool is currently in.

    OptionsBar_CameraToolT* m_OptionsBar;   ///< The options bar for this tool.
};

#endif
