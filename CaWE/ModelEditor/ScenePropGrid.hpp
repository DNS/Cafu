/*
=================================================================================
This file is part of Cafu, the open-source game engine and graphics engine
for multiplayer, cross-platform, real-time 3D action.
Copyright (C) 2002-2011 Carsten Fuchs Software.

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

#ifndef _MODELEDITOR_SCENE_PROPGRID_HPP_
#define _MODELEDITOR_SCENE_PROPGRID_HPP_

#include "ObserverPattern.hpp"
#include "wx/wx.h"
#include "wx/propgrid/manager.h"


namespace MatSys { class TextureMapI; }


namespace ModelEditor
{
    class ChildFrameT;


    class ScenePropGridT : public wxPropertyGridManager, public ObserverT
    {
        public:

        ScenePropGridT(ChildFrameT* Parent, const wxSize& Size);
        ~ScenePropGridT();

        void RefreshPropGrid();

        // ObserverT implementation.
        void Notify_AnimStateChanged(SubjectT* Subject);
        void Notify_SubjectDies(SubjectT* dyingSubject);

        wxColour             m_BackgroundColor;
        bool                 m_ShowOrigin;
        bool                 m_ShowGrid;
        float                m_GridSpacing;
        bool                 m_GroundPlane_Show;
        float                m_GroundPlane_PosZ;
        bool                 m_GroundPlane_AutoZ;
        bool                 m_Model_ShowMesh;
        bool                 m_Model_ShowSkeleton;
        bool                 m_Model_ShowTriangleNormals;
        bool                 m_Model_ShowTangentSpace;
        wxColour             m_AmbientLightColor;
        MatSys::TextureMapI* m_AmbientTexture;    ///< A uniform 2x2 texture colored in the ambient light color, used as lightmap for the ground plane.


        private:

        void OnPropertyGridChanged(wxPropertyGridEvent& Event);
        void UpdateAmbientTexture();

        ChildFrameT*  m_Parent;
        bool          m_IsRecursiveSelfNotify;
        wxPGProperty* m_AnimFrameNrProp;
        wxPGProperty* m_AnimSpeedProp;
        wxPGProperty* m_AnimLoopProp;

        DECLARE_EVENT_TABLE()
    };
}

#endif
