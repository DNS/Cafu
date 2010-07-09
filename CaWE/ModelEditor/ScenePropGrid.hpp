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

#ifndef _MODELEDITOR_SCENE_PROPGRID_HPP_
#define _MODELEDITOR_SCENE_PROPGRID_HPP_

#include "../Camera.hpp"
#include "Templates/Array.hpp"
#include "wx/wx.h"
#include "wx/propgrid/manager.h"


class EditorMaterialI;
class GameConfigT;
namespace MatSys { class TextureMapI; }


namespace ModelEditor
{
    class ChildFrameT;


    class ScenePropGridT : public wxPropertyGridManager
    {
        public:

        struct LightT
        {
            bool      IsOn;         ///< Whether this light is currently on / active / being used.
            bool      CastShadows;  ///< Whether this light casts shadows.
            Vector3fT Pos;          ///< The lights position in world space.
            float     Radius;       ///< The lights radius in world space.
            wxColour  Color;        ///< The lights color (used for both the diffuse and specular component).
        };


        ScenePropGridT(ChildFrameT* Parent, const wxSize& Size, GameConfigT* GameConfig);
        ~ScenePropGridT();

        void RefreshPropGrid();
        void UpdateAmbientTexture();

        CameraT              m_Camera;            ///< The camera description. For simplicity, we "borrow" the CameraT class from the map editor.
        wxColour             m_BackgroundColor;
        bool                 m_ShowOrigin;
        bool                 m_GroundPlane_Show;
        float                m_GroundPlane_zPos;
        EditorMaterialI*     m_GroundPlane_Mat;   ///< The material used for rendering the ground plane.
        wxColour             m_AmbientLightColor;
        MatSys::TextureMapI* m_AmbientTexture;    ///< A uniform 2x2 texture colored in the ambient light color, used as lightmap for the ground plane.
        ArrayT<LightT>       m_Lights;


        private:

        ChildFrameT* m_Parent;
        GameConfigT* m_GameConfig;

        void OnPropertyGridChanged(wxPropertyGridEvent& Event);

        DECLARE_EVENT_TABLE()
    };
}

#endif
