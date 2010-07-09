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

#ifndef _MODELEDITOR_SCENE_VIEW_3D_HPP_
#define _MODELEDITOR_SCENE_VIEW_3D_HPP_

#include "Math3D/Vector3.hpp"
#include "wx/glcanvas.h"


class CameraT;
namespace MatSys { class RenderMaterialT; }


namespace ModelEditor
{
    class ChildFrameT;


    /// This class has many similarities with ViewWindow3DT from the map editor.
    class SceneView3DT : public wxGLCanvas
    {
        public:

        SceneView3DT(ChildFrameT* Parent);
        ~SceneView3DT();


        private:

        /// Renders a single pass of the scene.
        void RenderPass() const;

        ChildFrameT*  m_Parent;
        unsigned long m_TimeOfLastPaint;    ///< The time at which the OnPaint() event handler was last called.
     // ToolCameraT*  m_CameraTool;         ///< [Not needed here - this is a single camera party.] The camera tool that manages all cameras. The camera of this 3D view is always among the cameras in the tool.
        CameraT*      m_Camera;             ///< Pointer to the camera that is currently used for this 3D view. The actual instance of the camera is kept in the document.
        Vector3fT     m_CameraVel;          ///< The cameras current velocity, in camera space. Positive values for m_CameraVel.y mean forward movement, etc.

        MatSys::RenderMaterialT* m_RMatWireframe;       ///< The render material for wire-frame rendering.
        MatSys::RenderMaterialT* m_RMatWireframeOZ;     ///< The render material for wire-frame rendering (with polygon z-offset, e.g. for outlines).

        void OnPaint     (wxPaintEvent& PE);
        void OnIdle      (wxIdleEvent&  IE);
        void OnMouseWheel(wxMouseEvent& ME);
        void OnSize      (wxSizeEvent&  SE);
        void OnMouseMove (wxMouseEvent& ME);
        void OnLMouseDown(wxMouseEvent& ME);
        void OnLMouseUp  (wxMouseEvent& ME);
        void OnRMouseUp  (wxMouseEvent& ME);
        void OnKeyDown   (wxKeyEvent&   KE);

        DECLARE_EVENT_TABLE()
    };
}

#endif
